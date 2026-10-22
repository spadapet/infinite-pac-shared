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
    for (const [name, value] of Object.entries({ window, document, navigator: { getGamepads: () => [] }, HTMLElement: ElementStub, HTMLButtonElement: ElementStub })) {
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
    return { window, document, moves, controller, key, setActive: (value: boolean) => { active = value; } };
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
