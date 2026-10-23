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
    let active = true;
    let direction: Direction = 'left';
    const host = new ElementStub();
    const controller = new InputController(host, {
        active: () => active, move: value => moves.push(value), action() {}, pause() {}, mute() {}, fullscreen() {},
        direction: () => direction,
    });
    const key = (type: string, value: string, code: string, repeat = false) => window.dispatchEvent(new KeyEvent(type, value, code, repeat));
    const pointer = (type: string, x: number, y: number, id = 1, button = 0) => host.dispatchEvent(new PointerEventStub(type, x, y, id, button));
    return { window, document, host, moves, controller, key, pointer, setActive: (value: boolean) => { active = value; },
        setDirection: (value: Direction) => { direction = value; }, setPads: (value: Gamepad[]) => { pads = value; } };
}

function pad(pressed: number[] = [], axes = [0, 0], index = 0): Gamepad {
    return { index, connected: true, mapping: 'standard', id: `test-${index}`, timestamp: 0, axes,
        buttons: Array.from({ length: 16 }, (_, button) => ({ pressed: pressed.includes(button), touched: pressed.includes(button), value: pressed.includes(button) ? 1 : 0 })),
        vibrationActuator: { playEffect: async () => 'complete', reset: async () => 'complete' } };
}

test('releasing Up immediately restores still-held Right without another keydown or OS repeat', t => {
    const { key, moves, controller } = setup(t);
    key('keydown', 'ArrowRight', 'ArrowRight');
    key('keydown', 'ArrowUp', 'ArrowUp');
    key('keyup', 'ArrowUp', 'ArrowUp');
    assert.deepEqual(moves, [{ x: 1, y: 0 }, { x: 1, y: -1 }, { x: 1, y: 0 }]);
    controller.poll();
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    key('keyup', 'ArrowRight', 'ArrowRight');
    assert.equal(moves.at(-1), null, 'release cancels a buffered turn without stopping forward movement');
});

test('arrows and WASD retain independent held states and native opposite-key priority', t => {
    const { key, moves } = setup(t);
    key('keydown', 'ArrowRight', 'ArrowRight');
    key('keydown', 'd', 'KeyD');
    key('keyup', 'ArrowRight', 'ArrowRight');
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    key('keydown', 'ArrowLeft', 'ArrowLeft');
    assert.deepEqual(moves.at(-1), { x: -1, y: 0 });
    key('keydown', 'd', 'KeyD', true);
    assert.deepEqual(moves.at(-1), { x: -1, y: 0 }, 'repeat does not steal priority');
    key('keydown', 's', 'KeyS');
    key('keydown', 'w', 'KeyW');
    assert.deepEqual(moves.at(-1), { x: -1, y: -1 });
    key('keyup', 'w', 'KeyW');
    assert.deepEqual(moves.at(-1), { x: -1, y: 1 });
    key('keyup', 'ArrowLeft', 'ArrowLeft');
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 });
});

test('blur and hidden-tab transitions clear held keys, including releases during pause', t => {
    const { key, moves, controller, window, document, setActive } = setup(t);
    key('keydown', 'ArrowRight', 'ArrowRight');
    window.dispatchEvent(new Event('blur'));
    assert.equal(moves.at(-1), null);
    const afterBlur = moves.length;
    controller.poll();
    assert.equal(moves.length, afterBlur);
    key('keydown', 'ArrowUp', 'ArrowUp');
    setActive(false);
    key('keyup', 'ArrowUp', 'ArrowUp');
    setActive(true);
    const afterPause = moves.length;
    controller.poll();
    assert.equal(moves.length, afterPause);
    key('keydown', 'a', 'KeyA');
    document.hidden = true;
    document.dispatchEvent(new Event('visibilitychange'));
    assert.equal(moves.at(-1), null);
    document.hidden = false;
    const afterHidden = moves.length;
    controller.poll();
    assert.equal(moves.length, afterHidden);
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
        controller.poll();
        assert.deepEqual(moves.at(-1), expected);
        setPads([pad([], [...axes])]);
        controller.poll();
        assert.deepEqual(moves.at(-1), expected);
    }
    setPads([pad()]);
    controller.poll();
    assert.equal(moves.at(-1), null);
});

test('stick uses native 0.5625 press and 0.5 release hysteresis on each axis', t => {
    const { controller, moves, setPads } = setup(t);
    for (const [axis, expected] of [
        [0.55, null], [0.5625, { x: 1, y: 0 }], [0.5, { x: 1, y: 0 }],
        [0.499, null], [0.55, null], [-0.5625, { x: -1, y: 0 }],
    ] as const) {
        const before = moves.length;
        setPads([pad([], [axis, 0])]);
        controller.poll();
        if (expected) assert.deepEqual(moves.at(-1), expected);
        else if (moves.length > before) assert.equal(moves.at(-1), null);
        else assert.equal(moves.length, before);
    }
});

test('keyboard, sticks, D-pad and multiple pads share native Up/Left priority', t => {
    const { key, controller, moves, setPads } = setup(t);
    key('keydown', 'ArrowRight', 'ArrowRight');
    setPads([pad([12])]);
    controller.poll();
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
    setPads([pad([13, 15], [0, 0], 0), pad([12, 14], [0, 0], 1)]);
    controller.poll();
    assert.deepEqual(moves.at(-1), { x: -1, y: -1 });
    setPads([]);
    controller.poll();
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 }, 'disconnect restores the still-held keyboard direction');
    key('keyup', 'ArrowRight', 'ArrowRight');
    assert.equal(moves.at(-1), null);
});

test('D-pad and stick combine without dominant-axis selection; disconnect and pause clear stale turns', t => {
    const { controller, moves, setPads, setActive } = setup(t);
    setPads([pad([15], [0, -0.9])]);
    controller.poll();
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
    setActive(false);
    setPads([]);
    controller.poll();
    assert.equal(moves.at(-1), null);
    setActive(true);
    const count = moves.length;
    controller.poll();
    assert.equal(moves.length, count);
});

test('pointer down defines a stationary joystick origin, never a target relative to Pac', t => {
    const { pointer, host, moves, controller } = setup(t);
    pointer('pointerdown', 500, 300);
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 });
    assert.equal(host.hasPointerCapture(1), true);
    pointer('pointermove', 560, 300);
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    pointer('pointermove', 530, 330);
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 }, 'offset is still relative to the initial down, not the last move');
    assert.deepEqual(controller.joystick?.offset, { x: 30, y: 30 });
    pointer('pointermove', 500, 300);
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 }, 'moving back inside the dead zone clears the held turn');
});

test('pointer dead zone is radial, inclusive at 20 physical pixels, and accounts for display density', t => {
    const { pointer, moves, window } = setup(t);
    pointer('pointerdown', 100, 100);
    pointer('pointermove', 119, 100);
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 });
    pointer('pointermove', 120, 100);
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    pointer('pointermove', 115, 115);
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 }, 'radial length, not dominant-axis length, crosses the threshold');
    window.devicePixelRatio = 2;
    pointer('pointermove', 110, 100);
    assert.deepEqual(moves.at(-1), { x: 1, y: 0 });
    pointer('pointermove', 109.99, 100);
    assert.deepEqual(moves.at(-1), { x: 0, y: 0 });
});

test('pointer angular sectors match all eight native directions, including diagonal boundaries', t => {
    const { pointer, moves } = setup(t);
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
        assert.deepEqual(moves.at(-1), expected, `${angle} degrees`);
    }
});

test('released diagonal keeps only an unconsumed turn and clears it on the next fixed update after turning', t => {
    const { pointer, moves, controller, setDirection } = setup(t);
    setDirection('right');
    pointer('pointerdown', 100, 100);
    pointer('pointermove', 160, 40);
    assert.deepEqual(moves.at(-1), { x: 1, y: -1 });
    pointer('pointerup', 160, 40);
    assert.equal(controller.joystick, null);
    assert.deepEqual(moves.at(-1), { x: 0, y: -1 });
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
    assert.deepEqual(moves.at(-1), { x: 0, y: -1 });
    pointer('pointermove', 160, 160);
    assert.deepEqual(moves.at(-1), { x: 0, y: -1 });
    key('keyup', 'ArrowUp', 'ArrowUp');
    assert.deepEqual(moves.at(-1), { x: 1, y: 1 });
    setPads([pad([14])]);
    controller.poll();
    assert.deepEqual(moves.at(-1), { x: -1, y: 0 });
    setPads([]);
    controller.poll();
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
        if (end === 'blur') window.dispatchEvent(new Event('blur'));
        else if (end === 'hidden') {
            document.hidden = true;
            document.dispatchEvent(new Event('visibilitychange'));
            document.hidden = false;
        } else if (end === 'pause') { setActive(false); controller.poll(); }
        else pointer(end, 100, 30);
        assert.equal(controller.joystick, null, end);
        assert.equal(host.hasPointerCapture(1), false, end);
        assert.equal(moves.at(-1), null, end);
        setActive(true);
        const count = moves.length;
        controller.advance();
        assert.equal(moves.length, count, `${end}: no stale turn after resuming`);
    }
});
