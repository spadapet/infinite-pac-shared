import assert from 'node:assert/strict';
import { test } from 'node:test';
import { Game } from '../src/game';
import type { GameEvent, GameOptions } from '../src/types';

function advanceUntil(game: Game, predicate: () => boolean, limit: number): GameEvent[] {
    const events: GameEvent[] = [];
    for (let tick = 0; tick < limit && !predicate(); tick++) {
        game.advance();
        events.push(...game.drainEvents());
    }
    assert.equal(predicate(), true, `expected transition within ${limit} updates`);
    return events;
}

for (const character of ['mr', 'ms'] as const) for (const difficulty of ['easy', 'normal', 'hard'] as const) {
    for (const initialLives of [[3], [3, 3], [2, 3], [3, 2]]) {
        test(`${character}/${difficulty}/${initialLives.join('-')}: complete collision-driven match preserves both players and emits each final score once`, () => {
            const options: GameOptions = { character, mazeSet: character, difficulty, players: initialLives.length };
            const game = new Game(options, () => 0.25);
            const records = game['players'];
            const scores = [1110, 2220].slice(0, options.players);
            const levels = [1, 4].slice(0, options.players);
            records.forEach((player, index) => {
                player.lives = initialLives[index];
                player.score = scores[index];
                player.level = levels[index] - 1;
                player.run = game['createLevel'](player.level, index === 0);
                for (const row of player.run.maze.tiles) for (const tile of row) {
                    if (tile.kind === 'dot' || tile.kind === 'power') tile.kind = 'path';
                }
            });
            const remaining = [...initialLives];
            const ended = initialLives.map(() => false);
            const submitted: GameEvent[] = [];
            let expectedPlayer = 0;
            let deaths = 0;
            while (ended.some(value => !value)) {
                assert.equal(game.view.player, expectedPlayer + 1);
                assert.deepEqual(game.view.scores, scores);
                assert.equal(game.view.level, levels[expectedPlayer]);
                assert.equal(game.view.score, scores[expectedPlayer]);
                assert.equal(game.view.lives, remaining[expectedPlayer]);
                const readyEvents = advanceUntil(game, () => game.view.state === 'playing', 274);
                assert.deepEqual(readyEvents.filter(event => event.type === 'gameOver'), []);
                const player = records[expectedPlayer];
                const run = player.run;
                const other = records[1 - expectedPlayer];
                const inactiveTick = other?.run.stateTick;
                Object.assign(run.ghosts[0], { x: run.pac.x, y: run.pac.y, house: 'outside', move: 'normal' });
                game.advance();
                assert.equal(game.view.state, 'caught', 'an actual ghost collision starts death');
                game.drainEvents();
                const deathEvents = advanceUntil(game, () => player.run !== run || player.gameOver, 215);
                assert.deepEqual(deathEvents.filter(event => event.type === 'gameOver'), []);
                deaths++;
                if (remaining[expectedPlayer] > 1) {
                    remaining[expectedPlayer]--;
                    assert.equal(player.gameOver, false);
                    assert.equal(player.run.state, 'ready');
                    assert.equal(player.lives, remaining[expectedPlayer]);
                } else {
                    ended[expectedPlayer] = true;
                    assert.equal(game.view.playerGameOver, true);
                    assert.equal(game.view.state, 'dead');
                    const frozenTick = player.run.stateTick;
                    for (let tick = 0; tick < 119; tick++) {
                        game.advance();
                        assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), []);
                        assert.equal(player.run.stateTick, frozenTick);
                    }
                    game.advance();
                    const events = game.drainEvents().filter(event => event.type === 'gameOver');
                    assert.deepEqual(events, [{ type: 'gameOver', player: expectedPlayer + 1,
                        score: scores[expectedPlayer], level: levels[expectedPlayer] }]);
                    submitted.push(...events);
                    assert.equal(game.view.player, expectedPlayer + 1, 'notification precedes player handoff');
                }
                assert.equal(other?.run.stateTick, inactiveTick, 'inactive player remains frozen throughout the death');
                const next = records[1 - expectedPlayer];
                if (next && !ended[1 - expectedPlayer]) expectedPlayer = 1 - expectedPlayer;
                game.advance();
                assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), []);
                if (ended.every(Boolean)) break;
                assert.equal(game.view.player, expectedPlayer + 1);
                assert.equal(game.view.state, 'ready');
            }
            assert.equal(deaths, initialLives.reduce((sum, lives) => sum + lives, 0));
            assert.deepEqual(submitted.map(event => event.type === 'gameOver' ? event.player : 0).sort(), initialLives.map((_, index) => index + 1));
            assert.equal(game.view.state, 'gameover');
            assert.equal(game.view.gameOverDone, false);
            assert.deepEqual(game.view.scores, scores);
            for (let tick = 0; tick < 118; tick++) game.advance();
            assert.equal(game.view.gameOverDone, false);
            game.advance();
            assert.equal(game.view.gameOverDone, true);
            for (let tick = 0; tick < 600; tick++) game.advance();
            assert.deepEqual(game.drainEvents().filter(event => event.type === 'gameOver'), []);
            assert.deepEqual(game.view.scores, scores);
        });
    }
}
