import assert from 'node:assert/strict';
import { test } from 'node:test';
import { Game } from '../src/game';
import type { GameOptions } from '../src/types';

function fixture(mazeSet: GameOptions['mazeSet'] = 'mr', level = 0, random = () => 0) {
    const game = new Game({ character: mazeSet, mazeSet, difficulty: 'normal', players: 1 }, random);
    const player = game['players'][0];
    player.level = level;
    player.run = game['createLevel'](level);
    const run = player.run;
    run.state = 'playing';
    return { game, player, run };
}

function openMaze(run: Game['players'][number]['run']): void {
    for (const row of run.maze.tiles) for (const tile of row) {
        tile.kind = 'path';
        tile.zone = 'normal';
    }
}

test('held diagonal unlocks after tile changes and walls without a new input event', () => {
    const { game, run } = fixture();
    openMaze(run);
    run.maze.tiles[1][2].kind = 'wall';
    Object.assign(run.pac, { x: 20, y: 28, dx: 1, dy: 0, canTurn: true });
    game.press({ x: 1, y: -1 });
    game['advancePac'](run.pac, run);
    assert.deepEqual([run.pac.x, run.pac.y, run.pac.canTurn], [20, 27, false]);
    for (let i = 0; i < 3; i++) game['advancePac'](run.pac, run);
    assert.deepEqual([run.pac.x, run.pac.y, run.pac.canTurn], [20, 24, true]);
    game['advancePac'](run.pac, run);
    assert.deepEqual([run.pac.x, run.pac.y, run.pac.dx, run.pac.dy], [21, 23, 1, 0], 'the unchanged held Right turns with native corner gravitation');

    Object.assign(run.pac, { x: 20, y: 20, dx: 0, dy: -1, canTurn: false, stuck: false });
    game['advancePac'](run.pac, run);
    assert.equal(run.pac.stuck, true);
    assert.equal(run.pac.canTurn, true);
    game['advancePac'](run.pac, run);
    assert.deepEqual([run.pac.x, run.pac.y, run.pac.dx, run.pac.dy], [21, 20, 1, 0]);
});

test('turn lock does not block reversal and tunnel wrapping still runs on a tile change', () => {
    const { game, run } = fixture();
    openMaze(run);
    Object.assign(run.pac, { x: 20, y: 20, dx: 1, dy: 0, canTurn: false });
    game.press('left');
    run.pac.canTurn = false;
    game['advancePac'](run.pac, run);
    assert.deepEqual([run.pac.x, run.pac.y, run.pac.dx], [19, 20, -1]);
    Object.assign(run.pac, { x: -8, y: 20, dx: -1, dy: 0, canTurn: false });
    game['advancePac'](run.pac, run);
    assert.equal(run.pac.x, (run.maze.width + 1) * 8);
    assert.equal(run.pac.canTurn, true);
});

test('ghost chain survives each 54-update pause and resets only with fright state', () => {
    const { game, player, run } = fixture();
    game['scareGhosts'](run, true);
    const frightenedFrames = run.ghostScaredCountdown;
    for (const [index, score] of [200, 400, 800, 1600, 200].entries()) {
        const ghost = run.ghosts[index % 4];
        game['eatGhost'](player, run, ghost);
        assert.equal(run.points.at(-1)?.value, score);
        assert.equal(run.ghostEatenIndex, index + 1);
        assert.equal(run.ghostEatenCountdown, 54);
        assert.ok(game.drainEvents().some(event => event.type === 'sound' && event.name === 'mr-eat-ghost'));
        for (let i = 0; i < 53; i++) game['updateGhostMode'](run);
        assert.equal(ghost.move, 'eaten');
        assert.equal(run.ghostScaredCountdown, frightenedFrames, 'fright time is frozen while eating');
        game['updateGhostMode'](run);
        assert.equal(ghost.move, 'eyes');
        assert.equal(run.ghostEatenIndex, index + 1);
    }
    assert.equal(player.score, 3200);
    game['scareGhosts'](run, true);
    assert.equal(run.ghostEatenIndex, 0, 'a new power pellet starts a new chain');
    run.ghostEatenIndex = 3;
    run.ghostScaredCountdown = 1;
    game['updateGhostMode'](run);
    assert.equal(run.ghostEatenIndex, 0, 'expiration ends the chain');
});

test('powered art follows active ghost states while speed continues to follow the timer', () => {
    const { game, run } = fixture();
    run.ghostScaredCountdown = 600;
    for (const ghost of run.ghosts) ghost.move = 'eyes';
    game['updateSpeeds'](run, 0);
    assert.equal(game.view.pac.powered, false);
    assert.equal(run.pac.speed, 85);
    run.ghostScaredCountdown = 0;
    for (const move of ['scared', 'eaten', 'waiting'] as const) {
        run.ghosts[0].move = move;
        assert.equal(game.view.pac.powered, true, move);
    }
    run.ghosts[0].active = false;
    assert.equal(game.view.pac.powered, false);
    game['updateSpeeds'](run, 0);
    assert.equal(run.pac.speed, 85);
});

test('fruit roaming retries visited directed choices and slow tunnels, then uses the original random fallback', () => {
    let draw = 0;
    const { game, run } = fixture('mr', 0, () => draw);
    openMaze(run);
    const current = { x: 5, y: 5 };
    const up = { x: 5, y: 4 }, left = { x: 4, y: 5 }, right = { x: 6, y: 5 };
    const candidates = [up, left, right];
    run.fruitCounter = 100;
    run.maze.tiles[4][5].zone = 'ghostSlow';
    assert.deepEqual(game['fruitChooseTile'](run, current, candidates), left);
    assert.deepEqual(game['fruitChooseTile'](run, current, candidates), right);
    assert.deepEqual([...run.fruit.chosenRandom], ['5,5:4,5', '5,5:6,5']);
    assert.deepEqual(game['fruitChooseTile'](run, { x: 4, y: 4 }, candidates), left, 'history includes the source tile');
    draw = 0.99;
    assert.deepEqual(game['fruitChooseTile'](run, current, candidates), right, 'all rejected falls back to an original candidate');
    assert.equal(run.fruit.chosenRandom.size, 3, 'fallback is not recorded');
    assert.equal(run.fruit.chosenExit.size, 0);
});

test('fruit exit uses independent history, first-choice distance ties and allows slow tunnels', () => {
    const { game, run } = fixture();
    openMaze(run);
    const current = { x: 5, y: 5 };
    const candidates = [{ x: 5, y: 4 }, { x: 4, y: 5 }, { x: 6, y: 5 }];
    run.fruitCounter = 0;
    run.fruit.exitTile = { x: 5, y: 5 };
    run.maze.tiles[4][5].zone = 'ghostSlow';
    run.fruit.chosenRandom.add('5,5:5,4');
    assert.deepEqual(game['fruitChooseTile'](run, current, candidates), candidates[0]);
    assert.deepEqual(game['fruitChooseTile'](run, current, candidates), candidates[1]);
    assert.deepEqual(game['fruitChooseTile'](run, current, candidates), candidates[2]);
    assert.deepEqual(game['fruitChooseTile'](run, current, candidates), candidates[0]);
    assert.equal(run.fruit.chosenExit.size, 3);
});

test('fruit corridor choices bypass brain history and do not reverse at intersections', () => {
    const { game, run } = fixture();
    openMaze(run);
    Object.assign(run.fruit, { active: true, x: 44, y: 44, dx: 1, dy: 0 });
    run.fruitCounter = 100;
    run.maze.tiles[4][5].kind = 'wall';
    run.maze.tiles[6][5].kind = 'wall';
    assert.equal(game['ghostDecideDirection'](run.fruit, run, { x: 1, y: 0 }, null, true), null, 'continuing forward has no queued press');
    assert.equal(run.fruit.chosenRandom.size, 0);
    run.maze.tiles[5][6].kind = 'wall';
    assert.deepEqual(game['ghostDecideDirection'](run.fruit, run, { x: 1, y: 0 }, null, true), { x: -1, y: -0 });
});

test('fruit eating, exit and expiration reset movement fractions and both route histories', () => {
    for (const reason of ['eat', 'exit', 'expire'] as const) {
        const { game, player, run } = fixture();
        const fruit = run.fruit;
        Object.assign(fruit, { active: true, x: -8, y: 116, dx: -1, dy: 0, speed: 35, advance: 28863, advancePosition: 12345, type: 3 });
        fruit.chosenRandom.add('1,1:2,1');
        fruit.chosenExit.add('1,1:1,2');
        if (reason === 'eat') game['eatFruit'](player, run);
        else if (reason === 'exit') game['advanceFruit'](fruit, run);
        else {
            fruit.dx = 0;
            run.fruitCounter = 1;
            game['updateFruitMode'](run, player);
        }
        assert.deepEqual([fruit.active, fruit.x, fruit.y, fruit.dx, fruit.dy, fruit.speed, fruit.advance, fruit.advancePosition, fruit.type],
            [false, 0, 0, 0, 0, 0, 0, 0, 0], reason);
        assert.equal(fruit.chosenRandom.size + fruit.chosenExit.size, 0, reason);
        if (reason === 'eat') {
            assert.equal(player.score, 700);
            assert.deepEqual(run.points[0], { x: -8, y: 116, value: 700, remaining: 120, color: 0xffb8ff });
            assert.ok(game.drainEvents().some(event => event.type === 'sound' && event.name === 'mr-eat-fruit'));
        }
        run.fruitCounter = 0;
        run.dotCount = run.fruitThresholds[0];
        game['updateFruitMode'](run, player);
        game['updateSpeeds'](run, 0);
        assert.equal(fruit.active, true, `${reason}: next fruit spawns`);
        assert.equal(fruit.speed, 35);
        assert.ok(fruit.advance > 0, 'the new fruit moves at its freshly initialized speed');
        assert.equal(fruit.advancePosition, 0);
    }
});

test('fruit exits every entrance/exit combination in all sixteen original maze variants', () => {
    let seed = 0x12345678;
    const random = () => { seed = (Math.imul(seed, 1664525) + 1013904223) >>> 0; return seed / 0x100000000; };
    for (const set of ['mr', 'ms'] as const) for (let level = 0; level < 8; level++) {
        const { game, run } = fixture(set, level, random);
        for (const start of run.fruitStarts) for (const exit of run.fruitStarts) {
            game['resetFruit'](run.fruit);
            Object.assign(run.fruit, { active: true, x: start.tile.x * 8 + 4, y: start.tile.y * 8 + 4,
                dx: start.dir.x, dy: start.dir.y, exitTile: exit.tile });
            run.fruitCounter = 0;
            let steps = 0;
            for (; steps < 100000 && run.fruit.active; steps++) {
                game['advanceFruit'](run.fruit, run);
                game.drainEvents();
            }
            assert.equal(run.fruit.active, false, `${set} level ${level + 1}: ${JSON.stringify(start.tile)} -> ${JSON.stringify(exit.tile)}`);
        }
    }
});

test('orange close chase uses Ms. Pac random scatter corners, but Mr. Pac keeps its own corner', () => {
    for (const set of ['mr', 'ms'] as const) for (const draw of [0, 0.25, 0.5, 0.75]) {
        const { game, run } = fixture(set, 0, () => draw);
        run.ghostScatterCountdown = 0;
        Object.assign(run.pac, { x: 100, y: 100 });
        Object.assign(run.ghosts[3], { x: 100, y: 108 });
        const corners = [{ x: 204, y: -20 }, { x: 20, y: -20 }, { x: 220, y: 260 }, { x: 4, y: 260 }];
        assert.deepEqual(game['ghostTarget'](3, 'normal', run), corners[set === 'ms' ? draw * 4 : 3]);
        Object.assign(run.ghosts[3], { x: 164, y: 164 });
        assert.deepEqual(game['ghostTarget'](3, 'normal', run), { x: 100, y: 100 }, 'the distance threshold is inclusive at 8192');
        assert.deepEqual(game['ghostTarget'](3, 'eyes', run), run.ghostStartPixel);
    }
});

test('native chase targets, candidate tie priority, no-turn zones and queued reversals are preserved', () => {
    const { game, run } = fixture();
    openMaze(run);
    run.ghostScatterCountdown = 0;
    Object.assign(run.pac, { x: 100, y: 100, dx: 0, dy: -1 });
    Object.assign(run.ghosts[0], { x: 80, y: 108, dx: 1, dy: 0, house: 'outside' });
    assert.deepEqual(game['ghostTarget'](0, 'normal', run), { x: 100, y: 100 });
    assert.deepEqual(game['ghostTarget'](1, 'normal', run), { x: 100, y: 68 });
    assert.deepEqual(game['ghostTarget'](2, 'normal', run), { x: 120, y: 92 });
    const ghost = run.ghosts[0];
    Object.assign(ghost, { x: 44, y: 44 });
    Object.assign(run.pac, { x: 52, y: 44 });
    assert.deepEqual(game['ghostDecideDirection'](ghost, run, { x: 1, y: 0 }, 'normal'), { x: 0, y: -1 }, 'Up wins equal distances before Down and Right');
    run.maze.tiles[5][6].zone = 'ghostNoTurn';
    assert.equal(game['ghostDecideDirection'](ghost, run, { x: 1, y: 0 }, 'normal'), null);
    assert.deepEqual(game['ghostDecideDirection'](ghost, run, { x: 1, y: 0 }, 'scared'), { x: 0, y: -1 });
    game['flipGhosts'](run);
    assert.deepEqual([ghost.dx, ghost.dy, ghost.pressX, ghost.pressY], [1, 0, -1, -0], 'mode change queues rather than immediately applies reversal');
});

test('house dot quotas release ghosts in order, one per update, with the native inactivity timeout', () => {
    const { game, player, run } = fixture();
    assert.deepEqual(run.ghosts.map(ghost => ghost.dotCounter), [0, 0, 30, 60]);
    game['updateGhostHouse'](run);
    assert.deepEqual(run.ghosts.map(ghost => ghost.house), ['outside', 'leaving', 'inside', 'inside']);
    for (let i = 0; i < 30; i++) game['eatDot'](player, run, false);
    assert.deepEqual(run.ghosts.map(ghost => ghost.dotCounter), [0, 0, 0, 60]);
    game['updateGhostHouse'](run);
    assert.equal(run.ghosts[2].house, 'leaving');
    assert.equal(run.ghosts[3].house, 'inside');
    run.lastDotCounter = 239;
    game['updateGhostHouse'](run);
    assert.equal(run.ghosts[3].house, 'inside', 'exactly four seconds does not trigger the strict-greater timeout');
    game['updateGhostHouse'](run);
    assert.equal(run.ghosts[3].house, 'leaving');
});

test('zero-quota ghosts release on consecutive fixed updates and overlap at the doorway', () => {
    const { game, player, run } = fixture('ms', 2);
    run.pac.active = false;
    assert.deepEqual(run.ghosts.map(ghost => ghost.dotCounter), [0, 0, 0, 0]);
    assert.deepEqual([run.ghosts[2].x, run.ghosts[3].x], [run.ghostStartPixel.x - 16, run.ghostStartPixel.x + 16]);
    const releases: [number, number][] = [];
    for (let tick = 0; tick <= 30; tick++) {
        const previous = run.ghosts.map(ghost => ghost.house);
        game['advanceActors'](player, run);
        run.ghosts.forEach((ghost, index) => {
            if (previous[index] === 'inside' && ghost.house === 'leaving') releases.push([index, tick]);
        });
    }
    assert.deepEqual(releases, [[1, 0], [2, 1], [3, 2]]);
    assert.deepEqual([run.ghosts[2].x, run.ghosts[2].y], [112, 115]);
    assert.deepEqual([run.ghosts[3].x, run.ghosts[3].y], [112, 115], 'there is no native doorway occupancy or ghost-to-ghost collision gate');
});

test('post-death global quotas use 7/17/32 dots and clear when the last waiting ghost leaves', () => {
    const { game, run } = fixture();
    run.globalDotIndex = 1;
    for (const [index, threshold] of [7, 17, 32].entries()) {
        run.globalDotCounter = threshold - 1;
        game['updateGhostHouse'](run);
        assert.equal(run.ghosts[index + 1].house, 'inside');
        run.globalDotCounter = threshold;
        game['updateGhostHouse'](run);
        assert.equal(run.ghosts[index + 1].house, 'leaving');
    }
    assert.equal(run.globalDotIndex, 0);
});

test('simultaneous ghost collisions queue the full score chain through real fixed updates', () => {
    const { game, player, run } = fixture();
    openMaze(run);
    Object.assign(run.pac, { x: 44, y: 44, dx: 1, dy: 0 });
    game['scareGhosts'](run, true);
    for (const ghost of run.ghosts) Object.assign(ghost, { x: 44, y: 44, house: 'outside' });
    game['checkCollisions'](player, run);
    assert.deepEqual(run.ghosts.map(ghost => ghost.move), ['eaten', 'waiting', 'waiting', 'waiting']);
    const frightenedFrames = run.ghostScaredCountdown;
    for (const total of [200, 600, 1400, 3000]) {
        assert.equal(player.score, total);
        assert.equal(run.ghostEatenCountdown, 54);
        const frozen = { ...game.view.pac.position };
        for (let tick = 0; tick < 53; tick++) game.advance();
        assert.deepEqual(game.view.pac.position, frozen);
        assert.equal(player.score, total);
        game.advance();
        assert.equal(run.ghostScaredCountdown, frightenedFrames);
    }
    assert.equal(run.ghostEatenCountdown, 0);
    assert.equal(game.view.pac.powered, false);
    assert.deepEqual(run.ghosts.map(ghost => ghost.move), ['normal', 'normal', 'eyes', 'eyes'], 'earlier eyes keep moving during later eating pauses and can already return to the house');
    assert.deepEqual(run.ghosts.slice(0, 2).map(ghost => ghost.house), ['leaving', 'leaving']);
});

test('all twelve fruit scores use native values and score-display timing', () => {
    const expected = [100, 300, 500, 700, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000];
    for (const [type, score] of expected.entries()) {
        const { game, player, run } = fixture();
        Object.assign(run.fruit, { active: true, type, x: 100, y: 120 });
        game['eatFruit'](player, run);
        assert.equal(player.score, score);
        assert.deepEqual(run.points[0], { x: 100, y: 120, value: score, remaining: 120, color: 0xffb8ff });
        assert.equal(run.fruit.active, false);
    }
});

test('fruit spawn thresholds, timer draws and roaming-to-exit transition match native rules', () => {
    const { game, player, run } = fixture('ms', 0, () => 0.999);
    assert.deepEqual(run.fruitThresholds, [Math.floor(run.dotCountTotal * 143 / 200), Math.floor(run.dotCountTotal * 61 / 200)]);
    run.dotCount = run.fruitThresholds[0] + 1;
    game['updateFruitMode'](run, player);
    assert.equal(run.fruit.active, false);
    run.dotCount--;
    game['updateFruitMode'](run, player);
    assert.equal(run.fruitCounter, 899);
    assert.equal(run.fruitIndex, 1);
    assert.equal(run.fruit.active, true);
    const start = run.fruitStarts.at(-1)!;
    assert.deepEqual([run.fruit.x, run.fruit.y, run.fruit.dx, run.fruit.dy], [start.tile.x * 8 + 4, start.tile.y * 8 + 4, start.dir.x, start.dir.y]);
    assert.deepEqual(run.fruit.exitTile, start.tile);
    run.fruitCounter = 1;
    game['updateFruitMode'](run, player);
    assert.equal(run.fruitCounter, 0);
    assert.equal(run.fruit.active, true, 'moving fruit must exit, not disappear when its roaming timer runs out');
});

test('native ghost speeds cover eyes, house, tunnel, frightened and both Elroy thresholds', () => {
    const { game, run } = fixture();
    openMaze(run);
    for (const ghost of run.ghosts) Object.assign(ghost, { x: 44, y: 44, dx: 1, dy: 0, house: 'outside' });
    for (const [dots, expected] of [[21, 65], [20, 70], [10, 75]] as const) {
        run.dotCount = dots;
        game['updateSpeeds'](run, 0);
        assert.equal(run.ghosts[0].speed, expected);
        assert.equal(run.ghosts[1].speed, 65, 'only red receives Elroy speed');
    }
    const ghost = run.ghosts[0];
    ghost.move = 'scared';
    game['updateSpeeds'](run, 0);
    assert.equal(ghost.speed, 45);
    run.maze.tiles[5][5].zone = 'ghostSlow';
    game['updateSpeeds'](run, 0);
    assert.equal(ghost.speed, 35, 'tunnel speed takes priority over scared speed');
    ghost.move = 'eyes';
    game['updateSpeeds'](run, 0);
    assert.equal(ghost.speed, 150, 'eyes take priority over house and tunnel speeds');
    ghost.move = 'normal';
    ghost.house = 'leaving';
    run.maze.tiles[5][5].zone = 'normal';
    game['updateSpeeds'](run, 0);
    assert.equal(ghost.speed, 35);
    assert.equal(ghost.advance, Math.floor(35 * 75.5 / 6000 * 65536));
});

test('life reset preserves eaten dots and house quotas but clears fruit and ghost-chain state', () => {
    const { game, player, run } = fixture();
    game['eatDot'](player, run, false);
    run.ghosts[2].dotCounter = 19;
    run.ghostEatenIndex = 3;
    run.fruit.chosenRandom.add('1,1:2,1');
    Object.assign(run.fruit, { active: true, speed: 35, advancePosition: 123 });
    game['finishDeath'](player);
    const next = player.run;
    assert.equal(player.lives, 2);
    assert.equal(next.maze, run.maze);
    assert.equal(next.ghosts[2].dotCounter, 19);
    assert.equal(next.globalDotIndex, 1);
    assert.equal(next.globalDotCounter, 0);
    assert.equal(next.ghostEatenIndex, 0);
    assert.equal(next.ghostScaredCountdown, 0);
    assert.deepEqual([next.fruit.active, next.fruit.speed, next.fruit.advancePosition, next.fruit.chosenRandom.size, next.fruit.chosenExit.size], [false, 0, 0, 0, 0]);
});

test('Pac cuts all eight corner orientations diagonally while ghosts wait for the center line', () => {
    for (const from of [{ x: 1, y: 0 }, { x: -1, y: 0 }, { x: 0, y: 1 }, { x: 0, y: -1 }]) {
        for (const side of [-1, 1]) {
            const { game, run } = fixture();
            openMaze(run);
            const turn = from.x ? { x: 0, y: side } : { x: side, y: 0 };
            const start = { x: 44 - from.x * 3, y: 44 - from.y * 3 };
            Object.assign(run.pac, { ...start, dx: from.x, dy: from.y, canTurn: true });
            const ghost = run.ghosts[0];
            Object.assign(ghost, { ...start, dx: from.x, dy: from.y, pressX: turn.x, pressY: turn.y, house: 'outside' });
            game.press(turn);
            for (let step = 1; step <= 3; step++) {
                game['advancePac'](run.pac, run);
                game['advanceGhost'](ghost, run);
                assert.deepEqual([run.pac.x, run.pac.y], [start.x + (from.x + turn.x) * step, start.y + (from.y + turn.y) * step]);
                assert.deepEqual([ghost.x, ghost.y], [start.x + from.x * step, start.y + from.y * step]);
            }
            game['advancePac'](run.pac, run);
            game['advanceGhost'](ghost, run);
            assert.deepEqual([run.pac.x, run.pac.y], [44 + turn.x * 4, 44 + turn.y * 4]);
            assert.deepEqual([ghost.x, ghost.y], [44 + turn.x, 44 + turn.y], 'ghost takes the new lane three pixel advances later than Pac');
        }
    }
});

test('all four tunnel directions retain native bounds and exact off-maze destinations', () => {
    const { game, run } = fixture();
    const cases = [
        { dx: -1, dy: 0, before: { x: -7, y: 116 }, beyond: { x: -8, y: 116 }, expected: { x: 232, y: 116 } },
        { dx: 1, dy: 0, before: { x: 231, y: 116 }, beyond: { x: 232, y: 116 }, expected: { x: -8, y: 116 } },
        { dx: 0, dy: -1, before: { x: 100, y: -7 }, beyond: { x: 100, y: -8 }, expected: { x: 100, y: 256 } },
        { dx: 0, dy: 1, before: { x: 100, y: 255 }, beyond: { x: 100, y: 256 }, expected: { x: 100, y: -8 } },
    ];
    for (const item of cases) {
        Object.assign(run.pac, item.before, { dx: item.dx, dy: item.dy });
        assert.equal(game['checkTunnel'](run.pac, run.maze), false);
        assert.deepEqual([run.pac.x, run.pac.y], [item.before.x, item.before.y]);
        Object.assign(run.pac, item.beyond);
        assert.equal(game['checkTunnel'](run.pac, run.maze), true);
        assert.deepEqual([run.pac.x, run.pac.y], [item.expected.x, item.expected.y]);
    }
});

test('continuing straight clears ghost queued intent exactly like native GhostDecidePress', () => {
    const { game, run } = fixture();
    openMaze(run);
    const ghost = run.ghosts[0];
    Object.assign(ghost, { x: 44, y: 44, dx: 1, dy: 0, pressX: 0, pressY: 0, house: 'outside' });
    run.maze.tiles[4][6].kind = 'wall';
    run.maze.tiles[6][6].kind = 'wall';
    game['advanceGhost'](ghost, run);
    assert.deepEqual([ghost.x, ghost.y, ghost.dx, ghost.dy, ghost.pressX, ghost.pressY], [45, 44, 1, 0, 0, 0]);
});
