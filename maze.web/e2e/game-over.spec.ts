import { test, expect, type Page } from '@playwright/test';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { build } from 'esbuild';
import type { GameView, GameOptions } from '../src/types';
import type { HighScore } from '../src/storage';

interface MatchSnapshot {
    mode: 'menu' | 'game';
    paused: boolean;
    closing: boolean;
    fade: number;
    view: GameView;
    entry: { score: number; slot: number; name: string; selected: number } | null;
    rendered: { kind: string; foreground: boolean; texts: string[] } | null;
    players: { score: number; level: number; lives: number; over: boolean }[];
}
interface MatchHarness {
    snapshot(): MatchSnapshot;
    tick(frames: number): MatchSnapshot;
    seed(players: { score: number; lives: number; level: number }[]): void;
    collide(): void;
}
declare global {
    interface Window { matchHarness: MatchHarness }
}

let bundle: string;
let html: string;
test.beforeAll(async () => {
    const [main, template, css] = await Promise.all([
        readFile(new URL('../src/main.ts', import.meta.url), 'utf8'),
        readFile(new URL('../index.html', import.meta.url), 'utf8'),
        readFile(new URL('../src/style.css', import.meta.url), 'utf8'),
    ]);
    expect(main.match(/requestAnimationFrame\(frame\)/g)).toHaveLength(2);
    // Only this in-memory test bundle exposes fixtures and a fast clock. All game,
    // screen, WebGL, audio, controller and persistence code remains the real code.
    const hook = `
        window.matchHarness = {
            snapshot() {
                const rendered = renderer?.['lastScreen'];
                return { mode, paused, closing: scoreClosing, fade: scoreFade, view: game.view,
                    entry: scoreScreen ? { score: scoreScreen.score, slot: scoreScreen.slot,
                        name: scoreScreen.name, selected: scoreScreen.selected } : null,
                    rendered: rendered ? { kind: rendered.kind, foreground: rendered.showForeground !== false,
                        texts: rendered.texts.map(text => text.text) } : null,
                    players: game['players'].map(player => ({ score: player.score, level: player.level + 1,
                        lives: player.lives, over: player.gameOver })) };
            },
            tick(frames) {
                for (let index = 0; index < frames; index++) window.matchClock.step();
                return this.snapshot();
            },
            seed(players) {
                if (mode !== 'game' || game.view.state !== 'ready') throw new Error('Seed only a fresh match');
                if (players.length !== game['players'].length) throw new Error('Wrong player count');
                players.forEach((seed, index) => {
                    const player = game['players'][index];
                    player.score = seed.score;
                    player.lives = seed.lives;
                    player.level = seed.level - 1;
                    player.nextFreeLife = 0;
                    player.run = game['createLevel'](player.level, index === 0);
                    for (const row of player.run.maze.tiles) for (const tile of row) {
                        if (tile.kind === 'dot' || tile.kind === 'power') tile.kind = 'path';
                    }
                });
            },
            collide() {
                if (mode !== 'game' || paused || scoreScreen || game.view.state !== 'playing') throw new Error('Collision requires active play');
                const run = game['players'][game['currentPlayer']].run;
                Object.assign(run.ghosts[0], { x: run.pac.x, y: run.pac.y, house: 'outside', move: 'normal' });
            },
        };`;
    const result = await build({ stdin: {
        contents: main.replaceAll('requestAnimationFrame(frame)', 'window.matchClock.request(frame)') + hook,
        resolveDir: fileURLToPath(new URL('../src', import.meta.url)), loader: 'ts', sourcefile: 'match-main.ts',
    }, bundle: true, write: false, format: 'iife', platform: 'browser', target: 'es2022',
        define: { 'import.meta.env.BASE_URL': JSON.stringify('./') }, loader: { '.css': 'empty' } });
    bundle = result.outputFiles[0].text;
    html = template.replace('./src/main.ts', './match-main.js').replace('</head>', `<style>${css}</style></head>`);
});

async function startMatch(page: Page, options: Partial<GameOptions> = {}, table: HighScore[] = []): Promise<void> {
    const settings = { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 2, sound: false, vibration: false, ...options };
    await page.addInitScript(({ settings, table }) => {
        if (!localStorage.getItem('infinite-pac-web.v1')) localStorage.setItem('infinite-pac-web.v1', JSON.stringify({ version: 1, settings,
            scores: { [`${settings.mazeSet}-${settings.difficulty}`]: table } }));
        let next: FrameRequestCallback | undefined;
        let now = 1000;
        Object.assign(window, { matchClock: {
            request(callback: FrameRequestCallback) { next = callback; },
            step() {
                if (!next) throw new Error('Game frame was not scheduled');
                const callback = next;
                next = undefined;
                now += 100;
                callback(now);
            },
        } });
        Object.defineProperty(navigator, 'getGamepads', { value: () => [{ index: 0, id: 'Xbox Wireless Controller', connected: true,
            mapping: 'standard', axes: [0, 0], buttons: Array.from({ length: 16 }, (_, index) => ({
                pressed: document.documentElement.dataset.pad === String(index),
            })) }] });
    }, { settings, table });
    await page.route('**/match-main.js', route => route.fulfill({ contentType: 'text/javascript', body: bundle }));
    await page.route('http://127.0.0.1:4173/', route => route.fulfill({ contentType: 'text/html', body: html }));
    await page.goto('/');
    await expect(page.locator('#menu')).toHaveJSProperty('hidden', false, { timeout: 30_000 });
    await step(page, 12);
    await expect(page.locator('#menu')).toBeVisible();
    await page.getByRole('button', { name: settings.mazeSet === 'mr' ? 'Play Mr. Pac' : 'Play Ms. Lila', exact: true }).click();
    await step(page, 2);
    await expect(page.locator('#menu')).toBeHidden();
}

async function step(page: Page, frames = 1): Promise<MatchSnapshot> {
    return page.evaluate(frames => window.matchHarness.tick(frames), frames);
}
async function snapshot(page: Page): Promise<MatchSnapshot> {
    return page.evaluate(() => window.matchHarness.snapshot());
}
async function reach(page: Page, target: 'playing' | 'score' | 'closed' | 'menu', player?: number): Promise<MatchSnapshot> {
    const result = await page.evaluate(({ target, player }) => {
        for (let index = 0; index < 180; index++) {
            const state = window.matchHarness.snapshot();
            const matches = target === 'score' ? state.entry !== null && !state.closing && state.fade === 0.875
                : target === 'closed' ? state.entry === null
                : target === 'menu' ? state.mode === 'menu'
                : state.mode === 'game' && !state.entry && state.view.state === 'playing' && state.view.player === player;
            if (matches) return state;
            window.matchHarness.tick(1);
        }
        throw new Error(`Did not reach ${target} for player ${player}: ${JSON.stringify(window.matchHarness.snapshot())}`);
    }, { target, player });
    return result;
}
async function seed(page: Page, players: { score: number; lives: number; level: number }[]): Promise<void> {
    await page.evaluate(players => window.matchHarness.seed(players), players);
}
async function die(page: Page): Promise<void> {
    await page.evaluate(() => window.matchHarness.collide());
    const state = await step(page);
    expect(state.view.state).toBe('caught');
}
async function pad(page: Page, button: number): Promise<void> {
    await page.evaluate(button => { document.documentElement.dataset.pad = String(button); }, button);
    await step(page);
    await page.evaluate(() => { document.documentElement.dataset.pad = ''; });
    await step(page);
}
async function scores(page: Page): Promise<Record<string, HighScore[]>> {
    return page.evaluate(() => JSON.parse(localStorage.getItem('infinite-pac-web.v1') ?? '{}').scores);
}
const previous = (score: number, name = 'previous'): HighScore => ({ name, score, level: 1, date: '2026-10-23' });

for (const character of ['mr', 'ms'] as const) for (const difficulty of ['easy', 'normal', 'hard'] as const) {
    test(`${character}/${difficulty}: both players qualify, save independently, finish and persist in the matching table`, async ({ page }, testInfo) => {
        const errors: string[] = [];
        page.on('pageerror', error => errors.push(error.message));
        page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
        await startMatch(page, { character, mazeSet: character, difficulty });
        await seed(page, [{ score: 1230, lives: 1, level: 2 }, { score: 4560, lives: 1, level: 5 }]);
        await reach(page, 'playing', 1);
        await die(page);
        let state = await reach(page, 'score');
        expect(state.entry).toEqual({ score: 1230, slot: 0, name: '', selected: 0 });
        expect(state.view.player).toBe(1);
        expect(state.view.state).toBe('dead');
        expect(state.view.playerGameOver).toBe(true);
        expect(state.rendered?.foreground).toBe(true);
        expect(state.rendered?.texts).toContain(' New high score 01!');
        const frozen = state.view.tick;
        await step(page, 30);
        expect((await snapshot(page)).view.tick).toBe(frozen);
        await page.keyboard.type('FIRST');
        await page.keyboard.press('Enter');
        await page.keyboard.press('Enter');
        await reach(page, 'closed');
        await reach(page, 'playing', 2);
        state = await snapshot(page);
        expect(state.players).toEqual([{ score: 1230, lives: 1, level: 2, over: true }, { score: 4560, lives: 1, level: 5, over: false }]);
        await die(page);
        state = await reach(page, 'score');
        expect(state.entry).toEqual({ score: 4560, slot: 0, name: '', selected: 0 });
        if (character === 'ms' && difficulty === 'normal') await page.screenshot({ path: testInfo.outputPath('second-player-score.png') });
        await page.keyboard.type('SECOND');
        await page.getByRole('button', { name: 'Save score', exact: true }).click();
        await reach(page, 'closed');
        state = await snapshot(page);
        expect(state.view.state).toBe('dead');
        await reach(page, 'menu');
        const saved = await scores(page);
        expect(saved).toEqual({ [`${character}-${difficulty}`]: [
            { name: 'second', score: 4560, level: 5, date: expect.any(String) },
            { name: 'first', score: 1230, level: 2, date: expect.any(String) },
        ] });
        await page.reload();
        await expect(page.locator('#menu')).toHaveJSProperty('hidden', false, { timeout: 30_000 });
        await step(page, 12);
        await expect(page.locator('#menu')).toBeVisible();
        expect(await scores(page)).toEqual(saved);
        expect(errors).toEqual([]);
    });
}

for (const scenario of [
    { label: 'neither qualifies', first: 99, second: 100, cancelFirst: false, table: Array.from({ length: 10 }, (_, index) => previous(1000 - index * 100)), expected: [] },
    { label: 'only P2 qualifies', first: 100, second: 101, cancelFirst: false, table: Array.from({ length: 10 }, (_, index) => previous(1000 - index * 100)), expected: ['second'] },
    { label: 'only P1 qualifies', first: 101, second: 100, cancelFirst: false, table: Array.from({ length: 10 }, (_, index) => previous(1000 - index * 100)), expected: ['first'] },
    { label: 'P1 fills last slot and tied P2 is rejected', first: 100, second: 100, cancelFirst: false, table: Array.from({ length: 9 }, (_, index) => previous(1000 - index * 100)), expected: ['first'] },
    { label: 'cancelled P1 leaves last slot for tied P2', first: 100, second: 100, cancelFirst: true, table: Array.from({ length: 9 }, (_, index) => previous(1000 - index * 100)), expected: ['second'] },
    { label: 'P2 beats P1 and displaces the last slot', first: 100, second: 101, cancelFirst: false, table: Array.from({ length: 9 }, (_, index) => previous(1000 - index * 100)), expected: ['second'] },
    { label: 'zero never qualifies even with an empty table', first: 0, second: 0, cancelFirst: false, table: [], expected: [] },
    { label: 'equal qualifying scores retain player order', first: 500, second: 500, cancelFirst: false, table: [previous(500, 'earlier')], expected: ['first', 'second'] },
]) {
    test(`real multiplayer ranking: ${scenario.label}`, async ({ page }) => {
        await startMatch(page, {}, scenario.table);
        await seed(page, [{ score: scenario.first, lives: 1, level: 1 }, { score: scenario.second, lives: 1, level: 3 }]);
        let expected = scenario.table.map(({ name, score, level }) => ({ name, score, level }));
        for (const [index, score] of [scenario.first, scenario.second].entries()) {
            await reach(page, 'playing', index + 1);
            await die(page);
            const qualifies = score > 0 && (expected.length < 10 || score > expected[9].score);
            if (qualifies) {
                const state = await reach(page, 'score');
                expect(state.entry?.score).toBe(score);
                expect(state.entry?.slot).toBe(expected.filter(entry => entry.score >= score).length);
                expect(state.entry?.name).toBe('');
                if (index === 0 && scenario.cancelFirst) {
                    await page.keyboard.type('discarded');
                    await pad(page, 1);
                } else {
                    const name = index === 0 ? 'first' : 'second';
                    await page.keyboard.type(name);
                    await page.keyboard.press('Enter');
                    expected = [...expected, { name, score, level: index === 0 ? 1 : 3 }]
                        .sort((a, b) => b.score - a.score).slice(0, 10);
                }
                await reach(page, 'closed');
            }
        }
        await reach(page, 'menu');
        const saved = (await scores(page))['mr-normal'];
        expect(saved.map(({ name, score, level }) => ({ name, score, level }))).toEqual(expected);
        const inserted = saved.filter(entry => entry.name === 'first' || entry.name === 'second');
        expect(inserted.map(entry => entry.name)).toEqual(scenario.expected);
        for (const entry of inserted) expect(Number.isNaN(Date.parse(entry.date))).toBe(false);
        expect(saved.length).toBeLessThanOrEqual(10);
        await expect(page.locator('#score-entry')).toBeHidden();
    });
}

test('single-player entry supports controller-only editing, cancellation, and a clean second match', async ({ page }) => {
    await startMatch(page, { players: 1 });
    await seed(page, [{ score: 900, lives: 1, level: 1 }]);
    await reach(page, 'playing', 1);
    await die(page);
    await reach(page, 'score');
    await pad(page, 0);
    await pad(page, 15);
    await pad(page, 0);
    expect((await snapshot(page)).entry?.name).toBe('ab');
    await pad(page, 1);
    await reach(page, 'menu');
    expect((await scores(page))['mr-normal']).toEqual([]);
    await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
    await step(page, 2);
    await seed(page, [{ score: 1200, lives: 1, level: 7 }]);
    await reach(page, 'playing', 1);
    await die(page);
    expect((await reach(page, 'score')).entry).toEqual({ score: 1200, slot: 0, name: '', selected: 0 });
    for (let index = 0; index < 4; index++) await pad(page, 13);
    expect((await snapshot(page)).entry?.selected).toBe(28);
    await pad(page, 0);
    expect((await snapshot(page)).entry?.name).toBe('');
    await pad(page, 15);
    expect((await snapshot(page)).entry?.selected).toBe(29);
    await pad(page, 0);
    await reach(page, 'menu');
    expect((await scores(page))['mr-normal']).toEqual([{ name: '', score: 1200, level: 7, date: expect.any(String) }]);
});

test('name limits, invalid characters and duplicate End during outgoing fade never corrupt the other player', async ({ page }) => {
    await startMatch(page);
    await seed(page, [{ score: 800, lives: 1, level: 1 }, { score: 700, lives: 1, level: 2 }]);
    await reach(page, 'playing', 1);
    await die(page);
    await reach(page, 'score');
    for (let index = 0; index < 3; index++) await page.keyboard.press('Backspace');
    await page.keyboard.type('ABCDEFGHIJKLM123!');
    expect((await snapshot(page)).entry?.name).toBe('abcdefghi');
    await page.keyboard.press('Backspace');
    await page.keyboard.type('.');
    expect((await snapshot(page)).entry?.name).toBe('abcdefgh.');
    await page.keyboard.press('Enter');
    expect((await snapshot(page)).closing).toBe(true);
    await page.keyboard.type('shouldnot');
    await page.keyboard.press('Enter');
    await pad(page, 0);
    await reach(page, 'closed');
    expect((await scores(page))['mr-normal']).toEqual([{ name: 'abcdefgh.', score: 800, level: 1, date: expect.any(String) }]);
    await reach(page, 'playing', 2);
    await die(page);
    expect((await reach(page, 'score')).entry).toEqual({ score: 700, slot: 1, name: '', selected: 0 });
    await page.keyboard.press('Escape');
    await reach(page, 'menu');
    expect((await scores(page))['mr-normal']).toHaveLength(1);
});

test('P2 can finish first after a nonfatal P1 death, then P1 continues and submits last', async ({ page }) => {
    await startMatch(page);
    await seed(page, [{ score: 650, lives: 2, level: 4 }, { score: 750, lives: 1, level: 8 }]);
    await reach(page, 'playing', 1);
    await die(page);
    let state = await reach(page, 'playing', 2);
    expect(state.entry).toBeNull();
    expect(state.players).toEqual([{ score: 650, lives: 1, level: 4, over: false }, { score: 750, lives: 1, level: 8, over: false }]);
    await die(page);
    state = await reach(page, 'score');
    expect(state.entry?.score).toBe(750);
    expect(state.view.player).toBe(2);
    await page.keyboard.type('second');
    await page.keyboard.press('Enter');
    await page.evaluate(() => { document.documentElement.dataset.pad = '0'; });
    await reach(page, 'closed');
    await reach(page, 'playing', 1);
    await die(page);
    state = await reach(page, 'score');
    expect(state.entry).toEqual({ score: 650, slot: 1, name: '', selected: 0 });
    await step(page, 20);
    expect((await snapshot(page)).entry?.name).toBe('');
    await page.evaluate(() => { document.documentElement.dataset.pad = ''; });
    await step(page);
    await page.keyboard.type('first');
    await page.keyboard.press('Enter');
    await reach(page, 'menu');
    expect((await scores(page))['mr-normal']).toEqual([
        { name: 'second', score: 750, level: 8, date: expect.any(String) },
        { name: 'first', score: 650, level: 4, date: expect.any(String) },
    ]);
});

test('focus loss during name entry keeps the next player paused until explicit resume', async ({ page }) => {
    await startMatch(page);
    await seed(page, [{ score: 400, lives: 1, level: 1 }, { score: 600, lives: 2, level: 2 }]);
    await reach(page, 'playing', 1);
    await die(page);
    await reach(page, 'score');
    await page.keyboard.type('focus');
    await page.evaluate(() => window.dispatchEvent(new Event('blur')));
    await page.keyboard.press('Enter');
    const state = await reach(page, 'closed');
    expect(state.paused).toBe(true);
    await expect(page.locator('#pause-menu')).toBeVisible();
    const frozen = state.view.tick;
    await step(page, 30);
    expect((await snapshot(page)).view.tick).toBe(frozen);
    await page.evaluate(() => window.dispatchEvent(new Event('focus')));
    await page.getByRole('button', { name: 'Resume game', exact: true }).click();
    await reach(page, 'playing', 2);
    expect((await scores(page))['mr-normal'][0]).toMatchObject({ name: 'focus', score: 400, level: 1 });
});

test('real WebGL loss during outgoing score fade preserves the save and requires resume after recovery', async ({ page }) => {
    const errors: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    await startMatch(page);
    await seed(page, [{ score: 400, lives: 1, level: 1 }, { score: 600, lives: 2, level: 2 }]);
    await reach(page, 'playing', 1);
    await die(page);
    await reach(page, 'score');
    await page.keyboard.type('recover');
    await page.keyboard.press('Enter');
    await step(page);
    expect((await snapshot(page)).closing).toBe(true);
    const extension = await page.locator('canvas').evaluateHandle(canvas => {
        if (!(canvas instanceof HTMLCanvasElement)) throw new Error('Missing canvas');
        const gl = canvas.getContext('webgl2') ?? canvas.getContext('webgl');
        const extension = gl?.getExtension('WEBGL_lose_context');
        if (!extension) throw new Error('WebGL loss testing is unavailable');
        return extension;
    });
    await extension.evaluate(value => value.loseContext());
    await expect(page.locator('#resume')).toBeDisabled();
    const lost = await snapshot(page);
    await step(page, 20);
    expect((await snapshot(page)).view.tick).toBe(lost.view.tick);
    expect((await snapshot(page)).fade).toBe(lost.fade);
    await extension.evaluate(value => new Promise<void>(resolve => {
        const canvas = document.querySelector('canvas');
        if (!canvas) throw new Error('Missing canvas');
        canvas.addEventListener('webglcontextrestored', () => resolve(), { once: true });
        value.restoreContext();
    }));
    await expect(page.locator('#resume')).toBeEnabled();
    const state = await reach(page, 'closed');
    expect(state.paused).toBe(true);
    await expect(page.locator('#pause-menu')).toBeVisible();
    await page.getByRole('button', { name: 'Resume game', exact: true }).click();
    await reach(page, 'playing', 2);
    expect((await scores(page))['mr-normal']).toEqual([{ name: 'recover', score: 400, level: 1, date: expect.any(String) }]);
    expect(errors).toEqual([]);
});

test('a hidden tab freezes score entry and returning does not automatically start the next player', async ({ page }) => {
    await startMatch(page);
    await seed(page, [{ score: 500, lives: 1, level: 1 }, { score: 600, lives: 2, level: 2 }]);
    await reach(page, 'playing', 1);
    await die(page);
    await reach(page, 'score');
    await page.keyboard.type('hidden');
    await page.evaluate(() => {
        Object.defineProperty(document, 'hidden', { configurable: true, get: () => document.documentElement.dataset.hidden === 'yes' });
        document.documentElement.dataset.hidden = 'yes';
        document.dispatchEvent(new Event('visibilitychange'));
    });
    const before = await snapshot(page);
    await step(page, 40);
    expect((await snapshot(page)).entry).toEqual(before.entry);
    expect((await snapshot(page)).view.tick).toBe(before.view.tick);
    await page.evaluate(() => {
        document.documentElement.dataset.hidden = '';
        document.dispatchEvent(new Event('visibilitychange'));
    });
    await page.keyboard.press('Escape');
    const state = await reach(page, 'closed');
    expect(state.paused).toBe(true);
    await expect(page.locator('#pause-menu')).toBeVisible();
    await page.getByRole('button', { name: 'Resume game', exact: true }).click();
    await reach(page, 'playing', 2);
    expect((await scores(page))['mr-normal']).toEqual([]);
});

test('a failed score write is reported while the in-memory table and multiplayer handoff remain usable', async ({ page }) => {
    await startMatch(page);
    await seed(page, [{ score: 500, lives: 1, level: 1 }, { score: 600, lives: 1, level: 2 }]);
    await reach(page, 'playing', 1);
    await die(page);
    await reach(page, 'score');
    await page.evaluate(() => {
        Object.defineProperty(Storage.prototype, 'setItem', { value: () => { throw new Error('Quota exceeded'); } });
    });
    await page.keyboard.type('unsaved');
    await page.keyboard.press('Enter');
    await expect(page.locator('#status')).toContainText('Progress could not be saved');
    await expect(page.locator('#status')).toContainText('Quota exceeded');
    await reach(page, 'closed');
    await reach(page, 'playing', 2);
    await die(page);
    const state = await reach(page, 'score');
    expect(state.entry?.slot).toBe(0);
    await page.keyboard.press('Escape');
    await reach(page, 'menu');
    expect((await snapshot(page)).rendered?.texts).toContain('    500  unsaved');
    expect((await scores(page))['mr-normal']).toEqual([]);
});

test('reloading while editing discards only the unfinished name and preserves previously saved multiplayer scores', async ({ page }) => {
    await startMatch(page);
    await seed(page, [{ score: 300, lives: 1, level: 1 }, { score: 400, lives: 1, level: 2 }]);
    await reach(page, 'playing', 1);
    await die(page);
    await reach(page, 'score');
    await page.keyboard.type('saved');
    await page.keyboard.press('Enter');
    await reach(page, 'closed');
    await reach(page, 'playing', 2);
    await die(page);
    await reach(page, 'score');
    await page.keyboard.type('unfinished');
    await page.reload();
    await expect(page.locator('#menu')).toHaveJSProperty('hidden', false, { timeout: 30_000 });
    await step(page, 12);
    expect((await snapshot(page)).entry).toBeNull();
    await expect(page.locator('#score-entry')).toBeHidden();
    expect((await scores(page))['mr-normal']).toEqual([{ name: 'saved', score: 300, level: 1, date: expect.any(String) }]);
});

test('both qualifying players can cancel, including repeated cancellation during the outgoing fade', async ({ page }) => {
    await startMatch(page);
    await seed(page, [{ score: 500, lives: 1, level: 1 }, { score: 600, lives: 1, level: 2 }]);
    for (const player of [1, 2]) {
        await reach(page, 'playing', player);
        await die(page);
        await reach(page, 'score');
        await page.keyboard.type('discarded');
        await page.keyboard.press('Escape');
        await page.keyboard.press('Escape');
        await pad(page, 1);
        expect((await snapshot(page)).closing).toBe(true);
        await reach(page, 'closed');
        expect((await scores(page))['mr-normal']).toEqual([]);
    }
    await reach(page, 'menu');
    expect((await scores(page))['mr-normal']).toEqual([]);
    await expect(page.locator('#score-entry')).toBeHidden();
});

test('real score rendering obeys incoming/outgoing fades and keeps gameplay frozen until entry closes', async ({ page }) => {
    await startMatch(page, { players: 1 });
    await seed(page, [{ score: 9999990, lives: 1, level: 29 }]);
    await reach(page, 'playing', 1);
    await die(page);
    let state = await page.evaluate(() => {
        for (let frame = 0; frame < 100; frame++) {
            const state = window.matchHarness.snapshot();
            if (state.entry) return state;
            window.matchHarness.tick(1);
        }
        throw new Error('Score entry never opened');
    });
    expect(state.fade).toBeLessThan(0.875);
    expect(state.rendered?.foreground).toBe(false);
    const frozen = state.view.tick;
    state = await reach(page, 'score');
    expect(state.rendered?.foreground).toBe(true);
    expect(state.rendered?.texts).toContain('      9999990');
    expect(state.entry?.score).toBe(9999990);
    expect(state.view.tick).toBe(frozen);
    await page.keyboard.type('         ');
    expect((await snapshot(page)).entry?.name).toBe('         ');
    await page.keyboard.press('Enter');
    state = await step(page);
    expect(state.closing).toBe(true);
    expect(state.rendered?.foreground).toBe(false);
    expect(state.view.tick).toBe(frozen);
    await reach(page, 'menu');
    expect((await scores(page))['mr-normal']).toEqual([{ name: '         ', score: 9999990, level: 29, date: expect.any(String) }]);
});

test.describe('touch high-score entry', () => {
    test.use({ viewport: { width: 390, height: 844 }, isMobile: true, hasTouch: true, deviceScaleFactor: 3 });
    test('real rendered alphabet supports two-player touch-only editing and blank-name saving', async ({ page }) => {
        await startMatch(page, { character: 'ms', mazeSet: 'ms' });
        await seed(page, [{ score: 500, lives: 1, level: 1 }, { score: 600, lives: 1, level: 2 }]);
        await reach(page, 'playing', 1);
        await die(page);
        await reach(page, 'score');
        for (const name of ['a', 'Space', '.', 'Back', 'b']) await page.getByRole('button', { name, exact: true }).tap();
        expect((await snapshot(page)).entry?.name).toBe('a b');
        await page.getByRole('button', { name: 'Save score', exact: true }).tap();
        await reach(page, 'closed');
        await reach(page, 'playing', 2);
        await die(page);
        expect((await reach(page, 'score')).entry?.name).toBe('');
        await page.getByRole('button', { name: 'Save score', exact: true }).tap();
        await reach(page, 'menu');
        expect((await scores(page))['ms-normal']).toEqual([
            { name: '', score: 600, level: 2, date: expect.any(String) },
            { name: 'a b', score: 500, level: 1, date: expect.any(String) },
        ]);
        expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBeLessThanOrEqual(390);
    });
});
