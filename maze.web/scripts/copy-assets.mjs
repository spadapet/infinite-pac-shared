import { copyFile, mkdir, readdir } from 'node:fs/promises';

const source = new URL('../../maze/assets/', import.meta.url);
const destination = new URL('../public/assets/', import.meta.url);
await mkdir(destination, { recursive: true });
let count = 0;
for (const entry of await readdir(source, { withFileTypes: true })) {
    if (entry.isFile() && /\.(png|wav|json|ico)$/i.test(entry.name)) {
        await copyFile(new URL(entry.name, source), new URL(entry.name, destination));
        count++;
    }
}
console.log(`Copied ${count} original game assets.`);
