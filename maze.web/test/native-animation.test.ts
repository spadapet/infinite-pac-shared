import { test } from 'node:test';
import assert from 'node:assert/strict';
import { existsSync } from 'node:fs';
import { animationDefinition, nativeAnimations, nativeSprites, sampleAnimation } from '../src/native-animation';

const sprite = (name: string, frame: number) => sampleAnimation(name, frame)[0].sprite;

test('all 32 original character/effect/ghost resources expand templates and resolve original sprite references', () => {
    assert.equal(nativeAnimations.size, 32);
    assert.equal(nativeSprites.size, 286);
    for (const [name, animation] of nativeAnimations) {
        for (let frame = 0; frame < animation.length; frame += 0.25) {
            const visuals = sampleAnimation(name, frame);
            assert.ok(visuals.length > 0, `${name} has no visuals at ${frame}`);
            for (const visual of visuals) {
                const source = nativeSprites.get(visual.sprite);
                assert.ok(source, `${name}: unresolved sprite ${visual.sprite}`);
                assert.ok(existsSync(new URL(`../../maze/assets/${source.file}`, import.meta.url)), `Missing ${source.file}`);
                assert.ok([visual.position.x, visual.position.y, visual.scale.x, visual.scale.y, visual.rotation, ...visual.color].every(Number.isFinite));
                assert.equal(visual.color.length, 4);
            }
            if (animation.loop) assert.deepEqual(sampleAnimation(name, frame + animation.length), visuals);
        }
        if (!animation.loop) assert.deepEqual(sampleAnimation(name, animation.length), []);
    }
    assert.throws(() => animationDefinition('missing-animation'), /Unknown native animation/);
});

test('both character templates preserve the mouth sequence, held final frame, powered artwork and ten-frame loop', () => {
    const sequence = [0, 1, 2, 3, 4, 3, 2, 1, 0, 0];
    for (const character of ['mr', 'ms']) {
        for (const [animation, artwork] of [['char', 'move'], ['power', 'power']]) {
            const name = `${character}-${animation}-anim`;
            assert.equal(animationDefinition(name).fps, 45);
            for (let frame = 0; frame < sequence.length; frame++) {
                assert.equal(sprite(name, frame + 0.5), `char-sprites.${character}-${artwork}[${sequence[frame]}]`);
            }
            assert.equal(sprite(name, 10), `char-sprites.${character}-${artwork}[0]`);
        }
    }
});

test('ghost templates use native four/three/five-frame loops without an extra repeated endpoint', () => {
    for (const ghost of [1, 4]) {
        for (const kind of ['move', 'eyes', 'pupils', 'scared', 'flash']) {
            const sequence = kind === 'scared' || kind === 'flash' ? [0, 1, 2] : [0, 1, 2, 1];
            const name = `ghost-${ghost}-${kind}-anim`;
            assert.equal(animationDefinition(name).length, sequence.length);
            assert.equal(animationDefinition(name).fps, 15);
            for (let frame = 0; frame < sequence.length; frame++) {
                assert.equal(sprite(name, frame + 0.5), `char-sprites.ghost-${ghost}-${kind}[${sequence[frame]}]`);
            }
            assert.equal(sprite(name, sequence.length), `char-sprites.ghost-${ghost}-${kind}[0]`);
        }
    }
    for (const kind of ['move', 'scared', 'flash']) {
        const name = `ghost-2-${kind}-anim`;
        assert.equal(animationDefinition(name).length, 5);
        for (let frame = 0; frame < 5; frame++) assert.equal(sprite(name, frame + 0.5), `char-sprites.ghost-2-${kind}[${frame}]`);
        assert.equal(sprite(name, 5), `char-sprites.ghost-2-${kind}[0]`);
    }
});

test('third ghost preserves three separately rotating arms behind the body in all body states', () => {
    for (const state of ['move', 'scared', 'flash']) {
        const visuals = sampleAnimation(`ghost-3-${state}-anim`, 0.25);
        assert.deepEqual(visuals.map(value => value.sprite), [
            `char-sprites.ghost-3-${state}[1]`, `char-sprites.ghost-3-${state}[1]`,
            `char-sprites.ghost-3-${state}[1]`, `char-sprites.ghost-3-${state}[0]`,
        ]);
        assert.deepEqual(visuals.map(value => value.rotation), [-90, -150, -210, 0]);
    }
});

test('pellet and aura use original linear color/scale/rotation tracks, not sine or visual-level spline approximations', () => {
    const pellet = sampleAnimation('power-anim', 0.5)[0];
    assert.deepEqual(pellet.scale, { x: 3.5, y: 3.5 });
    assert.deepEqual(pellet.color, [1, 1, 1, 0.75]);
    assert.equal(pellet.rotation, -90);
    const aura = sampleAnimation('power-aura-anim', 0.5)[0];
    assert.deepEqual(aura.scale, { x: 5, y: 5 });
    assert.deepEqual(aura.color, [1, 1, 1, 0.4375]);
    assert.equal(aura.rotation, -45);
    for (const character of ['mr', 'ms']) {
        const death = sampleAnimation(`${character}-death-anim`, 5.5)[0];
        assert.equal(death.sprite, `char-sprites.${character}-dead`);
        assert.equal(death.rotation, -360);
        assert.deepEqual(death.scale, { x: 4, y: 4 });
    }
});

test('sprite definitions retain repeat offsets, off-center handles, signed wall scale and fruit scale', () => {
    assert.deepEqual(nativeSprites.get('char-sprites.power[0]')?.handle, { x: 16, y: 14 });
    assert.deepEqual(nativeSprites.get('char-sprites.bubble')?.handle, { x: 16, y: 14 });
    assert.deepEqual(nativeSprites.get('wall-border-sprites.10'), {
        name: 'wall-border-sprites.10', file: 'edges.png', rect: { x: 38, y: 2, width: 16, height: 16 },
        handle: { x: 16, y: 0 }, scale: { x: -4, y: 4 },
    });
    assert.deepEqual(nativeSprites.get('char-sprites.fruit[0]')?.scale, { x: 4, y: 4 });
    const first = nativeSprites.get('char-sprites.mr-move[0]');
    const second = nativeSprites.get('char-sprites.mr-move[1]');
    assert.ok(first && second);
    assert.equal(second.rect.x - first.rect.x, first.rect.width);
    assert.equal(second.rect.y, first.rect.y);
});
