import { test, expect, type Page } from '@playwright/test';

async function xboxController(page: Page, reject = false): Promise<void> {
    await page.addInitScript(({ reject }) => {
        if (!localStorage.getItem('infinite-pac-web.v1')) localStorage.setItem('infinite-pac-web.v1', JSON.stringify({ version: 1,
            settings: { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: false, vibration: true }, scores: {} }));
        const actuator = {
            async playEffect(type: string, parameters: GamepadEffectParameters) {
                if (reject) throw new Error('Bluetooth haptics unavailable');
                const calls = JSON.parse(document.documentElement.dataset.rumble ?? '[]');
                calls.push({ type, ...parameters });
                document.documentElement.dataset.rumble = JSON.stringify(calls);
                return 'complete';
            },
            async reset() {
                document.documentElement.dataset.resets = String(Number(document.documentElement.dataset.resets ?? 0) + 1);
                return 'complete';
            },
        };
        Object.defineProperty(navigator, 'getGamepads', {
            value: () => [{ index: 0, id: 'Xbox Wireless Controller', connected: true, mapping: '', axes: [0, 0],
                buttons: Array.from({ length: 16 }, (_, index) => ({
                    pressed: (document.documentElement.dataset.pad ?? '14').split(',').includes(String(index)),
                })), vibrationActuator: actuator }],
        });
    }, { reject });
    await page.goto('/');
    await expect(page.locator('#menu')).toBeVisible({ timeout: 30_000 });
    await expect(page.getByRole('button', { name: 'Sound off', exact: true })).toBeVisible();
}

const effects = (page: Page) => page.evaluate(() => JSON.parse(document.documentElement.dataset.rumble ?? '[]') as
    Array<{ type: string; duration: number; startDelay: number; strongMagnitude: number; weakMagnitude: number }>);
const resets = (page: Page) => page.evaluate(() => Number(document.documentElement.dataset.resets ?? 0));

test('native title navigation and options rumble with sound off and honor the vibration toggle', async ({ page }) => {
    await xboxController(page);
    await page.evaluate(() => { document.documentElement.dataset.pad = ''; });
    await page.keyboard.press('ArrowDown');
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '1');
    expect(await effects(page)).toEqual([]);
    await page.keyboard.press('ArrowDown');
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '2');
    await expect.poll(async () => (await effects(page)).length).toBe(1);
    expect((await effects(page))[0]).toMatchObject({ type: 'dual-rumble', startDelay: 0, strongMagnitude: 0, weakMagnitude: 0.5 });
    expect((await effects(page))[0].duration).toBeGreaterThan(0);
    expect((await effects(page))[0].duration).toBeLessThanOrEqual(100);
    await expect.poll(() => resets(page)).toBe(1);
    await page.keyboard.press('ArrowDown');
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '3');
    expect((await effects(page)).length).toBe(1);
    await page.locator('#difficulty').click();
    await expect.poll(async () => (await effects(page)).length).toBe(2);
    expect((await effects(page))[1]).toMatchObject({ type: 'dual-rumble', startDelay: 0, strongMagnitude: 0.5, weakMagnitude: 0 });
    expect((await effects(page))[1].duration).toBeGreaterThan(0);
    expect((await effects(page))[1].duration).toBeLessThanOrEqual(100);
    await page.locator('#vibration').click();
    await expect(page.locator('#vibration')).toHaveAttribute('aria-label', 'VIBRATE:OFF');
    await page.keyboard.press('ArrowDown');
    await expect(page.locator('#game')).toHaveAttribute('data-selection', '4');
    expect((await effects(page)).length).toBe(2);
    await page.locator('#vibration').click();
    await expect(page.locator('#vibration')).toHaveAttribute('aria-label', 'VIBRATE:ON');
    await expect.poll(async () => (await effects(page)).length).toBe(3);
    expect((await effects(page))[2]).toMatchObject({ strongMagnitude: 0.5, weakMagnitude: 0 });
    await expect(page.getByRole('button', { name: 'Sound off', exact: true })).toBeVisible();
});

test('Xbox rumble follows real dot scoring with sound off and stops on pause and focus loss', async ({ page }) => {
    const errors: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    await xboxController(page);
    await expect(page.locator('#vibration')).toHaveAttribute('aria-label', 'VIBRATE:ON');
    await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
    await expect.poll(async () => (await effects(page)).length, { timeout: 12_000 }).toBeGreaterThan(0);
    const first = (await effects(page))[0];
    expect(first).toMatchObject({ type: 'dual-rumble', startDelay: 0, strongMagnitude: 0, weakMagnitude: 0.5 });
    expect(first.duration).toBeGreaterThan(0);
    expect(first.duration).toBeLessThanOrEqual(100);
    await page.keyboard.press('p');
    await expect(page.locator('#pause-menu')).toBeVisible();
    await expect.poll(() => resets(page)).toBeGreaterThan(0);
    const count = (await effects(page)).length;
    await page.waitForTimeout(250);
    expect((await effects(page)).length).toBe(count);
    await page.evaluate(() => { document.documentElement.dataset.pad = '13,15'; });
    await page.getByRole('button', { name: 'Resume game', exact: true }).click();
    await expect.poll(async () => (await effects(page)).length, { intervals: [10] }).toBeGreaterThan(count);
    const beforeBlur = await resets(page);
    await page.evaluate(() => window.dispatchEvent(new Event('blur')));
    await expect(page.locator('#pause-menu')).toBeVisible();
    await expect.poll(() => resets(page)).toBeGreaterThan(beforeBlur);
    const afterBlur = (await effects(page)).length;
    await page.waitForTimeout(250);
    expect((await effects(page)).length).toBe(afterBlur);
    expect(errors).toEqual([]);
});

test('the saved Vibration option disables controller rumble without disabling gameplay', async ({ page }) => {
    await xboxController(page);
    await page.locator('#vibration').click();
    await page.reload();
    await expect(page.locator('#menu')).toBeVisible();
    await expect(page.locator('#vibration')).toHaveAttribute('aria-label', 'VIBRATE:OFF');
    await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-score')), { timeout: 12_000 }).toBeGreaterThan(0);
    expect(await effects(page)).toEqual([]);
    expect(await resets(page)).toBe(0);
});

test('a rejected Bluetooth rumble request is reported while gameplay keeps advancing', async ({ page }) => {
    const errors: string[] = [];
    page.on('pageerror', error => errors.push(error.message));
    await xboxController(page, true);
    await expect(page.locator('#vibration')).toHaveAttribute('aria-label', 'VIBRATE:ON');
    await page.getByRole('button', { name: 'Play Mr. Pac', exact: true }).click();
    await expect(page.locator('#status')).toContainText('Controller rumble could not play: Bluetooth haptics unavailable', { timeout: 12_000 });
    const tick = Number(await page.locator('#game').getAttribute('data-tick'));
    await expect.poll(async () => Number(await page.locator('#game').getAttribute('data-tick'))).toBeGreaterThan(tick);
    await expect(page.locator('#pause-menu')).toBeHidden();
    expect(errors).toEqual([]);
});
