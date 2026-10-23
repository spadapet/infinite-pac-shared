import { test, expect, type Page } from '@playwright/test';

async function boot(page: Page): Promise<void> {
    await page.addInitScript(() => {
        localStorage.setItem('infinite-pac-web.v1', JSON.stringify({ version: 1,
            settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: false }, scores: {} }));
        Object.defineProperty(navigator, 'getGamepads', { value: () => {
            const pressed = document.documentElement.dataset.pad;
            document.documentElement.dataset.pad = '';
            return [{ index: 0, connected: true, mapping: 'standard', axes: [0, 0],
                buttons: Array.from({ length: 16 }, (_, index) => ({ pressed: pressed === String(index) })) }];
        } });
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
}
async function pad(page: Page, button: number): Promise<void> {
    await page.evaluate(button => { document.documentElement.dataset.pad = String(button); }, button);
    await expect.poll(() => page.evaluate(() => document.documentElement.dataset.pad)).toBe('');
    await page.waitForTimeout(50);
}

test('controller opens About, navigates all links, and returns with A, B or Start without changing title settings', async ({ page }) => {
    const errors: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    await boot(page);
    for (let index = 1; index <= 7; index++) {
        await pad(page, 13);
        await expect(page.locator('#game')).toHaveAttribute('data-selection', String(index));
    }
    const settings = await page.evaluate(() => JSON.parse(localStorage.getItem('infinite-pac-web.v1') ?? '{}').settings);
    await pad(page, 0);
    await expect(page.locator('#about-screen')).toBeVisible();
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'about');
    await expect(page.locator('dialog')).toHaveCount(0);
    await expect(page.locator('#menu')).toBeHidden();
    await expect(page.locator('#about-back')).toBeFocused();
    for (const name of ['peterspada.com', 'PixiJS', 'TypeScript', 'Vite']) {
        await pad(page, 15);
        await expect(page.getByRole('link', { name, exact: true })).toBeFocused();
    }
    await pad(page, 13);
    await expect(page.locator('#about-back')).toBeFocused();
    await pad(page, 0);
    await expect(page.locator('#about-screen')).toBeHidden();
    await expect(page.locator('#menu')).toBeVisible();
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '7');
    await pad(page, 9);
    await expect(page.locator('#about-screen')).toBeVisible();
    await pad(page, 9);
    await expect(page.locator('#about-screen')).toBeHidden();
    await pad(page, 0);
    await expect(page.locator('#about-screen')).toBeVisible();
    await pad(page, 1);
    await expect(page.locator('#about-screen')).toBeHidden();
    expect(await page.evaluate(() => JSON.parse(localStorage.getItem('infinite-pac-web.v1') ?? '{}').settings)).toEqual(settings);
    expect(errors).toEqual([]);
});

test('keyboard and pointer About navigation show creator credits and working website link without a popup', async ({ page }) => {
    await boot(page);
    await page.getByRole('button', { name: 'ABOUT', exact: true }).click();
    await expect(page.locator('#about-screen')).toContainText('Game, programming, graphics, and audio by Peter Spada.');
    await expect(page.locator('#about-content li')).toHaveText(['PixiJS', 'TypeScript', 'Vite']);
    await expect(page.getByRole('link', { name: 'peterspada.com', exact: true })).toHaveAttribute('href', 'http://peterspada.com');
    await expect(page.getByRole('link', { name: 'PixiJS', exact: true })).toHaveAttribute('href', 'https://pixijs.com');
    await expect(page.getByRole('link', { name: 'TypeScript', exact: true })).toHaveAttribute('href', 'https://www.typescriptlang.org');
    await expect(page.getByRole('link', { name: 'Vite', exact: true })).toHaveAttribute('href', 'https://vite.dev');
    await page.keyboard.press('ArrowDown');
    await expect(page.getByRole('link', { name: 'peterspada.com', exact: true })).toBeFocused();
    await page.keyboard.press('Tab');
    await expect(page.getByRole('link', { name: 'PixiJS', exact: true })).toBeFocused();
    await page.keyboard.press('Escape');
    await expect(page.locator('#about-screen')).toBeHidden();
    await page.getByRole('button', { name: 'ABOUT', exact: true }).click();
    await page.keyboard.press('Enter');
    await expect(page.locator('#about-screen')).toBeHidden();
    await page.getByRole('button', { name: 'ABOUT', exact: true }).click();
    await page.getByRole('button', { name: 'Back to title', exact: true }).click();
    await expect(page.locator('#about-screen')).toBeHidden();
    await page.getByRole('button', { name: 'ABOUT', exact: true }).click();
    await page.route('http://peterspada.com/', route => route.fulfill({ contentType: 'text/html', body: '<h1>Website link reached</h1>' }));
    await pad(page, 15);
    await expect(page.getByRole('link', { name: 'peterspada.com', exact: true })).toBeFocused();
    await page.evaluate(() => { document.documentElement.dataset.pad = '0'; });
    await expect(page).toHaveURL('http://peterspada.com/');
    await expect(page.getByRole('heading', { name: 'Website link reached' })).toBeVisible();
    expect(page.context().pages()).toHaveLength(1);
});

for (const viewport of [{ width: 390, height: 844 }, { width: 844, height: 390 }]) {
    test(`About credits scroll while Back remains visible at ${viewport.width}x${viewport.height}`, async ({ page }) => {
        await page.setViewportSize(viewport);
        await boot(page);
        await page.getByRole('button', { name: 'ABOUT', exact: true }).click();
        await expect(page.getByRole('heading', { name: 'Infinite Pac', exact: true })).toBeInViewport();
        await expect(page.locator('#about-back')).toBeInViewport();
        for (const name of ['peterspada.com', 'PixiJS', 'TypeScript', 'Vite']) {
            await page.keyboard.press('ArrowDown');
            await expect(page.getByRole('link', { name, exact: true })).toBeInViewport();
            await expect(page.locator('#about-back')).toBeInViewport();
        }
        expect(await page.evaluate(() => document.documentElement.scrollWidth)).toBeLessThanOrEqual(viewport.width);
        await page.keyboard.press('Escape');
        await expect(page.locator('#menu')).toBeVisible();
    });
}

test.describe('touch About screen', () => {
    test.use({ hasTouch: true, isMobile: true, viewport: { width: 390, height: 600 } });
    test('tap opens and closes About and a touch swipe scrolls credits', async ({ page }) => {
        await boot(page);
        await page.locator('#about').tap();
        await expect(page.locator('#about-screen')).toBeVisible();
        const content = page.locator('.about-content');
        const bounds = await content.boundingBox();
        if (!bounds) throw new Error('About credits have no layout bounds');
        const session = await page.context().newCDPSession(page);
        const x = bounds.x + bounds.width / 2;
        const y = bounds.y + bounds.height * 0.8;
        await session.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [{ x, y }] });
        for (let step = 1; step <= 8; step++) {
            await session.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: [{ x, y: y - step * 20 }] });
            await page.waitForTimeout(20);
        }
        await session.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
        await expect.poll(() => content.evaluate(element => element.scrollTop)).toBeGreaterThan(50);
        await expect(page.locator('#about-back')).toBeInViewport();
        await page.locator('#about-back').tap();
        await expect(page.locator('#about-screen')).toBeHidden();
        await expect(page.locator('#menu')).toBeVisible();
        await session.detach();
    });
});
