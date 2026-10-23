import { test, expect, type Page } from '@playwright/test';
import { readFile } from 'node:fs/promises';
import ts from 'typescript';

// Exercise the real browser entry point and storage with deterministic game-over events.
// Rendering, audio and movement are covered by the full-game browser tests.
async function scorePage(page: Page): Promise<void> {
    const [html, main, storage, vibration] = await Promise.all([
        readFile(new URL('../index.html', import.meta.url), 'utf8'),
        readFile(new URL('../src/main.ts', import.meta.url), 'utf8'),
        readFile(new URL('../src/storage.ts', import.meta.url), 'utf8'),
        readFile(new URL('../src/vibration.ts', import.meta.url), 'utf8'),
    ]);
    const compile = (source: string) => ts.transpileModule(source, {
        compilerOptions: { target: ts.ScriptTarget.ES2022, module: ts.ModuleKind.ESNext },
    }).outputText;
    const modules: Record<string, string> = {
        '/score-main.js': compile(main.replace("import './style.css';", '')),
        '/storage': compile(storage),
        '/vibration': compile(vibration),
        '/game': `export class Game {
            view = { state: 'playing', score: 0, tick: 0, highScore: 0, level: 1, player: 1,
                pac: { direction: 'left', position: { x: 0, y: 0 } } };
            events = [];
            nextPlayer = null;
            transitionPending = false;
            constructor() { window.scoreHarness = this; }
            advance() {
                this.view.tick++;
                if (this.transitionPending) {
                    this.transitionPending = false;
                    this.view.state = this.nextPlayer === null ? 'gameover' : 'playing';
                    if (this.nextPlayer !== null) this.view.player = this.nextPlayer;
                }
            }
            drainEvents() {
                const events = this.events.splice(0);
                if (this.view.state === 'dead' && events.some(event => event.type === 'gameOver')) this.transitionPending = true;
                return events;
            }
            press() {}
        }`,
        '/renderer': 'export class GameRenderer { static async create() { return new GameRenderer(); } render() {} }',
        '/audio': `export class GameAudio {
            setMuted() {} stopAll() {} handle() {} async unlock() {} async setPaused() {}
        }`,
        '/input': 'export class InputController { joystick = null; poll() {} advance() {} }',
    };
    await page.route('**/*', async route => {
        const path = new URL(route.request().url()).pathname;
        if (path === '/') {
            await route.fulfill({ contentType: 'text/html', body: html.replace('./src/main.ts', './score-main.js') });
        } else if (modules[path]) {
            await route.fulfill({ contentType: 'text/javascript', body: modules[path] });
        } else await route.continue();
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible();
    await page.getByRole('button', { name: 'Play game', exact: true }).click();
    await expect(page.locator('#menu')).toBeHidden();
}

async function gameOver(page: Page, player: number, score: number, final = true): Promise<void> {
    await page.evaluate(({ player, score, final }) => {
        const game = (window as unknown as { scoreHarness: {
            view: { state: string; player: number }; nextPlayer: number | null;
            events: { type: string; player: number; score: number; level: number }[];
        } }).scoreHarness;
        game.view.state = 'dead';
        game.view.player = player;
        game.nextPlayer = final ? null : player + 1;
        game.events.push({ type: 'gameOver', player, score, level: 3 });
    }, { player, score, final });
    await expect(page.locator('#score-entry')).toBeVisible();
    expect(await page.evaluate(() => (window as unknown as { scoreHarness: {
        view: { state: string }; transitionPending: boolean;
    } }).scoreHarness.view.state)).toBe('dead');
    expect(await page.evaluate(() => (window as unknown as { scoreHarness: {
        transitionPending: boolean;
    } }).scoreHarness.transitionPending)).toBe(true);
}

async function savedScores(page: Page): Promise<unknown> {
    return page.evaluate(() => JSON.parse(localStorage.getItem('infinite-pac-web.v1') ?? '{}').scores ?? {});
}

test('blank names save, return to the menu, and survive reload in the correct category', async ({ page }) => {
    const errors: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    await scorePage(page);
    await gameOver(page, 1, 12340);
    await expect(page.locator('#score-name')).toBeFocused();
    await expect(page.locator('#score-name')).toHaveValue('');
    await expect(page.locator('#score-name')).toHaveAttribute('maxlength', '9');
    await expect(page.locator('#pause')).toBeDisabled();
    await page.getByRole('button', { name: 'Save score', exact: true }).click();
    await expect(page.locator('#score-entry')).toBeHidden();
    await expect(page.locator('#menu')).toBeVisible();
    const saved = await savedScores(page) as Record<string, { name: string; score: number; level: number; date: string }[]>;
    expect(Object.keys(saved)).toEqual(['mr-normal']);
    expect(saved['mr-normal']).toEqual([{ name: '', score: 12340, level: 3, date: expect.any(String) }]);
    expect(Number.isNaN(Date.parse(saved['mr-normal'][0].date))).toBe(false);
    await page.reload();
    await expect(page.locator('#menu')).toBeVisible();
    await page.getByRole('button', { name: 'High scores', exact: true }).click();
    await expect(page.locator('#score-list li')).toHaveCount(1);
    await expect(page.locator('#score-list li')).toContainText('---');
    await expect(page.locator('#score-list li span')).toHaveText('12,340');
    expect(errors).toEqual([]);
});

test('Escape skips a score without saving and the next player can save a native-format name', async ({ page }) => {
    await scorePage(page);
    await gameOver(page, 1, 500, false);
    await page.locator('#score-name').fill('DISCARDED');
    await page.keyboard.press('Escape');
    await expect(page.locator('#score-entry')).toBeHidden();
    await expect(page.locator('#menu')).toBeHidden();
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'playing');
    await expect(page.locator('#game')).toHaveAttribute('data-player', '2');
    expect(await savedScores(page)).toEqual({});
    await gameOver(page, 2, 700);
    await expect(page.locator('#score-summary')).toContainText('Player 2: 700 points - Level 3');
    await page.locator('#score-name').fill(' A. B123!');
    await expect(page.locator('#score-name')).toHaveValue(' a. b');
    await page.getByRole('button', { name: 'Save score', exact: true }).click();
    await expect(page.locator('#menu')).toBeVisible();
    expect(await savedScores(page)).toEqual({ 'mr-normal': [
        { name: ' a. b', score: 700, level: 3, date: expect.any(String) },
    ] });
});

test('touch skip works and graphics loss during entry requires an explicit resume', async ({ page }) => {
    await scorePage(page);
    await gameOver(page, 1, 900, false);
    await page.locator('#game').dispatchEvent('webglcontextlost');
    await page.getByRole('button', { name: 'Skip score', exact: true }).click();
    await expect(page.locator('#score-entry')).toBeHidden();
    await expect(page.locator('#pause-menu')).toBeVisible();
    await expect(page.locator('#resume')).toBeDisabled();
    const tick = await page.locator('#game').getAttribute('data-tick');
    await page.waitForTimeout(150);
    await expect(page.locator('#game')).toHaveAttribute('data-tick', tick!);
    await page.locator('#game').dispatchEvent('webglcontextrestored');
    await expect(page.locator('#resume')).toBeEnabled();
    await expect(page.locator('#pause-menu')).toBeVisible();
    await page.getByRole('button', { name: 'Resume game', exact: true }).click();
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'playing');
    expect(await savedScores(page)).toEqual({});
});

test('queued scores requalify after an earlier player fills the last slot', async ({ page }) => {
    await page.addInitScript(() => localStorage.setItem('infinite-pac-web.v1', JSON.stringify({
        version: 1,
        settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 2, sound: false, vibration: true },
        scores: { 'mr-normal': Array.from({ length: 9 }, (_, index) => ({
            name: 'previous', score: 1000 - index * 100, level: 1, date: '',
        })) },
    })));
    await scorePage(page);
    await page.evaluate(() => {
        const game = (window as unknown as { scoreHarness: {
            view: { state: string }; events: { type: string; player: number; score: number; level: number }[];
        } }).scoreHarness;
        game.view.state = 'gameover';
        game.events.push(...[1, 2].map(player => ({ type: 'gameOver', player, score: 100, level: 1 })));
    });
    await expect(page.locator('#score-entry')).toBeVisible();
    await expect(page.locator('#score-summary')).toContainText('Player 1:');
    await page.locator('#score-name').fill('first');
    await page.getByRole('button', { name: 'Save score', exact: true }).click();
    await expect(page.locator('#score-entry')).toBeHidden();
    await expect(page.locator('#menu')).toBeVisible();
    const saved = await savedScores(page) as Record<string, { name: string; score: number }[]>;
    expect(saved['mr-normal']).toHaveLength(10);
    expect(saved['mr-normal'].filter(entry => entry.score === 100).map(entry => entry.name)).toEqual(['first']);
});
