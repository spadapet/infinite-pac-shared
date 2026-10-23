import assert from 'node:assert/strict';
import { test } from 'node:test';
import paletteResources from '../../maze/assets/Values.Maze.res.json';
import { Game, parseMazeResource } from '../src/game';
import { animationDefinition } from '../src/native-animation';
import type { GameOptions } from '../src/types';

const mazeNames = ['mr', 'ms'].flatMap(character =>
    Array.from({ length: 4 }, (_, index) => [0, 1].map(variant => `${character}-maze-${index}-${variant}`)).flat());

function options(mazeSet: GameOptions['mazeSet'] = 'mr', players = 1, difficulty: GameOptions['difficulty'] = 'normal'): GameOptions {
    return { character: 'mr', mazeSet, difficulty, players };
}

function startPlaying(game: Game): void {
    for (let tick = 0; tick < 300 && game.view.state === 'ready'; tick++) game.advance();
    assert.equal(game.view.state, 'playing');
}

function advanceUntil(game: Game, condition: () => boolean, maximumTicks: number): void {
    for (let tick = 0; tick < maximumTicks && !condition(); tick++) game.advance();
    assert.ok(condition(), `game did not reach the expected state within ${maximumTicks} ticks`);
}

test('all native maze resources parse into mirrored 28-by-31 browser mazes', () => {
    for (const name of mazeNames) {
        const maze = parseMazeResource(name);
        assert.equal(maze.id, name);
        assert.equal(maze.width, 28);
        assert.equal(maze.height, 31);
        assert.equal(maze.tiles.length, maze.height);
        assert.ok(maze.tiles.every(row => row.length === maze.width));
        assert.ok(maze.tiles[0].every(tile => tile.kind === 'wall'));
        assert.ok(maze.tiles.flat().some(tile => tile.kind === 'door'));
        assert.ok(maze.tiles.flat().some(tile => tile.kind === 'power'));
        assert.ok(maze.tiles.flat().some(tile => tile.kind === 'dot'));
        assert.ok(maze.tiles.flat().some(tile => tile.zone === 'ghostSlow'));
        assert.ok(maze.borderColor > 0);
        assert.equal(maze.backgroundColor, 0xff101026);
    }
});

test('native maze palette preserves RGBA including transparent colors', () => {
    const source = paletteResources as { 'res:values': Record<string, number[] | string>; values_maze: { global: Record<string, Record<string, unknown>> } };
    for (const name of ['mr-maze-0-0', 'ms-maze-3-1', 'title-maze-front', 'title-maze-back', 'high-score-maze']) {
        const maze = parseMazeResource(name);
        const resource = source.values_maze.global[name];
        const resolve = (value: unknown): number[] => {
            while (typeof value === 'string' && value.startsWith('res:')) value = source['res:values'][value.slice(4)];
            return value as number[];
        };
        const rgba = (value: unknown) => {
            const [r, g, b, a] = resolve(value);
            return ((Math.round(a * 255) << 24) | (Math.round(r * 255) << 16) | (Math.round(g * 255) << 8) | Math.round(b * 255)) >>> 0;
        };
        assert.deepEqual([maze.borderColor, maze.fillColor, maze.backgroundColor],
            [rgba(resource.borderColor), rgba(resource.fillColor), rgba(resource.backgroundColor)], name);
        if (name.startsWith('title-maze-') || name === 'high-score-maze') {
            assert.equal(maze.fillColor >>> 24, 0);
            assert.equal(maze.backgroundColor >>> 24, 0);
        }
    }
});

test('native zone and special-star parsing retain path semantics and mirrored dots', () => {
    const maze = parseMazeResource('ms-maze-0-0');
    const noTurnDot = maze.tiles.flat().find(tile => tile.zone === 'ghostNoTurn' && tile.kind === 'dot');
    assert.ok(noTurnDot, 'the native adjacent-dot * marker must become a dot and keep its no-turn zone');
    assert.ok(maze.tiles.flat().some(tile => tile.zone === 'outOfBounds' && tile.kind === 'path'));
    assert.ok(maze.tiles.flat().some(tile => tile.zone === 'ghostSlow' && tile.kind === 'path'));
    const mirroredNoTurnDots = maze.tiles.flat().filter(tile => tile.zone === 'ghostNoTurn' && tile.kind === 'dot');
    assert.equal(mirroredNoTurnDots.length % 2, 0);
});

test('every maze set and difficulty constructs gameplay data with original actor positions', () => {
    for (const mazeSet of ['mr', 'ms'] as const) {
        for (const difficulty of ['easy', 'normal', 'hard'] as const) {
            const game = new Game({ ...options(mazeSet, 2, difficulty), character: 'ms' }, () => 0.25);
            const view = game.view;
            assert.equal(view.state, 'ready');
            assert.equal(view.player, 1);
            assert.equal(view.playerCount, 2);
            assert.equal(view.level, 1);
            assert.equal(view.lives, 3);
            assert.equal(view.maze.width, 28);
            assert.equal(view.maze.height, 31);
            assert.equal(view.pac.position.x % 8, 0);
            assert.equal(view.pac.position.y % 8, 4);
            assert.equal(view.pac.direction, 'left');
            assert.ok(view.ghosts.length >= 2 && view.ghosts.length <= 4);
            assert.equal(view.character, 'ms');
        }
    }
});

test('fixed-step movement scores dots, stops at a wall, and honors reversal input', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    const initial = game.view.pac.position;
    game.press('right');
    startPlaying(game);
    for (let tick = 0; tick < 100 && game.view.score === 0; tick++) game.advance();

    const scored = game.view;
    assert.equal(scored.pac.direction, 'right');
    assert.ok(scored.pac.position.x > initial.x);
    assert.ok(scored.score >= 10);
    assert.ok(scored.particles.length > 0);
    const events = game.drainEvents();
    assert.ok(events.some(event => event.type === 'sound' && event.name === 'mr-eat-dot-1'));
    assert.ok(events.some(event => event.type === 'vibrate' && event.duration === 100));

    for (let tick = 0; tick < 100; tick++) game.advance();
    const stopped = game.view;
    for (let tick = 0; tick < 90; tick++) game.advance();
    assert.deepEqual(game.view.pac.position, stopped.pac.position, 'the actor remains blocked by the wall');
    game.press('left');
    for (let tick = 0; tick < 20; tick++) game.advance();
    assert.equal(game.view.pac.direction, 'left');
    assert.ok(game.view.pac.position.x < stopped.pac.position.x);
});

test('power pellets frighten ghosts, award power points, and expire back to normal modes', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    startPlaying(game);
    const { position, direction } = game.view.pac;
    const vector = { up: [0, -1], left: [-1, 0], down: [0, 1], right: [1, 0] }[direction];
    const tileX = Math.floor((position.x * 2 + vector[0]) / 16);
    const tileY = Math.floor((position.y * 2 + vector[1]) / 16);
    game.view.maze.tiles[tileY][tileX].kind = 'power';

    game.advance();
    assert.equal(game.view.score, 50);
    assert.equal(game.view.pac.powered, true);
    assert.ok(game.view.ghosts.some(ghost => ghost.state === 'scared' || ghost.state === 'flash'));
    assert.ok(game.drainEvents().some(event => event.type === 'vibrate' && event.duration === 500));

    advanceUntil(game, () => !game.view.pac.powered, 1200);
    assert.ok(game.view.ghosts.every(ghost => ghost.state === 'scatter' || ghost.state === 'chase' || ghost.state === 'eyes'));
});

test('eating a frightened ghost awards chained score and sends it back as eyes', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    startPlaying(game);
    for (const row of game.view.maze.tiles) {
        for (const tile of row) tile.kind = 'path';
    }

    let powerPlaced = false;
    for (let tick = 0; tick < 700 && game.view.score < 250; tick++) {
        const view = game.view;
        const pac = view.pac.position;
        const nearestGhost = view.ghosts
            .map(ghost => ({ ghost, distance: (ghost.position.x - pac.x) ** 2 + (ghost.position.y - pac.y) ** 2 }))
            .sort((a, b) => a.distance - b.distance)[0];
        if (!powerPlaced && nearestGhost.distance < 900) {
            const vector = { up: [0, -1], left: [-1, 0], down: [0, 1], right: [1, 0] }[view.pac.direction];
            const tileX = Math.floor((pac.x * 2 + vector[0]) / 16);
            const tileY = Math.floor((pac.y * 2 + vector[1]) / 16);
            view.maze.tiles[tileY][tileX].kind = 'power';
            powerPlaced = true;
        }
        if (powerPlaced) {
            const dx = nearestGhost.ghost.position.x - pac.x;
            const dy = nearestGhost.ghost.position.y - pac.y;
            game.press(Math.abs(dx) > Math.abs(dy) ? (dx < 0 ? 'left' : 'right') : (dy < 0 ? 'up' : 'down'));
        }
        game.advance();
    }

    assert.equal(powerPlaced, true);
    assert.equal(game.view.score, 250);
    assert.equal(game.view.pac.visible, false, 'Pac is hidden during the native ghost-eat freeze');
    assert.ok(game.view.points.some(point => point.value === 200 && point.color === 0x00ffff));
    assert.ok(game.drainEvents().some(event => event.type === 'sound' && event.name === 'mr-eat-ghost'));
    const frozen = game.view.pac.position;
    for (let tick = 0; tick < 53; tick++) game.advance();
    assert.deepEqual(game.view.pac.position, frozen);
    assert.equal(game.view.pac.visible, false);
    game.advance();
    assert.equal(game.view.pac.visible, true);
    assert.ok(game.view.ghosts.some(ghost => ghost.state === 'eyes'), 'eyes appear after, not during, the eating freeze');
});

test('inactive Pac ignores pressed directions like native PlayingActor::SetPressDir', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    const pac = game['players'][0].run.pac;
    pac.active = false;
    pac.canTurn = false;
    game.press('up');
    assert.deepEqual([pac.pressX, pac.pressY, pac.canTurn], [0, 0, false]);
});

test('point displays keep zero through the countdown update and expire on the next update', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    const run = game['players'][0].run;
    for (const [initial, lifetime] of [[54, 55], [120, 121]]) {
        run.points = [{ x: 0, y: 0, value: 200, remaining: initial, color: 0x00ffff }];
        for (let tick = 0; tick < lifetime - 1; tick++) game['updatePoints'](run);
        assert.equal(run.points.length, 1, `${initial} countdown updates retain the popup at zero`);
        assert.equal(run.points[0].remaining, 0);
        game['updatePoints'](run);
        assert.equal(run.points.length, 0, `popup expires on update ${lifetime}`);
    }
});

test('Pac animation frame rate comes from the selected native character animation', () => {
    for (const character of ['mr', 'ms'] as const) {
        const game = new Game({ ...options('mr', 1, 'easy'), character }, () => 0);
        const run = game['players'][0].run;
        run.state = 'playing';
        run.pac.stuck = false;
        run.animation.pacTicks = 3;
        game['advanceAnimation'](run);
        assert.equal(run.animation.pacFrame, 3 * animationDefinition(`${character}-char-anim`).fps / 60);
    }
});

test('death consumes a life and the final death emits game over exactly once', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    startPlaying(game);
    advanceUntil(game, () => game.view.lives === 2, 1600);
    assert.equal(game.view.level, 1);
    assert.equal(game.view.state, 'ready');

    advanceUntil(game, () => game.view.state === 'gameover', 6000);
    assert.equal(game.view.lives, 1);
    const gameOverEvents = game.drainEvents().filter(event => event.type === 'gameOver');
    assert.equal(gameOverEvents.length, 1);
    assert.deepEqual(gameOverEvents[0], { type: 'gameOver', player: 1, score: game.view.score, level: 1 });
    game.advance();
    assert.equal(game.drainEvents().filter(event => event.type === 'gameOver').length, 0);
});

test('two players alternate after a death while retaining independent lives and scores', () => {
    const game = new Game(options('mr', 2, 'easy'), () => 0);
    startPlaying(game);
    advanceUntil(game, () => game.view.player === 2, 1600);
    assert.equal(game.view.state, 'ready');
    assert.equal(game.view.level, 1);
    assert.equal(game.view.lives, 3);
    assert.equal(game.view.score, 0);

    advanceUntil(game, () => game.view.player === 1, 1600);
    assert.equal(game.view.state, 'ready');
    assert.equal(game.view.lives, 2);
    assert.equal(game.view.level, 1);
});

test('clearing the final dot enters the win pause and advances the maze and level', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    startPlaying(game);
    const originalMazeId = game.view.maze.id;
    const directionVectors = { up: [0, -1], left: [-1, 0], down: [0, 1], right: [1, 0] };

    for (let tick = 0; tick < 1000 && game.view.state !== 'winning'; tick++) {
        const view = game.view;
        const [dx, dy] = directionVectors[view.pac.direction];
        const tileX = Math.floor((view.pac.position.x * 2 + dx) / 16);
        const tileY = Math.floor((view.pac.position.y * 2 + dy) / 16);
        view.maze.tiles[tileY][tileX].kind = 'dot';
        game.advance();
    }

    assert.equal(game.view.state, 'winning');
    const scoreAfterWinning = game.view.score;
    assert.ok(scoreAfterWinning > 0);
    advanceUntil(game, () => game.view.state === 'ready' && game.view.level === 2, 300);
    assert.notEqual(game.view.maze.id, originalMazeId);
    assert.equal(game.view.score, scoreAfterWinning);
});

test('two-player games expose 1-based player metadata and reject invalid player counts', () => {
    const game = new Game(options('ms', 2), () => 0.5);
    assert.match(game.view.maze.id, /^ms-maze-0-0:/);
    assert.equal(game.view.player, 1);
    assert.equal(game.view.playerCount, 2);
    assert.throws(() => new Game(options('mr', 0)), RangeError);
    assert.throws(() => new Game(options('mr', 3)), RangeError);
});

test('the first-player intro plays once and stops exactly when the native ready sequence ends', () => {
    const game = new Game(options(), () => 0);
    assert.equal(game.view.playerReady, false);
    assert.equal(game.view.beforeReady, true);
    assert.equal(game.view.pac.visible, false);
    game.advance();
    assert.equal(game.view.playerReady, true);
    assert.deepEqual(game.drainEvents().filter(event => event.type === 'sound'), [{ type: 'sound', name: 'mr-intro' }]);
    for (let tick = 0; tick < 151; tick++) game.advance();
    assert.equal(game.view.state, 'ready');
    assert.equal(game.view.playerReady, false);
    assert.equal(game.view.pac.visible, true);
    startPlaying(game);
    const events = game.drainEvents();
    assert.equal(events.filter(event => event.type === 'sound' && event.name === 'mr-intro').length, 0);
    assert.deepEqual(events.filter(event => event.type === 'stopSound' && event.name === 'mr-intro'), [{ type: 'stopSound', name: 'mr-intro' }]);
});

test('wall contact freezes the mouth pose without resetting it while dot and ghost clocks continue', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    game.press('right');
    startPlaying(game);
    for (let tick = 0; tick < 200; tick++) game.advance();
    assert.equal(game.view.state, 'playing');
    assert.equal(game.view.pac.moving, false);
    const before = game.view;
    assert.ok(before.animation.pacFrame > 0);
    for (let tick = 0; tick < 20; tick++) game.advance();
    assert.deepEqual(game.view.pac.position, before.pac.position);
    assert.equal(game.view.animation.pacFrame, before.animation.pacFrame);
    assert.equal(game.view.animation.ghostTicks, before.animation.ghostTicks + 20);
    assert.equal(game.view.animation.dotTicks, before.animation.dotTicks + 20);
});

test('Pac frame timing follows the selected normal or powered resource FPS', t => {
    const game = new Game(options(), () => 0);
    const run = game['players'][0].run;
    run.state = 'playing';
    run.pac.stuck = false;
    const normal = animationDefinition('mr-char-anim');
    const powered = animationDefinition('mr-power-anim');
    const original = [normal.fps, powered.fps];
    t.after(() => { normal.fps = original[0]; powered.fps = original[1]; });
    normal.fps = 30;
    powered.fps = 24;
    run.animation.pacTicks = 8;
    game['advanceAnimation'](run);
    assert.equal(game.view.animation.pacFrame, 4);
    run.ghosts[0].move = 'scared';
    run.animation.pacTicks = 10;
    game['advanceAnimation'](run);
    assert.equal(game.view.animation.pacFrame, 4);
});

test('caught holds the mouth clock and dying advances smooth eighth-frame rotation after its delay', () => {
    const game = new Game(options('mr', 1, 'easy'), () => 0);
    startPlaying(game);
    advanceUntil(game, () => game.view.state === 'caught', 1600);
    const caughtFrame = game.view.animation.pacFrame;
    for (let tick = 0; tick < 20; tick++) game.advance();
    assert.equal(game.view.state, 'caught');
    assert.equal(game.view.animation.pacFrame, caughtFrame);
    advanceUntil(game, () => game.view.state === 'dying', 70);
    for (let tick = 0; tick < 29; tick++) game.advance();
    assert.equal(game.view.animation.deathFrame, 0);
    advanceUntil(game, () => game.view.animation.deathFrame > 0, 5);
    assert.equal(game.view.animation.deathFrame, 0.125);
    game.advance();
    assert.equal(game.view.animation.deathFrame, 0.25);
});
