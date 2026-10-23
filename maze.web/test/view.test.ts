import assert from 'node:assert/strict';
import { test } from 'node:test';
import { Game } from '../src/game';
import type { GameOptions } from '../src/types';

const options: GameOptions = { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 2 };

test('caller-owned game views reuse all steady-state objects while independent snapshots remain detached', () => {
    const game = new Game(options, () => 0.25);
    const run = game['players'][0].run;
    run.fruit.active = true;
    run.points.push({ x: 4, y: 8, value: 200, remaining: 54, color: 0x00ffff });
    run.particles.push({ x: 12, y: 16, animation: 0, frame: 0.5, scale: 1.25, vy: -0.05, timeScale: 1 });
    const target = game.view;
    const snapshot = game.view;
    const original = structuredClone(snapshot);
    const references = [target, target.animation, target.scores, target.pac, target.pac.position,
        target.ghosts, ...target.ghosts, ...target.ghosts.map(ghost => ghost.position),
        target.fruit, target.fruit?.position, target.points, target.points[0], target.points[0].position,
        target.particles, target.particles[0], target.particles[0].position, target.fruitHistory];
    for (let tick = 1; tick <= 200; tick++) {
        game['tickCount']++;
        run.stateTick++;
        run.pac.x++;
        run.points[0].x++;
        run.particles[0].frame += 0.125;
        run.fruit.y++;
        game['players'][0].score += 10;
        target.highScore = 9999999;
        assert.equal(game.readView(target), target);
        assert.equal(target.tick, tick);
        assert.equal(target.score, tick * 10);
        assert.equal(target.highScore, tick * 10, 'previous presentation overrides must not leak into the next view');
        assert.equal(target.pac.position.x, original.pac.position.x + tick);
        assert.equal(target.points[0].position.x, 4 + tick);
        assert.equal(target.particles[0].frame, 0.5 + tick * 0.125);
        assert.equal(target.fruit?.position.y, original.fruit!.position.y + tick);
        const current = [target, target.animation, target.scores, target.pac, target.pac.position,
            target.ghosts, ...target.ghosts, ...target.ghosts.map(ghost => ghost.position),
            target.fruit, target.fruit?.position, target.points, target.points[0], target.points[0].position,
            target.particles, target.particles[0], target.particles[0].position, target.fruitHistory];
        current.forEach((value, index) => assert.equal(value, references[index], `reference ${index} at tick ${tick}`));
        assert.deepEqual(target, game.view);
    }
    assert.deepEqual(snapshot, original, 'reading into a reusable view must not mutate an earlier independent snapshot');
    target.pac.position.x = -999;
    target.ghosts[0].position.x = -999;
    assert.notEqual(run.pac.x, -999);
    assert.notEqual(run.ghosts[0].x, -999);
});

test('reused views clear disappearing actors and displays, and refresh completely across player and game changes', () => {
    const game = new Game(options, () => 0);
    const run = game['players'][0].run;
    run.points.push({ x: 4, y: 8, value: 200, remaining: 54, color: 0x00ffff });
    run.particles.push({ x: 12, y: 16, animation: 0, frame: 0.5, scale: 1, vy: 0, timeScale: 1 });
    run.fruit.active = true;
    const target = game.view;
    assert.equal(target.points.length, 1);
    assert.equal(target.particles.length, 1);
    for (const ghost of run.ghosts) ghost.move = 'eaten';
    run.points.length = 0;
    run.particles.length = 0;
    run.fruit.active = false;
    game.readView(target);
    assert.deepEqual(target.ghosts, []);
    assert.deepEqual(target.points, []);
    assert.deepEqual(target.particles, []);
    assert.equal(target.fruit, null);
    assert.equal(target.pac.powered, true, 'eaten ghosts still power Pac even though they are not drawn');
    game['currentPlayer'] = 1;
    game['players'][1].score = 1230;
    game.readView(target);
    assert.equal(target.player, 2);
    assert.equal(target.highScore, 1230);
    assert.notEqual(target.maze, run.maze);
    assert.deepEqual(target, game.view);
    const other = new Game({ ...options, players: 1, character: 'ms', mazeSet: 'ms' });
    other.readView(target);
    assert.equal(target.character, 'ms');
    assert.deepEqual(target.scores, [0]);
    assert.equal(target.playerCount, 1);
    assert.deepEqual(target, other.view);
});

test('scalar input/completion queries never construct a presentation snapshot', () => {
    const game = new Game(options);
    Object.defineProperty(game, 'view', { get() { throw new Error('Unexpected snapshot allocation'); } });
    const run = game['players'][0].run;
    assert.equal(game.state, 'ready');
    assert.equal(game.direction, 'left');
    assert.equal(game.gameOverDone, false);
    run.state = 'playing';
    run.pac.dx = 0;
    run.pac.dy = -1;
    assert.equal(game.state, 'playing');
    assert.equal(game.direction, 'up');
    game['gameOver'] = true;
    game['gameOverCounter'] = 120;
    assert.equal(game.state, 'gameover');
    assert.equal(game.gameOverDone, false);
    game['gameOverCounter']++;
    assert.equal(game.gameOverDone, true);
});
