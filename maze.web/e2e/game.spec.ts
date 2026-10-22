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
    await page.screenshot({ path: testInfo.outputPath('menu.png') });
    await page.getByRole('button', { name: 'Play game', exact: true }).click();
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
    await page.getByRole('button', { name: 'Sound on', exact: true }).click();
    await expect(page.getByRole('button', { name: 'Sound off', exact: true })).toBeVisible();
    expect(errors).toEqual([]);
    expect(missing).toEqual([]);
});

test('options and sound persist and both character/maze sets start correctly', async ({ page }) => {
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.locator('#character').selectOption('ms');
    await page.locator('#mazeSet').selectOption('ms');
    await page.locator('#difficulty').selectOption('easy');
    await page.locator('#players').selectOption('2');
    await page.getByRole('button', { name: 'Sound on', exact: true }).click();
    await page.reload();
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.locator('#character')).toHaveValue('ms');
    await expect(page.locator('#mazeSet')).toHaveValue('ms');
    await expect(page.locator('#difficulty')).toHaveValue('easy');
    await expect(page.locator('#players')).toHaveValue('2');
    await expect(page.getByRole('button', { name: 'Sound off', exact: true })).toBeVisible();
    await page.getByRole('button', { name: 'High scores', exact: true }).click();
    await expect(page.locator('#score-category')).toContainText('Ms. Pac mazes - easy');
    await page.getByRole('button', { name: 'Back', exact: true }).click();
    await page.getByRole('button', { name: 'Play game', exact: true }).click();
    await page.keyboard.press('ArrowLeft');
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-score')), { timeout: 12_000 }).toBeGreaterThan(0);
    await page.keyboard.press('Escape');
    await page.getByRole('button', { name: 'End game & return to menu', exact: true }).click();
    await expect(page.locator('#menu')).toBeVisible();
});

test.describe('touch screen', () => {
    test.use({ viewport: { width: 390, height: 844 }, isMobile: true, hasTouch: true, deviceScaleFactor: 3 });
    test('fits a phone and accepts touch direction controls', async ({ page }) => {
        await page.goto('/');
        await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
        await page.getByRole('button', { name: 'Play game', exact: true }).tap();
        await expect(page.locator('#touch-controls')).toBeVisible();
        await page.getByRole('button', { name: 'Move left', exact: true }).tap();
        await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-score')), { timeout: 12_000 }).toBeGreaterThan(0);
        const width = await page.evaluate(() => document.documentElement.scrollWidth);
        expect(width).toBeLessThanOrEqual(390);
        await page.getByRole('button', { name: 'Pause', exact: true }).tap();
        await expect(page.locator('#pause-menu')).toBeVisible();
    });
});
