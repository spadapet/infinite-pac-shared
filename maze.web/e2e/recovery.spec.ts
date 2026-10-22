import { test, expect } from '@playwright/test';

test('losing WebGL pauses simulation and restores graphics before the player resumes', async ({ page }, testInfo) => {
    const errors: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.getByRole('button', { name: 'Play game', exact: true }).click();
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'playing', { timeout: 12_000 });
    const extension = await page.locator('canvas').evaluateHandle(canvas => {
        if (!(canvas instanceof HTMLCanvasElement)) throw new Error('Expected a canvas');
        const gl = canvas.getContext('webgl2') ?? canvas.getContext('webgl');
        if (!gl) throw new Error('WebGL is unavailable');
        return gl.getExtension('WEBGL_lose_context');
    });
    expect(await extension.evaluate(value => value !== null)).toBe(true);
    await extension.evaluate(value => value?.loseContext());
    await expect(page.locator('#pause-menu')).toBeVisible();
    await expect(page.getByRole('button', { name: 'Resume game', exact: true })).toBeDisabled();
    await expect(page.locator('#status')).toContainText('Waiting for recovery');
    const tick = await page.locator('#game').getAttribute('data-tick');
    await page.waitForTimeout(350);
    expect(await page.locator('#game').getAttribute('data-tick')).toBe(tick);
    await extension.evaluate(value => new Promise<void>(resolve => {
        const canvas = document.querySelector('canvas');
        if (!canvas || !value) throw new Error('Missing graphics context');
        canvas.addEventListener('webglcontextrestored', () => resolve(), { once: true });
        value.restoreContext();
    }));
    await expect(page.getByRole('button', { name: 'Resume game', exact: true })).toBeEnabled();
    await expect(page.locator('#pause-menu')).toBeVisible();
    expect(await page.locator('#game').getAttribute('data-tick')).toBe(tick);
    await page.getByRole('button', { name: 'Resume game', exact: true }).click();
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-tick'))).toBeGreaterThan(Number(tick));
    await page.screenshot({ path: testInfo.outputPath('recovered.png') });
    expect(errors).toEqual([]);
});
