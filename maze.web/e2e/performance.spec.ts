import { test, expect, type Page } from '@playwright/test';

declare global {
    interface Window { performanceDrawCalls: number; performanceContextLoss?: WEBGL_lose_context }
}

async function boot(page: Page): Promise<void> {
    await page.addInitScript(() => {
        localStorage.setItem('infinite-pac-web.v1', JSON.stringify({ version: 1,
            settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: false }, scores: {} }));
        window.performanceDrawCalls = 0;
        for (const type of [WebGLRenderingContext, WebGL2RenderingContext]) {
            const original = type.prototype.drawElements;
            type.prototype.drawElements = function(mode, count, elementType, offset) {
                window.performanceDrawCalls++;
                return original.call(this, mode, count, elementType, offset);
            };
        }
        Object.defineProperty(navigator, 'getGamepads', { value: () => {
            const pressed = document.documentElement.dataset.pad;
            document.documentElement.dataset.pad = '';
            return [{ index: 0, connected: true, mapping: 'standard', axes: [0, 0],
                buttons: Array.from({ length: 16 }, (_, index) => ({ pressed: pressed === String(index) })) }];
        } });
    });
    await page.goto('/');
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'menu', { timeout: 30_000 });
}

async function expectNoDraws(page: Page): Promise<number> {
    await page.waitForTimeout(150);
    const before = await page.evaluate(() => window.performanceDrawCalls);
    await page.waitForTimeout(300);
    expect(await page.evaluate(() => window.performanceDrawCalls)).toBe(before);
    return before;
}

test('About submits no covered canvas frames but controller B still returns to the animated title', async ({ page }) => {
    await boot(page);
    await page.locator('#about').click();
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'about');
    const before = await expectNoDraws(page);
    await page.evaluate(() => { document.documentElement.dataset.pad = '1'; });
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'menu');
    await expect.poll(() => page.evaluate(() => window.performanceDrawCalls)).toBeGreaterThan(before);
});

test('paused gameplay redraws on resize, DPR and WebGL recovery, not on unchanged animation frames', async ({ page }) => {
    await boot(page);
    await page.locator('#start').click();
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'playing', { timeout: 15_000 });
    await page.keyboard.press('p');
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'paused');
    const tick = await page.locator('#game').getAttribute('data-tick');
    let before = await expectNoDraws(page);
    await page.setViewportSize({ width: 900, height: 600 });
    await expect.poll(() => page.evaluate(() => window.performanceDrawCalls)).toBeGreaterThan(before);
    before = await expectNoDraws(page);
    const width = await page.locator('#game canvas').evaluate(canvas => (canvas as HTMLCanvasElement).width);
    const session = await page.context().newCDPSession(page);
    await session.send('Emulation.setDeviceMetricsOverride', { width: 900, height: 600, deviceScaleFactor: 2, mobile: false });
    await expect.poll(() => page.locator('#game canvas').evaluate(canvas => (canvas as HTMLCanvasElement).width)).toBeGreaterThan(width * 1.9);
    await expect.poll(() => page.evaluate(() => window.performanceDrawCalls)).toBeGreaterThan(before);
    before = await expectNoDraws(page);
    await page.evaluate(() => {
        const canvas = document.querySelector<HTMLCanvasElement>('#game canvas');
        const extension = canvas?.getContext('webgl2')?.getExtension('WEBGL_lose_context');
        if (!extension) throw new Error('WebGL context-loss extension is unavailable');
        window.performanceContextLoss = extension;
        extension.loseContext();
    });
    await expect(page.locator('#resume')).toBeDisabled();
    await page.evaluate(() => {
        if (!window.performanceContextLoss) throw new Error('Context-loss extension was not retained');
        window.performanceContextLoss.restoreContext();
    });
    await expect(page.locator('#resume')).toBeEnabled();
    await expect.poll(() => page.evaluate(() => window.performanceDrawCalls)).toBeGreaterThan(before);
    await expectNoDraws(page);
    await expect(page.locator('#game')).toHaveAttribute('data-tick', tick!);
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'paused');
    await page.keyboard.press('p');
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'playing');
    await expect.poll(() => page.locator('#game').getAttribute('data-tick')).not.toBe(tick);
    await session.detach();
});
