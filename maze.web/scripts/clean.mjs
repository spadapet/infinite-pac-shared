import { rm } from 'node:fs/promises';

for (const directory of ['dist/', 'public/assets/', 'test-results/', 'playwright-report/']) {
    await rm(new URL(`../${directory}`, import.meta.url), { recursive: true, force: true });
}
