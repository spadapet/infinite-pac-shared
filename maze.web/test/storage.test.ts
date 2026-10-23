import { test } from 'node:test';
import assert from 'node:assert/strict';
import { GameStorage, defaultSettings, type StorageLike } from '../src/storage';

function memory(initial: string | null = null): StorageLike {
    let text = initial;
    return { getItem: () => text, setItem: (_key, value) => { text = value; } };
}
const options = { ...defaultSettings };

test('settings and score categories survive reload without leaking mutable references', () => {
    const backing = memory();
    const errors: string[] = [];
    const store = new GameStorage(backing, message => errors.push(message));
    store.saveSettings({ ...options, character: 'ms', players: 2, sound: false });
    store.addScore(options, { name: 'ACE', score: 12340, level: 3, date: '2026-10-22' });
    store.addScore({ ...options, difficulty: 'hard' }, { name: 'HARD', score: 500, level: 1, date: '2026-10-22' });
    const restored = new GameStorage(backing, message => errors.push(message));
    assert.equal(restored.settings.character, 'ms');
    assert.equal(restored.settings.players, 2);
    assert.equal(restored.settings.sound, false);
    assert.deepEqual(restored.scores(options), [{ name: 'ace', score: 12340, level: 3, date: '2026-10-22' }]);
    const scores = restored.scores(options);
    scores[0].score = 0;
    assert.equal(restored.scores(options)[0].score, 12340);
    assert.equal(restored.scores({ ...options, difficulty: 'hard' })[0].name, 'hard');
    assert.deepEqual(errors, []);
});

test('high score tables keep the best ten and reject a tie for the last slot', () => {
    const store = new GameStorage(memory(), assert.fail);
    for (let score = 1; score <= 15; score++) store.addScore(options, { name: `P${score}`, score: score * 100, level: 1, date: '' });
    assert.deepEqual(store.scores(options).map(entry => entry.score), [1500, 1400, 1300, 1200, 1100, 1000, 900, 800, 700, 600]);
    assert.equal(store.qualifies(options, 600), false);
    assert.equal(store.qualifies(options, 601), true);
    assert.equal(store.qualifies(options, 0), false);
    assert.throws(() => store.addScore(options, { name: 'X', score: -1, level: 1, date: '' }), /Invalid high score/);
});

test('corrupt and inaccessible storage are reported rather than blocking gameplay', () => {
    const errors: string[] = [];
    const broken = new GameStorage(memory('{invalid'), message => errors.push(message));
    assert.deepEqual(broken.settings, defaultSettings);
    assert.match(errors[0], /could not be loaded/);
    const denied = new GameStorage({ getItem: () => null, setItem: () => { throw new Error('Quota exceeded'); } }, message => errors.push(message));
    denied.saveSettings({ ...options, sound: false });
    assert.match(errors[1], /could not be saved.*Quota exceeded/);
    assert.equal(denied.settings.sound, false);
});

test('schema validation rejects unsafe options and malformed score records', () => {
    const errors: string[] = [];
    const broken = new GameStorage(memory(JSON.stringify({ version: 1, settings: { ...options, players: 99 }, scores: {} })), message => errors.push(message));
    assert.equal(broken.settings.players, 1);
    assert.match(errors[0], /Invalid saved options/);
    new GameStorage(memory(JSON.stringify({ version: 1, settings: options, scores: { normal: [{ name: 'P', score: '9000' }] } })), message => errors.push(message));
    assert.match(errors[1], /Invalid saved high scores/);
});

test('native names allow blanks, preserve spaces and periods, and stop at nine characters', () => {
    const backing = memory();
    const store = new GameStorage(backing, assert.fail);
    for (const [index, name] of ['', '  A. B  ', 'ABCDEFGHIJKLM', 'P1!@Ä. C'].entries()) {
        store.addScore(options, { name, score: (index + 1) * 100, level: index + 1, date: '2026-10-23' });
    }
    assert.deepEqual(new GameStorage(backing, assert.fail).scores(options), [
        { name: 'p. c', score: 400, level: 4, date: '2026-10-23' },
        { name: 'abcdefghi', score: 300, level: 3, date: '2026-10-23' },
        { name: '  a. b  ', score: 200, level: 2, date: '2026-10-23' },
        { name: '', score: 100, level: 1, date: '2026-10-23' },
    ]);
});

test('native strict ranking appends equal scores after earlier ties and never inserts zero', () => {
    const store = new GameStorage(memory(), assert.fail);
    const add = (name: string, score: number) => store.addScore(options, { name, score, level: 1, date: '' });
    add('zero', 0);
    assert.deepEqual(store.scores(options), []);
    add('first', 1000);
    add('second', 1000);
    add('third', 1000);
    for (let i = 0; i < 7; i++) add('tail', 100);
    assert.equal(store.qualifies(options, 100), false);
    add('rejected', 100);
    add('fourth', 1000);
    assert.deepEqual(store.scores(options).map(entry => entry.name), ['first', 'second', 'third', 'fourth', ...Array(6).fill('tail')]);
    for (const score of [NaN, Infinity, -1, 0, 1.5, Number.MAX_SAFE_INTEGER + 1]) assert.equal(store.qualifies(options, score), false);
});

test('native categories depend only on mazes and difficulty, not character or player count', () => {
    const backing = memory();
    const store = new GameStorage(backing, assert.fail);
    store.addScore(options, { name: 'shared', score: 100, level: 1, date: '' });
    assert.deepEqual(store.scores({ ...options, character: 'ms', players: 2 }), store.scores(options));
    assert.deepEqual(store.scores({ ...options, mazeSet: 'ms' }), []);
    assert.deepEqual(store.scores({ ...options, difficulty: 'easy' }), []);
    store.addScore({ ...options, mazeSet: 'ms' }, { name: 'other', score: 200, level: 2, date: '' });
    const restored = new GameStorage(backing, assert.fail);
    assert.deepEqual(restored.scores(options).map(entry => entry.score), [100]);
    assert.deepEqual(restored.scores({ ...options, mazeSet: 'ms' }).map(entry => entry.score), [200]);
});

test('legacy browser scores migrate names without losing scores or blank-name records', () => {
    const store = new GameStorage(memory(JSON.stringify({ version: 1, settings: options, scores: { 'mr-normal': [
        { name: 'PLAYER 123456789', score: 900, level: 2, date: 'old' },
        { name: '', score: 800, level: 1, date: 'old' },
        { name: 'placeholder', score: 0, level: 0, date: '' },
    ] } })), assert.fail);
    assert.deepEqual(store.scores(options), [
        { name: 'player ', score: 900, level: 2, date: 'old' },
        { name: '', score: 800, level: 1, date: 'old' },
    ]);
});
