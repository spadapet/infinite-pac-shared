import { spawnSync } from 'node:child_process';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import { Game } from '../../maze.web/src/game';

const here = path.dirname(fileURLToPath(import.meta.url));
const executable = path.join(here, 'movement-oracle.exe');
const directions = [{ x: 0, y: -1 }, { x: -1, y: 0 }, { x: 0, y: 1 }, { x: 1, y: 0 }];
const presses = [{ x: 0, y: 0 }, ...directions, { x: -1, y: -1 }, { x: 1, y: -1 }, { x: -1, y: 1 }, { x: 1, y: 1 }];
let compared = 0;
let mismatches = 0;
const reports: unknown[] = [];
let commands: string[] = [];
let expected: { key: string; result: string }[] = [];
let board = '';
function flush() {
    if (!expected.length) return;
    const process = spawnSync(executable, [], { input: board + '\n' + commands.join('\n') + '\n', encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
    if (process.status !== 0) throw new Error(`Native oracle failed: ${process.status} ${process.stderr}`);
    const actual = process.stdout.trim().split(/\r?\n/);
    if (actual.length !== expected.length) throw new Error(`Native oracle returned ${actual.length} rows for ${expected.length} cases`);
    expected.forEach((item, index) => {
        compared++;
        if (item.result !== actual[index]) {
            mismatches++;
            if (reports.length < 30) reports.push({ key: item.key, web: item.result, native: actual[index] });
        }
    });
    commands = [];
    expected = [];
}
function add(command: string, result: (number | boolean)[], key: string) {
    commands.push(command);
    expected.push({ key, result: result.map(value => Number(value)).join(' ') });
    if (expected.length >= 10000) flush();
}
function snapshot(actor: Game['players'][number]['run']['pac']) {
    return [actor.x, actor.y, actor.dx, actor.dy, actor.pressX, actor.pressY, actor.canTurn, actor.stuck];
}
for (const randomDraw of [0, 0.25, 0.5, 0.75, 0.999]) for (const mazeSet of ['mr', 'ms'] as const) for (let level = 0; level < 8; level++) {
    const game = new Game({ character: mazeSet, mazeSet, difficulty: 'normal', players: 1 }, () => randomDraw);
    const player = game['players'][0];
    player.level = level;
    const run = player.run = game['createLevel'](level);
    const { maze } = run;
    const content = maze.tiles.flat().map(tile => ({ wall: 1, ghostWall: 2, door: 3, dot: 0, power: 0, path: 0, void: 0 })[tile.kind]);
    const zones = maze.tiles.flat().map(tile => ['normal', 'ghostSlow', 'ghostNoTurn', 'outOfBounds'].indexOf(tile.zone));
    board = `M ${maze.width} ${maze.height} ${run.ghostStartPixel.x} ${run.ghostStartPixel.y} ${run.ghostHouse.top} ${run.ghostHouse.bottom}\n${content.join(' ')}\n${zones.join(' ')}\nR ${randomDraw}`;
    if (randomDraw === 0) for (let y = 0; y < maze.height; y++) for (let x = 0; x < maze.width; x++) {
        if (['wall', 'ghostWall', 'door'].includes(maze.tiles[y][x].kind)) continue;
        for (const dir of directions) for (let along = -3; along <= 4; along++) for (const press of presses) for (const canTurn of [true, false]) {
            const px = x * 8 + 4 + dir.x * along, py = y * 8 + 4 + dir.y * along;
            Object.assign(run.pac, { x: px, y: py, dx: dir.x, dy: dir.y, pressX: press.x, pressY: press.y, canTurn, stuck: false });
            game['advancePac'](run.pac, run);
            add(`P ${px} ${py} ${dir.x} ${dir.y} ${press.x} ${press.y} ${Number(canTurn)}`, snapshot(run.pac), `${mazeSet}/${level}/P/${x},${y}/${dir.x},${dir.y}/${along}/${press.x},${press.y}/${canTurn}`);
        }
    }
    flush();
    const moves = ['normal', 'scared', 'eyes', 'eaten', 'waiting'] as const;
    const houses = ['outside', 'inside', 'leaving'] as const;
    for (let index = 0; index < 4; index++) for (let y = 0; y < maze.height; y++) for (let x = 0; x < maze.width; x++) {
        if (['wall', 'ghostWall', 'door'].includes(maze.tiles[y][x].kind)) continue;
        for (const dir of directions) for (const press of presses.slice(0, 5)) for (const move of moves.slice(0, 3)) for (const scatter of [0, 1]) {
            const actor = run.ghosts[index];
            const px = x * 8 + 4, py = y * 8 + 4;
            Object.assign(run.pac, { x: 100, y: 100, dx: 0, dy: -1 });
            if (index !== 0) Object.assign(run.ghosts[0], { x: 80, y: 108 });
            Object.assign(actor, { active: true, x: px, y: py, dx: dir.x, dy: dir.y, pressX: press.x, pressY: press.y, move, house: 'outside' });
            run.ghostScatterCountdown = scatter;
            game['advanceGhost'](actor, run);
            add(`G ${index} ${px} ${py} ${dir.x} ${dir.y} ${press.x} ${press.y} ${moves.indexOf(move)} 0 100 100 0 -1 80 108 ${scatter} ${Number(mazeSet === 'ms')}`,
                [actor.x, actor.y, actor.dx, actor.dy, actor.pressX, actor.pressY, moves.indexOf(actor.move), houses.indexOf(actor.house)], `${mazeSet}/${level}/G/${index}/${x},${y}/${dir.x},${dir.y}/${press.x},${press.y}/${move}/${scatter}`);
        }
    }
    flush();
    for (let index = 0; index < 4; index++) for (const house of houses) for (const move of moves) {
        for (let x = run.ghostStartPixel.x - 16; x <= run.ghostStartPixel.x + 16; x += 4) for (let y = run.ghostStartPixel.y; y <= run.ghostHouse.bottom + 1; y++) {
            for (const dir of directions) {
                const actor = run.ghosts[index];
                Object.assign(run.pac, { x: 100, y: 100, dx: 0, dy: -1 });
                if (index !== 0) Object.assign(run.ghosts[0], { x: 80, y: 108 });
                Object.assign(actor, { active: true, x, y, dx: dir.x, dy: dir.y, pressX: 0, pressY: 0, move, house });
                run.ghostScatterCountdown = 1;
                game['advanceGhost'](actor, run);
                add(`G ${index} ${x} ${y} ${dir.x} ${dir.y} 0 0 ${moves.indexOf(move)} ${houses.indexOf(house)} 100 100 0 -1 80 108 1 ${Number(mazeSet === 'ms')}`,
                    [actor.x, actor.y, actor.dx, actor.dy, actor.pressX, actor.pressY, moves.indexOf(actor.move), houses.indexOf(actor.house)], `${mazeSet}/${level}/house/${index}/${x},${y}/${dir.x},${dir.y}/${move}/${house}`);
            }
        }
    }
    flush();
    for (const start of run.fruitStarts) for (const exit of run.fruitStarts) for (const exiting of [false, true]) {
        flush();
        game['resetFruit'](run.fruit);
        Object.assign(run.fruit, { active: true, x: start.tile.x * 8 + 4, y: start.tile.y * 8 + 4, dx: start.dir.x, dy: start.dir.y, exitTile: exit.tile });
        run.fruitCounter = exiting ? 0 : 100;
        commands.push(`F ${run.fruit.x} ${run.fruit.y} ${run.fruit.dx} ${run.fruit.dy} ${exit.tile.x} ${exit.tile.y} ${Number(exiting)}`);
        for (let step = 0; step < 3000 && run.fruit.active; step++) {
            game['advanceFruit'](run.fruit, run);
            game.drainEvents();
            add('f', [run.fruit.x, run.fruit.y, run.fruit.dx, run.fruit.dy, run.fruit.pressX, run.fruit.pressY, run.fruit.active], `${mazeSet}/${level}/F/${JSON.stringify(start.tile)}/${JSON.stringify(exit.tile)}/${exiting}/${step}`);
        }
        flush();
    }
    console.log(`${mazeSet} level ${level + 1}, draw ${randomDraw}: ${compared.toLocaleString()} movement cases compared; ${mismatches} mismatches`);
}
console.log(JSON.stringify({ compared, mismatches, reports }, null, 2));
if (mismatches) process.exitCode = 1;
