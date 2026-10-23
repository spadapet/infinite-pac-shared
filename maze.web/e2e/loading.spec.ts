import { test, expect } from '@playwright/test';

for (const viewport of [{ width: 1280, height: 720 }, { width: 390, height: 844 }]) {
    test(`above-center loading spinner appears before JavaScript and remains until assets load at ${viewport.width}x${viewport.height}`, async ({ page }) => {
        await page.setViewportSize(viewport);
        let releaseScript!: () => void;
        let releaseAssets!: () => void;
        const scriptGate = new Promise<void>(resolve => { releaseScript = resolve; });
        const assetGate = new Promise<void>(resolve => { releaseAssets = resolve; });
        await page.route('**/*.js', async route => { await scriptGate; await route.continue(); });
        await page.route('**/*.png', async route => { await assetGate; await route.continue(); });
        await page.goto('/', { waitUntil: 'commit' });
        try {
            await expect(page.locator('#loading')).toBeVisible();
            await expect(page.locator('#loading')).toHaveText('Loading...');
            await expect(page.locator('#status')).toBeEmpty();
            const layout = await page.locator('#loading').evaluate(element => {
                const bounds = element.getBoundingClientRect();
                const text = element.firstElementChild?.getBoundingClientRect();
                const spinner = document.getElementById('loading-spinner');
                if (!text || !spinner) throw new Error('Missing loading content');
                const spinnerBottom = bounds.top + spinner.offsetTop + spinner.offsetHeight;
                return { x: bounds.x, y: bounds.y, width: bounds.width, height: bounds.height,
                    textCenter: text.x + text.width / 2, contentCenterY: (text.top + spinnerBottom) / 2,
                    fontSize: parseFloat(getComputedStyle(element).fontSize),
                    animation: getComputedStyle(spinner).animationName,
                    animations: spinner.getAnimations().length };
            });
            expect(layout).toMatchObject({ x: 0, y: 0, width: viewport.width, height: viewport.height, animation: 'loading-spin', animations: 1 });
            expect(Math.abs(layout.textCenter - viewport.width / 2)).toBeLessThanOrEqual(1);
            expect(Math.abs(layout.contentCenterY - viewport.height * 0.42)).toBeLessThanOrEqual(1);
            expect(layout.fontSize).toBeGreaterThanOrEqual(28);
            const assetRequested = page.waitForRequest(request => new URL(request.url()).pathname.endsWith('.png'));
            releaseScript();
            await assetRequested;
            await expect(page.locator('#loading')).toBeVisible();
            await expect(page.locator('#menu')).toBeHidden();
            releaseAssets();
            await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
            await expect(page.locator('#loading')).toBeHidden();
            await expect(page.locator('#game canvas')).toBeVisible();
        } finally {
            releaseScript();
            releaseAssets();
        }
    });
}

test('reduced motion keeps the loading indicator static', async ({ page }) => {
    await page.emulateMedia({ reducedMotion: 'reduce' });
    let release!: () => void;
    const gate = new Promise<void>(resolve => { release = resolve; });
    await page.route('**/*.js', async route => { await gate; await route.continue(); });
    await page.goto('/', { waitUntil: 'commit' });
    try {
        await expect(page.locator('#loading')).toBeVisible();
        await expect(page.locator('#loading-spinner')).toHaveCSS('animation-name', 'none');
    } finally { release(); }
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.locator('#loading')).toBeHidden();
});
