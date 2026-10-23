import { test, expect } from '@playwright/test';

test('a browser without WebGL receives an explicit startup error', async ({ page }) => {
    await page.addInitScript(() => {
        const original = HTMLCanvasElement.prototype.getContext;
        Object.defineProperty(HTMLCanvasElement.prototype, 'getContext', {
            value: function(this: HTMLCanvasElement, id: string, ...args: unknown[]) {
                if (id === 'webgl' || id === 'webgl2' || id === 'experimental-webgl') return null;
                return Reflect.apply(original, this, [id, ...args]);
            },
        });
    });
    await page.goto('/');
    await expect(page.locator('#status')).toContainText('WebGL hardware rendering is required', { timeout: 30_000 });
    await expect(page.locator('#loading')).toBeHidden();
    await expect(page.locator('#menu')).toBeHidden();
});

test('legacy high-score names migrate to native characters and render as plain text', async ({ page }) => {
    await page.addInitScript(() => {
        localStorage.setItem('infinite-pac-web.v1', JSON.stringify({
            version: 1,
            settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: false },
            scores: { 'mr-normal': [{ name: '<b>ACE</b>', score: 12340, level: 3, date: '2026-10-22' }] },
        }));
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.locator('#game canvas')).toBeVisible();
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '0');
    await expect(page.locator('#menu b')).toHaveCount(0);
    await page.locator('#players').click();
    expect(await page.evaluate(() => JSON.parse(localStorage.getItem('infinite-pac-web.v1') ?? '{}').scores['mr-normal']))
        .toEqual([{ name: 'baceb', score: 12340, level: 3, date: '2026-10-22' }]);
});

test('unavailable storage is reported without blocking the game', async ({ page }) => {
    await page.addInitScript(() => {
        Object.defineProperty(Storage.prototype, 'getItem', { value: () => { throw new Error('Storage blocked'); } });
        Object.defineProperty(Storage.prototype, 'setItem', { value: () => { throw new Error('Storage blocked'); } });
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.locator('#status')).toContainText('Saved data could not be loaded');
    await page.locator('#difficulty').click();
    await expect(page.locator('#status')).toContainText('Progress could not be saved');
});

test('standard gamepad starts, moves and pauses on button edges, with an audio gesture notice', async ({ page }) => {
    await page.addInitScript(() => {
        Object.defineProperty(navigator, 'userActivation', {
            get: () => ({ hasBeenActive: document.documentElement.dataset.activated === 'yes' }),
        });
        document.addEventListener('keydown', () => { document.documentElement.dataset.activated = 'yes'; });
        Object.defineProperty(navigator, 'getGamepads', {
            value: () => [{ index: 0, connected: true, mapping: 'standard', axes: [0, 0],
                buttons: Array.from({ length: 16 }, (_, index) => ({ pressed: (document.documentElement.dataset.pad ?? '').split(',').includes(String(index)) })) }],
        });
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.evaluate(() => { document.documentElement.dataset.pad = '0,14'; });
    await expect(page.locator('#menu')).toBeHidden();
    await expect(page.locator('#status')).toContainText('press a key to enable sound');
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-score')), { timeout: 12_000 }).toBeGreaterThan(0);
    await page.evaluate(() => { document.documentElement.dataset.pad = '9'; });
    await expect(page.locator('#pause-menu')).toBeVisible();
    const tick = await page.locator('#game').getAttribute('data-tick');
    await page.waitForTimeout(350);
    expect(await page.locator('#game').getAttribute('data-tick')).toBe(tick);
    await page.evaluate(() => { document.documentElement.dataset.pad = '0'; });
    await expect(page.locator('#pause-menu')).toBeHidden();
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-tick'))).toBeGreaterThan(Number(tick));
    await page.keyboard.press('x');
    await expect(page.locator('#status')).toBeEmpty();
});

test('controller title navigation selects Ms artwork and mazes together, with Start distinct from Pause', async ({ page }) => {
    await page.addInitScript(() => {
        localStorage.setItem('infinite-pac-web.v1', JSON.stringify({ version: 1,
            settings: { character: 'ms', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: false }, scores: {} }));
        Object.defineProperty(navigator, 'getGamepads', { value: () => [{ index: 0, connected: true, mapping: 'standard', axes: [0, 0],
            buttons: Array.from({ length: 16 }, (_, index) => ({ pressed: document.documentElement.dataset.pad === String(index) })) }] });
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.locator('#game')).toHaveAttribute('data-character', 'mr');
    await page.keyboard.press('p');
    await page.waitForTimeout(150);
    await expect(page.locator('#menu')).toBeVisible();
    await page.evaluate(() => { document.documentElement.dataset.pad = '13'; });
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '1');
    await page.evaluate(() => { document.documentElement.dataset.pad = ''; });
    await expect(page.locator('#game')).toHaveAttribute('data-maze-set', 'ms');
    await page.evaluate(() => { document.documentElement.dataset.pad = '9'; });
    await expect(page.locator('#menu')).toBeHidden();
    await expect(page.locator('#game')).toHaveAttribute('data-character', 'ms');
    await expect(page.locator('#game')).toHaveAttribute('data-maze-set', 'ms');
});
