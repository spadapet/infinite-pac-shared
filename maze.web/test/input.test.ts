import { test, type TestContext } from 'node:test';
import assert from 'node:assert/strict';
import { InputController } from '../src/input';
import type { Direction, Vec } from '../src/types';

class ElementStub extends EventTarget {
    private readonly captured = new Set<number>();
    focus(): void {}
    setPointerCapture(id: number): void { this.captured.add(id); }
    hasPointerCapture(id: number): boolean { return this.captured.has(id); }
    releasePointerCapture(id: number): void {
        this.captured.delete(id);
        this.dispatchEvent(new PointerEventStub('lostpointercapture', 0, 0, id));
    }
}
class KeyEvent extends Event {
    constructor(type: string, readonly key: string, readonly code: string, readonly repeat = false) { super(type, { cancelable: true }); }
}
class PointerEventStub extends Event {
    constructor(type: string, readonly clientX: number, readonly clientY: number, readonly pointerId = 1, readonly button = 0) { super(type, { cancelable: true }); }
}
function setup(t: TestContext) {
    const window = Object.assign(new EventTarget(), { devicePixelRatio: 1 });
    const document = Object.assign(new EventTarget(), { hidden: false, querySelectorAll: () => [] });
    let pads: Gamepad[] = [];
    for (const [name, value] of Object.entries({ window, document, navigator: { getGamepads: () => pads }, HTMLElement: ElementStub, HTMLButtonElement: ElementStub })) {
        const previous = Object.getOwnPropertyDescriptor(globalThis, name);
        Object.defineProperty(globalThis, name, { configurable: true, value });
        t.after(() => { if (previous) Object.defineProperty(globalThis, name, previous); else Reflect.deleteProperty(globalThis, name); });
    }
    const moves: (Direction | Vec | null)[] = [];
    let actions = 0;
    let pauses = 0;
    let active = true;
    let direction: Direction = 'left';
    const host = new ElementStub();
    const controller = new InputController(host, {
        active: () => active, move: value => moves.push(value), action: () => { actions++; }, pause: () => { pauses++; }, mute() {}, fullscreen() {},
        direction: () => direction,
    });
    const key = (type: string, value: string, code: string, repeat = false) => window.dispatchEvent(new KeyEvent(type, value, code, repeat));
    const pointer = (type: string, x: number, y: number, id = 1, button = 0) => host.dispatchEvent(new PointerEventStub(type, x, y, id, button));
    return { window, document, host, moves, controller, key, pointer, setActive: (value: boolean) => { active = value; },
        setDirection: (value: Direction) => { direction = value; }, setPads: (value: Gamepad[]) => { pads = value; },
        get actionCount() { return actions; }, get pauseCount() { return pauses; } };
}

function pad(pressed: number[] = [], axes = [0, 0], index = 0, id = `test-${index}`, mapping: Gamepad['mapping'] = 'standard'): Gamepad {
    return { index, connected: true, mapping, id, timestamp: 0, axes,
        buttons: Array.from({ length: 16 }, (_, button) => ({ pressed: pressed.includes(button), touched: pressed.includes(button), value: pressed.includes(button) ? 1 : 0 })),
        vibrationActuator: { playEffect: async () => 'complete', reset: async () => 'complete' } };
}

test('transient key changes between fixed updates never reach gameplay; held axes are sampled once per update', t => {
    const { key, moves, controller } = setup(t);
    key('keydown', 'ArrowRight', 'ArrowRight');
    key('keydown', 'ArrowUp', 'ArrowUp');
    key('keyup', 'ArrowUp', 'ArrowUp');
    assert.deepEqual(moves, [], 'keydown and keyup do not apply directions outside a simulation update');
    controller.advance();
    assert.deepEqual(moves, [{ x: 1, y: 0 }], 'only the direction still held at the fixed update is sampled');
    controller.poll();
    assert.equal(moves.length, 1, 'render-frame polling does not apply a gameplay direction');
    key('keyup', 'ArrowRight', 'ArrowRight');
    assert.deepEqual(moves, [{ x: 1, y: 0 }], 'keyup takes effect at the next fixed update');
    controller.advance();
    assert.equal(moves.at(-1), null, 'the released direction clears on the next simulation update');
});

test('arrows and WASD retain independent held states and native opposite-key priority', t => {
    const { key, moves, controller } = setup(t);
    key('keydown', 'ArrowRight', 'ArrowRight');
    key('keydown', 'd', 'KeyD');
    key('keyup', 'ArrowRight', 'ArrowRight');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    key('keydown', 'ArrowLeft', 'ArrowLeft');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: -1, y: 0 });
    key('keydown', 'd', 'KeyD', true);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: -1, y: 0 }, 'repeat does not steal priority');
    key('keydown', 's', 'KeyS');
    key('keydown', 'w', 'KeyW');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: -1, y: -1 });
    key('keyup', 'w', 'KeyW');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: -1, y: 1 });
    key('keyup', 'ArrowLeft', 'ArrowLeft');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 });
});

test('blur and hidden-tab transitions clear held keys without applying gameplay input between updates', t => {
    const { key, moves, controller, window, document, setActive } = setup(t);
    key('keydown', 'ArrowRight', 'ArrowRight');
    assert.deepEqual(moves, []);
    window.dispatchEvent(new Event('blur'));
    controller.advance();
    assert.equal(moves.at(-1), null);
    const afterBlur = moves.length;
    controller.poll();
    assert.equal(moves.length, afterBlur);
    key('keydown', 'ArrowUp', 'ArrowUp');
    setActive(false);
    key('keyup', 'ArrowUp', 'ArrowUp');
    setActive(true);
    controller.advance();
    assert.equal(moves.at(-1), null, 'releases during pause do not leave a stale turn');
    key('keydown', 'a', 'KeyA');
    document.hidden = true;
    document.dispatchEvent(new Event('visibilitychange'));
    document.hidden = false;
    controller.advance();
    assert.equal(moves.at(-1), null);
});

test('D-pad and analog stick preserve all four diagonals independently', t => {
    const { controller, moves, setPads } = setup(t);
    for (const [buttons, axes, expected] of [
        [[12, 15], [0.8, -0.9], { x: 1, y: -1 }],
        [[12, 14], [-0.8, -0.9], { x: -1, y: -1 }],
        [[13, 15], [0.8, 0.9], { x: 1, y: 1 }],
        [[13, 14], [-0.8, 0.9], { x: -1, y: 1 }],
    ] as const) {
        setPads([pad([...buttons])]);
        const beforePoll = moves.length;
        controller.poll();
        assert.equal(moves.length, beforePoll, 'polling only records gamepad state');
        controller.advance();
        assert.deepEqual(moves.at(-1), expected);
        setPads([pad([], [...axes])]);
        controller.poll();
        controller.advance();
        assert.deepEqual(moves.at(-1), expected);
    }
    setPads([pad()]);
    controller.poll();
    controller.advance();
    assert.equal(moves.at(-1), null);
});

test('Xbox controllers with an unmapped Bluetooth profile use the standard Xbox control indices', t => {
    const harness = setup(t);
    const { controller, moves, setPads } = harness;
    setPads([pad([0], [0.8, -0.9], 0, 'Xbox Wireless Controller', '')]);
    controller.poll();
    assert.deepEqual(moves, []);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
    assert.equal(harness.actionCount, 1, 'A is the standard Xbox action button');

    setPads([pad([9], [0, 0], 0, 'Xbox Wireless Controller', '')]);
    controller.poll();
    assert.equal(harness.pauseCount, 1, 'Start is the standard Xbox pause button');

    setPads([pad([12, 15], [0, 0], 0, 'Xbox Wireless Controller', '')]);
    controller.poll();
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
});

test('unmapped non-Xbox gamepads remain ignored rather than guessing their button layout', t => {
    const { controller, moves, setPads } = setup(t);
    setPads([pad([0], [0.8, -0.9], 0, 'Unknown Bluetooth Controller', '')]);
    controller.poll();
    assert.deepEqual(moves, []);
});

test('stick uses native 0.5625 press and 0.5 release hysteresis on each axis', t => {
    const { controller, moves, setPads } = setup(t);
    for (const [axis, expected] of [
        [0.55, null], [0.5625, { x: 1, y: 0 }], [0.5, { x: 1, y: 0 }],
        [0.499, null], [0.55, null], [-0.5625, { x: -1, y: 0 }],
    ] as const) {
        setPads([pad([], [axis, 0])]);
        controller.poll();
        controller.advance();
        if (expected) assert.deepEqual(moves.at(-1), expected);
        else assert.equal(moves.at(-1), null);
    }
});

test('keyboard, sticks, D-pad and multiple pads share native Up/Left priority', t => {
    const { key, controller, moves, setPads } = setup(t);
    key('keydown', 'ArrowRight', 'ArrowRight');
    setPads([pad([12])]);
    controller.poll();
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
    setPads([pad([13, 15], [0, 0], 0), pad([12, 14], [0, 0], 1)]);
    controller.poll();
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: -1, y: -1 });
    setPads([]);
    controller.poll();
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 }, 'disconnect restores the still-held keyboard direction');
    key('keyup', 'ArrowRight', 'ArrowRight');
    controller.advance();
    assert.equal(moves.at(-1), null);
});

test('D-pad and stick combine without dominant-axis selection; disconnect and pause clear stale turns', t => {
    const { controller, moves, setPads, setActive } = setup(t);
    setPads([pad([15], [0, -0.9])]);
    controller.poll();
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
    setActive(false);
    setPads([]);
    controller.poll();
    setActive(true);
    controller.advance();
    assert.equal(moves.at(-1), null, 'a cleared gamepad direction is applied only on a simulation update');
});

test('pointer down defines a stationary joystick origin, never a target relative to Pac', t => {
    const { pointer, host, moves, controller } = setup(t);
    pointer('pointerdown', 500, 300);
    assert.deepEqual(moves, [], 'pointer events do not sample gameplay directions');
    assert.equal(host.hasPointerCapture(1), true);
    pointer('pointermove', 560, 300);
    assert.deepEqual(controller.joystick?.offset, { x: 60, y: 0 }, 'joystick presentation responds immediately');
    assert.deepEqual(moves, []);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    pointer('pointermove', 530, 330);
    assert.deepEqual(controller.joystick?.offset, { x: 30, y: 30 }, 'offset is still relative to the initial down, not the last move');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 });
    pointer('pointermove', 500, 300);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 }, 'moving back inside the dead zone clears the held turn');
});

test('pointer dead zone is radial, inclusive at 20 physical pixels, and accounts for display density', t => {
    const { pointer, moves, controller, window } = setup(t);
    pointer('pointerdown', 100, 100);
    pointer('pointermove', 119, 100);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 });
    pointer('pointermove', 120, 100);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    pointer('pointermove', 115, 115);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 }, 'radial length, not dominant-axis length, crosses the threshold');
    window.devicePixelRatio = 2;
    pointer('pointermove', 110, 100);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    pointer('pointermove', 109.99, 100);
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 });
});

test('pointer angular sectors match all eight native directions, including diagonal boundaries', t => {
    const { pointer, moves, controller } = setup(t);
    pointer('pointerdown', 200, 200);
    const sectors: [number, Vec][] = [
        [0, { x: 1, y: 0 }], [22.49, { x: 1, y: 0 }], [22.51, { x: 1, y: 1 }],
        [45, { x: 1, y: 1 }], [67.49, { x: 1, y: 1 }], [67.51, { x: 0, y: 1 }],
        [90, { x: 0, y: 1 }], [135, { x: -1, y: 1 }], [180, { x: -1, y: 0 }],
        [-180, { x: -1, y: 0 }], [-135, { x: -1, y: -1 }], [-90, { x: 0, y: -1 }],
        [-67.49, { x: 1, y: -1 }], [-67.51, { x: 0, y: -1 }], [-45, { x: 1, y: -1 }],
        [-22.49, { x: 1, y: 0 }], [-22.51, { x: 1, y: -1 }],
    ];
    for (const [angle, expected] of sectors) {
        pointer('pointermove', 200 + 100 * Math.cos(angle * Math.PI / 180), 200 + 100 * Math.sin(angle * Math.PI / 180));
        controller.advance();
        assert.deepEqual(moves.at(-1), expected, `${angle} degrees`);
    }
});

test('released diagonal keeps only an unconsumed turn and clears it on the next fixed update after turning', t => {
    const { pointer, moves, controller, setDirection } = setup(t);
    setDirection('right');
    pointer('pointerdown', 100, 100);
    pointer('pointermove', 160, 40);
    assert.deepEqual(controller.joystick?.offset, { x: 60, y: -60 });
    assert.deepEqual(moves, [], 'dragging is presented immediately but sampled only by the fixed update');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
    pointer('pointerup', 160, 40);
    assert.equal(controller.joystick, null);
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 }, 'release queues the pending turn without applying it');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 0, y: -1 });
    setDirection('up');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 });
    setDirection('left');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 }, 'the released swipe must not reapply at later corners');
});

test('keyboard and gamepad override the joystick until released, without moving its origin', t => {
    const { pointer, moves, controller, key, setPads } = setup(t);
    pointer('pointerdown', 100, 100);
    pointer('pointermove', 160, 100);
    key('keydown', 'ArrowUp', 'ArrowUp');
    assert.deepEqual(moves, [], 'keyboard changes do not immediately override sampled gameplay input');
    pointer('pointermove', 160, 160);
    assert.deepEqual(controller.joystick?.offset, { x: 60, y: 60 });
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 0, y: -1 });
    key('keyup', 'ArrowUp', 'ArrowUp');
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 });
    setPads([pad([14])]);
    controller.poll();
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: -1, y: 0 });
    setPads([]);
    controller.poll();
    controller.advance();
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 });
    assert.deepEqual(controller.joystick?.offset, { x: 60, y: 60 });
});

test('secondary contacts and mouse buttons cannot replace an active joystick origin', t => {
    const { pointer, controller } = setup(t);
    pointer('pointerdown', 10, 10, 5, 2);
    assert.equal(controller.joystick?.offset, undefined);
    pointer('pointerdown', 100, 100);
    pointer('pointerdown', 500, 500, 2);
    pointer('pointermove', 560, 500, 2);
    pointer('pointerup', 560, 500, 2);
    assert.deepEqual(controller.joystick?.offset, { x: 0, y: 0 });
    pointer('pointermove', 150, 100);
    assert.deepEqual(controller.joystick?.offset, { x: 50, y: 0 });
});

test('cancel, capture loss, blur, tab hiding and pause remove both the arrow and pending turns', t => {
    const { pointer, host, controller, moves, window, document, setActive } = setup(t);
    for (const end of ['pointercancel', 'lostpointercapture', 'blur', 'hidden', 'pause']) {
        pointer('pointerdown', 100, 100);
        pointer('pointermove', 100, 30);
        controller.advance();
        if (end === 'blur') window.dispatchEvent(new Event('blur'));
        else if (end === 'hidden') {
            document.hidden = true;
            document.dispatchEvent(new Event('visibilitychange'));
            document.hidden = false;
        } else if (end === 'pause') { setActive(false); controller.poll(); }
        else pointer(end, 100, 30);
        assert.equal(controller.joystick, null, end);
        assert.equal(host.hasPointerCapture(1), false, end);
        setActive(true);
        controller.advance();
        assert.equal(moves.at(-1), null, `${end}: no stale turn after cancellation`);
        const count = moves.length;
        controller.advance();
        assert.equal(moves.length, count + 1, `${end}: only the next fixed update samples the cleared state`);
    }
});

test('native screens receive controller direction edges and B cancellation without gameplay mutation', t => {
    const { controller, setPads, setActive, moves, key } = setup(t);
    const navigation: Direction[] = [];
    let cancellations = 0;
    let starts = 0;
    controller['actions'].start = () => { starts++; };
    controller['actions'].menuActive = () => true;
    controller['actions'].navigate = direction => navigation.push(direction);
    controller['actions'].cancel = () => { cancellations++; };
    setActive(false);
    setPads([pad([13])]);
    controller.poll();
    controller.poll();
    assert.deepEqual(navigation, ['down']);
    setPads([pad([15])]);
    controller.poll();
    assert.deepEqual(navigation, ['down', 'right']);
    setPads([pad([1])]);
    controller.poll(); controller.poll();
    assert.equal(cancellations, 1);
    setPads([]); controller.poll();
    setPads([pad([1])]); controller.poll();
    assert.equal(cancellations, 2);
    key('keydown', 'Escape', 'Escape');
    assert.equal(cancellations, 3);
    setPads([pad([9])]);
    controller.poll(); controller.poll();
    assert.equal(starts, 1);
    key('keydown', 'p', 'KeyP');
    assert.equal(starts, 1, 'the pause key is not a title-screen launch action');
    assert.deepEqual(moves, []);
});

test('controller polling reuses state and scratch collections but forgets disconnected stick hysteresis', t => {
    const { controller, setPads, moves, pointer } = setup(t);
    setPads([pad([], [0.8, -0.8])]);
    controller.poll();
    const state = controller['buttons'].get(0)!;
    const directions = state.directions;
    const connected = controller['connected'];
    const held = controller['held'];
    for (let index = 0; index < 100; index++) {
        controller.poll();
        controller.advance();
        assert.equal(controller['buttons'].get(0), state);
        assert.equal(state.directions, directions);
        assert.equal(controller['connected'], connected);
        assert.equal(controller['held'], held);
        assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
    }
    setPads([]);
    controller.poll();
    assert.equal(controller['buttons'].size, 0);
    setPads([pad([], [0.55, -0.55])]);
    controller.poll();
    controller.advance();
    assert.notEqual(controller['buttons'].get(0), state);
    assert.equal(moves.at(-1), null, 'a reconnected stick must cross the press threshold again');
    pointer('pointerdown', 100, 100);
    const position = controller['pointer']!.position;
    pointer('pointermove', 150, 120);
    assert.equal(controller['pointer']!.position, position);
    assert.deepEqual(position, { x: 150, y: 120 });
});
