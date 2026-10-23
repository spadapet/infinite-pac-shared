import { test, expect } from '@playwright/test';

test('WebGL game loads original assets, plays, pauses and resumes without browser errors', async ({ page }, testInfo) => {
    const errors: string[] = [];
    const missing: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    page.on('response', response => { if (response.status() >= 400) missing.push(`${response.status()} ${response.url()}`); });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.locator('#game canvas')).toBeVisible();
    const webgl = await page.locator('canvas').evaluate(canvas => {
        if (!(canvas instanceof HTMLCanvasElement)) throw new Error('Expected the game canvas');
        const gl = canvas.getContext('webgl2') ?? canvas.getContext('webgl');
        const bounds = canvas.getBoundingClientRect();
        const host = document.getElementById('game')?.getBoundingClientRect();
        if (!host) throw new Error('Expected the game host');
        return { version: gl ? String(gl.getParameter(gl.VERSION)) : '',
            pixelWidth: canvas.width, displayWidth: bounds.width * devicePixelRatio,
            pixelHeight: canvas.height, displayHeight: bounds.height * devicePixelRatio,
            centerOffset: bounds.x + bounds.width / 2 - host.x - host.width / 2 };
    });
    expect(webgl.version).toContain('WebGL');
    expect(Math.abs(webgl.pixelWidth - webgl.displayWidth)).toBeLessThanOrEqual(1);
    expect(Math.abs(webgl.pixelHeight - webgl.displayHeight)).toBeLessThanOrEqual(1);
    expect(Math.abs(webgl.centerOffset)).toBeLessThanOrEqual(1);
    await expect(page.locator('#game')).toHaveAttribute('data-character', 'mr');
    await expect(page.locator('#character, #mazeSet, .panel, .topbar')).toHaveCount(0);
    await page.waitForTimeout(1100);
    await page.screenshot({ path: testInfo.outputPath('menu.png') });
    await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
    await expect(page.locator('#menu')).toBeHidden();
    await page.keyboard.press('ArrowLeft');
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-score')), { timeout: 12_000 }).toBeGreaterThan(0);
    await page.screenshot({ path: testInfo.outputPath('playing.png') });
    await page.keyboard.press('p');
    await expect(page.locator('#pause-menu')).toBeVisible();
    const tick = await page.locator('#game').getAttribute('data-tick');
    await page.waitForTimeout(350);
    expect(await page.locator('#game').getAttribute('data-tick')).toBe(tick);
    await page.getByRole('button', { name: 'Resume game', exact: true }).click();
    await expect(page.locator('#pause-menu')).toBeHidden();
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-tick'))).toBeGreaterThan(Number(tick));
    await page.keyboard.press('m');
    expect(await page.evaluate(() => JSON.parse(localStorage.getItem('infinite-pac-web.v1') ?? '{}').settings.sound)).toBe(false);
    expect(errors).toEqual([]);
    expect(missing).toEqual([]);
});

test('options and sound persist and both character/maze sets start correctly', async ({ page }) => {
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.keyboard.press('ArrowDown');
    await expect(page.locator('#game')).toHaveAttribute('data-character', 'ms');
    await expect(page.locator('#game')).toHaveAttribute('data-maze-set', 'ms');
    await page.locator('#difficulty').click();
    await page.locator('#difficulty').click();
    await page.locator('#players').click();
    await page.getByRole('button', { name: 'Sound on', exact: true }).click();
    await page.reload();
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '1');
    await expect(page.locator('#difficulty')).toHaveAttribute('aria-label', 'DIFFICULTY:EASY');
    await expect(page.locator('#players')).toHaveAttribute('aria-label', 'PLAYERS:TWO');
    await expect(page.getByRole('button', { name: 'Sound off', exact: true })).toBeVisible();
    await page.getByRole('button', { name: 'Play Ms. Lila', exact: true }).click();
    await expect(page.locator('#menu')).toBeHidden();
    await expect(page.locator('#game')).toHaveAttribute('data-character', 'ms');
    await expect(page.locator('#game')).toHaveAttribute('data-maze-set', 'ms');
    await page.keyboard.press('ArrowLeft');
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-score')), { timeout: 12_000 }).toBeGreaterThan(0);
    await page.keyboard.press('p');
    await page.getByRole('button', { name: 'End game & return to menu', exact: true }).click();
    await expect(page.locator('#menu')).toBeVisible();
});

test.describe('touch screen', () => {
    test.use({ viewport: { width: 390, height: 844 }, isMobile: true, hasTouch: true, deviceScaleFactor: 3 });
    test('fits a phone and starts directly from the native title with touch', async ({ page }) => {
        await page.goto('/');
        await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
        await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).tap();
        await expect(page.locator('#menu')).toBeHidden();
        await expect(page.locator('#touch-controls')).toHaveCount(0);
        await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-score')), { timeout: 12_000 }).toBeGreaterThan(0);
        const width = await page.evaluate(() => document.documentElement.scrollWidth);
        expect(width).toBeLessThanOrEqual(390);
        await page.getByRole('button', { name: 'Pause', exact: true }).tap();
        await expect(page.locator('#pause-menu')).toBeVisible();
    });
});
