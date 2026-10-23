import mazeValues from '../../maze/assets/Values.Maze.res.json';
import mazesValues from '../../maze/assets/Values.Mazes.res.json';
import tilesValues from '../../maze/assets/Values.Tiles.res.json';
import { animationDefinition } from './native-animation';
import type {
    BubbleAnimation,
    Direction,
    GameEvent,
    GameOptions,
    GameState,
    GameView,
    GhostState,
    MazeView,
    Tile,
    Vec,
} from './types';

type Point = { x: number; y: number };
type Zone = 'normal' | 'ghostSlow' | 'ghostNoTurn' | 'outOfBounds';
type MoveMode = 'normal' | 'scared' | 'eyes' | 'eaten' | 'waiting';
type HouseMode = 'outside' | 'inside' | 'leaving';
type JsonRecord = Record<string, unknown>;

const mazeStarts = new WeakMap<MazeView, Point>();

interface Actor {
    x: number;
    y: number;
    dx: number;
    dy: number;
    pressX: number;
    pressY: number;
    speed: number;
    advance: number;
    advancePosition: number;
    active: boolean;
}

interface Ghost extends Actor {
    move: MoveMode;
    house: HouseMode;
    dotCounter: number;
}

interface Fruit extends Actor {
    type: number;
    exitTile: Point;
    chosenRandom: Set<string>;
    chosenExit: Set<string>;
}

interface PointDisplay {
    x: number;
    y: number;
    value: number;
    remaining: number;
    color: number;
}

interface Particle {
    x: number;
    y: number;
    vy: number;
    animation: BubbleAnimation;
    frame: number;
    scale: number;
    timeScale: number;
}

interface DifficultyData {
    ghostCount: number;
    pacSpeed: number;
    pacSubtract: number;
    pacAdd: number;
    elroyDots: number;
    scaredSeconds: number;
    ghostModeSeconds: number[];
    ghostDots: number[];
    lastDotSeconds: number;
    fruitType: number;
}

interface PlayerRecord {
    score: number;
    lives: number;
    level: number;
    nextFreeLife: number;
    freeLifeRepeat: number;
    freeLivesLeft: number;
    gameOver: boolean;
    run: LevelRun;
}

interface LevelRun {
    maze: MazeView;
    state: GameState;
    stateTick: number;
    playerReadyTicks: number;
    playIntro: boolean;
    ghostCount: number;
    gameOverEmitted: boolean;
    animation: GameView['animation'] & { pacTicks: number; deathTicks: number };
    pac: Actor & { canTurn: boolean; stuck: boolean };
    ghosts: Ghost[];
    fruit: Fruit;
    points: PointDisplay[];
    particles: Particle[];
    dotCount: number;
    dotCountTotal: number;
    lastDotCounter: number;
    globalDotIndex: number;
    globalDotCounter: number;
    dotEffect: number;
    ghostScatterCountdown: number;
    ghostChaseCountdown: number;
    ghostScaredCountdown: number;
    ghostEatenCountdown: number;
    ghostEatenIndex: number;
    ghostScatterChaseIndex: number;
    ghostStartPixel: Point;
    ghostStartTile: Point;
    ghostHouse: { top: number; bottom: number };
    fruitCounter: number;
    fruitIndex: number;
    fruitThresholds: number[];
    fruitPixel: Point;
    fruitStarts: Array<{ tile: Point; dir: Point }>;
    fruitHistory: number[];
    currentBackground: string | null;
}

const FIXED = 0x10000;
const TILE_PIXELS = 8;
const FRUIT_POINTS = [100, 300, 500, 700, 1000, 1500, 2000, 2500, 3000, 3500, 4000, 5000];
const GHOST_POINTS = [200, 400, 800, 1600];
const DIRECTION_VECTORS: Record<Direction, Point> = {
    up: { x: 0, y: -1 },
    left: { x: -1, y: 0 },
    down: { x: 0, y: 1 },
    right: { x: 1, y: 0 },
};
const VECTOR_DIRECTIONS: Record<string, Direction> = {
    '0,-1': 'up',
    '-1,0': 'left',
    '0,1': 'down',
    '1,0': 'right',
};

function record(value: unknown): JsonRecord {
    return value as JsonRecord;
}

function resourceValues(resource: unknown, key: string): JsonRecord {
    return record(record(resource)[key]);
}

function globalValues(resource: unknown, key: string): JsonRecord {
    return record(resourceValues(resource, key).global);
}

function vector(value: unknown): number[] {
    return Array.isArray(value) ? value.map(Number) : [];
}

function numberAt(value: unknown, index: number, fallback: number): number {
    return Array.isArray(value) ? Number(value[index] ?? fallback) : Number(value ?? fallback);
}

function resolveReference(values: JsonRecord, value: unknown): unknown {
    const seen = new Set<string>();
    while (typeof value === 'string' && value.startsWith('res:')) {
        const reference = value.slice(4);
        if (seen.has(reference) || !(reference in values)) return undefined;
        seen.add(reference);
        value = values[reference];
    }
    return value;
}

function colorNumber(values: JsonRecord, reference: unknown): number {
    const color = resolveReference(values, reference);
    if (!Array.isArray(color)) return 0;
    const [red = 0, green = 0, blue = 0] = color.map(Number);
    return ((Math.round(red * 255) << 16) | (Math.round(green * 255) << 8) | Math.round(blue * 255)) >>> 0;
}

function parsedTile(character: string, previous: string, next: string): Tile {
    let kind: Tile['kind'] = 'path';
    let zone: Zone = 'normal';
    switch (character) {
        case 'X': kind = 'wall'; break;
        case 'O': kind = 'ghostWall'; break;
        case '-': kind = 'door'; break;
        case '.': kind = 'dot'; break;
        case ',': kind = 'power'; break;
        case '=': zone = 'ghostSlow'; break;
        case '*':
            zone = 'ghostNoTurn';
            if (previous === '.' || next === '.') kind = 'dot';
            break;
        case '|': zone = 'outOfBounds'; break;
    }
    return { kind, zone };
}

/** Parse one native half-tile resource using the exact character and mirroring rules. */
export function parseMazeResource(name: string, id = name): MazeView {
    const tilesGlobal = globalValues(tilesValues, 'values_tiles');
    const mazeGlobal = globalValues(mazeValues, 'values_maze');
    const palette = resourceValues(mazeValues, 'res:values');
    const maze = record(mazeGlobal[name]);
    const tileResource = record(tilesGlobal[String(maze.tiles)]);
    const halfRows = tileResource['half-tiles'];
    if (!Array.isArray(halfRows) || halfRows.length === 0 || typeof halfRows[0] !== 'string') {
        throw new Error(`Maze resource '${name}' does not contain half-tiles.`);
    }
    const rows = halfRows.map(String);
    const halfWidth = rows[0].length;
    if (rows.some(row => row.length !== halfWidth)) {
        throw new Error(`Maze resource '${name}' has inconsistent row widths.`);
    }

    let pacStart: Point | undefined;
    const parsedRows: Tile[][] = rows.map((row, y) => {
        const result: Tile[] = [];
        for (let x = 0; x < halfWidth; x++) {
            const character = row[x] ?? ' ';
            const cell = parsedTile(character, row[x - 1] ?? '', row[x + 1] ?? '');
            result[x] = cell;
            result[halfWidth * 2 - 1 - x] = { ...cell };
            if (character === '+' && !pacStart) pacStart = point(Math.min(x, halfWidth * 2 - 1 - x), y);
        }
        return result;
    });

    const view: MazeView = {
        id,
        width: halfWidth * 2,
        height: rows.length,
        tiles: parsedRows,
        borderColor: colorNumber(palette, maze.borderColor),
        fillColor: colorNumber(palette, maze.fillColor),
        backgroundColor: colorNumber(palette, maze.backgroundColor),
    };
    if (pacStart) mazeStarts.set(view, pacStart);
    return view;
}

function difficultyFor(options: GameOptions, level: number): DifficultyData {
    const values = resourceValues(mazesValues, 'res:values');
    const mazeSetId = `${options.mazeSet}-mazes-${options.difficulty}`;
    const set = record(globalValues(mazesValues, 'values_mazes')[mazeSetId]);
    const rawDifficulties = resolveReference(values, set.difficulties);
    if (!Array.isArray(rawDifficulties) || rawDifficulties.length === 0) {
        throw new Error(`Difficulty data '${mazeSetId}' is missing.`);
    }
    const expanded: DifficultyData[] = [];
    for (const raw of rawDifficulties) {
        const item = record(raw);
        const ghostModes = vector(resolveReference(values, item.ghostModeSeconds));
        const ghostDots = vector(resolveReference(values, item.ghostDots));
        const count = Math.max(1, ...[
            item.ghosts, item.speed, item.speedSubtract, item.speedAdd, item.elroyDots,
            item.scaredSeconds, item.lastDotSecondsUntilGhost, item.fruit,
        ].map(value => Array.isArray(value) ? value.length : 1));
        for (let i = 0; i < count; i++) {
            expanded.push({
                ghostCount: numberAt(item.ghosts, i, 4),
                pacSpeed: numberAt(item.speed, i, 80),
                pacSubtract: numberAt(item.speedSubtract, i, 0),
                pacAdd: numberAt(item.speedAdd, i, 0),
                elroyDots: numberAt(item.elroyDots, i, 20),
                scaredSeconds: numberAt(item.scaredSeconds, i, 6),
                ghostModeSeconds: ghostModes.length ? ghostModes : [7, 20, 7, 20, 5, 20, 5, 0],
                ghostDots: ghostDots.length ? ghostDots : [0, 0, 30, 60],
                lastDotSeconds: numberAt(item.lastDotSecondsUntilGhost, i, 4),
                fruitType: numberAt(item.fruit, i, 0),
            });
        }
    }
    return expanded[Math.min(level, expanded.length - 1)];
}

function mazeNameFor(options: GameOptions, level: number): string {
    const values = resourceValues(mazesValues, 'res:values');
    const set = record(globalValues(mazesValues, 'values_mazes')[`${options.mazeSet}-mazes-${options.difficulty}`]);
    const order = resolveReference(values, set.mazes);
    if (!Array.isArray(order) || order.length === 0) throw new Error(`Maze order for '${options.mazeSet}' is missing.`);
    return String(order[level % order.length]);
}

function point(x = 0, y = 0): Point { return { x, y }; }
function tileAtPixel(pixel: Point, direction: Point): Point {
    return { x: Math.floor((pixel.x * 2 + direction.x) / 16), y: Math.floor((pixel.y * 2 + direction.y) / 16) };
}
function tileCenter(tile: Point): Point { return { x: tile.x * TILE_PIXELS + 4, y: tile.y * TILE_PIXELS + 4 }; }
function tileMiddleRight(tile: Point): Point { return { x: tile.x * TILE_PIXELS + 8, y: tile.y * TILE_PIXELS + 4 }; }
function sign(value: number): number { return value < 0 ? -1 : value > 0 ? 1 : 0; }
function same(a: Point, b: Point): boolean { return a.x === b.x && a.y === b.y; }
function directionName(actor: Actor): Direction { return VECTOR_DIRECTIONS[`${actor.dx},${actor.dy}`] ?? 'left'; }
function directionFor(direction: Direction): Point { return DIRECTION_VECTORS[direction]; }
function actor(): Actor {
    return { x: 0, y: 0, dx: 0, dy: 0, pressX: 0, pressY: 0, speed: 0, advance: 0, advancePosition: 0, active: false };
}
function fixedAdvance(speed: number): number {
    return Math.floor((speed * 75.5 / (100 * 60)) * FIXED);
}
function movementSteps(value: Actor): number {
    if (!value.active) return 0;
    value.advancePosition += value.advance;
    let count = 0;
    while (value.advancePosition >= FIXED) {
        value.advancePosition -= FIXED;
        count++;
    }
    return count;
}
export class Game {
    private readonly options: GameOptions;
    private readonly random: () => number;
    private readonly players: PlayerRecord[];
    private readonly events: GameEvent[] = [];
    private tickCount = 0;
    private currentPlayer = 0;
    private switchCountdown = 0;
    private gameOver = false;
    private mazeSerial = 0;

    constructor(options: GameOptions, random: () => number = Math.random) {
        if (!Number.isInteger(options.players) || options.players < 1 || options.players > 2) {
            throw new RangeError('players must be 1 or 2.');
        }
        this.options = { ...options };
        this.random = random;
        this.players = Array.from({ length: options.players }, (_, index) => {
            const player: PlayerRecord = {
                score: 0,
                lives: 3,
                level: 0,
                nextFreeLife: 10000,
                freeLifeRepeat: options.difficulty === 'hard' ? 20000 : 10000,
                freeLivesLeft: options.difficulty === 'easy' ? 3 : 2,
                gameOver: false,
                run: this.createLevel(0, index === 0),
            };
            return player;
        });
    }

    get view(): GameView {
        const player = this.players[this.currentPlayer];
        const run = player.run;
        const ghosts = run.ghosts.flatMap((ghost, index) => ghost.active && ghost.move !== 'eaten' && ghost.move !== 'waiting' ? [{
            index,
            position: { x: ghost.x, y: ghost.y },
            direction: directionName(ghost),
            eyeDirection: ghost.house === 'outside' && (ghost.pressX || ghost.pressY)
                ? VECTOR_DIRECTIONS[`${ghost.pressX},${ghost.pressY}`] : directionName(ghost),
            state: this.ghostViewState(ghost, run),
        }] : []);
        return {
            state: this.gameOver ? 'gameover' : run.state,
            tick: this.tickCount,
            stateTick: run.stateTick,
            playerReady: run.state === 'ready' && run.stateTick <= run.playerReadyTicks && run.playerReadyTicks > 0,
            animation: { ...run.animation },
            level: player.level + 1,
            score: player.score,
            scores: this.players.map(item => item.score),
            highScore: Math.max(...this.players.map(item => item.score)),
            lives: player.lives,
            player: this.currentPlayer + 1,
            playerCount: this.players.length,
            character: this.options.character,
            maze: run.maze,
            pac: {
                position: { x: run.pac.x, y: run.pac.y },
                direction: directionName(run.pac),
                powered: run.ghosts.some(ghost => ghost.active && ['scared', 'eaten', 'waiting'].includes(ghost.move)),
                moving: !run.pac.stuck && (run.pac.dx !== 0 || run.pac.dy !== 0),
                visible: run.pac.active && run.ghostEatenCountdown === 0 && (run.state !== 'ready' || run.stateTick > run.playerReadyTicks || run.playerReadyTicks === 0),
            },
            ghosts,
            fruit: run.fruit.active ? { position: { x: run.fruit.x, y: run.fruit.y }, type: run.fruit.type, moving: run.fruit.dx !== 0 || run.fruit.dy !== 0 } : null,
            points: run.points.map(display => ({
                position: { x: display.x, y: display.y }, value: display.value, remaining: display.remaining, color: display.color,
            })),
            particles: run.particles.map(particle => ({
                position: { x: particle.x, y: particle.y },
                animation: particle.animation,
                frame: particle.frame,
                scale: particle.scale,
            })),
            fruitHistory: run.fruitHistory.slice(),
        };
    }

    advance(): void {
        this.tickCount++;
        if (this.gameOver) return;
        if (this.switchCountdown > 0) {
            this.switchCountdown--;
            if (this.switchCountdown === 0) this.advancePlayerSwitch();
            return;
        }

        const player = this.players[this.currentPlayer];
        if (!player || player.gameOver) return;
        const run = player.run;
        switch (run.state) {
            case 'ready':
                if (run.playIntro && run.stateTick === 0) this.sound('intro');
                if (run.stateTick > 120 + run.playerReadyTicks) {
                    this.setState(run, 'playing');
                    if (run.playIntro) this.events.push({ type: 'stopSound', name: 'mr-intro' });
                }
                break;
            case 'playing':
                this.advanceActors(player, run);
                break;
            case 'caught':
                if (run.stateTick > 60) this.setState(run, 'dying');
                break;
            case 'dying':
                if (run.stateTick === 30) {
                    this.sound('die');
                    this.events.push({ type: 'vibrate', duration: 1250 });
                }
                if (run.stateTick > 120) this.setState(run, 'dead');
                break;
            case 'dead':
                if (run.stateTick > 30) this.finishDeath(player);
                break;
            case 'winning':
                if (run.stateTick > 240) this.finishLevel(player);
                break;
        }
        if (run.state !== 'playing') this.stopBackground(run);
        if (!player.gameOver) this.checkFreeLife(player);
        this.advanceAnimation(run);
        run.stateTick++;
    }

    private advanceAnimation(run: LevelRun): void {
        const animation = run.animation;
        if (run.state !== 'ready' && !(run.state === 'playing' && run.pac.stuck)) {
            if (run.state === 'dying') {
                animation.deathFrame = Math.min(Math.max(0, animation.deathTicks - 30), 11 * 8) / 8;
                animation.deathTicks++;
            } else if (run.state !== 'dead' && run.state !== 'gameover') {
                animation.pacFrame = run.state === 'winning' ? 0 : animation.pacTicks * 45 / 60;
                if (run.state !== 'caught') animation.pacTicks++;
            }
        }
        if (run.state !== 'ready') {
            animation.ghostTicks++;
            animation.dotTicks++;
        }
        animation.totalTicks++;
    }

    press(direction: Direction | Vec | null): void {
        const run = this.players[this.currentPlayer].run;
        const vector = direction === null ? point() : typeof direction === 'string' ? directionFor(direction) : direction;
        if (run.pac.pressX !== vector.x || run.pac.pressY !== vector.y) {
            run.pac.pressX = vector.x;
            run.pac.pressY = vector.y;
            run.pac.canTurn = true;
        }
    }

    drainEvents(): GameEvent[] {
        return this.events.splice(0, this.events.length);
    }

    private createLevel(level: number, playerReady = false): LevelRun {
        const mazeName = mazeNameFor(this.options, level);
        const maze = parseMazeResource(mazeName, `${mazeName}:${++this.mazeSerial}`);
        const difficulty = difficultyFor(this.options, level);
        const pac = Object.assign(actor(), { canTurn: true, stuck: false });
        const fruit: Fruit = Object.assign(actor(), { type: 0, exitTile: point(), chosenRandom: new Set<string>(), chosenExit: new Set<string>() });
        const ghosts: Ghost[] = Array.from({ length: 4 }, () => Object.assign(actor(), {
            move: 'normal' as MoveMode, house: 'outside' as HouseMode, dotCounter: 0,
        }));
        const run: LevelRun = {
            maze, state: 'ready', stateTick: 0, playerReadyTicks: playerReady ? 150 : 0, playIntro: playerReady && level === 0,
            ghostCount: 0, gameOverEmitted: false, pac, ghosts, fruit, points: [], particles: [],
            animation: { pacFrame: 0, deathFrame: 0, ghostTicks: 0, dotTicks: 0, totalTicks: 0, pacTicks: 0, deathTicks: 0 },
            dotCount: 0, dotCountTotal: 0, lastDotCounter: 0, globalDotIndex: 0, globalDotCounter: 0,
            dotEffect: 0, ghostScatterCountdown: 0, ghostChaseCountdown: 0, ghostScaredCountdown: 0,
            ghostEatenCountdown: 0, ghostEatenIndex: 0, ghostScatterChaseIndex: 0,
            ghostStartPixel: point(), ghostStartTile: point(), ghostHouse: { top: 0, bottom: 0 },
            fruitCounter: 0, fruitIndex: 0, fruitThresholds: [], fruitPixel: point(), fruitStarts: [],
            fruitHistory: this.fruitHistory(level), currentBackground: null,
        };

        let foundDoor = false;
        for (let y = 0; y < maze.height; y++) {
            for (let x = 0; x < maze.width; x++) {
                const cell = maze.tiles[y][x];
                if (cell.kind === 'dot' || cell.kind === 'power') run.dotCount++;
                if (cell.kind === 'door' && !foundDoor) {
                    foundDoor = true;
                    run.ghostCount = Math.min(difficulty.ghostCount, 4);
                    run.ghostStartTile = point(x, y - 1);
                    run.ghostStartPixel = tileMiddleRight(run.ghostStartTile);
                    run.fruitPixel = tileMiddleRight(point(x, y + 5));
                    run.ghostHouse = { top: (y + 2) * 8, bottom: (y + 3) * 8 };
                    const starts = [point(x, y - 1), point(x, y + 2), point(x - 2, y + 2), point(x + 2, y + 2)];
                    const dirs = [point(-1, 0), point(0, 1), point(0, -1), point(0, -1)];
                    for (let i = 0; i < run.ghosts.length; i++) {
                        const ghost = run.ghosts[i];
                        ghost.active = i < run.ghostCount;
                        if (ghost.active) {
                            const position = tileMiddleRight(starts[i]);
                            ghost.x = position.x;
                            ghost.y = position.y;
                            ghost.dx = dirs[i].x;
                            ghost.dy = dirs[i].y;
                            ghost.house = i === 0 ? 'outside' : 'inside';
                            ghost.dotCounter = difficulty.ghostDots[i] ?? 0;
                        }
                    }
                }
                if (cell.zone === 'ghostSlow') {
                    if (x === 0) run.fruitStarts.push({ tile: point(-1, y), dir: point(1, 0) });
                    else if (x === maze.width - 1) run.fruitStarts.push({ tile: point(x + 1, y), dir: point(-1, 0) });
                    else if (y === 0) run.fruitStarts.push({ tile: point(x, -1), dir: point(0, 1) });
                    else if (y === maze.height - 1) run.fruitStarts.push({ tile: point(x, y + 1), dir: point(0, -1) });
                }
            }
        }
        const pacStart = mazeStarts.get(maze);
        if (pacStart) {
            const position = tileMiddleRight(pacStart);
            pac.active = true;
            pac.x = position.x;
            pac.y = position.y;
            pac.dx = -1;
        }
        run.dotCountTotal = run.dotCount;
        run.fruitThresholds = [Math.floor(run.dotCount * 143 / 200), Math.floor(run.dotCount * 61 / 200)];
        const scatter = difficulty.ghostModeSeconds[0] || 1;
        run.ghostScatterCountdown = Math.max(1, scatter) * 60;
        run.ghostChaseCountdown = Math.min(5 * 60 * 60, (difficulty.ghostModeSeconds[1] || 300) * 60);
        if (!pac.active || !run.ghostCount) throw new Error(`Maze resource '${mazeName}' is missing gameplay actors.`);
        return run;
    }

    private fruitHistory(level: number): number[] {
        const history: number[] = [];
        let previous = -1;
        for (let index = level; index >= 0 && history.length < 7; index--) {
            const fruit = difficultyFor(this.options, index).fruitType;
            if (fruit !== 13 && (fruit !== 12 || fruit !== previous)) history.push(fruit);
            previous = fruit;
        }
        return history;
    }

    private setState(run: LevelRun, state: GameState): void {
        run.state = state;
        run.stateTick = 0;
    }

    private advanceActors(player: PlayerRecord, run: LevelRun): void {
        this.updateGhostMode(run);
        this.updatePoints(run);
        if (!run.ghostEatenCountdown) {
            this.updateGhostHouse(run);
            this.updateFruitMode(run, player);
            this.updateSpeeds(run, player.level);
        }

        for (const ghost of run.ghosts) {
            for (let steps = movementSteps(ghost); steps > 0; steps--) {
                if (!run.ghostEatenCountdown || ghost.move === 'eyes') this.advanceGhost(ghost, run);
            }
        }
        if (!run.ghostEatenCountdown) {
            for (let steps = movementSteps(run.fruit); steps > 0; steps--) this.advanceFruit(run.fruit, run);
            for (let steps = movementSteps(run.pac); steps > 0; steps--) {
                this.advancePac(run.pac, run);
                if (steps > 1) this.checkCollisions(player, run);
            }
            this.advanceParticles(run);
            this.checkCollisions(player, run);
        }
        if (run.state === 'playing') this.advanceBackground(run);
        else this.stopBackground(run);
    }

    private advancePac(pac: LevelRun['pac'], run: LevelRun): void {
        let dx = pac.dx;
        let dy = pac.dy;
        let pixel = point(pac.x, pac.y);
        const tile = tileAtPixel(pixel, point(dx, dy));
        const center = tileCenter(tile);
        const press = point(pac.pressX, pac.pressY);
        if ((dx && press.x === -dx) || (dy && press.y === -dy)) {
            dx = -dx;
            dy = -dy;
        } else if (pac.canTurn && this.zoneAt(run.maze, tile) !== 'outOfBounds') {
            if (dx && press.y && pixel.y === center.y && !this.isWall(run.maze, point(tile.x, tile.y + press.y))) {
                dx = 0; dy = press.y;
            } else if (dy && press.x && pixel.x === center.x && !this.isWall(run.maze, point(tile.x + press.x, tile.y))) {
                dx = press.x; dy = 0;
            }
            if (dx !== pac.dx || dy !== pac.dy) pac.canTurn = false;
        }
        if (dx && pixel.y !== center.y) pixel.y += sign(center.y - pixel.y);
        else if (dy && pixel.x !== center.x) pixel.x += sign(center.x - pixel.x);

        if (((dx && pixel.x === center.x) || (dy && pixel.y === center.y)) && this.isWall(run.maze, point(tile.x + dx, tile.y + dy))) {
            pac.stuck = true;
            pac.canTurn = true;
        } else {
            pac.stuck = false;
            pixel.x += dx;
            pixel.y += dy;
        }
        // Native SetDir precedes SetPixel, whose tile-change callback unlocks the next turn.
        const oldTile = tileAtPixel(point(pac.x, pac.y), point(dx, dy));
        pac.dx = dx; pac.dy = dy; pac.x = pixel.x; pac.y = pixel.y;
        const warped = this.checkTunnel(pac, run.maze);
        if (!same(oldTile, tileAtPixel(pixel, point(dx, dy))) || warped) pac.canTurn = true;
    }

    private advanceGhost(ghost: Ghost, run: LevelRun): void {
        let dx = ghost.dx;
        let dy = ghost.dy;
        const tile = tileAtPixel(point(ghost.x, ghost.y), point(dx, dy));
        const center = tileCenter(tile);
        if (ghost.house === 'inside') {
            if (ghost.move === 'eyes') {
                let targetX = run.ghostStartPixel.x;
                if (run.ghosts.indexOf(ghost) === 2) targetX -= 16;
                else if (run.ghosts.indexOf(ghost) === 3) targetX += 16;
                if (ghost.y < run.ghostHouse.bottom) { dx = 0; dy = 1; }
                else if (ghost.x !== targetX) { dx = sign(targetX - ghost.x); dy = 0; }
                else { dx = 0; dy = -1; ghost.move = 'normal'; ghost.house = 'inside'; }
            } else if (ghost.y === run.ghostHouse.top || ghost.y === run.ghostHouse.bottom) {
                dy = -dy;
            }
        } else if (ghost.house === 'leaving') {
            if (ghost.x !== run.ghostStartPixel.x) { dx = sign(run.ghostStartPixel.x - ghost.x); dy = 0; }
            else if (ghost.y > run.ghostStartPixel.y) { dx = 0; dy = -1; }
            else { ghost.house = 'outside'; dx = -1; dy = 0; }
        } else if (ghost.move === 'eyes' && ghost.x === run.ghostStartPixel.x && ghost.y === run.ghostStartPixel.y) {
            ghost.house = 'inside';
        } else if (ghost.x === center.x && ghost.y === center.y) {
            if (ghost.pressX || ghost.pressY) {
                dx = ghost.pressX; dy = ghost.pressY;
                ghost.pressX = 0; ghost.pressY = 0;
            }
            const next = this.ghostDecideDirection(ghost, run, point(dx, dy), ghost.move);
            if (next) { ghost.pressX = next.x; ghost.pressY = next.y; }
        }
        ghost.x += dx; ghost.y += dy;
        ghost.dx = dx; ghost.dy = dy;
        this.checkTunnel(ghost, run.maze);
    }

    private advanceFruit(fruit: Fruit, run: LevelRun): void {
        if (!fruit.active || (!fruit.dx && !fruit.dy)) return;
        let dx = fruit.dx; let dy = fruit.dy;
        const tile = tileAtPixel(point(fruit.x, fruit.y), point(dx, dy));
        const center = tileCenter(tile);
        if (fruit.x === center.x && fruit.y === center.y) {
            const next = this.ghostDecideDirection(fruit, run, point(dx, dy), null, true);
            if (next) { dx = next.x; dy = next.y; }
            this.sound('fruit-bounce');
        }
        fruit.x += dx; fruit.y += dy; fruit.dx = dx; fruit.dy = dy;
        if (this.checkTunnel(fruit, run.maze)) this.resetFruit(fruit);
    }

    private ghostDecideDirection(actor: Actor, run: LevelRun, direction: Point, move: MoveMode | null, fruit = false): Point | null {
        const currentTile = tileAtPixel(point(actor.x, actor.y), direction);
        const tile = fruit ? currentTile : point(currentTile.x + direction.x, currentTile.y + direction.y);
        const previous = point(tile.x - direction.x, tile.y - direction.y);
        const zone = this.zoneAt(run.maze, tile);
        const allowTurn = zone !== 'outOfBounds' && (move !== 'normal' || !direction.x || zone !== 'ghostNoTurn');
        if (!allowTurn) return null;
        const candidates = [point(tile.x, tile.y - 1), point(tile.x - 1, tile.y), point(tile.x, tile.y + 1), point(tile.x + 1, tile.y)]
            .filter(next => !same(next, previous) && !this.isWall(run.maze, next));
        if (!candidates.length) return point(-direction.x, -direction.y);
        let choice: Point;
        if (candidates.length === 1) choice = candidates[0];
        else if (fruit) choice = this.fruitChooseTile(run, currentTile, candidates);
        else if (move === 'scared' || move === 'waiting') choice = candidates[Math.floor(this.random() * candidates.length) % candidates.length];
        else {
            const index = run.ghosts.findIndex(item => item === actor);
            choice = this.closestTile(candidates, this.ghostTarget(index, move, run));
        }
        const press = point(choice.x - tile.x, choice.y - tile.y);
        return same(press, direction) ? null : press;
    }

    private fruitChooseTile(run: LevelRun, currentTile: Point, candidates: Point[]): Point {
        const exiting = run.fruitCounter === 0;
        const chosen = exiting ? run.fruit.chosenExit : run.fruit.chosenRandom;
        const remaining = candidates.slice();
        while (remaining.length) {
            const choice = exiting
                ? this.closestTile(remaining, tileCenter(run.fruit.exitTile))
                : remaining[Math.floor(this.random() * remaining.length)];
            const key = `${currentTile.x},${currentTile.y}:${choice.x},${choice.y}`;
            if (chosen.has(key) || (!exiting && this.zoneAt(run.maze, choice) === 'ghostSlow')) {
                remaining.splice(remaining.indexOf(choice), 1);
            } else {
                chosen.add(key);
                return choice;
            }
        }
        return candidates[Math.floor(this.random() * candidates.length)];
    }

    private closestTile(candidates: Point[], target: Point): Point {
        return candidates.reduce((best, candidate) =>
            this.distanceSquared(tileCenter(candidate), target) < this.distanceSquared(tileCenter(best), target) ? candidate : best, candidates[0]);
    }

    private ghostScatterTarget(index: number, run: LevelRun): Point {
        const size = run.maze;
        if (this.options.mazeSet === 'ms') index = Math.floor(this.random() * 4);
        return tileCenter([point(size.width - 3, -3), point(2, -3), point(size.width - 1, size.height + 1), point(0, size.height + 1)][index]);
    }

    private ghostTarget(index: number, move: MoveMode | null, run: LevelRun): Point {
        if (move === 'eyes') return run.ghostStartPixel;
        if (run.ghostScatterCountdown > 0 || !run.pac.active) return this.ghostScatterTarget(index, run);
        const pac = point(run.pac.x, run.pac.y);
        if (index === 0) return pac;
        if (index === 1) return point(pac.x + run.pac.dx * 32, pac.y + run.pac.dy * 32);
        if (index === 2) {
            const red = run.ghosts[0];
            return point(pac.x * 2 - red.x, pac.y * 2 - red.y);
        }
        const orange = run.ghosts[3];
        return this.distanceSquared(pac, point(orange.x, orange.y)) >= 8192 ? pac : this.ghostScatterTarget(index, run);
    }

    private updateGhostMode(run: LevelRun): void {
        if (run.ghostEatenCountdown > 0) {
            if (--run.ghostEatenCountdown === 0) {
                for (const ghost of run.ghosts) if (ghost.active && ghost.move === 'eaten') ghost.move = 'eyes';
            }
        } else if (run.ghostScaredCountdown > 0) {
            if (--run.ghostScaredCountdown === 0) this.scareGhosts(run, false);
        } else if (run.ghostScatterCountdown > 0) {
            if (--run.ghostScatterCountdown === 0) this.flipGhosts(run);
        } else if (run.ghostChaseCountdown > 0) {
            if (--run.ghostChaseCountdown === 0) {
                this.flipGhosts(run);
                run.ghostScatterChaseIndex++;
                const difficulty = difficultyFor(this.options, this.players[this.currentPlayer].level);
                const index = run.ghostScatterChaseIndex * 2;
                run.ghostScatterCountdown = Math.max(1, difficulty.ghostModeSeconds[index] || 1) * 60;
                run.ghostChaseCountdown = Math.min(18000, (difficulty.ghostModeSeconds[index + 1] || 300) * 60);
            }
        }
    }

    private updateGhostHouse(run: LevelRun): void {
        const red = run.ghosts[0];
        if (red.active && red.move === 'normal' && red.house === 'inside') red.house = 'leaving';
        run.lastDotCounter++;
        const difficulty = difficultyFor(this.options, this.players[this.currentPlayer].level);
        if (run.lastDotCounter > difficulty.lastDotSeconds * 60) this.releaseGhost(run);
        else if (run.globalDotIndex > 0) {
            const thresholds = [0, 7, 17, 32];
            if (run.globalDotCounter >= (thresholds[run.globalDotIndex] ?? 32) && this.countGhostsInHouse(run)) {
                this.releaseGhost(run);
                run.globalDotIndex++;
                if (!this.countGhostsInHouse(run)) run.globalDotIndex = 0;
            }
        } else {
            for (const ghost of run.ghosts) {
                if (ghost.active && ghost.move === 'normal' && ghost.house === 'inside' && ghost.dotCounter === 0) {
                    this.releaseGhost(run);
                    break;
                }
            }
        }
    }

    private updateSpeeds(run: LevelRun, level: number): void {
        const difficulty = difficultyFor(this.options, level);
        const powered = run.ghostScaredCountdown > 0;
        let pacSpeed = difficulty.pacSpeed - difficulty.pacSubtract + difficulty.pacAdd;
        if (powered) pacSpeed = Math.max(pacSpeed, Math.floor((difficulty.pacSpeed + 100) / 2));
        this.setSpeed(run.pac, pacSpeed);
        this.setSpeed(run.fruit, Math.floor(difficulty.pacSpeed / 2));
        for (let index = 0; index < run.ghosts.length; index++) {
            const ghost = run.ghosts[index];
            if (!ghost.active) continue;
            const zone = this.zoneAt(run.maze, tileAtPixel(point(ghost.x, ghost.y), point(ghost.dx, ghost.dy)));
            let speed: number;
            if (ghost.move === 'eyes') speed = Math.max(150, difficulty.pacSpeed - 5);
            else if (zone === 'ghostSlow' || zone === 'outOfBounds' || ghost.house !== 'outside') speed = Math.floor(difficulty.pacSpeed / 2);
            else if (ghost.move === 'scared') speed = Math.floor(difficulty.pacSpeed / 2) + 10;
            else if (index === 0 && run.dotCount <= difficulty.elroyDots / 2) speed = difficulty.pacSpeed + 5;
            else if (index === 0 && run.dotCount <= difficulty.elroyDots) speed = difficulty.pacSpeed;
            else speed = difficulty.pacSpeed - 5;
            this.setSpeed(ghost, speed);
        }
    }

    private setSpeed(value: Actor, speed: number): void {
        if (value.speed !== speed && value.active) {
            value.speed = speed;
            value.advance = fixedAdvance(speed);
        }
    }

    private checkCollisions(player: PlayerRecord, run: LevelRun): void {
        if (this.actorsCollide(run.pac, run.fruit)) this.eatFruit(player, run);
        const tile = tileAtPixel(point(run.pac.x, run.pac.y), point(run.pac.dx, run.pac.dy));
        const cell = this.cellAt(run.maze, tile);
        if (cell.kind === 'dot' || cell.kind === 'power') {
            const power = cell.kind === 'power';
            cell.kind = 'path';
            this.eatDot(player, run, power);
        }
        if (run.state === 'winning' || run.ghostEatenCountdown > 0) return;
        for (const ghost of run.ghosts) {
            if (!ghost.active) continue;
            const collide = this.actorsCollide(run.pac, ghost);
            if (collide && ghost.move === 'normal') {
                this.setState(run, 'caught');
                return;
            }
            if ((collide && ghost.move === 'scared') || ghost.move === 'waiting') {
                if (!run.ghostEatenCountdown) this.eatGhost(player, run, ghost);
                else ghost.move = 'waiting';
            }
        }
    }

    private eatDot(player: PlayerRecord, run: LevelRun, power: boolean): void {
        run.lastDotCounter = 0;
        if (run.dotCount > 0) run.dotCount--;
        if (run.globalDotIndex > 0) run.globalDotCounter++;
        else {
            const houseGhost = run.ghosts.find(ghost => ghost.active && ghost.move === 'normal' && ghost.house === 'inside' && ghost.dotCounter > 0);
            if (houseGhost) houseGhost.dotCounter--;
        }
        if (run.dotCount === 0) this.setState(run, 'winning');
        player.score += power ? 50 : 10;
        this.checkFreeLife(player);
        const vibratesForDot = run.dotEffect === 0;
        if (power) {
            this.scareGhosts(run, true);
            run.pac.advancePosition -= 3 * FIXED;
            this.events.push({ type: 'vibrate', duration: 500 });
        } else {
            run.pac.advancePosition -= FIXED;
            if (vibratesForDot) this.events.push({ type: 'vibrate', duration: 100 });
        }
        if (run.state === 'winning') this.sound('level-win');
        else {
            this.sound(`eat-dot-${run.dotEffect ? '2' : '1'}`);
            run.dotEffect = run.dotEffect ? 0 : 1;
        }
        this.addParticles(run, point(run.pac.x, run.pac.y), power ? 20 : 2, power ? 13 : 7);
    }

    private eatGhost(player: PlayerRecord, run: LevelRun, ghost: Ghost): void {
        ghost.move = 'eaten';
        run.ghostEatenCountdown = 54;
        const score = GHOST_POINTS[run.ghostEatenIndex] ?? GHOST_POINTS[0];
        run.ghostEatenIndex++;
        player.score += score;
        run.points.push({ x: ghost.x, y: ghost.y, value: score, remaining: 54, color: 0x00ffff });
        this.checkFreeLife(player);
        this.sound('eat-ghost');
        this.events.push({ type: 'vibrate', duration: 750 });
        this.addParticles(run, point(ghost.x, ghost.y), 30, 13);
    }

    private eatFruit(player: PlayerRecord, run: LevelRun): void {
        run.fruitCounter = 0;
        const score = FRUIT_POINTS[run.fruit.type] ?? 0;
        player.score += score;
        this.checkFreeLife(player);
        run.points.push({ x: run.fruit.x, y: run.fruit.y, value: score, remaining: 120, color: 0xffb8ff });
        this.sound('eat-fruit');
        this.events.push({ type: 'vibrate', duration: 500 });
        this.addParticles(run, point(run.fruit.x, run.fruit.y), 20, 13);
        this.resetFruit(run.fruit);
    }

    private resetFruit(fruit: Fruit): void {
        Object.assign(fruit, actor(), { active: false, type: 0, exitTile: point() });
        fruit.chosenRandom.clear();
        fruit.chosenExit.clear();
    }

    private updateFruitMode(run: LevelRun, player: PlayerRecord): void {
        if (run.fruitCounter > 0) run.fruitCounter--;
        else if (!run.fruit.active && run.fruitIndex < 2 && run.dotCount > 0 && run.dotCount === run.fruitThresholds[run.fruitIndex] && difficultyFor(this.options, player.level).fruitType !== 13) {
            run.fruitIndex++;
            const difficulty = difficultyFor(this.options, player.level);
            run.fruitCounter = 14 * 60 + Math.floor(this.random() * 60);
            run.fruit.active = true;
            if (run.fruitStarts.length) {
                const start = run.fruitStarts[Math.floor(this.random() * run.fruitStarts.length) % run.fruitStarts.length];
                const position = tileCenter(start.tile);
                run.fruit.x = position.x; run.fruit.y = position.y;
                run.fruit.dx = start.dir.x; run.fruit.dy = start.dir.y;
                const exit = run.fruitStarts[Math.floor(this.random() * run.fruitStarts.length) % run.fruitStarts.length];
                run.fruit.exitTile = exit.tile;
            } else {
                run.fruit.x = run.fruitPixel.x; run.fruit.y = run.fruitPixel.y;
                run.fruit.dx = 0; run.fruit.dy = 0;
            }
            const type = difficulty.fruitType;
            run.fruit.type = type === 12 ? Math.floor(this.random() * 12) : type;
        }
        if (run.fruitCounter === 0 && run.fruit.active && run.fruit.dx === 0 && run.fruit.dy === 0) this.resetFruit(run.fruit);
    }

    private updatePoints(run: LevelRun): void {
        for (let index = run.points.length - 1; index >= 0; index--) {
            if (--run.points[index].remaining <= 0) run.points.splice(index, 1);
        }
    }

    private advanceParticles(run: LevelRun): void {
        for (let index = run.particles.length - 1; index >= 0; index--) {
            const particle = run.particles[index];
            const animation = animationDefinition(`bubble-${particle.animation + 1}-anim`);
            particle.y += particle.vy;
            particle.frame += animation.fps * particle.timeScale / 60;
            if (particle.frame >= animation.length) run.particles.splice(index, 1);
        }
    }

    private addParticles(run: LevelRun, position: Point, maximum: number, spread: number): void {
        const count = maximum - Math.floor(this.random() * (maximum / 2));
        for (let index = 0; index < count; index++) {
            run.particles.push({
                x: position.x + Math.floor(this.random() * spread) - Math.floor(spread / 2),
                y: position.y + Math.floor(this.random() * spread) - Math.floor(spread / 2),
                scale: 1 + (Math.floor(this.random() * 9) - 4) / 12,
                timeScale: 1 + (Math.floor(this.random() * 9) - 4) / 12,
                vy: Math.floor(this.random() * 10) / -100,
                animation: ([0, 1, 2] as const)[Math.floor(this.random() * 3)],
                frame: 0,
            });
        }
    }

    private advanceBackground(run: LevelRun): void {
        let name: string | null = null;
        const hasEyes = run.ghosts.some(ghost => ghost.active && ghost.move === 'eyes');
        const hasScared = run.ghosts.some(ghost => ghost.active && ['scared', 'eaten', 'waiting'].includes(ghost.move));
        if (hasEyes) name = 'bg-eaten';
        else if (hasScared) name = 'bg-scared';
        else if (run.dotCount < (run.dotCountTotal >> 3)) name = 'bg-4';
        else if (run.dotCount < (run.dotCountTotal >> 2)) name = 'bg-3';
        else if (run.dotCount < (run.dotCountTotal >> 1)) name = 'bg-2';
        else name = 'bg-1';
        const soundName = `mr-${name}`;
        if (run.currentBackground !== soundName) {
            if (run.currentBackground) this.events.push({ type: 'stopSound', name: run.currentBackground });
            this.sound(name ?? 'bg-1');
            run.currentBackground = soundName;
        }
    }

    private stopBackground(run: LevelRun): void {
        if (run.currentBackground) {
            this.events.push({ type: 'stopSound', name: run.currentBackground });
            run.currentBackground = null;
        }
    }

    private scareGhosts(run: LevelRun, scared: boolean): void {
        run.ghostEatenIndex = 0;
        if (scared) {
            this.flipGhosts(run);
            run.ghostScaredCountdown = difficultyFor(this.options, this.players[this.currentPlayer].level).scaredSeconds * 60;
        }
        if (!scared || run.ghostScaredCountdown > 0) {
            for (const ghost of run.ghosts) {
                if (!ghost.active) continue;
                if (scared && ghost.move === 'normal') ghost.move = 'scared';
                else if (!scared && ghost.move !== 'eyes' && ghost.move !== 'eaten' && ghost.move !== 'waiting') ghost.move = 'normal';
            }
        }
    }

    private flipGhosts(run: LevelRun): void {
        for (const ghost of run.ghosts) {
            if (ghost.active && !['eyes', 'eaten', 'waiting'].includes(ghost.move) && ghost.house === 'outside') {
                ghost.pressX = -ghost.dx;
                ghost.pressY = -ghost.dy;
            }
        }
    }

    private releaseGhost(run: LevelRun): boolean {
        const ghost = run.ghosts.find(item => item.active && item.move === 'normal' && item.house === 'inside');
        if (!ghost) return false;
        ghost.house = 'leaving';
        run.lastDotCounter = 0;
        return true;
    }

    private countGhostsInHouse(run: LevelRun): number {
        return run.ghosts.filter(ghost => ghost.active && ghost.move === 'normal' && ghost.house === 'inside').length;
    }

    private actorsCollide(a: Actor, b: Actor): boolean {
        if (!a.active || !b.active) return false;
        return this.distanceSquared(point(a.x, a.y), point(b.x, b.y)) < 32;
    }

    private distanceSquared(a: Point, b: Point): number {
        return (a.x - b.x) ** 2 + (a.y - b.y) ** 2;
    }

    private checkTunnel(value: Actor, maze: MazeView): boolean {
        const tile = tileAtPixel(point(value.x, value.y), point(value.dx, value.dy));
        let moved = false;
        if (value.dx < 0 && tile.x < -1) { value.x = (maze.width + 1) * 8; moved = true; }
        else if (value.dx > 0 && tile.x > maze.width) { value.x = -8; moved = true; }
        else if (value.dy < 0 && tile.y < -1) { value.y = (maze.height + 1) * 8; moved = true; }
        else if (value.dy > 0 && tile.y > maze.height) { value.y = -8; moved = true; }
        return moved;
    }

    private isWall(maze: MazeView, tile: Point): boolean {
        const kind = this.cellAt(maze, tile).kind;
        return kind === 'wall' || kind === 'ghostWall' || kind === 'door';
    }

    private zoneAt(maze: MazeView, tile: Point): Zone {
        if (tile.x < 0 || tile.y < 0 || tile.x >= maze.width || tile.y >= maze.height) return 'outOfBounds';
        return maze.tiles[tile.y][tile.x].zone as Zone;
    }

    private cellAt(maze: MazeView, tile: Point): Tile {
        if (tile.x < 0 || tile.y < 0 || tile.x >= maze.width || tile.y >= maze.height) {
            return { kind: 'void', zone: 'outOfBounds' };
        }
        return maze.tiles[tile.y][tile.x];
    }

    private ghostViewState(ghost: Ghost, run: LevelRun): GhostState {
        if (ghost.move === 'eaten' || ghost.move === 'waiting') return 'eyes';
        if (ghost.move === 'eyes') return 'eyes';
        if (ghost.move === 'scared') {
            return run.ghostScaredCountdown < 122 && run.ghostScaredCountdown % 27 < 14 ? 'flash' : 'scared';
        }
        return run.ghostScatterCountdown === 0 && run.pac.active ? 'chase' : 'scatter';
    }

    private sound(name: string): void {
        this.events.push({ type: 'sound', name: `mr-${name}` });
    }

    private checkFreeLife(player: PlayerRecord): void {
        if (player.nextFreeLife && player.freeLivesLeft && player.score >= player.nextFreeLife) {
            this.sound('free-life');
            if (player.lives <= 3) {
                player.lives++;
                player.freeLivesLeft--;
            }
            player.nextFreeLife = player.freeLifeRepeat ? player.nextFreeLife + player.freeLifeRepeat : 0;
            if (!player.freeLivesLeft) player.nextFreeLife = 0;
        }
    }

    private finishDeath(player: PlayerRecord): void {
        this.stopBackground(player.run);
        if (player.lives > 1) {
            player.lives--;
            const oldRun = player.run;
            player.run = this.createLevel(player.level);
            player.run.maze = oldRun.maze;
            player.run.dotCount = oldRun.maze.tiles.reduce((total, row) => total + row.filter(tile => tile.kind === 'dot' || tile.kind === 'power').length, 0);
            player.run.dotCountTotal = oldRun.dotCountTotal;
            player.run.fruitThresholds = oldRun.fruitThresholds.slice();
            player.run.fruitIndex = oldRun.fruitIndex;
            player.run.globalDotIndex = 1;
            player.run.globalDotCounter = 0;
            player.run.lastDotCounter = 0;
            player.run.dotEffect = oldRun.dotEffect;
            player.run.fruitCounter = 0;
            player.run.fruitHistory = oldRun.fruitHistory.slice();
            oldRun.ghosts.forEach((ghost, index) => { player.run.ghosts[index].dotCounter = ghost.dotCounter; });
            if (this.players.length > 1) this.schedulePlayerSwitch();
        } else {
            player.gameOver = true;
            if (!player.run.gameOverEmitted) {
                this.events.push({ type: 'gameOver', player: this.currentPlayer + 1, score: player.score, level: player.level + 1 });
                player.run.gameOverEmitted = true;
            }
            if (this.players.length > 1 && this.players.some(other => !other.gameOver)) this.switchCountdown = 120;
            else {
                this.gameOver = true;
                this.setState(player.run, 'gameover');
            }
        }
    }

    private finishLevel(player: PlayerRecord): void {
        player.level++;
        this.stopBackground(player.run);
        player.run = this.createLevel(player.level, this.currentPlayer === 0);
    }

    private schedulePlayerSwitch(): void {
        const other = this.currentPlayer === 0 ? 1 : 0;
        if (this.players[other] && !this.players[other].gameOver) {
            this.switchCountdown = 1;
        }
    }

    private advancePlayerSwitch(): void {
        if (this.players.length < 2) return;
        const other = this.currentPlayer === 0 ? 1 : 0;
        if (!this.players[other].gameOver) {
            this.currentPlayer = other;
            return;
        }
        if (this.players[this.currentPlayer].gameOver) {
            this.gameOver = true;
            this.setState(this.players[this.currentPlayer].run, 'gameover');
        }
    }
}
