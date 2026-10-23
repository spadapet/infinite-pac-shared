import { test, expect } from '@playwright/test';

test('gameplay waits for every original WAV to decode, with no stale effects replayed after loading', async ({ page }) => {
    const errors: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    await page.addInitScript(() => {
        let pending = 0, decoded = 0, starts = 0, earlyStarts = 0;
        const decode = AudioContext.prototype.decodeAudioData;
        AudioContext.prototype.decodeAudioData = function(data: ArrayBuffer) {
            pending++;
            return decode.call(this, data).then(buffer => {
                pending--;
                document.documentElement.dataset.audioDecoded = String(++decoded);
                return buffer;
            });
        };
        const create = AudioContext.prototype.createBufferSource;
        AudioContext.prototype.createBufferSource = function() {
            const source = create.call(this);
            const start = source.start.bind(source);
            source.start = (...args: Parameters<AudioBufferSourceNode['start']>) => {
                if (pending) earlyStarts++;
                document.documentElement.dataset.audioStarts = String(++starts);
                document.documentElement.dataset.audioEarlyStarts = String(earlyStarts);
                start(...args);
            };
            return source;
        };
    });
    let release!: () => void;
    const gate = new Promise<void>(resolve => { release = resolve; });
    let requests = 0;
    await page.route('**/assets/*.wav', async route => { requests++; await gate; await route.continue(); });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
    try {
        await expect(page.locator('#start')).toBeDisabled();
        await expect.poll(() => requests).toBeGreaterThan(0);
        const tick = await page.locator('#game').getAttribute('data-tick');
        await page.waitForTimeout(250);
        await expect(page.locator('#menu')).toBeVisible();
        await expect(page.locator('#game')).toHaveAttribute('data-state', 'menu');
        await expect(page.locator('#game')).toHaveAttribute('data-tick', tick!);
        expect(await page.locator('html').getAttribute('data-audio-starts')).toBeNull();
    } finally {
        release();
    }
    await expect(page.locator('#menu')).toBeHidden();
    await expect.poll(async () => Number(await page.locator('html').getAttribute('data-audio-decoded'))).toBe(requests);
    await expect.poll(async () => Number(await page.locator('html').getAttribute('data-audio-starts'))).toBeGreaterThan(0);
    await expect(page.locator('html')).toHaveAttribute('data-audio-early-starts', '0');
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-score')), { timeout: 12_000 }).toBeGreaterThan(0);
    await expect(page.locator('html')).toHaveAttribute('data-audio-early-starts', '0');
    expect(errors).toEqual([]);
});

test('sound load errors keep the game at the menu and a retry can start normally', async ({ page }) => {
    await page.route('**/assets/*.wav', route => route.fulfill({ status: 503, body: 'Unavailable' }));
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
    await expect(page.locator('#status')).toContainText('Could not load sound');
    await expect(page.locator('#menu')).toBeVisible();
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'menu');
    await expect(page.locator('#start')).toBeEnabled();
    await page.unroute('**/assets/*.wav');
    await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
    await expect(page.locator('#menu')).toBeHidden();
    await expect(page.locator('#game')).toHaveAttribute('data-state', 'ready');
    await expect(page.locator('#status')).not.toHaveClass(/error/);
});
