import { defineConfig, devices } from '@playwright/test';

export default defineConfig({
    testDir: './e2e',
    fullyParallel: false,
    workers: 1,
    use: {
        ...devices['Desktop Edge'],
        channel: 'msedge',
        baseURL: 'http://127.0.0.1:4173',
        launchOptions: { args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader'] },
        screenshot: 'only-on-failure',
    },
    webServer: {
        command: 'npm run preview',
        url: 'http://127.0.0.1:4173',
        reuseExistingServer: false,
        timeout: 30_000,
    },
});
