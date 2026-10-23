import { test, type TestContext } from 'node:test';
import assert from 'node:assert/strict';
import { Container, Graphics, Rectangle, Sprite, Texture, TextureSource } from 'pixi.js';
import { Game, parseMazeResource } from '../src/game';
import { AnimationSampler, animationDefinition, nativeSprites, sampleAnimation } from '../src/native-animation';
import { GameRenderer } from '../src/renderer';

function graphicsFillAlpha(graphics: Graphics): unknown {
    const instruction = graphics.context.instructions[0];
    assert.ok(instruction);
    const style = instruction.data.style;
    assert.ok(typeof style === 'object' && style !== null && 'alpha' in style);
    return graphics.visible ? Number(style.alpha) * graphics.alpha : 0;
}

function setup(t: TestContext) {
    const source = new TextureSource({ width: 2048, height: 2048 });
    const frames = new Map<string, Texture[]>();
    const textures: Texture[] = [];
    for (const [name, def] of nativeSprites) {
        const match = /^(.*)\[(\d+)\]$/.exec(name);
        const group = match?.[1] ?? name;
        const { x, y, width, height } = def.rect;
        const texture = new Texture({ source, frame: new Rectangle(x, y, width, height) });
        const array = frames.get(group) ?? [];
        array[match ? Number(match[2]) : 0] = texture;
        frames.set(group, array);
        textures.push(texture);
    }
    const actorPool: Sprite[] = [], dotPool: Sprite[] = [], livesPool: Sprite[] = [], fruitPool: Sprite[] = [], pointTextPool: Sprite[] = [], joystickPool: Sprite[] = [];
    const screenDotPool: Sprite[] = [], screenActorPool: Sprite[] = [], screenTextPool: Sprite[] = [];
    const actorLayer = new Container(), dotLayer = new Container(), wallLayer = new Container(), hudLayer = new Container(), overlayLayer = new Container(), joystickLayer = new Container();
    const screenLayer = new Container(), screenWallLayer = new Container(), screenDotLayer = new Container(), screenActorLayer = new Container(), screenTextLayer = new Container();
    const background = new Graphics(), screenWash = new Graphics(), screenFade = new Graphics(), pauseFade = new Graphics();
    screenLayer.addChild(screenWash, screenWallLayer, screenDotLayer, screenActorLayer, screenTextLayer, screenFade);
    overlayLayer.addChild(pauseFade);
    const renderer: GameRenderer = Object.create(GameRenderer.prototype);
    Object.assign(renderer, { textures: { frames }, boardTop: 24, layoutWidth: 224, layoutHeight: 288,
        spriteDescriptors: new Map(), animationSamplers: new Map(),
        dotCaches: new WeakMap(), glyphCaches: new WeakMap(), rectangles: new WeakMap(), scoreValues: [], scoreTexts: [],
        dotSamplers: [], powerSampler: new AnimationSampler('power-anim'),
        actorPool, actorLayer, dotPool, dotLayer, wallLayer, background, livesPool, fruitPool, pointTextPool, hudLayer, hudTextPool: [], joystickPool, joystickLayer,
        screenDotPool, screenActorPool, screenTextPool, screenLayer, screenWallLayer, screenDotLayer, screenActorLayer, screenTextLayer, screenWash, screenFade, pauseFade,
        overlayLayer, overlayTextPool: [], particleLayer: new Container(), bubblePool: [], destroyed: false, contextLost: false, mazeCache: null });
    const draw = (method: string, ...args: unknown[]) => Reflect.apply(Reflect.get(GameRenderer.prototype, method), renderer, args);
    const game = new Game({ character: 'mr', mazeSet: 'mr', difficulty: 'easy', players: 1 }, () => 0);
    const view = game.view;
    view.state = 'playing';
    view.playerReady = false;
    view.beforeReady = false;
    view.pac.visible = false;
    view.ghosts = [];
    t.after(() => {
        for (const layer of [actorLayer, dotLayer, wallLayer, hudLayer, overlayLayer, joystickLayer, screenLayer, renderer['particleLayer']]) layer.destroy({ children: true });
        background.destroy();
        for (const texture of textures) texture.destroy();
        source.destroy();
    });
    return { renderer, draw, view, frames, actorPool, dotPool, livesPool, fruitPool, pointTextPool, wallLayer, joystickPool, screenActorPool, screenDotPool, screenWallLayer };
}

test('all sixteen native maze variants render every wall/door through the original three-layer patterns', t => {
    const { draw, view, wallLayer } = setup(t);
    for (const set of ['mr', 'ms']) {
        for (let index = 0; index < 4; index++) {
            for (const variant of [0, 1]) {
                const maze = parseMazeResource(`${set}-maze-${index}-${variant}`);
                draw('rebuildStaticMaze', maze, false);
                const wallCount = maze.tiles.flat().filter(tile => ['wall', 'ghostWall', 'door'].includes(tile.kind)).length;
                assert.equal(wallLayer.children.length, wallCount * 3 + 1, maze.id);
                for (const child of wallLayer.children.slice(1)) {
                    assert.ok(child instanceof Sprite);
                    assert.equal(child.width, 8);
                    assert.equal(child.height, 8);
                    assert.ok(Math.abs(child.scale.x) === 0.5 && Math.abs(child.scale.y) === 0.5);
                }
                view.maze = maze;
                draw('drawDots', view);
            }
        }
    }
});

test('dots share exact-phase samples and retain sprite and descriptor identities over a full cycle', t => {
    const { renderer, draw, view, dotPool } = setup(t);
    draw('drawDots', view);
    const sprites = [...dotPool];
    const descriptors = [...renderer['spriteDescriptors'].entries()];
    const samplers = renderer['dotSamplers'].filter(sampler => sampler !== undefined);
    const phases = new Set<number>();
    for (let y = 0; y < view.maze.height; y++) for (let x = 0; x < view.maze.width; x++) {
        if (view.maze.tiles[y][x].kind === 'dot') phases.add(Math.floor(x / 2) + Math.floor(y / 2));
    }
    assert.equal(samplers.length, phases.size);
    assert.ok(samplers.length + 1 < sprites.length / 5, 'sample groups reduce actual sampling by more than 80%');
    const samples = [...samplers, renderer['powerSampler']].map(sampler => t.mock.method(sampler, 'sample'));
    const dot = animationDefinition('dot-anim');
    const power = animationDefinition('power-anim');
    for (let tick = 0; tick <= 160; tick++) {
        view.animation.dotTicks = tick;
        draw('drawDots', view);
        let index = 0;
        for (let y = 0; y < view.maze.height; y++) for (let x = 0; x < view.maze.width; x++) {
            const tile = view.maze.tiles[y][x];
            if (tile.kind !== 'dot' && tile.kind !== 'power') continue;
            const isPower = tile.kind === 'power';
            const animation = isPower ? power : dot;
            const frame = tick * animation.fps / 60 + (isPower ? animation.length / 2 : Math.floor(x / 2) + Math.floor(y / 2));
            const expected = sampleAnimation(isPower ? 'power-anim' : 'dot-anim', frame)[0];
            const sprite = dotPool[index];
            assert.equal(sprite, sprites[index++]);
            assert.ok(Math.abs(sprite.x - (x * 8 + 4 + expected.position.x * 0.125)) < 1e-10);
            assert.ok(Math.abs(sprite.y - (24 + y * 8 + 4 + expected.position.y * 0.125)) < 1e-10);
            assert.ok(Math.abs(sprite.rotation - expected.rotation * Math.PI / 180) < 1e-10);
            assert.ok(Math.abs(sprite.alpha - expected.color[3]) < 1e-10);
            assert.ok(Math.abs(sprite.scale.x - expected.scale.x * 0.125 * nativeSprites.get(expected.sprite)!.scale.x) < 1e-10);
            assert.equal(sprite.tint, Math.round(expected.color[0] * 255) << 16 | Math.round(expected.color[1] * 255) << 8 | Math.round(expected.color[2] * 255));
        }
        assert.equal(index, sprites.length);
        for (const sample of samples) assert.equal(sample.mock.callCount(), tick, 'unchanged frame zero is already sampled');
        assert.equal(renderer['spriteDescriptors'].size, descriptors.length);
        for (const [name, descriptor] of descriptors) assert.equal(renderer['spriteDescriptors'].get(name), descriptor);
    }
});

test('transparent native maze colors remain transparent in wall layers', t => {
    const { draw, view, wallLayer } = setup(t);
    const maze = { ...view.maze, borderColor: 0x00112233, fillColor: 0x00445566, backgroundColor: 0x00778899 };
    draw('rebuildStaticMaze', maze, false);

    let spriteIndex = 1;
    for (const tile of maze.tiles.flat()) {
        if (!['wall', 'ghostWall', 'door'].includes(tile.kind)) continue;
        const [background, fill, border] = wallLayer.children.slice(spriteIndex, spriteIndex + 3) as Sprite[];
        const door = tile.kind === 'door';
        assert.deepEqual([background.alpha, fill.alpha, border.alpha], door ? [0, 1, 1] : [0, 0, 0]);
        spriteIndex += 3;
    }
    assert.equal(spriteIndex, wallLayer.children.length);
});

test('pellets honor off-center handles, linear native pulse, rotation and one-eighth maze scale', t => {
    const { draw, view, dotPool, frames } = setup(t);
    draw('drawDots', view);
    const power = dotPool.find(sprite => sprite.texture === frames.get('char-sprites.power')?.[0]);
    assert.ok(power);
    assert.equal(power.width, 16);
    assert.equal(power.height, 16);
    assert.equal(power.anchor.y, 14 / 32);
    assert.equal(power.alpha, 1);
    assert.equal(power.rotation, -Math.PI);
    view.animation.dotTicks = 15;
    draw('drawDots', view);
    assert.equal(power.width, 14);
    assert.equal(power.alpha, 0.75);
    assert.equal(power.rotation, -1.5 * Math.PI);
    view.state = 'dead';
    view.stateTick = 20;
    draw('drawDots', view);
    assert.ok(dotPool.every(sprite => !sprite.visible));
});

test('ghost pupils use the queued eye direction at one pixel and actor visibility follows native states', t => {
    const { draw, view, actorPool, frames } = setup(t);
    view.ghosts = [{ index: 0, state: 'chase', direction: 'left', eyeDirection: 'up', position: { x: 40, y: 40 } }];
    draw('drawActors', view);
    assert.equal(actorPool[0].texture, frames.get('char-sprites.ghost-1-move')?.[0]);
    assert.equal(actorPool[1].texture, frames.get('char-sprites.ghost-1-pupils')?.[0]);
    assert.deepEqual({ x: actorPool[0].x, y: actorPool[0].y }, { x: 40, y: 64 });
    assert.deepEqual({ x: actorPool[1].x, y: actorPool[1].y }, { x: 40, y: 63 });
    assert.equal(actorPool[0].width, 16);
    for (const state of ['dying', 'dead', 'winning', 'gameover'] as const) {
        view.state = state;
        draw('drawActors', view);
        assert.ok(actorPool.every(sprite => !sprite.visible), state);
    }
    view.state = 'ready';
    view.playerReady = true;
    draw('drawActors', view);
    assert.ok(actorPool.every(sprite => !sprite.visible));
});

test('fruit retains the original scale, bounces at tile centers only when moving, and scores have source-specific colors', t => {
    const { draw, view, actorPool, pointTextPool } = setup(t);
    view.fruit = { type: 0, position: { x: 84, y: 100 }, moving: false };
    draw('drawActors', view);
    assert.equal(actorPool[0].width, 16);
    assert.equal(actorPool[0].y, 124);
    view.fruit.moving = true;
    draw('drawActors', view);
    assert.equal(actorPool[0].y, 126);
    view.points = [
        { position: { x: 40, y: 40 }, value: 200, remaining: 54, color: 0x00ffff },
        { position: { x: 80, y: 40 }, value: 100, remaining: 120, color: 0xffb8ff },
    ];
    draw('drawHud', view);
    assert.deepEqual(pointTextPool.map(sprite => sprite.tint), [0x00ffff, 0x00ffff, 0x00ffff, 0xffb8ff, 0xffb8ff, 0xffb8ff]);
    view.state = 'caught';
    draw('drawHud', view);
    assert.ok(pointTextPool.every(sprite => !sprite.visible));
});

test('status icons use original animation/sprite transforms and retain native sizes, caps and positions', t => {
    const { draw, view, livesPool, fruitPool } = setup(t);
    view.lives = 10;
    view.fruitHistory = [0, 1];
    draw('drawHud', view);
    assert.equal(livesPool.length, 5);
    assert.ok(livesPool.every(sprite => sprite.width === 14 && sprite.height === 14));
    assert.deepEqual(livesPool.map(sprite => sprite.x), [24, 40, 56, 72, 88]);
    assert.deepEqual(livesPool.map(sprite => sprite.y), [280, 280, 280, 280, 280]);
    assert.ok(fruitPool.every(sprite => sprite.width === 14 && sprite.height === 14));
    assert.deepEqual(fruitPool.map(sprite => sprite.x), [200, 184]);
    assert.deepEqual(fruitPool.map(sprite => sprite.y), [280, 280]);
});

test('canvas height contains exactly two native status tiles', t => {
    const oldWindow = Object.getOwnPropertyDescriptor(globalThis, 'window');
    Object.defineProperty(globalThis, 'window', { configurable: true, value: { devicePixelRatio: 1 } });
    t.after(() => { if (oldWindow) Object.defineProperty(globalThis, 'window', oldWindow); else Reflect.deleteProperty(globalThis, 'window'); });
    const renderer = Object.create(GameRenderer.prototype) as GameRenderer;
    Object.assign(renderer, {
        app: { render() {} }, destroyed: false, contextLost: false, layoutWidth: 0, layoutHeight: 0, displayPixelRatio: 0,
        wallLayer: { visible: true }, dotLayer: { visible: true }, actorLayer: { visible: true }, particleLayer: { visible: true },
        hudLayer: { visible: true }, overlayLayer: { visible: true }, joystickLayer: { visible: true }, screenLayer: { visible: true },
        resizeCanvas() {}, rebuildStaticMaze() {}, drawDots() {}, drawActors() {},
        drawParticles() {}, drawHud() {}, drawOverlay() {}, drawJoystick() {},
    });
    const view = new Game({ character: 'mr', mazeSet: 'mr', difficulty: 'easy', players: 1 }, () => 0).view;
    renderer.render(view, false);
    assert.deepEqual([renderer['layoutWidth'], renderer['layoutHeight']], [224, 288]);
});

test('native title and score screens use full-canvas dimensions, layers, cursor orientation and supplied text coordinates', t => {
    const oldWindow = Object.getOwnPropertyDescriptor(globalThis, 'window');
    Object.defineProperty(globalThis, 'window', { configurable: true, value: { devicePixelRatio: 1 } });
    t.after(() => { if (oldWindow) Object.defineProperty(globalThis, 'window', oldWindow); else Reflect.deleteProperty(globalThis, 'window'); });
    const { renderer, view, screenWallLayer } = setup(t);
    const drawAnimationCalls: unknown[][] = [];
    const textCalls: unknown[][] = [];
    Object.assign(renderer, {
        app: { render() {}, canvas: { getBoundingClientRect: () => ({ left: 20, top: 30, width: 224, height: 288 }) } },
        displayPixelRatio: 1,
        resizeCanvas() {},
        drawAnimation(name: string, frame: number, x: number, y: number, scaleX: number, scaleY: number, _pool: Sprite[], _parent: Container, used: number) {
            drawAnimationCalls.push([name, frame, { x, y }, { x: scaleX, y: scaleY }]);
            return used;
        },
        drawBitmapText(text: string, x: number, y: number, color: number, _pool: Sprite[], count = 0, _parent?: Container, scale = 1, alpha = 1) {
            textCalls.push([text, x, y, color, scale, alpha]);
            return count + text.length;
        },
    });
    const screen = {
        kind: 'title' as const, maze: view.maze, background: view,
        texts: [{ text: 'TITLE', x: 48, y: 16, color: 0xffffff, alpha: 0.75 }],
        pac: { x: 104, y: 120, character: 'ms' as const, frame: 3 }, fade: 0.25,
    };
    renderer.renderScreen(screen);
    assert.deepEqual([renderer['layoutWidth'], renderer['layoutHeight'], renderer['boardTop']], [224, 248, 0]);
    assert.deepEqual(drawAnimationCalls.at(-1), ['ms-char-anim', 3, { x: 104, y: 120 }, { x: -0.125, y: 0.125 }]);
    assert.deepEqual(textCalls.at(-1), ['TITLE', 48, 16, 0xffffff, 1, 0.75]);
    assert.equal(renderer['screenLayer'].visible, true);
    assert.ok(screenWallLayer.children.length > 1);
    const cachedWallSprite = screenWallLayer.children[1];
    renderer.renderScreen(screen);
    assert.equal(screenWallLayer.children[1], cachedWallSprite, 'static screen maze sprites are reused across frames');
    assert.ok(renderer['screenDotPool'].some(sprite => sprite.visible), 'foreground dots use shared phase samples');
    assert.equal(graphicsFillAlpha(renderer['screenWash']), 0.75);
    assert.equal(graphicsFillAlpha(renderer['screenFade']), 0.25);
    assert.deepEqual(renderer.screenPoint(132, 174), { x: 112, y: 124 }, 'screen coordinates retain the full canvas origin, including score-area pixels');
    assert.equal(renderer.screenPoint(19, 30), null);

    drawAnimationCalls.length = 0;
    textCalls.length = 0;
    renderer.renderScreen({ ...screen, kind: 'score', background: undefined, texts: [{ text: 'ENTER NAME', x: 72, y: 144, color: 0xffffff }] });
    assert.deepEqual([renderer['layoutWidth'], renderer['layoutHeight'], renderer['boardTop']], [224, 288, 24]);
    assert.deepEqual(textCalls.at(-1), ['ENTER NAME', 72, 144, 0xffffff, 1, 1]);
    assert.equal(renderer['wallLayer'].visible, false);
    assert.deepEqual(renderer.screenPoint(132, 174), { x: 112, y: 144 });
    assert.equal(renderer.screenToGame(132, 174), null);
});

test('title composition caches distinct foreground and background wall sprites across animation frames', t => {
    const oldWindow = Object.getOwnPropertyDescriptor(globalThis, 'window');
    Object.defineProperty(globalThis, 'window', { configurable: true, value: { devicePixelRatio: 1 } });
    t.after(() => { if (oldWindow) Object.defineProperty(globalThis, 'window', oldWindow); else Reflect.deleteProperty(globalThis, 'window'); });
    const { renderer, view, wallLayer, screenWallLayer } = setup(t);
    Object.assign(renderer, { app: { render() {} }, displayPixelRatio: 1, resizeCanvas() {} });
    const screen = {
        kind: 'title' as const, maze: parseMazeResource('title-maze-front'), background: view,
        texts: [{ text: 'START GAME', x: 48, y: 16, color: 0xffffff }],
        pac: { x: 40, y: 32, character: 'mr' as const, frame: 0 }, fade: 0,
    };
    assert.notEqual(screen.maze.id, view.maze.id);
    const buildLayers: Container[] = [];
    const renderMazeLayer = renderer['renderMazeLayer'].bind(renderer);
    renderer['renderMazeLayer'] = (maze, layer, winningFlash) => {
        buildLayers.push(layer);
        renderMazeLayer(maze, layer, winningFlash);
    };
    renderer.renderScreen(screen);
    assert.deepEqual(buildLayers, [wallLayer, screenWallLayer]);
    const backgroundSprites = [...wallLayer.children];
    const foregroundSprites = [...screenWallLayer.children];
    assert.ok(backgroundSprites.length > 1);
    assert.ok(foregroundSprites.length > 1);
    for (let frame = 1; frame <= 60; frame++) {
        view.animation.dotTicks = frame;
        view.animation.ghostTicks = frame;
        renderer.renderScreen({ ...screen, pac: { ...screen.pac, frame: frame % 4 } });
        assert.equal(wallLayer.children.length, backgroundSprites.length);
        assert.equal(screenWallLayer.children.length, foregroundSprites.length);
        backgroundSprites.forEach((sprite, index) => assert.equal(wallLayer.children[index], sprite));
        foregroundSprites.forEach((sprite, index) => assert.equal(screenWallLayer.children[index], sprite));
    }
    assert.deepEqual(buildLayers, [wallLayer, screenWallLayer], 'each static maze is built exactly once, not alternated through one cache');
    assert.equal(renderer['mazeCache']?.id, view.maze.id);
    assert.equal(renderer['screenMazeCache']?.id, screen.maze.id);
});

test('score entry dims complete pushed gameplay below its foreground and preserves full-canvas coordinates', t => {
    const oldWindow = Object.getOwnPropertyDescriptor(globalThis, 'window');
    Object.defineProperty(globalThis, 'window', { configurable: true, value: { devicePixelRatio: 1 } });
    t.after(() => { if (oldWindow) Object.defineProperty(globalThis, 'window', oldWindow); else Reflect.deleteProperty(globalThis, 'window'); });
    const { renderer, view, actorPool, livesPool, screenActorPool, screenWallLayer } = setup(t);
    Object.assign(renderer, {
        app: { render() {}, canvas: { getBoundingClientRect: () => ({ left: 20, top: 30, width: 448, height: 576 }) } },
        displayPixelRatio: 1,
        resizeCanvas() {},
    });
    view.pac.visible = true;
    view.pac.position = { x: 100, y: 120 };
    const screen = {
        kind: 'score' as const, maze: parseMazeResource('high-score-maze'), background: view,
        texts: [{ text: 'NAME', x: 72, y: 144, color: 0xffffff }],
        pac: { x: 52, y: 184, character: 'mr' as const, frame: 0 }, fade: 1,
    };
    renderer.renderScreen(screen);
    assert.deepEqual([renderer['boardTop'], renderer['layoutWidth'], renderer['layoutHeight']], [24, screen.maze.width * 8, screen.maze.height * 8 + 40]);
    assert.equal(renderer['hudLayer'].visible, true);
    assert.equal(renderer['overlayLayer'].visible, true);
    assert.ok(renderer['hudTextPool'].length > 0);
    assert.ok(livesPool.length > 0);
    assert.equal(actorPool[0].y, 144, 'background Pac retains normal gameplay score-area translation');
    assert.deepEqual([screenActorPool[0].x, screenActorPool[0].y], [52, 184]);
    assert.deepEqual([renderer['screenTextPool'][0].x, renderer['screenTextPool'][0].y], [72, 144]);
    assert.equal(graphicsFillAlpha(renderer['screenWash']), 0.875);
    assert.deepEqual(renderer['screenLayer'].children, [renderer['screenWash'], screenWallLayer, renderer['screenDotLayer'], renderer['screenActorLayer'], renderer['screenTextLayer'], renderer['screenFade']]);
    assert.equal(renderer['screenFade'].visible, false, 'title fade must not darken score-entry foreground');
    assert.deepEqual(renderer.screenPoint(164, 318), { x: 72, y: 144 });
    assert.deepEqual(renderer.screenPoint(20, 30), { x: 0, y: 0 });

    const cachedForeground = [...screenWallLayer.children];
    const originalDrawScreenMaze = renderer['drawScreenMaze'].bind(renderer);
    let foregroundDraws = 0;
    renderer['drawScreenMaze'] = maze => {
        foregroundDraws++;
        originalDrawScreenMaze(maze);
    };
    for (const backgroundFade of [0, 1 / 64, 0.875 - 1 / 64]) {
        renderer.renderScreen({ ...screen, backgroundFade, showForeground: false });
        assert.equal(graphicsFillAlpha(renderer['screenWash']), backgroundFade);
        assert.equal(renderer['hudLayer'].visible, true);
        assert.equal(renderer['wallLayer'].visible, true);
        for (const layer of [screenWallLayer, renderer['screenDotLayer'], renderer['screenActorLayer'], renderer['screenTextLayer']]) {
            assert.equal(layer.visible, false);
        }
        assert.equal(foregroundDraws, 0, 'foreground rendering is skipped while the pushed game fades');
        assert.deepEqual(renderer.screenPoint(164, 318), { x: 72, y: 144 });
    }
    renderer.renderScreen({ ...screen, backgroundFade: 0.875, showForeground: true });
    assert.equal(foregroundDraws, 1);
    assert.equal(graphicsFillAlpha(renderer['screenWash']), 0.875);
    for (const layer of [screenWallLayer, renderer['screenDotLayer'], renderer['screenActorLayer'], renderer['screenTextLayer']]) {
        assert.equal(layer.visible, true);
    }
    assert.deepEqual(screenWallLayer.children, cachedForeground, 'foreground returns using its existing maze cache');
    for (const backgroundFade of [0.875, 0.875 - 1 / 64, 1 / 64, 0]) {
        renderer.renderScreen({ ...screen, backgroundFade, showForeground: false });
        assert.equal(graphicsFillAlpha(renderer['screenWash']), backgroundFade);
        assert.equal(renderer['screenActorLayer'].visible, false, 'ending score entry hides the cursor immediately');
        assert.equal(renderer['screenTextLayer'].visible, false);
        assert.equal(screenWallLayer.visible, false);
        assert.equal(foregroundDraws, 1);
    }

    renderer.renderScreen({ ...screen, kind: 'title', fade: 1 });
    assert.equal(renderer['hudLayer'].visible, false);
    assert.equal(renderer['overlayLayer'].visible, false);
    assert.equal(actorPool[0].y, 120);
    assert.equal(graphicsFillAlpha(renderer['screenWash']), 0.75);
    assert.equal(graphicsFillAlpha(renderer['screenFade']), 1);
    for (const fade of [1 - 1 / 64, 1 / 64, 0, 1 / 64, 1]) {
        renderer.renderScreen({ ...screen, kind: 'title', fade });
        if (fade === 0) assert.equal(renderer['screenFade'].visible, false);
        else assert.equal(graphicsFillAlpha(renderer['screenFade']), fade);
    }
    renderer.renderScreen(screen);
    assert.equal(actorPool[0].y, 144, 'returning from title invalidates the background maze offset');
    assert.equal(renderer['mazeCache']?.boardTop, 24);
    renderer.renderScreen({ ...screen, background: undefined });
    assert.equal(renderer['hudLayer'].visible, false);
    assert.equal(renderer['overlayLayer'].visible, false);
    assert.equal(renderer['wallLayer'].visible, false);
    assert.equal(renderer['screenWash'].visible, false);
});

test('bitmap text supports native pause scaling, alpha, and line breaks', t => {
    const { draw } = setup(t);
    const pool: Sprite[] = [];
    const parent = new Container();
    try {
        const used = draw('drawBitmapText', 'A\nB', 10, 20, 0xff0000, pool, 0, parent, 2, 0.5) as number;
        assert.equal(used, 2);
        assert.deepEqual(pool.map(sprite => ({ x: sprite.x, y: sprite.y, tint: sprite.tint, alpha: sprite.alpha, width: sprite.width })), [
            { x: 10, y: 20, tint: 0xff0000, alpha: 0.5, width: 16 },
            { x: 10, y: 36, tint: 0xff0000, alpha: 0.5, width: 16 },
        ]);
    } finally {
        parent.destroy({ children: true });
    }
});

test('pause overlay uses a half-black wash and native three-layer scaled text at the ghost start tile', t => {
    const { draw, renderer, view } = setup(t);
    const labels: Array<{ text: string; x: number; y: number; tint: number; scale: number; alpha: number }> = [];
    Object.assign(renderer, { overlayTextPool: [], layoutWidth: 224, layoutHeight: 288, boardTop: 24 });
    renderer['drawBitmapText'] = (text, x, y, tint, _pool, count = 0, _parent, scale = 1, alpha = 1) => {
        labels.push({ text, x, y, tint, scale, alpha });
        return count + text.length;
    };
    const doorRow = view.maze.tiles.findIndex(row => row.some(tile => tile.kind === 'door'));
    const doorColumn = view.maze.tiles[doorRow].findIndex(tile => tile.kind === 'door');
    draw('drawOverlay', view, true);
    assert.deepEqual(labels, [
        { text: 'PAUSED', x: (doorColumn - 5) * 8 - 0.5, y: (doorRow + 3) * 8 + 23.5, tint: 0, scale: 2, alpha: 1 },
        { text: 'PAUSED', x: (doorColumn - 5) * 8 + 0.5, y: (doorRow + 3) * 8 + 24.5, tint: 0xffffff, scale: 2, alpha: 1 },
        { text: 'PAUSED', x: (doorColumn - 5) * 8, y: (doorRow + 3) * 8 + 24, tint: 0xff0000, scale: 2, alpha: 0.5 },
    ]);
    assert.equal(graphicsFillAlpha(renderer['pauseFade']), 0.5);
});

test('held pointer draws the original orange arrow with native handle, angle, scale, opacity and Pac-relative position', t => {
    const { draw, view, joystickPool, frames } = setup(t);
    view.pac.position = { x: 100, y: 120 };
    draw('drawJoystick', view, { offset: { x: 60, y: -60 }, length: 120 });
    assert.equal(joystickPool.length, 1);
    const arrow = joystickPool[0];
    assert.equal(arrow.texture, frames.get('char-sprites.move-arrow')?.[0]);
    assert.deepEqual({ x: arrow.x, y: arrow.y }, { x: 100, y: 144 }, 'arrow origin shares Pac screen coordinates, including the score-area transform only once');
    assert.deepEqual({ x: arrow.anchor.x, y: arrow.anchor.y }, { x: 16 / 50, y: 0.5 });
    assert.equal(arrow.rotation, -Math.PI / 4);
    assert.equal(arrow.width, 50);
    assert.equal(arrow.height, 32);
    assert.equal(arrow.alpha, 0.5);
    draw('drawJoystick', view, { offset: { x: 0, y: 500 }, length: 500 });
    assert.equal(arrow.rotation, Math.PI / 2);
    assert.equal(arrow.width, 75, 'scale caps at three times the native half-size sprite');
    assert.equal(arrow.alpha, 1 / 3);
    draw('drawJoystick', view, { offset: { x: 10, y: 0 }, length: 10 });
    assert.equal(arrow.width, 22.5, 'native feedback remains visible for nonzero drags inside the input dead zone');
    assert.equal(arrow.alpha, 0.75);
    draw('drawJoystick', view, { offset: { x: 0, y: 0 }, length: 0 });
    assert.equal(arrow.visible, false);
    draw('drawJoystick', view, null);
    assert.equal(arrow.visible, false);
    assert.equal(joystickPool.length, 1, 'the arrow is reused rather than recreated per frame');
});

test('ready and player-gameover overlays retain native phase visibility, anchors and colors', t => {
    const { renderer, draw, view } = setup(t);
    const labels: Array<{ text: string; x: number; y: number; tint: number }> = [];
    Object.assign(renderer, { overlayTextPool: [] });
    renderer['drawBitmapText'] = (text, x, y, tint, _pool, count = 0) => {
        if (text) labels.push({ text, x, y, tint });
        return count + text.length;
    };
    view.state = 'ready';
    view.beforeReady = true;
    draw('drawOverlay', view, false);
    assert.deepEqual(labels, [], 'BEFORE_TIME does not announce READY early');
    view.beforeReady = false;
    view.playerReady = true;
    view.level = 100;
    draw('drawOverlay', view, false);
    assert.deepEqual(labels, [
        { text: 'PLAYER ONE', x: 72, y: 112, tint: 0x00ffff },
        { text: 'LEVEL 100', x: 80, y: 160, tint: 0xffff00 },
    ]);
    labels.length = 0;
    view.playerReady = false;
    draw('drawOverlay', view, false);
    assert.deepEqual(labels, [{ text: 'READY!', x: 88, y: 160, tint: 0xffff00 }]);
    labels.length = 0;
    view.state = 'dead';
    view.playerGameOver = true;
    view.player = 2;
    draw('drawOverlay', view, false);
    assert.deepEqual(labels, [
        { text: 'GAME OVER', x: 72, y: 160, tint: 0xff0000 },
        { text: 'PLAYER TWO', x: 72, y: 112, tint: 0xff0000 },
    ]);
    labels.length = 0;
    view.state = 'gameover';
    view.playerGameOver = false;
    draw('drawOverlay', view, false);
    assert.deepEqual(labels, [{ text: 'GAME OVER', x: 72, y: 160, tint: 0xff0000 }]);
});

test('winning flashes and palette changes retain every wall and floor across all sixteen mazes', t => {
    const { renderer, draw, wallLayer } = setup(t);
    const build = t.mock.fn(renderer['renderMazeLayer'].bind(renderer));
    renderer['renderMazeLayer'] = build;
    for (const set of ['mr', 'ms']) for (let index = 0; index < 4; index++) for (const variant of [0, 1]) {
        const maze = parseMazeResource(`${set}-maze-${index}-${variant}`);
        draw('updateStaticMaze', maze, false);
        const children = [...wallLayer.children];
        const floor = children[0];
        assert.ok(floor instanceof Graphics);
        const instructions = [...floor.context.instructions];
        const builds = build.mock.callCount();
        for (const flash of [true, false, true, false]) {
            draw('updateStaticMaze', maze, flash);
            assert.equal(build.mock.callCount(), builds, 'palette transitions never rerun wall matching');
            assert.equal(floor.visible, !flash);
            assert.deepEqual(floor.context.instructions, instructions);
            children.forEach((child, slot) => assert.equal(wallLayer.children[slot], child));
            let slot = 1;
            for (const row of maze.tiles) for (const tile of row) {
                if (tile.kind !== 'wall' && tile.kind !== 'ghostWall' && tile.kind !== 'door') continue;
                const background = children[slot++], fill = children[slot++], border = children[slot++];
                assert.ok(background instanceof Sprite && fill instanceof Sprite && border instanceof Sprite);
                const door = tile.kind === 'door';
                assert.equal(background.alpha, flash ? 0 : (maze.backgroundColor >>> 24) / 255);
                assert.equal(fill.alpha, door ? 1 : flash ? 0 : (maze.fillColor >>> 24) / 255);
                assert.equal(border.alpha, flash || door ? 1 : (maze.borderColor >>> 24) / 255);
                assert.equal(fill.tint, door ? 0xffb8ff : flash ? 0 : maze.fillColor & 0xffffff);
                assert.equal(border.tint, door ? 0xffb8ff : flash ? 0xffffff : maze.borderColor & 0xffffff);
            }
        }
        const recolored = { ...maze, backgroundColor: 0x80112233, fillColor: 0x40445566, borderColor: 0x20778899 };
        draw('updateStaticMaze', recolored, false);
        assert.equal(build.mock.callCount(), builds);
        assert.equal(floor.tint, 0x112233);
        assert.equal(floor.alpha, 128 / 255);
        children.forEach((child, slot) => assert.equal(wallLayer.children[slot], child));
    }
    assert.equal(build.mock.callCount(), 16, 'a real board change rebuilds each geometry once');
});

test('consuming early, middle and final dots never reassigns surviving sprites or resurrects consumed tiles', t => {
    const { renderer, draw, view, dotPool } = setup(t);
    draw('drawDots', view);
    const sprites = [...dotPool];
    const entries = renderer['dotCaches'].get(dotPool)!.entries;
    const paint = t.mock.fn(renderer['paintSprite'].bind(renderer));
    renderer['paintSprite'] = paint;
    draw('drawDots', view);
    assert.equal(paint.mock.callCount(), 0, 'an unchanged dot tick does not repaint static foreground pellets');
    const consumed = [0, Math.floor(entries.length / 2), entries.length - 1];
    for (const index of consumed) entries[index].tile.kind = 'path';
    draw('drawDots', view);
    for (const index of consumed) assert.equal(dotPool[index].visible, false, 'consumption still hides dots on an unchanged animation tick');
    for (const state of ['playing', 'dead', 'playing'] as const) {
        view.state = state;
        view.stateTick = 20;
        view.animation.dotTicks++;
        draw('drawDots', view);
        for (let index = 0; index < entries.length; index++) {
            assert.equal(dotPool[index], sprites[index]);
            assert.equal(dotPool[index].visible, state !== 'dead' && !consumed.includes(index));
        }
    }
    view.maze = new Game({ character: 'ms', mazeSet: 'ms', difficulty: 'normal', players: 2 }).view.maze;
    draw('drawDots', view);
    const count = view.maze.tiles.flat().filter(tile => tile.kind === 'dot' || tile.kind === 'power').length;
    assert.equal(dotPool.filter(sprite => sprite.visible).length, count);
    assert.equal(renderer['dotCaches'].get(dotPool)!.tiles, view.maze.tiles);
});

test('warm bitmap glyphs skip painting while visibility, color, opacity, layout and replacement text stay correct', t => {
    const { renderer, draw } = setup(t);
    const pool: Sprite[] = [];
    const parent = new Container();
    const paint = t.mock.fn(renderer['paintSprite'].bind(renderer));
    renderer['paintSprite'] = paint;
    try {
        draw('drawBitmapText', 'AB', 10, 20, 0xffffff, pool, 0, parent);
        assert.equal(paint.mock.callCount(), 2);
        draw('hideSprites', pool, 0);
        draw('drawBitmapText', 'AB', 10, 20, 0xff00ff, pool, 0, parent, 1, 0.25);
        assert.equal(paint.mock.callCount(), 2, 'unchanged glyph transforms are not repainted');
        assert.deepEqual(pool.map(sprite => [sprite.visible, sprite.tint, sprite.alpha]), [[true, 0xff00ff, 0.25], [true, 0xff00ff, 0.25]]);
        draw('drawBitmapText', 'BA', 10, 20, 0xffffff, pool, 0, parent, 2, 1);
        assert.equal(paint.mock.callCount(), 4);
        assert.deepEqual(pool.map(sprite => [sprite.x, sprite.y, sprite.width, sprite.alpha]), [[10, 20, 16, 1], [26, 20, 16, 1]]);
        draw('drawBitmapText', 'A', 10, 20, 0xffffff, pool, 1, parent);
        assert.equal(pool[1].x, 10, 'overlapping text slots cannot return stale cached transforms');
        assert.equal(pool[1].width, 8);
    } finally { parent.destroy({ children: true }); }
});

test('fade steps retain graphics instructions and resize geometry only when logical dimensions change', t => {
    const { renderer, draw } = setup(t);
    const fade = renderer['screenFade'];
    draw('setRectangle', fade, 0.25);
    const instruction = fade.context.instructions[0];
    for (const alpha of [0, 0.125, 0.5, 0.875, 1]) {
        draw('setRectangle', fade, alpha);
        assert.equal(fade.context.instructions[0], instruction);
        assert.equal(graphicsFillAlpha(fade), alpha);
        assert.equal(fade.visible, alpha > 0);
    }
    renderer['layoutHeight'] = 248;
    draw('setRectangle', fade, 0.5);
    assert.notEqual(fade.context.instructions[0], instruction);
    assert.equal(fade.getLocalBounds().height, 248);
});
