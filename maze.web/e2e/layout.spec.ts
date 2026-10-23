import { test, expect, type Page } from '@playwright/test';

async function expectClearLayout(page: Page): Promise<void> {
    await expect.poll(() => page.evaluate(() => {
        const canvas = document.querySelector('#game canvas')?.getBoundingClientRect();
        const cabinet = document.getElementById('cabinet')?.getBoundingClientRect();
        const status = document.getElementById('status')?.getBoundingClientRect();
        if (!canvas || !cabinet || !status) return false;
        return canvas.width > 0 && canvas.height > 0
            && canvas.left >= 12 && canvas.top >= 12
            && canvas.right <= innerWidth - 12 && canvas.bottom <= cabinet.bottom + 1
            && status.top >= cabinet.bottom + 7 && status.bottom <= innerHeight - 12
            && document.documentElement.scrollWidth <= innerWidth
            && document.documentElement.scrollHeight <= innerHeight;
    })).toBe(true);
}

for (const viewport of [{ width: 1280, height: 720 }, { width: 390, height: 844 }, { width: 844, height: 390 }]) {
    test(`game and multiline notices stay separate at ${viewport.width}x${viewport.height}`, async ({ page }) => {
        await page.setViewportSize(viewport);
        await page.addInitScript(() => {
            localStorage.setItem('infinite-pac-web.v1', JSON.stringify({ version: 1,
                settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: false }, scores: {} }));
            Object.defineProperty(Storage.prototype, 'setItem', { value: () => {
                throw new Error('Browser storage is blocked. Your settings and high scores cannot be saved until storage is enabled for this site.');
            } });
        });
        await page.goto('/');
        await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
        await expect(page.locator('#status')).toBeEmpty();
        await expectClearLayout(page);
        const initialHeight = await page.locator('#cabinet').evaluate(element => element.getBoundingClientRect().height);
        await page.locator('#difficulty').click();
        await expect(page.locator('#status')).toContainText('Progress could not be saved');
        await expectClearLayout(page);
        const statusHeight = await page.locator('#status').evaluate(element => element.getBoundingClientRect().height);
        const cabinetHeight = await page.locator('#cabinet').evaluate(element => element.getBoundingClientRect().height);
        expect(Math.abs(initialHeight - cabinetHeight - (statusHeight - 36))).toBeLessThanOrEqual(1);
        if (viewport.width === 390) expect(statusHeight).toBeGreaterThan(36);
        await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
        await expect(page.locator('#menu')).toBeHidden();
        await expectClearLayout(page);
        await page.keyboard.press('p');
        await expect(page.locator('#pause-menu')).toBeVisible();
        await expectClearLayout(page);
    });
}

test('fullscreen gives controller users an actionable hint and clears it after keyboard or pointer entry', async ({ page }) => {
    await page.addInitScript(() => {
        localStorage.setItem('infinite-pac-web.v1', JSON.stringify({ version: 1,
            settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: false }, scores: {} }));
        Object.defineProperty(navigator, 'userActivation', { get: () => ({
            hasBeenActive: true, isActive: document.documentElement.dataset.gesture === 'yes',
        }) });
        document.addEventListener('keydown', () => { document.documentElement.dataset.gesture = 'yes'; });
        document.addEventListener('pointerdown', () => { document.documentElement.dataset.gesture = 'yes'; });
        Object.defineProperty(navigator, 'getGamepads', { value: () => {
            const pressed = document.documentElement.dataset.pad;
            document.documentElement.dataset.pad = '';
            return [{ index: 0, connected: true, mapping: 'standard', axes: [0, 0],
                buttons: Array.from({ length: 16 }, (_, index) => ({ pressed: pressed === String(index) })) }];
        } });
        Object.defineProperty(document, 'fullscreenEnabled', { get: () => true });
        let fullscreenElement: HTMLElement | null = null;
        Object.defineProperty(document, 'fullscreenElement', { get: () => fullscreenElement });
        Object.defineProperty(HTMLElement.prototype, 'requestFullscreen', { value: function(this: HTMLElement) {
            document.documentElement.dataset.requests = String(Number(document.documentElement.dataset.requests ?? 0) + 1);
            if (document.documentElement.dataset.reject === 'yes') return Promise.reject(new DOMException('permission check failed', 'NotAllowedError'));
            fullscreenElement = this;
            document.dispatchEvent(new Event('fullscreenchange'));
            return Promise.resolve();
        } });
        Object.defineProperty(document, 'exitFullscreen', { value: () => {
            fullscreenElement = null;
            document.dispatchEvent(new Event('fullscreenchange'));
            return Promise.resolve();
        } });
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    for (let index = 0; index < 6; index++) {
        await page.evaluate(() => { document.documentElement.dataset.pad = '13'; });
        await expect(page.locator('#game')).toHaveAttribute('data-selection', String(index + 1));
        await page.evaluate(() => { document.documentElement.dataset.pad = ''; });
        await page.waitForTimeout(50);
    }
    await page.evaluate(() => { document.documentElement.dataset.pad = '0'; });
    await expect(page.locator('#status')).toContainText('Press F to enter fullscreen');
    expect(await page.evaluate(() => document.documentElement.dataset.requests ?? '0')).toBe('0');
    await expectClearLayout(page);
    await page.evaluate(() => { document.documentElement.dataset.pad = ''; });
    await page.waitForTimeout(8100);
    await expect(page.locator('#status')).toBeEmpty();
    await expect(page.locator('#status')).not.toHaveClass('error');
    await page.keyboard.press('f');
    await expect(page.locator('#fullscreen')).toHaveAttribute('aria-label', 'FULL SCREEN:ON');
    await expect(page.locator('#status')).toBeEmpty();
    await expect(page.locator('#status')).not.toHaveClass('error');
    await page.keyboard.press('f');
    await expect(page.locator('#fullscreen')).toHaveAttribute('aria-label', 'FULL SCREEN:OFF');
    await page.evaluate(() => { document.documentElement.dataset.reject = 'yes'; });
    await page.locator('#fullscreen').click();
    await expect(page.locator('#status')).toContainText('Press F to enter fullscreen');
    await expect(page.locator('#status')).not.toContainText('permission check failed');
    await page.evaluate(() => { document.documentElement.dataset.reject = ''; });
    await page.locator('#fullscreen').click();
    await expect(page.locator('#fullscreen')).toHaveAttribute('aria-label', 'FULL SCREEN:ON');
    await expect(page.locator('#status')).toBeEmpty();
});

test('real browser fullscreen keeps the game inset and status outside the maze', async ({ page }) => {
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.keyboard.press('f');
    await expect.poll(() => page.evaluate(() => document.fullscreenElement?.id)).toBe('app');
    await expectClearLayout(page);
    await page.keyboard.press('f');
    await expect.poll(() => page.evaluate(() => document.fullscreenElement === null)).toBe(true);
    await expectClearLayout(page);
});

test('errors expire eight seconds after the latest report without an older timer clearing them', async ({ page }) => {
    const errors: string[] = [];
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    await page.addInitScript(() => {
        localStorage.setItem('infinite-pac-web.v1', JSON.stringify({ version: 1,
            settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: false }, scores: {} }));
        Object.defineProperty(Storage.prototype, 'setItem', { value: () => { throw new Error('Storage blocked'); } });
        Object.defineProperty(document, 'fullscreenEnabled', { get: () => false });
    });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.locator('#difficulty').click();
    await expect(page.locator('#status')).toContainText('Progress could not be saved');
    await page.waitForTimeout(4500);
    await page.keyboard.press('f');
    await expect(page.locator('#status')).toHaveText('Fullscreen is unavailable in this browser.');
    await page.waitForTimeout(4000);
    await expect(page.locator('#status')).toHaveText('Fullscreen is unavailable in this browser.');
    await page.waitForTimeout(4200);
    await expect(page.locator('#status')).toBeEmpty();
    await expect(page.locator('#status')).not.toHaveClass('error');
    expect(errors.some(message => message.includes('Progress could not be saved'))).toBe(true);
    expect(errors).toContain('Fullscreen is unavailable in this browser.');
    await expectClearLayout(page);
});
