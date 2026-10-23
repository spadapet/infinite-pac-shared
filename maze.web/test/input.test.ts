import { test, type TestContext } from 'node:test';
import assert from 'node:assert/strict';
import { InputController } from '../src/input';
import type { Direction, Vec } from '../src/types';

class ElementStub extends EventTarget {}
class KeyEvent extends Event {
    constructor(type: string, readonly key: string, readonly code: string, readonly repeat = false) { super(type, { cancelable: true }); }
}
function setup(t: TestContext) {
    const window = new EventTarget();
    const document = Object.assign(new EventTarget(), { hidden: false, querySelectorAll: () => [] });
    let pads: Gamepad[] = [];
    for (const [name, value] of Object.entries({ window, document, navigator: { getGamepads: () => pads }, HTMLElement: ElementStub, HTMLButtonElement: ElementStub })) {
        const previous = Object.getOwnPropertyDescriptor(globalThis, name);
        Object.defineProperty(globalThis, name, { configurable: true, value });
        t.after(() => { if (previous) Object.defineProperty(globalThis, name, previous); else Reflect.deleteProperty(globalThis, name); });
    }
    const moves: (Direction | Vec | null)[] = [];
    let active = true;
    const controller = new InputController(new ElementStub() as HTMLElement, {
        active: () => active, move: direction => moves.push(direction), action() {}, pause() {}, mute() {}, fullscreen() {},
        point: () => null, pac: () => ({ x: 0, y: 0 }),
    });
    const key = (type: string, value: string, code: string, repeat = false) => window.dispatchEvent(new KeyEvent(type, value, code, repeat));
    return { window, document, moves, controller, key, setActive: (value: boolean) => { active = value; },
        setPads: (value: Gamepad[]) => { pads = value; } };
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
