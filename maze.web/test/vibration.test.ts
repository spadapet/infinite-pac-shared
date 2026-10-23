import assert from 'node:assert/strict';
import { readFile } from 'node:fs/promises';
import { test, type TestContext } from 'node:test';
import { Game } from '../src/game';
import { GameVibration } from '../src/vibration';
import type { GameEvent } from '../src/types';

type VibrationEvent = Extract<GameEvent, { type: 'vibrate' }>;
const pulse = (duration: number, strongMagnitude: number, weakMagnitude: number): VibrationEvent =>
    ({ type: 'vibrate', duration, strongMagnitude, weakMagnitude });

function setup(t: TestContext) {
    let now = 0;
    t.mock.method(performance, 'now', () => now);
    t.mock.timers.enable({ apis: ['setTimeout'] });
    const calls: { type: GamepadHapticEffectType; parameters: GamepadEffectParameters }[] = [];
    let resets = 0;
    const actuator: GamepadHapticActuator = {
        async playEffect(type, parameters = {}) { calls.push({ type, parameters }); return 'complete'; },
        async reset() { resets++; return 'complete'; },
    };
    const pad: Gamepad = { index: 0, id: 'Xbox Wireless Controller', connected: true, mapping: 'standard',
        axes: [0, 0], buttons: [], timestamp: 0, vibrationActuator: actuator };
    let pads: (Gamepad | null)[] = [pad];
    const deviceCalls: number[] = [];
    const document = { hidden: false };
    const descriptors = new Map<string, PropertyDescriptor | undefined>();
    for (const [name, value] of Object.entries({ document, navigator: {
        getGamepads: () => pads,
        vibrate: (duration: number) => { deviceCalls.push(duration); return true; },
    } })) {
        descriptors.set(name, Object.getOwnPropertyDescriptor(globalThis, name));
        Object.defineProperty(globalThis, name, { configurable: true, value });
    }
    const errors: string[] = [];
    const vibration = new GameVibration(message => errors.push(message));
    t.after(() => {
        vibration.stop();
        for (const [name, descriptor] of descriptors) {
            if (descriptor) Object.defineProperty(globalThis, name, descriptor);
            else Reflect.deleteProperty(globalThis, name);
        }
    });
    return { vibration, actuator, pad, calls, deviceCalls, document, errors,
        setPads: (value: (Gamepad | null)[]) => { pads = value; },
        tick: (milliseconds: number) => { now += milliseconds; t.mock.timers.tick(milliseconds); },
        get resets() { return resets; } };
}

test('Xbox dual-rumble starts synchronously with independent native motor strengths and the full death duration', t => {
    const harness = setup(t);
    harness.vibration.play(pulse(1250, 0.375, 0));
    assert.deepEqual(harness.calls, [{ type: 'dual-rumble', parameters: {
        startDelay: 0, duration: 1250, strongMagnitude: 0.375, weakMagnitude: 0,
    } }]);
    assert.deepEqual(harness.deviceCalls, [], 'controller rumble does not also vibrate the phone');
    harness.tick(1249);
    assert.equal(harness.resets, 0);
    harness.tick(1);
    assert.equal(harness.resets, 1);
});

test('overlaps keep independent motor maxima, then restore weaker pulses for their remaining lifetime', t => {
    const harness = setup(t);
    harness.vibration.play(pulse(1250, 0.375, 0));
    harness.tick(50);
    harness.vibration.play(pulse(750, 0.75, 0));
    harness.vibration.play(pulse(100, 0, 0.5));
    assert.deepEqual(harness.calls.at(-1)?.parameters, {
        startDelay: 0, duration: 100, strongMagnitude: 0.75, weakMagnitude: 0.5,
    });
    harness.tick(100);
    assert.deepEqual(harness.calls.at(-1)?.parameters, {
        startDelay: 0, duration: 650, strongMagnitude: 0.75, weakMagnitude: 0,
    });
    harness.tick(650);
    assert.deepEqual(harness.calls.at(-1)?.parameters, {
        startDelay: 0, duration: 450, strongMagnitude: 0.375, weakMagnitude: 0,
    });
    harness.tick(450);
    assert.equal(harness.resets, 1);
});

test('stop clears queued pulses and timers without replaying rumble', t => {
    const harness = setup(t);
    harness.vibration.play(pulse(750, 0.75, 0));
    harness.vibration.stop();
    assert.equal(harness.resets, 1);
    harness.tick(2000);
    assert.equal(harness.calls.length, 1);
    assert.equal(harness.resets, 1);
});

test('disconnected, replaced and hidden controllers cannot continue queued effects', t => {
    const harness = setup(t);
    harness.vibration.play(pulse(750, 0.75, 0));
    harness.setPads([null]);
    harness.tick(750);
    assert.equal(harness.resets, 1);
    harness.setPads([harness.pad]);
    harness.vibration.play(pulse(750, 0.75, 0));
    harness.setPads([{ ...harness.pad, id: 'Different controller' }]);
    harness.tick(750);
    assert.equal(harness.resets, 2);
    harness.document.hidden = true;
    harness.vibration.play(pulse(100, 0, 0.5));
    assert.equal(harness.calls.length, 2);
    assert.deepEqual(harness.deviceCalls, []);
});

test('controllers without haptics use cancellable device vibration; absent APIs are optional', t => {
    const harness = setup(t);
    Reflect.deleteProperty(harness.pad, 'vibrationActuator');
    harness.vibration.play(pulse(1250, 0.375, 0));
    assert.deepEqual(harness.deviceCalls, [1250]);
    harness.vibration.stop();
    assert.deepEqual(harness.deviceCalls, [1250, 0]);
    Reflect.deleteProperty(navigator, 'vibrate');
    Reflect.deleteProperty(navigator, 'getGamepads');
    harness.vibration.play(pulse(100, 0, 0.5));
    assert.deepEqual(harness.calls, []);
    assert.deepEqual(harness.errors, []);
});

test('preempted effects are normal; rejected or unsupported haptics report an error without rejecting gameplay', async t => {
    const harness = setup(t);
    t.mock.method(harness.actuator, 'playEffect', async () => 'preempted');
    harness.vibration.play(pulse(100, 0, 0.5));
    await Promise.resolve();
    assert.deepEqual(harness.errors, []);
    t.mock.method(harness.actuator, 'playEffect', () => Promise.reject(new Error('Bluetooth haptics unavailable')));
    harness.vibration.play(pulse(100, 0, 0.5));
    await Promise.resolve();
    assert.deepEqual(harness.errors, ['Controller rumble could not play: Bluetooth haptics unavailable']);
    assert.equal(harness.resets, 1);
    harness.vibration.play(pulse(100, 0, 0.5));
    assert.deepEqual(harness.deviceCalls, [100], 'a failed actuator is not retried on every dot');
});

test('synchronous play errors and reset rejections are reported without escaping into the game loop', async t => {
    const harness = setup(t);
    t.mock.method(harness.actuator, 'playEffect', () => { throw new Error('Permission denied'); });
    t.mock.method(harness.actuator, 'reset', () => Promise.reject(new Error('Controller disconnected')));
    harness.vibration.play(pulse(100, 0, 0.5));
    await Promise.resolve();
    assert.deepEqual(harness.errors, [
        'Controller rumble could not play: Permission denied',
        'Controller rumble could not stop: Controller disconnected',
    ]);
    harness.tick(2000);
    assert.equal(harness.errors.length, 2);
});

function fixture() {
    const game = new Game({ character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1 }, () => 0);
    const player = game['players'][0];
    const run = player.run;
    run.state = 'playing';
    game.drainEvents();
    const pulses = () => game.drainEvents().filter((event): event is VibrationEvent => event.type === 'vibrate');
    return { game, player, run, pulses };
}

test('gameplay rumble matches native Audio.cpp strengths and durations for every effect', async () => {
    const native = await readFile(new URL('../../maze/core/Audio.cpp', import.meta.url), 'utf8');
    const nativePulse = (effect: string) => {
        const match = native.match(new RegExp(`case ${effect}:[\\s\\S]*?\\.vibrate\\(([\\d.]+), ([\\d.]+), ([\\d.]+)\\)`));
        assert.ok(match, effect);
        return pulse(Number(match[3]) * 1000, Number(match[1]), Number(match[2]));
    };
    const { game, player, run, pulses } = fixture();
    game['eatDot'](player, run, false);
    assert.deepEqual(pulses(), [nativePulse('EFFECT_EAT_DOT1')]);
    game['eatDot'](player, run, false);
    assert.deepEqual(pulses(), [], 'the second dot has no pulse');
    for (let index = 0; index < 2; index++) {
        game['eatDot'](player, run, true);
        assert.deepEqual(pulses(), [nativePulse(index ? 'EFFECT_EAT_POWER2' : 'EFFECT_EAT_POWER1')]);
    }
    game['eatGhost'](player, run, run.ghosts[0]);
    assert.deepEqual(pulses(), [nativePulse('EFFECT_EAT_GHOST')]);
    game['eatFruit'](player, run);
    assert.deepEqual(pulses(), [nativePulse('EFFECT_EAT_FRUIT')]);
    Object.assign(run.fruit, { active: true, x: 84, y: 100, dx: 1, dy: 0 });
    game['advanceFruit'](run.fruit, run);
    assert.deepEqual(pulses(), [nativePulse('EFFECT_FRUIT_BOUNCE')]);
    run.state = 'dying';
    run.stateTick = 30;
    game.advance();
    assert.deepEqual(pulses(), [nativePulse('EFFECT_DYING')]);
});

for (const power of [false, true]) test(`the final ${power ? 'power pellet' : 'dot'} plays the native win effect without an eating pulse`, () => {
    const { game, player, run } = fixture();
    run.dotCount = 1;
    game['eatDot'](player, run, power);
    assert.deepEqual(game.drainEvents().filter(event => event.type === 'sound' || event.type === 'vibrate'),
        [{ type: 'sound', name: 'mr-level-win' }]);
});
