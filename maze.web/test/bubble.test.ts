import { test } from 'node:test';
import assert from 'node:assert/strict';
import { Container, Texture, TextureSource, Sprite } from 'pixi.js';
import animations from '../../maze/assets/Assets.Anim.res.json';
import { animationDefinition, nativeSprites, sampleAnimation } from '../src/native-animation';
import { Game } from '../src/game';
import { GameRenderer } from '../src/renderer';

const variants = [0, 1, 2] as const;
const bubbleAnimations = [animations['bubble-1-anim'], animations['bubble-2-anim'], animations['bubble-3-anim']];
const sampleBubbleAnimation = (variant: number, frame: number) => sampleAnimation(`bubble-${variant + 1}-anim`, frame)[0];

test('all three bubble animations sample the original JSON tracks, including linear midpoints', () => {
    for (const variant of variants) {
        const animation = bubbleAnimations[variant];
        assert.equal(animation.fps, 4);
        assert.equal(animation.length, 5);
        for (const key of animation.keys.position.values.filter(value => value.frame < animation.length)) {
            assert.deepEqual(sampleBubbleAnimation(variant, key.frame).position, { x: key.value[0], y: key.value[1] });
        }
        for (let index = 0; index < animation.keys.position.values.length - 1; index++) {
            const from = animation.keys.position.values[index];
            const to = animation.keys.position.values[index + 1];
            assert.deepEqual(sampleBubbleAnimation(variant, (from.frame + to.frame) / 2).position, {
                x: (from.value[0] + to.value[0]) / 2,
                y: (from.value[1] + to.value[1]) / 2,
            });
        }
        assert.deepEqual(sampleBubbleAnimation(variant, 0).scale, { x: 0.125, y: 0.125 });
        assert.deepEqual(sampleBubbleAnimation(variant, 4).scale, { x: 0.185, y: 0.185 });
        assert.equal(sampleBubbleAnimation(variant, 0).color[3], 0.75);
        assert.equal(sampleBubbleAnimation(variant, 4).color[3], 0.25);
        assert.deepEqual(sampleBubbleAnimation(variant, 2.5).scale, { x: 0.1625, y: 0.1625 });
        assert.equal(sampleBubbleAnimation(variant, 2.5).color[3], 0.4375);
        assert.deepEqual(sampleAnimation(`bubble-${variant + 1}-anim`, 5), []);
    }
});

test('renderer uses a textured 4-to-6.4-pixel bubble with the original handle and parent transform', () => {
    const texture = new Texture({ source: new TextureSource({ width: 32, height: 32 }) });
    const layer = new Container();
    const pool: Sprite[] = [];
    const renderer = Object.create(GameRenderer.prototype);
    Object.assign(renderer, {
        textures: { frames: new Map([['char-sprites.bubble', [texture]]]) },
        spriteDescriptors: new Map(), animationSamplers: new Map(),
        particleLayer: layer,
        bubblePool: pool,
        boardTop: 24,
    });
    const view = new Game({ character: 'mr', mazeSet: 'mr', difficulty: 'easy', players: 1 }, () => 0).view;
    view.playerReady = false;
    view.particles = [{ position: { x: 80, y: 100 }, animation: 0, frame: 0, scale: 1 }];
    const draw = Reflect.get(GameRenderer.prototype, 'drawParticles');
    try {
        Reflect.apply(draw, renderer, [view]);
        assert.equal(layer.children.length, 1);
        const sprite = pool[0];
        assert.ok(sprite instanceof Sprite);
        assert.equal(sprite.texture, texture);
        assert.equal(nativeSprites.get('char-sprites.bubble')?.file, 'bubble.png');
        assert.equal(sprite.width, 4);
        assert.equal(sprite.height, 4);
        assert.equal(sprite.anchor.x, 0.5);
        assert.equal(sprite.anchor.y, 14 / 32);
        assert.equal(sprite.x, 80);
        assert.equal(sprite.y, 124);
        assert.equal(sprite.alpha, 0.75);
        assert.equal(sprite.tint, 0xffffff);

        view.particles[0].frame = 2.5;
        view.particles[0].scale = 1.25;
        Reflect.apply(draw, renderer, [view]);
        assert.equal(sprite.width, 6.5);
        assert.equal(sprite.height, 6.5);
        assert.equal(sprite.x, 80);
        assert.equal(sprite.y, 111.5);
        assert.equal(sprite.alpha, 0.4375);
        assert.equal(layer.children.length, 1, 'reuse the sprite rather than allocate every frame');
        view.particles[0].frame = 5;
        Reflect.apply(draw, renderer, [view]);
        assert.equal(sprite.visible, false, 'the one-shot disappears exactly at its native end frame');

        view.particles = [];
        view.pac.powered = true;
        Reflect.apply(draw, renderer, [view]);
        assert.equal(sprite.visible, false, 'powered Pac must not invent looping attached bubbles');
        assert.equal(layer.children.length, 1);
    } finally {
        layer.destroy({ children: true });
        texture.destroy(true);
    }
});

test('bubbles remain alive at exactly the native animation length and expire only after it', () => {
    const game = new Game({ character: 'mr', mazeSet: 'mr', difficulty: 'easy', players: 1 }, () => 0);
    const run = game['players'][0].run;
    const animation = animationDefinition('bubble-1-anim');
    const timeScale = 1;
    const step = animation.fps * timeScale / 60;
    run.particles.push({ x: 0, y: 0, vy: 0, animation: 0, frame: animation.length - step, scale: 1, timeScale });
    game['advanceParticles'](run);
    assert.equal(run.particles[0]?.frame, animation.length);
    assert.equal(run.particles.length, 1, 'native SpriteAnimActor remains active at frame_length');
    game['advanceParticles'](run);
    assert.equal(run.particles.length, 0, 'the next frame exceeds frame_length and expires');
});

test('eating emits independent one-shot bubbles with native randomized scale, timing and drift', () => {
    for (const variant of variants) {
        const game = new Game({ character: 'mr', mazeSet: 'mr', difficulty: 'easy', players: 1 }, () => (variant + 0.5) / 3);
        for (let tick = 0; tick < 300 && game.view.state === 'ready'; tick++) game.advance();
        for (const row of game.view.maze.tiles) {
            for (const tile of row) if (tile.kind === 'dot' || tile.kind === 'power') tile.kind = 'path';
        }
        const pac = game.view.pac.position;
        const tileX = Math.floor((pac.x * 2 - 1) / 16);
        const tileY = Math.floor(pac.y / 8);
        game.view.maze.tiles[tileY][tileX].kind = 'dot';
        for (let tick = 0; tick < 10 && game.view.score === 0; tick++) game.advance();
        assert.equal(game.view.score, 10);
        const initial = game.view.particles;
        assert.equal(initial.length, 2);
        const random = (variant + 0.5) / 3;
        const scale = 1 + (Math.floor(random * 9) - 4) / 12;
        const velocity = Math.floor(random * 10) / -100;
        for (const bubble of initial) {
            assert.equal(bubble.animation, variant);
            assert.equal(bubble.scale, scale);
        }
        const firstPacX = game.view.pac.position.x;
        game.advance();
        const next = game.view.particles;
        assert.equal(next.length, 2);
        assert.equal(next[0].position.x, initial[0].position.x);
        assert.ok(Math.abs(next[0].position.y - initial[0].position.y - velocity) < 1e-12);
        assert.ok(Math.abs(next[0].frame - initial[0].frame - 4 * scale / 60) < 1e-12);
        for (let tick = 0; tick < 15; tick++) game.advance();
        assert.notEqual(game.view.pac.position.x, firstPacX);
        assert.equal(game.view.particles[0].position.x, initial[0].position.x, 'bubbles stay at the spawn location, not on Pac');
        for (let tick = 0; tick < 105; tick++) game.advance();
        assert.equal(game.view.particles.length, 0, 'all one-shot bubbles expire rather than repeat');
    }
});
