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
    assert.deepEqual(restored.scores(options), [{ name: 'ACE', score: 12340, level: 3, date: '2026-10-22' }]);
    const scores = restored.scores(options);
    scores[0].score = 0;
    assert.equal(restored.scores(options)[0].score, 12340);
    assert.equal(restored.scores({ ...options, difficulty: 'hard' })[0].name, 'HARD');
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
