import assert from 'node:assert/strict';
import { test } from 'node:test';
import { Game } from '../src/game';
import { ScoreScreen, TitleScreen, scoreLetters, titleLabels } from '../src/screens';
import { defaultSettings } from '../src/storage';

test('native title boots with original layered mazes, menu positions, score formatting, and selected character', () => {
    const title = new TitleScreen(defaultSettings);
    for (let i = 0; i < 64; i++) title.advance();
    const view = title.view(defaultSettings, [{ name: 'ace', score: 12340, level: 1, date: '' }], false);
    assert.equal(view.maze.width, 28);
    assert.equal(view.maze.height, 31);
    assert.equal(view.fade, 0);
    assert.equal(view.background?.maze.id.split(':')[0], 'title-maze-back');
    assert.equal(view.background?.maze.tiles.flat().some(tile => tile.kind === 'dot' || tile.kind === 'power'), false);
    assert.deepEqual(view.texts.slice(0, 8).map(text => [text.text, text.x, text.y]), [
        ['MR.PAC', 56, 28], ['MS.LILA', 56, 40], ['PLAYERS:ONE', 56, 64], ['DIFFICULTY:NORMAL', 56, 76],
        ['SOUND:ON', 56, 88], ['VIBRATE:ON', 56, 100], ['FULL SCREEN:OFF', 56, 112], ['ABOUT', 56, 124],
    ]);
    assert.deepEqual(view.texts.find(text => text.y === 152), { text: '  12340  ace', x: 48, y: 152, color: 0xffffff });
    assert.equal(view.texts.filter(text => text.y >= 152).length, 10);
    assert.equal(title.hit({ x: 56, y: 40 }, titleLabels(defaultSettings, false)), 1);
    assert.equal(title.hit({ x: 0, y: 0 }, titleLabels(defaultSettings, false)), -1);
    title.move('down');
    assert.equal(title.view(defaultSettings, [], false).pac.character, 'ms');
    assert.equal(title.view(defaultSettings, [], false).pac.y, 44);
    title.move('down');
    assert.equal(title.view(defaultSettings, [], false).texts.some(text => text.text === 'HIGH SCORES:'), false);
    title.done = true;
    title.advance();
    assert.equal(title.fade, 0, 'native completes the title without starting an extra fade-out');
});

test('title background reuses native ghost movement and empty difficulty without ending or emitting gameplay sounds', () => {
    const game = Game.titleBackground(() => 0.3);
    const start = game.view.ghosts.map(ghost => ghost.position);
    assert.equal(game.view.character, 'ms');
    assert.equal(game['players'][0].run.pac.active, false);
    assert.equal(game.view.pac.visible, false);
    const pacPosition = game.view.pac.position;
    for (let i = 0; i < 2000; i++) {
        game.advance();
        assert.equal(game.view.pac.visible, false);
        assert.deepEqual(game.view.pac.position, pacPosition);
        assert.equal(game.view.ghosts.every(ghost => ghost.state === 'scatter'), true);
    }
    assert.equal(game.view.state, 'playing');
    assert.equal(game.view.gameOverDone, false);
    assert.notDeepEqual(game.view.ghosts.map(ghost => ghost.position), start);
    assert.equal(game['players'][0].run.dotCount, 4);
    assert.equal(game['players'][0].run.ghosts[0].speed, 85);
    assert.deepEqual(game.drainEvents(), []);
});

test('native score alphabet, cursor, nine-character editing, End and Back positions match native source', () => {
    const score = new ScoreScreen(12340, 2);
    const background = new Game(defaultSettings).view;
    assert.deepEqual(scoreLetters[0], { text: 'a', x: 60, y: 184 });
    assert.deepEqual(scoreLetters[26], { text: ' ', x: 140, y: 232 });
    assert.deepEqual(scoreLetters[28], { text: 'Back', x: 60, y: 248 });
    assert.deepEqual(scoreLetters[29], { text: 'End', x: 140, y: 248 });
    assert.equal(score.hit({ x: 144, y: 252 }), 29);
    assert.equal(score.hit({ x: 0, y: 0 }), -1);
    assert.equal(score.view(background).texts.some(text => text.text === ' New high score 03!'), true);
    for (const key of 'A. B123!CDEFGHIJ') score.type(key);
    assert.equal(score.name, 'a. bcdefg');
    assert.equal(score.selected, 29);
    score.type('Backspace');
    assert.equal(score.name, 'a. bcdef');
    score.selected = 0;
    assert.equal(score.execute(), false);
    assert.equal(score.name, 'a. bcdefa');
    score.selected = 29;
    assert.equal(score.execute(), true);
    assert.deepEqual(score.view(background).pac, { x: 144, y: 252, character: 'mr', frame: 0 });
});

test('score directional navigation follows native nearest-aligned letter and fallback to Back/End', () => {
    const score = new ScoreScreen(100, 0);
    score.move('right'); assert.equal(score.selected, 1);
    score.move('down'); assert.equal(score.selected, 8);
    score.move('left'); assert.equal(score.selected, 7);
    score.selected = 21;
    score.move('down'); assert.equal(score.selected, 28);
    score.selected = 25;
    score.move('down'); assert.equal(score.selected, 29);
    score.move('left'); assert.equal(score.selected, 28);
    score.move('up'); assert.equal(score.selected, 21);
});
