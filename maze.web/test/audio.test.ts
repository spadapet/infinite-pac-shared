import { test, type TestContext } from 'node:test';
import assert from 'node:assert/strict';
import { GameAudio } from '../src/audio';

class FakeGain {
    gain = { value: 1, setValueAtTime: (value: number) => { this.gain.value = value; } };
    connect(): void {}
    disconnect(): void {}
}
class FakeSource {
    buffer: unknown;
    loop = false;
    playbackRate = { value: 1 };
    onended?: () => void;
    started = false;
    stopped = false;
    connect(): void {}
    disconnect(): void {}
    start(): void { this.started = true; }
    stop(): void { this.stopped = true; this.onended?.(); }
}
class FakeContext {
    static instances: FakeContext[] = [];
    static options: AudioContextOptions[] = [];
    sources: FakeSource[] = [];
    gains: FakeGain[] = [];
    currentTime = 0;
    destination = {};
    suspended = false;
    constructor(options?: AudioContextOptions) {
        FakeContext.instances.push(this);
        if (options) FakeContext.options.push(options);
    }
    createGain(): FakeGain { const gain = new FakeGain(); this.gains.push(gain); return gain; }
    createBufferSource(): FakeSource { const source = new FakeSource(); this.sources.push(source); return source; }
    async resume(): Promise<void> { this.suspended = false; }
    async suspend(): Promise<void> { this.suspended = true; }
    async decodeAudioData(): Promise<object> { return { duration: 1 }; }
}
function setup(t: TestContext): void {
    const original = Object.getOwnPropertyDescriptor(globalThis, 'AudioContext');
    Object.defineProperty(globalThis, 'AudioContext', { configurable: true, value: FakeContext });
    FakeContext.instances = [];
    FakeContext.options = [];
    t.after(() => {
        if (original) Object.defineProperty(globalThis, 'AudioContext', original);
        else Reflect.deleteProperty(globalThis, 'AudioContext');
    });
    t.mock.method(globalThis, 'fetch', async () => new Response(new ArrayBuffer(8)));
}

test('audio waits for a gesture, decodes original WAVs, and preserves background rate and volume', async t => {
    setup(t);
    const audio = new GameAudio();
    audio.handle({ type: 'sound', name: 'mr-bg-3' });
    assert.equal(FakeContext.instances.length, 0);
    await audio.unlock();
    const context = FakeContext.instances[0];
    assert.equal(FakeContext.options[0]?.latencyHint, 'interactive');
    assert.equal(context.sources.length, 1);
    assert.equal(context.sources[0].started, true);
    assert.equal(context.sources[0].loop, true);
    assert.equal(context.sources[0].playbackRate.value, 1.25);
    assert.equal(context.gains[1].gain.value, 0.2);
    audio.handle({ type: 'sound', name: 'mr-bg-3' });
    assert.equal(context.sources.length, 1, 'loop requests must not restart a playing background');
    audio.handle({ type: 'sound', name: 'mr-bg-scared' });
    assert.equal(context.sources[0].stopped, true);
    assert.equal(context.sources[1].started, true);
    audio.setMuted(true);
    assert.equal(context.gains[0].gain.value, 0);
    audio.handle({ type: 'sound', name: 'mr-eat-fruit' });
    assert.equal(context.sources[2].started, true, 'muted playback still routes through the master gain');
    assert.equal(context.gains[0].gain.value, 0);
    audio.setMuted(false);
    assert.equal(context.gains[0].gain.value, 1);
    await audio.setPaused(true);
    assert.equal(context.suspended, true);
    await audio.unlock();
    assert.equal(context.suspended, true, 'toggling audio while paused must not resume the game sounds');
    await audio.setPaused(false);
    assert.equal(context.suspended, false);
    audio.stopAll();
    assert.equal(context.sources[1].stopped, true);
});

test('a decoded one-shot starts synchronously when its sound event is handled', async t => {
    setup(t);
    const audio = new GameAudio();
    await audio.unlock();
    const context = FakeContext.instances[0];
    audio.handle({ type: 'sound', name: 'mr-eat-ghost' });
    assert.equal(context.sources.length, 1);
    assert.equal(context.sources[0].started, true);
    assert.equal(context.sources[0].loop, false);
    audio.handle({ type: 'sound', name: 'mr-eat-fruit' });
    assert.equal(context.sources.length, 2);
    assert.equal(context.sources[1].started, true, 'fruit also starts in the same synchronous event handler');
    assert.equal(context.sources[1].loop, false);
});

test('audible one-shots before readiness fail explicitly instead of being replayed late; current background starts when ready', async t => {
    setup(t);
    let release: () => void = () => assert.fail('Load gate not initialized');
    const gate = new Promise<void>(resolve => { release = resolve; });
    t.mock.method(globalThis, 'fetch', async () => { await gate; return new Response(new ArrayBuffer(8)); });
    const audio = new GameAudio();
    const loading = audio.unlock();
    assert.throws(() => audio.handle({ type: 'sound', name: 'mr-eat-ghost' }), /before decoding completed/);
    assert.throws(() => audio.handle({ type: 'sound', name: 'mr-eat-fruit' }), /before decoding completed/);
    audio.handle({ type: 'sound', name: 'mr-bg-2' });
    release();
    await loading;
    const context = FakeContext.instances[0];
    assert.equal(context.sources.length, 1);
    assert.equal(context.sources[0].started, true);
    assert.equal(context.sources[0].loop, true);
    assert.equal(context.sources[0].playbackRate.value, 1.125);
});

test('stopAll during loading cancels the requested background without deferring one-shots', async t => {
    setup(t);
    let release: () => void = () => assert.fail('Load gate not initialized');
    const gate = new Promise<void>(resolve => { release = resolve; });
    t.mock.method(globalThis, 'fetch', async () => { await gate; return new Response(new ArrayBuffer(8)); });
    const audio = new GameAudio();
    const loading = audio.unlock();
    assert.throws(() => audio.handle({ type: 'sound', name: 'mr-intro' }), /before decoding completed/);
    audio.handle({ type: 'sound', name: 'mr-bg-1' });
    audio.stopAll();
    release();
    await loading;
    assert.equal(FakeContext.instances[0].sources.length, 0);
    audio.handle({ type: 'sound', name: 'mr-eat-dot-1' });
    audio.handle({ type: 'sound', name: 'mr-eat-dot-1' });
    const sources = FakeContext.instances[0].sources;
    assert.equal(sources.length, 2);
    assert.equal(sources[0].stopped, true);
    assert.equal(sources[1].started, true);
    audio.handle({ type: 'stopSound', name: 'mr-eat-dot-1' });
    assert.equal(sources[1].stopped, true);
});

test('audio load failures reject visibly and can be retried', async t => {
    setup(t);
    t.mock.method(globalThis, 'fetch', async () => new Response('', { status: 404 }));
    const audio = new GameAudio();
    await assert.rejects(audio.unlock(), /Could not load sound.*HTTP 404/);
    t.mock.method(globalThis, 'fetch', async () => new Response(new ArrayBuffer(8)));
    await audio.unlock();
    audio.handle({ type: 'sound', name: 'mr-intro' });
    assert.equal(FakeContext.instances[0].sources[0].started, true);
    assert.throws(() => audio.handle({ type: 'sound', name: 'missing-effect' }), /Unknown sound effect/);
});
