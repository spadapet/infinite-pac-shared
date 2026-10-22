import { test } from 'node:test';
import assert from 'node:assert/strict';
import animations from '../../maze/assets/Assets.Anim.res.json';
import { sampleDotAnimation } from '../src/renderer';

test('dot animation matches native position and color keys at one-eighth coordinate scale', () => {
    const animation = animations['dot-anim'];
    for (const key of animation.keys.position.values) {
        const tick = key.frame / animation.fps * 60;
        assert.equal(sampleDotAnimation(tick, 0, 0).offsetY, key.value[1] * 0.125);
    }
    for (const key of animation.keys.color.values) {
        const tick = key.frame / animation.fps * 60;
        assert.equal(sampleDotAnimation(tick, 0, 0).brightness, key.value[0]);
    }
    const samples = Array.from({ length: 321 }, (_, tick) => sampleDotAnimation(tick, 0, 0).offsetY);
    assert.equal(Math.min(...samples), -1);
    assert.equal(Math.max(...samples), 1);
    assert.equal(sampleDotAnimation(40, 0, 0).offsetY, -1);
    assert.equal(sampleDotAnimation(120, 0, 0).offsetY, 1);
    for (let tick = 0; tick < 160; tick++) {
        const first = sampleDotAnimation(tick, 0, 0);
        const repeated = sampleDotAnimation(tick + 160, 0, 0);
        assert.ok(Math.abs(first.offsetY - repeated.offsetY) < 1e-12);
        assert.ok(Math.abs(first.brightness - repeated.brightness) < 1e-12);
    }
});

test('dot wave uses native integer tile division for both position and brightness', () => {
    for (let tick = 0; tick < 160; tick++) {
        assert.deepEqual(sampleDotAnimation(tick, 1, 1), sampleDotAnimation(tick, 0, 0));
        assert.deepEqual(sampleDotAnimation(tick, 3, 1), sampleDotAnimation(tick, 2, 0));
        assert.deepEqual(sampleDotAnimation(tick, 0, 2), sampleDotAnimation(tick, 2, 0));
    }
    assert.equal(sampleDotAnimation(0, 2, 0).offsetY, -1);
    assert.equal(sampleDotAnimation(0, 2, 2).brightness, 0.625);
});
