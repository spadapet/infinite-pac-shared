import assert from 'node:assert/strict';
import { test } from 'node:test';
import resources from '../../maze/assets/Values.Mazes.res.json';
import { Game } from '../src/game';
import type { GameOptions } from '../src/types';

type Resource = Record<string, unknown>;
const values: Resource = resources['res:values'];
const sets: Resource = resources.values_mazes.global;
function resolve(value: unknown): unknown {
    return typeof value === 'string' && value.startsWith('res:') ? resolve(values[value.slice(4)]) : value;
}
function fixture(overrides: Partial<GameOptions> = {}, level = 0) {
    const options: GameOptions = { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, ...overrides };
    const game = new Game(options, () => 0);
    const player = game['players'][0];
    player.level = level;
    const run = player.run = game['createLevel'](level);
    run.state = 'playing';
    for (const row of run.maze.tiles) for (const tile of row) Object.assign(tile, { kind: 'path', zone: 'normal' });
    return { game, player, run, options };
}

for (const mazeSet of ['mr', 'ms'] as const) for (const difficulty of ['easy', 'normal', 'hard'] as const) {
    test(`${mazeSet}/${difficulty}: all resource speed rows preserve Elroy boundaries and eyes/house/tunnel/scared priority`, () => {
        const { game, run, player, options } = fixture({ mazeSet, difficulty });
        const set = sets[`${options.mazeSet}-mazes-${options.difficulty}`] as Resource;
        const rows = (resolve(set.difficulties) as Resource[]).flatMap(row => {
            const count = Array.isArray(row.speed) ? (row.ghosts as number[]).length : 1;
            return Array.from({ length: count }, (_, index) => {
                const number = (key: string, fallback: number) => Number(Array.isArray(row[key]) ? row[key][index] : row[key] ?? fallback);
                return { speed: number('speed', 80), subtract: number('speedSubtract', 0), add: number('speedAdd', 0), elroy: number('elroyDots', 20), scared: number('scaredSeconds', 6), modes: resolve(row.ghostModeSeconds) as number[] };
            });
        });
        for (let level = 0; level <= rows.length + 1; level++) {
            player.level = level;
            const row = rows[Math.min(level, rows.length - 1)];
            run.fruit.active = true;
            for (const ghost of run.ghosts) Object.assign(ghost, { active: true, x: 44, y: 44, dx: 1, dy: 0 });
            game['scareGhosts'](run, true);
            assert.equal(run.ghostScaredCountdown, row.scared * 60, `power duration level${level + 1}`);
            run.ghostScaredCountdown = 0;
            for (let phase = 1; phase < 7; phase++) {
                run.ghostScatterCountdown = 0;
                run.ghostChaseCountdown = 1;
                run.ghostScatterChaseIndex = phase - 1;
                game['updateGhostMode'](run);
                const scatter = Math.max(60, (row.modes[phase * 2] || 1) * 60);
                const chase = Math.min(18000, (row.modes[phase * 2 + 1] || 300) * 60);
                assert.deepEqual([run.ghostScatterCountdown, run.ghostChaseCountdown], [scatter, chase], `schedule level${level + 1}/phase${phase}`);
            }
            for (const dots of [0, Math.floor(row.elroy / 2), Math.floor(row.elroy / 2) + 1, row.elroy, row.elroy + 1]) {
                for (const move of ['normal', 'scared', 'eyes'] as const) for (const house of ['outside', 'inside', 'leaving'] as const) {
                    for (const zone of ['normal', 'ghostSlow', 'outOfBounds'] as const) for (const powered of [false, true]) {
                        run.dotCount = dots;
                        run.ghostScaredCountdown = powered ? 1 : 0;
                        run.maze.tiles[5][5].zone = zone;
                        for (const ghost of run.ghosts) Object.assign(ghost, { move, house });
                        game['updateSpeeds'](run, level);
                        const pacSpeed = row.speed - row.subtract + row.add;
                        assert.equal(run.pac.speed, powered ? Math.max(pacSpeed, Math.floor((row.speed + 100) / 2)) : pacSpeed);
                        assert.equal(run.fruit.speed, Math.floor(row.speed / 2));
                        for (const [index, ghost] of run.ghosts.entries()) {
                            const expected = move === 'eyes' ? Math.max(150, row.speed - 5)
                                : zone !== 'normal' || house !== 'outside' ? Math.floor(row.speed / 2)
                                : move === 'scared' ? Math.floor(row.speed / 2) + 10
                                : index === 0 && dots <= Math.floor(row.elroy / 2) ? row.speed + 5
                                : index === 0 && dots <= row.elroy ? row.speed : row.speed - 5;
                            assert.equal(ghost.speed, expected, `level${level + 1}, ghost${index}, ${move}/${house}/${zone}, dots${dots}`);
                            assert.equal(ghost.advance, Math.floor(expected * 75.5 / 6000 * 65536));
                        }
                    }
                }
            }
        }
    });
}

test('fixed-point movement preserves negative pellet delays, residual fractions, multiple pixels and inactive actors', () => {
    for (const speed of [35, 45, 65, 70, 75, 85, 95, 100, 105, 120, 150, 160, 170]) for (const delay of [0, 1, 3]) {
        const { game, player, run } = fixture();
        for (const ghost of run.ghosts) ghost.active = false;
        game['updateSpeeds'] = () => {};
        game['updateGhostHouse'] = () => {};
        game['updateFruitMode'] = () => {};
        game['checkCollisions'] = () => {};
        game['advanceBackground'] = () => {};
        let moves = 0;
        game['advancePac'] = () => { moves++; };
        game['setSpeed'](run.pac, speed);
        run.pac.advancePosition = 12345 - delay * 65536;
        const increment = Math.floor(speed * 75.5 / 6000 * 65536);
        for (let tick = 1; tick <= 600; tick++) {
            game['advanceActors'](player, run);
            const total = 12345 - delay * 65536 + increment * tick;
            const expected = Math.max(0, Math.floor(total / 65536));
            assert.equal(moves, expected, `speed${speed}/delay${delay}/tick${tick}`);
            assert.equal(run.pac.advancePosition, total - expected * 65536);
        }
        const remainder = run.pac.advancePosition;
        run.pac.active = false;
        game['advanceActors'](player, run);
        assert.equal(run.pac.advancePosition, remainder);
        assert.equal(moves, Math.max(0, Math.floor((12345 - delay * 65536 + increment * 600) / 65536)));
    }
});

test('ghost-eating freezes Pac/fruit accumulators but consumes ghost fractions while eyes continue moving', () => {
    const { game, player, run } = fixture();
    run.ghostEatenCountdown = 3;
    run.fruit.active = true;
    const actors = [run.pac, run.fruit, ...run.ghosts];
    for (const actor of actors) { actor.advancePosition = 65000; actor.advance = 40000; }
    run.ghosts[0].move = 'eyes';
    const moved: number[] = [];
    game['advanceGhost'] = ghost => { moved.push(run.ghosts.indexOf(ghost)); };
    game['advancePac'] = () => { assert.fail('Pac must not move during ghost eating'); };
    game['advanceFruit'] = () => { assert.fail('Fruit must not move during ghost eating'); };
    game['advanceActors'](player, run);
    assert.deepEqual(moved, [0]);
    assert.deepEqual(actors.map(actor => actor.advancePosition), [65000, 65000, 39464, 39464, 39464, 39464]);
    assert.equal(run.ghostEatenCountdown, 2);
});

test('frightened and eating pauses suspend scatter/chase, then transitions reverse only eligible outside ghosts', () => {
    const { game, run } = fixture();
    run.ghostScatterCountdown = 1;
    run.ghostChaseCountdown = 2;
    run.ghostScaredCountdown = 2;
    run.ghostEatenCountdown = 2;
    Object.assign(run.ghosts[0], { house: 'outside', move: 'eaten', dx: 1, dy: 0 });
    Object.assign(run.ghosts[1], { house: 'outside', move: 'scared', dx: 1, dy: 0 });
    Object.assign(run.ghosts[2], { house: 'inside', move: 'scared' });
    Object.assign(run.ghosts[3], { house: 'outside', move: 'waiting' });
    for (let tick = 0; tick < 4; tick++) game['updateGhostMode'](run);
    assert.deepEqual([run.ghostEatenCountdown, run.ghostScaredCountdown, run.ghostScatterCountdown, run.ghostChaseCountdown], [0, 0, 1, 2]);
    assert.deepEqual(run.ghosts.map(ghost => ghost.move), ['eyes', 'normal', 'normal', 'waiting']);
    game['updateGhostMode'](run);
    assert.deepEqual(run.ghosts.map(ghost => [ghost.pressX, ghost.pressY]), [[0, 0], [-1, -0], [0, 0], [0, 0]]);
    assert.deepEqual([run.ghostScatterCountdown, run.ghostChaseCountdown], [0, 2]);
    game['updateGhostMode'](run);
    assert.equal(run.ghostChaseCountdown, 1);
    game['updateGhostMode'](run);
    assert.deepEqual([run.ghostScatterChaseIndex, run.ghostScatterCountdown, run.ghostChaseCountdown], [1, 420, 1200]);
});

test('zero-duration power pellets reverse ghosts without frightening them; flash boundaries remain native', () => {
    const { game, run } = fixture({ difficulty: 'hard' }, 28);
    for (const ghost of run.ghosts) Object.assign(ghost, { move: 'normal', house: 'outside', dx: 1, dy: 0 });
    game['scareGhosts'](run, true);
    assert.equal(run.ghostScaredCountdown, 0);
    assert.deepEqual(run.ghosts.map(ghost => ghost.move), ['normal', 'normal', 'normal', 'normal']);
    assert.deepEqual(run.ghosts.map(ghost => [ghost.pressX, ghost.pressY]), [[-1, -0], [-1, -0], [-1, -0], [-1, -0]]);
    run.ghosts[0].move = 'scared';
    for (const [ticks, state] of [[122, 'scared'], [121, 'flash'], [120, 'flash'], [108, 'flash'], [107, 'scared'], [95, 'scared'], [94, 'flash'], [27, 'flash'], [14, 'scared'], [13, 'flash']] as const) {
        run.ghostScaredCountdown = ticks;
        assert.equal(game['ghostViewState'](run.ghosts[0], run), state, `countdown${ticks}`);
    }
});

test('collision radius is strict and native fruit/power/ghost ordering lets a power pellet defeat the touching ghost', () => {
    const { game, player, run } = fixture();
    Object.assign(run.pac, { x: 44, y: 44, dx: 1, dy: 0 });
    for (const ghost of run.ghosts) ghost.active = false;
    const ghost = run.ghosts[0];
    Object.assign(ghost, { active: true, x: 48, y: 48, move: 'normal', house: 'outside' });
    assert.equal(game['actorsCollide'](run.pac, ghost), false);
    ghost.y = 47;
    assert.equal(game['actorsCollide'](run.pac, ghost), true);
    Object.assign(ghost, { x: 44, y: 44 });
    Object.assign(run.fruit, { active: true, x: 44, y: 44, type: 0 });
    run.maze.tiles[5][5].kind = 'power';
    run.dotCount = 2;
    game['checkCollisions'](player, run);
    assert.equal(player.score, 350);
    assert.equal(ghost.move, 'eaten');
    assert.equal(run.state, 'playing');
    assert.deepEqual(game.drainEvents().flatMap(event => event.type === 'sound' ? [event.name] : []), ['mr-eat-fruit', 'mr-eat-dot-1', 'mr-eat-ghost']);
    assert.equal(run.pac.advancePosition, -3 * 65536);
});

test('the last dot prevents ghost collision but a later normal ghost in the same collision pass can still catch Pac', () => {
    for (const lastDot of [true, false]) {
        const { game, player, run } = fixture();
        Object.assign(run.pac, { x: 44, y: 44, dx: 1, dy: 0 });
        for (const [index, ghost] of run.ghosts.entries()) Object.assign(ghost, { active: index < 2, x: 44, y: 44, move: index ? 'normal' : 'scared', house: 'outside' });
        if (lastDot) { run.maze.tiles[5][5].kind = 'dot'; run.dotCount = 1; }
        game['checkCollisions'](player, run);
        assert.equal(run.state, lastDot ? 'winning' : 'caught');
        assert.equal(player.score, lastDot ? 10 : 200);
        assert.equal(run.ghostEatenCountdown, lastDot ? 0 : 54);
    }
});

test('eaten/waiting ghost brain fallbacks use native chase targeting, not random frightened or scatter targeting', () => {
    const { game, run } = fixture();
    Object.assign(run.pac, { x: 100, y: 100, dx: 0, dy: -1 });
    run.ghostScatterCountdown = 1;
    for (const move of ['eaten', 'waiting'] as const) {
        const ghost = run.ghosts[0];
        Object.assign(ghost, { x: 100, y: 100, dx: 0, dy: -1, pressX: 0, pressY: 0, house: 'outside', move });
        game['advanceGhost'](ghost, run);
        assert.deepEqual([ghost.x, ghost.y, ghost.pressX, ghost.pressY], [100, 99, -1, 0]);
    }
});

test('multi-pixel Pac movement checks intermediate collisions before the final position', () => {
    const { game, player, run } = fixture();
    Object.assign(run.pac, { x: 44, y: 44, dx: 1, dy: 0, advance: Math.floor(170 * 75.5 / 6000 * 65536), advancePosition: 0 });
    for (const ghost of run.ghosts) ghost.active = false;
    Object.assign(run.ghosts[0], { active: true, x: 40, y: 44, dx: 0, dy: 0, move: 'normal', house: 'outside' });
    game['updateSpeeds'] = () => {};
    const check = game['checkCollisions'].bind(game);
    const checked: number[] = [];
    game['checkCollisions'] = (currentPlayer, currentRun) => { checked.push(currentRun.pac.x); check(currentPlayer, currentRun); };
    game['advanceActors'](player, run);
    assert.deepEqual(checked, [45, 46]);
    assert.equal(game['actorsCollide'](run.pac, run.ghosts[0]), false, 'the final position alone would miss the collision');
    assert.equal(run.state, 'caught');
    assert.deepEqual([run.pac.x, run.pac.y], [46, 44], 'native completes the already allocated pixel steps');
});
