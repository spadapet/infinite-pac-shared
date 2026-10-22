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
    await expect(page.locator('#menu')).toBeHidden();
});

test('stored high-score names are displayed as plain text', async ({ page }) => {
    await page.addInitScript(() => {
        localStorage.setItem('infinite-pac-web.v1', JSON.stringify({
            version: 1,
            settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: false },
            scores: { 'mr-normal': [{ name: '<b>ACE</b>', score: 12340, level: 3, date: '2026-10-22' }] },
        }));
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.getByRole('button', { name: 'High scores', exact: true }).click();
    await expect(page.locator('#score-list li')).toContainText('<b>ACE</b>');
    await expect(page.locator('#score-list b')).toHaveCount(0);
    await expect(page.locator('#score-list span')).toHaveText('12,340');
    await page.getByRole('button', { name: 'Back', exact: true }).click();
    await expect(page.locator('#menu')).toBeVisible();
});

test('unavailable storage is reported without blocking the game', async ({ page }) => {
    await page.addInitScript(() => {
        Object.defineProperty(Storage.prototype, 'getItem', { value: () => { throw new Error('Storage blocked'); } });
        Object.defineProperty(Storage.prototype, 'setItem', { value: () => { throw new Error('Storage blocked'); } });
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.locator('#status')).toContainText('Saved data could not be loaded');
    await page.locator('#difficulty').selectOption('hard');
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
    await expect(page.locator('#status')).toContainText('WebGL powered');
});
