import assert from 'node:assert/strict';
import { test } from 'node:test';
import resources from '../../maze/assets/Values.Mazes.res.json';
import { Game } from '../src/game';
import { GameRenderer } from '../src/renderer';
import type { GameOptions } from '../src/types';

type Resource = Record<string, unknown>;
type NativeDifficulty = { ghosts: number; fruit: number; dots: number[] };
const values: Resource = resources['res:values'];
const sets: Resource = resources.values_mazes.global;
const characters = ['mr', 'ms'] as const;
const difficulties = ['easy', 'normal', 'hard'] as const;

function resolve(value: unknown): unknown {
    return typeof value === 'string' && value.startsWith('res:') ? resolve(values[value.slice(4)]) : value;
}

function nativeSet(options: GameOptions): Resource {
    return sets[`${options.mazeSet}-mazes-${options.difficulty}`] as Resource;
}

// Difficulty.cpp: scalar speed produces one row; vector speed expands ghosts.size() rows.
function nativeDifficulties(options: GameOptions): NativeDifficulty[] {
    const rows = resolve(nativeSet(options).difficulties) as Resource[];
    return rows.flatMap(row => {
        const count = Array.isArray(row.speed) ? (row.ghosts as number[]).length : 1;
        return Array.from({ length: count }, (_, index) => ({
            ghosts: Number(Array.isArray(row.ghosts) ? row.ghosts[index] : row.ghosts ?? 4),
            fruit: Number(Array.isArray(row.fruit) ? row.fruit[index] : row.fruit ?? 0),
            dots: (resolve(row.ghostDots) ?? [0, 0, 30, 60]) as number[],
        }));
    });
}

// PlayingGame.cpp: SetLevel scans backwards, omitting NONE and adjacent RANDOMs, up to seven.
function nativeHistory(fruits: number[], level: number): number[] {
    const result: number[] = [];
    let previous = 13;
    for (let index = level; index >= 0 && result.length < 7; index--) {
        const fruit = fruits[Math.min(index, fruits.length - 1)];
        if (fruit !== 13 && (fruit !== 12 || fruit !== previous)) result.push(fruit);
        previous = fruit;
    }
    return result;
}

function fixture(overrides: Partial<GameOptions> = {}, level = 0) {
    const options: GameOptions = { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, ...overrides };
    const game = new Game(options, () => 0);
    const player = game['players'][0];
    if (level) {
        player.level = level;
        player.run = game['createLevel'](level, true);
    }
    // Progression tests must not accidentally exercise movement, routes, or collisions.
    game['advanceActors'] = () => {};
    return { options, game, player, run: player.run };
}

function advance(game: Game, count: number): void {
    for (let index = 0; index < count; index++) game.advance();
}

function sounds(game: Game): string[] {
    return game.drainEvents().flatMap(event => event.type === 'sound' ? [event.name] : []);
}

for (const character of characters) for (const mazeSet of characters) for (const difficulty of difficulties) {
    const label = `${character}/${mazeSet}/${difficulty}`;
    test(`${label}: native resource defaults, every expanded difficulty row, cyclic maze order and high-level saturation`, () => {
        const { game, options, player } = fixture({ character, mazeSet, difficulty });
        const set = nativeSet(options);
        assert.deepEqual([player.lives, player.nextFreeLife, player.freeLifeRepeat, player.freeLivesLeft],
            [resolve(set.lives), resolve(set.firstLife), resolve(set.repeatLife), resolve(set.maxFreeLives)]);
        const rows = nativeDifficulties(options);
        assert.equal(rows.length, difficulty === 'easy' ? 13 : 29);
        const order = resolve(set.mazes) as string[];
        const levels = [...Array.from({ length: 38 }, (_, index) => index), 63, 127];
        for (const level of levels) {
            player.level = level;
            const run = player.run = game['createLevel'](level);
            const expected: NativeDifficulty = rows[Math.min(level, rows.length - 1)];
            assert.equal(run.maze.id.split(':')[0], order[level % order.length], `maze level ${level + 1}`);
            assert.equal(game.view.level, level + 1);
            assert.equal(run.ghostCount, expected.ghosts, `ghost count level ${level + 1}`);
            assert.deepEqual(run.ghosts.filter(ghost => ghost.active).map(ghost => ghost.dotCounter),
                expected.dots.slice(0, expected.ghosts), `expanded dot quotas level ${level + 1}`);
            assert.deepEqual(game.view.fruitHistory, nativeHistory(rows.map(row => row.fruit), level), `history level ${level + 1}`);
            run.dotCount = run.fruitThresholds[0];
            game['updateFruitMode'](run, player);
            assert.equal(run.fruit.type, expected.fruit === 12 ? 0 : expected.fruit, `fruit level ${level + 1}`);
            assert.equal(run.fruitIndex, 1);
        }
    });

    test(`${label}: bonus thresholds advance even at the four-life cap without consuming the award quota`, () => {
        const { game, player } = fixture({ character, mazeSet, difficulty });
        const repeat = difficulty === 'hard' ? 20000 : 10000;
        const quota = difficulty === 'easy' ? 3 : 2;
        player.score = 9999;
        game['checkFreeLife'](player);
        assert.deepEqual([player.lives, player.freeLivesLeft, player.nextFreeLife], [3, quota, 10000]);
        assert.deepEqual(sounds(game), []);
        player.score = 10000;
        game['checkFreeLife'](player);
        assert.deepEqual([player.lives, player.freeLivesLeft], [4, quota - 1]);
        assert.deepEqual(sounds(game), ['mr-free-life']);
        player.score = 10000 + repeat;
        game['checkFreeLife'](player);
        assert.deepEqual([player.lives, player.freeLivesLeft, player.nextFreeLife], [4, quota - 1, 10000 + repeat * 2]);
        assert.deepEqual(sounds(game), ['mr-free-life'], 'native plays the sound even when no life can be granted');
        player.lives = 1;
        for (let remaining = quota - 1; remaining > 0; remaining--) {
            player.score = player.nextFreeLife;
            game['checkFreeLife'](player);
            assert.equal(player.freeLivesLeft, remaining - 1);
        }
        assert.equal(player.lives, quota);
        assert.equal(player.freeLivesLeft, 0);
        game.drainEvents();
        player.score += repeat * 10;
        game['checkFreeLife'](player);
        assert.equal(player.lives, quota, 'exhausted lifetime quota never replenishes');
        assert.deepEqual(sounds(game), []);
    });
}

for (const character of characters) {
    test(`${character}: pellet, power, all twelve fruits and ghost-chain values accumulate without score saturation`, () => {
        const { game, player, run } = fixture({ character });
        player.nextFreeLife = 0;
        player.score = 9_999_990;
        run.dotCount = 20;
        game['eatDot'](player, run, false);
        assert.equal(player.score, 10_000_000);
        game['eatDot'](player, run, true);
        assert.equal(player.score, 10_000_050);
        let expected: number = Number(player.score);
        for (const [fruit, points] of [100, 300, 500, 700, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000].entries()) {
            run.fruit.type = fruit;
            game['eatFruit'](player, run);
            expected += points;
            assert.equal(player.score, expected, `fruit ${fruit}`);
            assert.deepEqual([run.points.at(-1)?.value, run.points.at(-1)?.remaining], [points, 120]);
        }
        for (const [index, points] of [200, 400, 800, 1600, 200, 200].entries()) {
            run.ghostEatenIndex = index;
            game['eatGhost'](player, run, run.ghosts[0]);
            expected += points;
            assert.equal(player.score, expected, `ghost chain index ${index}`);
            assert.equal(run.points.at(-1)?.value, points);
        }
        assert.equal(game.view.score, expected);
        assert.equal(game.view.highScore, expected);
    });
}

for (const food of ['dot', 'power', 'fruit', 'ghost'] as const) {
    test(`${food}: native awards a crossed free-life threshold after maze scoring effects, not inside the scoring callback`, () => {
        const { game, player, run } = fixture();
        run.state = 'playing';
        run.dotCount = 20;
        const points = food === 'dot' ? 10 : food === 'power' ? 50 : food === 'fruit' ? 100 : 200;
        player.score = 10000 - points;
        game['advanceActors'] = () => {
            if (food === 'fruit') game['eatFruit'](player, run);
            else if (food === 'ghost') game['eatGhost'](player, run, run.ghosts[0]);
            else game['eatDot'](player, run, food === 'power');
            assert.equal(player.score, 10000);
            assert.equal(player.lives, 3, 'Player::CheckFreeLife runs only after PlayingMaze::Advance returns');
        };
        game.advance();
        assert.equal(player.lives, 4);
        const scoringSound = food === 'fruit' ? 'mr-eat-fruit' : food === 'ghost' ? 'mr-eat-ghost' : 'mr-eat-dot-1';
        assert.deepEqual(sounds(game), [scoringSound, 'mr-free-life']);
    });
}

test('one outer advance consumes at most one pending free-life threshold, even with several scoring callbacks', () => {
    const { game, player, run } = fixture({ difficulty: 'easy' });
    run.state = 'playing';
    run.dotCount = 20;
    player.score = 39980;
    player.lives = 1;
    game['advanceActors'] = () => {
        game['eatDot'](player, run, false);
        game['eatDot'](player, run, false);
    };
    game.advance();
    assert.deepEqual([player.score, player.lives, player.freeLivesLeft, player.nextFreeLife], [40000, 2, 2, 20000]);
    assert.equal(sounds(game).filter(name => name === 'mr-free-life').length, 1);
});

for (const character of characters) {
    test(`${character}: first-player announcement remains through update151, then READY starts at its own counter1`, () => {
        const { game } = fixture({ character });
        advance(game, 151);
        assert.equal(game.view.playerReady, true, 'native BEFORE_TIME enters PLAYER_READY at counter1 on update1');
        game.advance();
        assert.equal(game.view.playerReady, false);
        assert.equal(game.view.state, 'ready');
        assert.equal(game.view.stateTick, 1, 'the native READY phase resets its counter');
        assert.deepEqual(sounds(game), ['mr-intro']);
    });

    test(`${character}: first player starts gameplay on update273, not272`, () => {
        const { game } = fixture({ character });
        advance(game, 272);
        assert.equal(game.view.state, 'ready');
        game.advance();
        assert.equal(game.view.state, 'playing');
        assert.equal(game.view.stateTick, 1);
        assert.deepEqual(game.drainEvents().filter(event => event.type === 'stopSound'), [{ type: 'stopSound', name: 'mr-intro' }]);
    });

    test(`${character}: second player has only the122-update READY path and no intro on initial or subsequent levels`, () => {
        const { game } = fixture({ character, players: 2 });
        game['currentPlayer'] = 1;
        advance(game, 121);
        assert.equal(game.view.state, 'ready');
        assert.equal(game.view.playerReady, false);
        game.advance();
        assert.equal(game.view.state, 'playing');
        assert.deepEqual(sounds(game), []);
        const player = game['players'][1];
        game['finishLevel'](player);
        assert.equal(game.view.player, 2);
        assert.equal(game.view.level, 2);
        assert.equal(game.view.playerReady, false);
        advance(game, 121);
        assert.equal(game.view.state, 'ready');
        game.advance();
        assert.equal(game.view.state, 'playing');
        assert.deepEqual(sounds(game), []);
    });
}

for (const state of ['caught', 'dying'] as const) {
    test(`${state}: native strict-greater-than transition boundary is retained`, () => {
        const { game, run } = fixture();
        run.state = state;
        const limit = state === 'caught' ? 60 : 120;
        advance(game, limit + 1);
        assert.equal(run.state, state);
        assert.equal(run.stateTick, limit + 1);
        game.advance();
        assert.equal(run.state, state === 'caught' ? 'dying' : 'dead');
        assert.equal(run.stateTick, 1);
    });
}

test('dying sound fires at counter30 once; death reset occurs only after dead counter30', () => {
    const { game, player, run } = fixture();
    run.state = 'dying';
    advance(game, 30);
    assert.deepEqual(sounds(game), []);
    game.advance();
    assert.deepEqual(sounds(game), ['mr-die']);
    advance(game, 90);
    assert.equal(run.state, 'dying');
    game.advance();
    assert.equal(run.state, 'dead');
    advance(game, 30);
    assert.equal(player.lives, 3);
    assert.equal(run.stateTick, 31);
    game.advance();
    assert.equal(player.lives, 2);
    assert.notEqual(player.run, run);
    assert.equal(player.run.state, 'ready');
    assert.equal(player.run.stateTick, 0);
    advance(game, 121);
    assert.equal(player.run.state, 'ready');
    game.advance();
    assert.equal(player.run.state, 'playing');
    assert.equal(game.view.playerReady, false);
    assert.deepEqual(sounds(game), []);
});

for (const mazeSet of characters) for (const difficulty of difficulties) {
    test(`${mazeSet}/${difficulty}: death preserves consumed maze, fruit history, score and bonus accounting but clears transient displays`, () => {
        const { game, player, run } = fixture({ mazeSet, difficulty }, 10);
        const eaten = run.maze.tiles.flat().filter(tile => tile.kind === 'dot').slice(0, 9);
        for (const tile of eaten) tile.kind = 'path';
        run.dotCount -= eaten.length;
        run.fruitIndex = 1;
        run.dotEffect = 1;
        run.ghosts.forEach((ghost, index) => ghost.dotCounter = index + 5);
        run.fruit.active = true;
        run.fruitCounter = 300;
        run.points.push({ x: 1, y: 2, value: 100, remaining: 80, color: 0 });
        run.particles.push({ x: 1, y: 2, vy: -1, animation: 0, frame: 2, scale: 1, timeScale: 1 });
        player.score = 7650;
        const accounting = [player.nextFreeLife, player.freeLifeRepeat, player.freeLivesLeft];
        const history = game.view.fruitHistory;
        const thresholds = run.fruitThresholds.slice();
        game['finishDeath'](player);
        const reset = player.run;
        assert.equal(reset.maze, run.maze);
        assert.equal(reset.dotCount, run.dotCount);
        assert.equal(reset.dotCountTotal, run.dotCountTotal);
        assert.deepEqual(reset.fruitThresholds, thresholds);
        assert.equal(reset.fruitIndex, 1);
        assert.equal(reset.dotEffect, 1);
        assert.deepEqual(reset.ghosts.map(ghost => ghost.dotCounter), [5, 6, 7, 8]);
        assert.deepEqual([reset.globalDotIndex, reset.globalDotCounter, reset.lastDotCounter], [1, 0, 0]);
        assert.deepEqual([reset.fruit.active, reset.fruitCounter, reset.points.length, reset.particles.length], [false, 0, 0, 0]);
        assert.deepEqual(game.view.fruitHistory, history);
        assert.deepEqual([player.score, player.level, player.lives], [7650, 10, 2]);
        assert.deepEqual([player.nextFreeLife, player.freeLifeRepeat, player.freeLivesLeft], accounting);
        assert.equal(game.view.highScore, 7650);
    });
}

test('win waits through counter240, advances without changing player, and preserves score/lives/bonus quota', () => {
    const { game, player, run } = fixture({ players: 2 }, 7);
    run.state = 'winning';
    player.score = 7650;
    player.lives = 2;
    const accounting = [player.nextFreeLife, player.freeLifeRepeat, player.freeLivesLeft];
    advance(game, 241);
    assert.equal(player.level, 7);
    assert.equal(run.stateTick, 241);
    game.advance();
    assert.equal(player.level, 8);
    assert.equal(game.view.player, 1);
    assert.equal(game.view.level, 9);
    assert.equal(player.run.maze.id.split(':')[0], 'mr-maze-0-0');
    assert.deepEqual([player.score, player.lives], [7650, 2]);
    assert.deepEqual([player.nextFreeLife, player.freeLifeRepeat, player.freeLivesLeft], accounting);
    assert.deepEqual(game.view.fruitHistory, [8, 7, 6, 5, 4, 3, 2]);
    advance(game, 272);
    assert.equal(game.view.state, 'ready', 'new P1 levels repeat the native announcement plus READY sequence');
    game.advance();
    assert.equal(game.view.state, 'playing');
    assert.deepEqual(sounds(game), [], 'intro is enabled only for player0/level0');
});

test('switching after a nonfatal death advances the destination player on the very next update', () => {
    const { game, player } = fixture({ players: 2 });
    game['finishDeath'](player);
    assert.equal(game.view.player, 1);
    assert.equal(player.lives, 2);
    const other = game['players'][1];
    const before = other.run.stateTick;
    game.advance();
    assert.equal(game.view.player, 2);
    assert.equal(other.run.stateTick, before + 1);
    assert.equal(player.run.stateTick, 0, 'the inactive player stays frozen');
});

test('alternating turns preserve each score/history/level and the live high-score maximum', () => {
    const { game, player } = fixture({ players: 2 }, 6);
    const other = game['players'][1];
    other.level = 12;
    other.run = game['createLevel'](12);
    player.score = 3500;
    other.score = 7200;
    const firstHistory = game.view.fruitHistory;
    game['finishDeath'](player);
    game.advance();
    assert.deepEqual([game.view.player, game.view.level, game.view.score, game.view.highScore], [2, 13, 7200, 7200]);
    assert.deepEqual(game.view.scores, [3500, 7200]);
    assert.deepEqual(game.view.fruitHistory, [12, 11, 10, 9, 8, 7, 6]);
    game['finishDeath'](other);
    game.advance();
    assert.deepEqual([game.view.player, game.view.level, game.view.score, game.view.highScore], [1, 7, 3500, 7200]);
    assert.deepEqual(game.view.fruitHistory, firstHistory);
    const detached = game.view.fruitHistory;
    detached[0] = 99;
    assert.deepEqual(game.view.fruitHistory, firstHistory, 'view consumers cannot corrupt the player history');
});

for (const players of [1, 2]) {
    test(`${players} player(s): fatal death freezes120 updates before score notification, then switches/ends on the next update`, () => {
        const { game, player, run } = fixture({ players });
        player.lives = 1;
        player.score = 7650;
        run.state = 'dead';
        run.stateTick = 31;
        game.advance();
        assert.equal(player.gameOver, true);
        assert.equal(game['gameOver'], false, 'native global gameover is deferred until after the player notification');
        assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), []);
        const frozenTick = run.stateTick;
        advance(game, 119);
        assert.equal(game.view.player, 1);
        assert.equal(run.stateTick, frozenTick);
        assert.equal(game['gameOver'], false);
        assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), []);
        game.advance();
        assert.equal(game.view.player, 1, 'host is notified before switching players');
        assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), [{ type: 'gameOver', player: 1, score: 7650, level: 1 }]);
        game.advance();
        assert.equal(game['gameOver'], players === 1);
        assert.equal(game.view.player, players === 1 ? 1 : 2);
        if (players === 2) assert.equal(game['players'][1].run.stateTick, 1);
        advance(game, 200);
        assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), [], 'a player score is submitted only once');
    });
}

test('last surviving P2 still receives its120-update gameover hold and notification before global gameover', () => {
    const { game } = fixture({ players: 2 });
    game['players'][0].gameOver = true;
    game['currentPlayer'] = 1;
    const player = game['players'][1];
    player.lives = 1;
    player.run.state = 'dead';
    player.run.stateTick = 31;
    game.advance();
    assert.equal(game['gameOver'], false);
    assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), []);
    advance(game, 120);
    assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), [{ type: 'gameOver', player: 2, score: 0, level: 1 }]);
    assert.equal(game['gameOver'], false);
    game.advance();
    assert.equal(game['gameOver'], true);
    assert.equal(game.view.state, 'gameover');
});

function hudRecorder() {
    const renderer: GameRenderer = Object.create(GameRenderer.prototype);
    const text: string[] = [];
    const fruits: Array<{ name: string; x: number }> = [];
    let lives = 0;
    Object.assign(renderer, { layoutWidth: 224, boardTop: 24,
        hudTextPool: [], livesPool: [], fruitPool: [], pointTextPool: [] });
    renderer['drawBitmapText'] = (value, _x, _y, _tint, _pool, count) => {
        text.push(value);
        return (count ?? 0) + value.length;
    };
    renderer['drawAnimation'] = (_name, _frame, _position, _scale, _pool, _layer, count) => {
        lives++;
        return count + 1;
    };
    renderer['hideSprites'] = () => {};
    renderer['obtainSprite'] = () => Object.create(null) as ReturnType<GameRenderer['obtainSprite']>;
    renderer['paintSprite'] = (_sprite, name, position) => { fruits.push({ name, x: position.x }); };
    return {
        draw: (game: Game) => {
            text.length = 0;
            fruits.length = 0;
            lives = 0;
            renderer['drawHud'](game.view);
            return { text: text.slice(), fruits: fruits.slice(), lives };
        },
    };
}

test('score HUD keeps independent scores, clamps only rendered digits, and draws oldest history at the right edge', () => {
    const { game, player } = fixture({ players: 2 }, 8);
    const recorder = hudRecorder();
    player.score = 10_000_010;
    game['players'][1].score = 10;
    let hud = recorder.draw(game);
    assert.deepEqual(hud.text, ['1UP', '9999999', '2UP', '     10', 'HIGH SCORE', '9999999']);
    assert.equal(player.score, 10_000_010, 'FormatScoreAsString caps display, never player accounting');
    assert.equal(hud.lives, 3, 'PLAYER_READY displays every remaining life');
    assert.deepEqual(hud.fruits, [2, 3, 4, 5, 6, 7, 8].map((fruit, index) => ({ name: `char-sprites.fruit[${fruit}]`, x: 200 - 16 * index })));
    player.run.state = 'playing';
    game['tickCount'] = 15;
    hud = recorder.draw(game);
    assert.deepEqual(hud.text, ['9999999', '2UP', '     10', 'HIGH SCORE', '9999999']);
    assert.equal(hud.lives, 2, 'READY and later states exclude the active life');
    game['currentPlayer'] = 1;
    hud = recorder.draw(game);
    assert.deepEqual(hud.text, ['1UP', '9999999', '     10', 'HIGH SCORE', '9999999']);
    game['gameOver'] = true;
    hud = recorder.draw(game);
    assert.deepEqual(hud.text, ['1UP', '9999999', '2UP', '     10', 'HIGH SCORE', '9999999']);
});

test('score HUD forces the departing player label visible while a death schedules the next-player switch', () => {
    const { game, player } = fixture({ players: 2 });
    game['tickCount'] = 15;
    game['finishDeath'](player);
    const hud = hudRecorder().draw(game);
    assert.deepEqual(hud.text, ['1UP', '     00', '2UP', '     00', 'HIGH SCORE', '     00'],
        'PlayingGame.cpp468: pending _switchPlayer overrides the active-name blink');
});

test('free-life eligibility is checked before the final-death handler consumes the last life', () => {
    const { game, player, run } = fixture();
    player.lives = 1;
    player.score = 10000;
    run.state = 'dead';
    run.stateTick = 31;
    game.advance();
    assert.equal(player.gameOver, false, 'native Player::Advance checks bonus eligibility before OnPacDied');
    assert.equal(game['gameOver'], false);
    assert.deepEqual([player.lives, player.freeLivesLeft], [1, 1]);
    assert.equal(player.run.state, 'ready');
    assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), []);
});
