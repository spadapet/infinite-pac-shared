import { test, expect, type Page } from '@playwright/test';

async function orangeArrowPixels(page: Page): Promise<number> {
    return page.evaluate(() => new Promise<number>((resolve, reject) => {
        requestAnimationFrame(() => {
            const source = document.querySelector<HTMLCanvasElement>('#game canvas');
            const position = document.getElementById('game')?.dataset.position?.split(',').map(Number);
            if (!source || !position) { reject(new Error('Expected a rendered game and Pac position')); return; }
            const capture = document.createElement('canvas');
            capture.width = 224;
            capture.height = 296;
            const context = capture.getContext('2d');
            if (!context) { reject(new Error('Unable to inspect rendered joystick pixels')); return; }
            context.drawImage(source, 0, 0, 224, 296);
            const x = Math.max(0, Math.min(164, Math.floor(position[0] - 20)));
            const y = Math.max(0, Math.min(236, Math.floor(position[1] + 24 - 30)));
            const pixels = context.getImageData(x, y, 60, 60).data;
            let orange = 0;
            for (let i = 0; i < pixels.length; i += 4) {
                if (pixels[i] > 100 && pixels[i + 1] > 35 && pixels[i + 1] < pixels[i] * 0.85 && pixels[i + 2] < 70) orange++;
            }
            resolve(orange);
        });
    }));
}

async function start(page: Page): Promise<void> {
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.getByRole('button', { name: 'Sound on', exact: true }).click();
    await page.getByRole('button', { name: 'Play game', exact: true }).click();
    await expect(page.locator('#menu')).toBeHidden();
}

test('mouse drag shows the original orange arrow and release or pause removes it', async ({ page }, testInfo) => {
    const errors: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    await start(page);
    const bounds = await page.locator('#game canvas').boundingBox();
    if (!bounds) throw new Error('Expected a visible game canvas');
    const origin = { x: bounds.x + bounds.width * 0.5, y: bounds.y + bounds.height * 0.5 };
    expect(await orangeArrowPixels(page)).toBe(0);
    await page.mouse.move(origin.x, origin.y);
    await page.mouse.down();
    expect(await orangeArrowPixels(page), 'a stationary press has no direction indicator').toBe(0);
    await page.mouse.move(origin.x + 80, origin.y - 60);
    await expect.poll(() => orangeArrowPixels(page)).toBeGreaterThan(20);
    await page.screenshot({ path: testInfo.outputPath('mouse-joystick.png') });
    await page.mouse.up();
    await expect.poll(() => orangeArrowPixels(page)).toBe(0);
    await page.mouse.move(origin.x, origin.y);
    await page.mouse.down();
    await page.mouse.move(origin.x - 60, origin.y + 60);
    await expect.poll(() => orangeArrowPixels(page)).toBeGreaterThan(20);
    await page.keyboard.press('p');
    await expect(page.locator('#pause-menu')).toBeVisible();
    await page.mouse.up();
    await page.getByRole('button', { name: 'Resume game', exact: true }).click();
    await expect.poll(() => orangeArrowPixels(page)).toBe(0);
    expect(errors).toEqual([]);
});

test.describe('touch joystick', () => {
    test.use({ viewport: { width: 390, height: 844 }, isMobile: true, hasTouch: true, deviceScaleFactor: 3 });
    test('finger drag preserves its origin and hides the arrow after release', async ({ page }, testInfo) => {
        const errors: string[] = [];
        page.on('pageerror', error => errors.push(error.message));
        await start(page);
        const bounds = await page.locator('#game canvas').boundingBox();
        if (!bounds) throw new Error('Expected a visible phone game canvas');
        const session = await page.context().newCDPSession(page);
        const origin = { x: bounds.x + bounds.width * 0.5, y: bounds.y + bounds.height * 0.5 };
        await session.send('Input.dispatchTouchEvent', { type: 'touchStart', touchPoints: [{ ...origin, id: 1 }] });
        expect(await orangeArrowPixels(page)).toBe(0);
        await session.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: [{ x: origin.x + 30, y: origin.y - 30, id: 1 }] });
        await expect.poll(() => orangeArrowPixels(page)).toBeGreaterThan(20);
        await page.screenshot({ path: testInfo.outputPath('touch-joystick.png') });
        await session.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: [{ ...origin, id: 1 }] });
        await expect.poll(() => orangeArrowPixels(page)).toBe(0);
        await session.send('Input.dispatchTouchEvent', { type: 'touchMove', touchPoints: [{ x: origin.x - 30, y: origin.y + 30, id: 1 }] });
        await expect.poll(() => orangeArrowPixels(page)).toBeGreaterThan(20);
        await session.send('Input.dispatchTouchEvent', { type: 'touchEnd', touchPoints: [] });
        await expect.poll(() => orangeArrowPixels(page)).toBe(0);
        await session.detach();
        expect(errors).toEqual([]);
    });
});
