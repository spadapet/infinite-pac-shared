import { test, type TestContext } from 'node:test';
import assert from 'node:assert/strict';
import { Container, Graphics, Rectangle, Sprite, Texture, TextureSource } from 'pixi.js';
import { Game, parseMazeResource } from '../src/game';
import { nativeSprites } from '../src/native-animation';
import { GameRenderer } from '../src/renderer';

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
    const actorLayer = new Container(), dotLayer = new Container(), wallLayer = new Container(), hudLayer = new Container(), joystickLayer = new Container();
    const background = new Graphics();
    const renderer: GameRenderer = Object.create(GameRenderer.prototype);
    Object.assign(renderer, { textures: { frames }, boardTop: 24, layoutWidth: 224, layoutHeight: 296,
        actorPool, actorLayer, dotPool, dotLayer, wallLayer, background, livesPool, fruitPool, pointTextPool, hudLayer, hudTextPool: [], joystickPool, joystickLayer });
    const draw = (method: string, ...args: unknown[]) => Reflect.apply(Reflect.get(GameRenderer.prototype, method), renderer, args);
    const game = new Game({ character: 'mr', mazeSet: 'mr', difficulty: 'easy', players: 1 }, () => 0);
    const view = game.view;
    view.state = 'playing';
    view.playerReady = false;
    view.beforeReady = false;
    view.pac.visible = false;
    view.ghosts = [];
    t.after(() => {
        for (const layer of [actorLayer, dotLayer, wallLayer, hudLayer, joystickLayer]) layer.destroy({ children: true });
        background.destroy();
        for (const texture of textures) texture.destroy();
        source.destroy();
    });
    return { renderer, draw, view, frames, actorPool, dotPool, livesPool, fruitPool, pointTextPool, wallLayer, joystickPool };
}

test('all sixteen native maze variants render every wall/door through the original three-layer patterns', t => {
    const { draw, view, wallLayer } = setup(t);
    for (const set of ['mr', 'ms']) {
        for (let index = 0; index < 4; index++) {
            for (const variant of [0, 1]) {
                const maze = parseMazeResource(`${set}-maze-${index}-${variant}`);
                draw('rebuildStaticMaze', maze, maze.id, false);
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

test('transparent native maze colors remain transparent in wall layers', t => {
    const { draw, view, wallLayer } = setup(t);
    const maze = { ...view.maze, borderColor: 0x00112233, fillColor: 0x00445566, backgroundColor: 0x00778899 };
    draw('rebuildStaticMaze', maze, maze.id, false);

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
    assert.ok(fruitPool.every(sprite => sprite.width === 14 && sprite.height === 14));
    assert.deepEqual(fruitPool.map(sprite => sprite.x), [200, 184]);
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
