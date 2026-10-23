import { test, expect, type Page } from '@playwright/test';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import { build } from 'esbuild';

declare global {
    interface Window {
        performanceDrawCalls: number;
        performanceContextLoss?: WEBGL_lose_context;
        performanceRequest(callback: FrameRequestCallback): void;
        performanceStep(elapsed: number): void;
        performanceTicks(): number;
        performanceRenders: number;
    }
}

async function boot(page: Page, manualClock = false): Promise<void> {
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
    if (manualClock) {
        await expect(page.locator('#game canvas')).toBeVisible({ timeout: 30_000 });
        await page.evaluate(() => window.performanceStep(0));
    }
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

test('240 Hz presentation renders only changed simulation states, avoids steady DOM layout, and responds between ticks', async ({ page }) => {
    const [main, template, css] = await Promise.all([
        readFile(new URL('../src/main.ts', import.meta.url), 'utf8'),
        readFile(new URL('../index.html', import.meta.url), 'utf8'),
        readFile(new URL('../src/style.css', import.meta.url), 'utf8'),
    ]);
    expect(main.match(/requestAnimationFrame\(frame\)/g)).toHaveLength(2);
    const hook = `
        window.performanceTicks = () => title.ticks;
        const renderScreen = GameRenderer.prototype.renderScreen;
        GameRenderer.prototype.renderScreen = function(screen) {
            window.performanceRenders++;
            return renderScreen.call(this, screen);
        };`;
    const result = await build({ stdin: {
        contents: main.replaceAll('requestAnimationFrame(frame)', 'window.performanceRequest(frame)') + hook,
        resolveDir: fileURLToPath(new URL('../src', import.meta.url)), loader: 'ts', sourcefile: 'performance-main.ts',
    }, bundle: true, write: false, format: 'iife', platform: 'browser', target: 'es2022',
        define: { 'import.meta.env.BASE_URL': JSON.stringify('./') }, loader: { '.css': 'empty' } });
    await page.route('**/performance-main.js', route => route.fulfill({ contentType: 'text/javascript', body: result.outputFiles[0].text }));
    await page.route('http://127.0.0.1:4173/', route => route.fulfill({ contentType: 'text/html',
        body: template.replace('./src/main.ts', './performance-main.js').replace('</head>', `<style>${css}</style></head>`) }));
    await page.addInitScript(() => {
        let next: FrameRequestCallback | undefined;
        let now = 1000;
        window.performanceRenders = 0;
        window.performanceRequest = callback => { next = callback; };
        window.performanceStep = elapsed => {
            if (!next) throw new Error('Game frame was not scheduled');
            const callback = next;
            next = undefined;
            now += elapsed;
            callback(now);
        };
    });
    await boot(page, true);
    await page.evaluate(() => {
        for (let index = 0; index < 20; index++) window.performanceStep(100);
    });
    await page.waitForTimeout(100);
    const unchanged = await page.evaluate(() => {
        window.performanceStep(0);
        const before = window.performanceRenders, draws = window.performanceDrawCalls;
        for (let index = 0; index < 10; index++) window.performanceStep(0);
        return { renders: window.performanceRenders - before, draws: window.performanceDrawCalls - draws };
    });
    expect(unchanged).toEqual({ renders: 0, draws: 0 });
    const measured = await page.evaluate(() => {
        let layoutReads = 0;
        const original = Element.prototype.getBoundingClientRect;
        Element.prototype.getBoundingClientRect = function() { layoutReads++; return original.call(this); };
        const mutations: MutationRecord[] = [];
        const observer = new MutationObserver(records => mutations.push(...records));
        observer.observe(document.querySelector('#menu')!, { attributes: true, subtree: true });
        observer.observe(document.querySelector('#score-entry')!, { attributes: true, subtree: true });
        const before = window.performanceRenders, ticks = window.performanceTicks(), draws = window.performanceDrawCalls;
        for (let index = 0; index < 240; index++) window.performanceStep(1000 / 240);
        mutations.push(...observer.takeRecords());
        observer.disconnect();
        Element.prototype.getBoundingClientRect = original;
        return { renders: window.performanceRenders - before, ticks: window.performanceTicks() - ticks,
            draws: window.performanceDrawCalls - draws, layoutReads, mutations: mutations.length };
    });
    expect(measured.ticks).toBeGreaterThanOrEqual(59);
    expect(measured.ticks).toBeLessThanOrEqual(60);
    expect(measured.renders).toBe(measured.ticks);
    expect(measured.draws).toBeGreaterThan(0);
    expect(measured.draws).toBeLessThanOrEqual(measured.renders * 5);
    expect(measured.layoutReads).toBe(0);
    expect(measured.mutations).toBe(0);
    const ticks = await page.evaluate(() => window.performanceTicks());
    await page.keyboard.press('ArrowDown');
    await page.evaluate(() => window.performanceStep(0));
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '1');
    expect(await page.evaluate(() => window.performanceTicks())).toBe(ticks);
    await page.locator('#about').click();
    await page.evaluate(() => window.performanceStep(0));
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'about');
    await page.evaluate(() => { document.documentElement.dataset.pad = '1'; window.performanceStep(0); });
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'menu');
});
