import { Application, Assets, Container, Graphics, Rectangle, Sprite, Texture, WebGLRenderer } from 'pixi.js';
import { animationDefinition, nativeSprites, sampleAnimation, type AnimationVisual } from './native-animation';
import type { GameView, JoystickView, MazeView, Tile, Vec } from './types';

const TILE_PIXELS = 8;
const HUD_TOP = 24;
const HUD_BOTTOM = 24;
const GHOST_DOOR_COLOR = 0xffb8ff;
const WHITE = 0xffffff;

type SpriteFrames = Map<string, Texture[]>;
type SpriteAtlas = { frames: SpriteFrames };
type RenderableSprite = Sprite;
type WallPattern = readonly [number, string, string, string];

const WALL_MATCHES: ReadonlyArray<WallPattern> = [
    [0, 'WWW', 'WWW', 'WWW'], [1, ' WW', '.WW', ' WW'], [2, ' . ', 'WWW', 'WWW'],
    [3, 'WW ', 'WW.', 'WW '], [4, 'WWW', 'WWW', ' . '], [5, '...', '.WW', '.WW'],
    [6, '...', 'WW.', 'WW.'], [7, 'WW.', 'WW.', '...'], [8, '.WW', '.WW', '...'],
    [9, '.WW', 'WWW', 'WWW'], [10, 'WW.', 'WWW', 'WWW'], [11, 'WWW', 'WWW', 'WW.'],
    [12, 'WWW', 'WWW', '.WW'], [17, '.. ', '.WX', ' WO'], [18, ' ..', 'XW.', 'OW '],
    [19, 'OW ', 'XW.', ' ..'], [20, ' WO', '.WX', '.. '], [13, '   ', '.AO', '   '],
    [14, ' . ', ' A ', ' O '], [15, '   ', 'OA.', '   '], [16, ' O ', ' A ', ' . '],
    [21, '   ', ' - ', '   '], [22, '.W ', 'WWO', ' OO'], [23, ' W.', 'OWW', 'OO '],
    [24, 'OO ', 'OWW', ' W.'], [25, ' OO', 'WWO', '.W '], [26, '.WO', 'WWO', 'WWO'],
    [27, 'OW.', 'OWW', 'OWW'], [28, 'OWW', 'OWW', 'OW.'], [29, 'WWO', 'WWO', '.WO'],
    [30, '.WW', 'WWW', 'OOO'], [31, 'WW.', 'WWW', 'OOO'], [32, 'OOO', 'WWW', 'WW.'],
    [33, 'OOO', 'WWW', '.WW'], [34, '...', '.GG', '.GO'], [35, '...', 'GG.', 'OG.'],
    [36, 'OG.', 'GG.', '...'], [37, '.GO', '.GG', '...'], [38, 'O. ', 'OWW', 'OWW'],
    [39, ' .O', 'WWO', 'WWO'], [40, 'OWW', 'OWW', 'O. '], [41, 'WWO', 'WWO', ' .O'],
    [42, 'OOO', '.WW', ' WW'], [43, 'OOO', 'WW.', 'WW '], [44, ' WW', '.WW', 'OOO'],
    [45, 'WW ', 'WW.', 'OOO'],
];

function isOutOfBounds(tile: Tile | undefined): boolean {
    if (!tile) return true;
    const zone = tile.zone.toLowerCase();
    return tile.kind === 'void' || zone.includes('out') || zone === 'oob' || zone === 'void';
}

function isWall(tile: Tile | undefined): boolean {
    return tile?.kind === 'wall' || tile?.kind === 'ghostWall';
}

function tileAt(maze: MazeView, x: number, y: number): Tile | undefined {
    return maze.tiles[y]?.[x];
}

function matchesWallPattern(maze: MazeView, x: number, y: number, pattern: WallPattern): boolean {
    for (let dy = -1; dy <= 1; dy++) {
        for (let dx = -1; dx <= 1; dx++) {
            const tile = tileAt(maze, x + dx, y + dy);
            const kind = tile?.kind;
            const out = isOutOfBounds(tile);
            const cell = dy === -1 ? pattern[1][dx + 1] : dy === 0 ? pattern[2][dx + 1] : pattern[3][dx + 1];
            switch (cell) {
                case ' ':
                    break;
                case '.':
                    if (out || !tile || !['path', 'dot', 'power'].includes(tile.kind)) return false;
                    break;
                case 'O':
                    if (!out) return false;
                    break;
                case 'W':
                    if (kind !== 'wall') return false;
                    break;
                case 'G':
                    if (kind !== 'ghostWall') return false;
                    break;
                case 'A':
                    if (!isWall(tile)) return false;
                    break;
                case 'X':
                    if (!isWall(tile) && !out) return false;
                    break;
                case '-':
                    if (kind !== 'door') return false;
                    break;
                default:
                    return false;
            }
        }
    }
    return true;
}

function findWallSprite(maze: MazeView, x: number, y: number): number {
    for (const pattern of WALL_MATCHES) {
        if (matchesWallPattern(maze, x, y, pattern)) return pattern[0];
    }
    throw new Error(`No native wall pattern matches maze tile (${x}, ${y}).`);
}

async function loadSpriteFrames(): Promise<SpriteAtlas> {
    const assetRoot = `${import.meta.env.BASE_URL}assets/`;
    const loaded = new Map<string, Texture>();
    await Promise.all([...new Set([...nativeSprites.values()].map(sprite => sprite.file))].map(async file => {
        const texture = await Assets.load<Texture>(`${assetRoot}${file}`);
        texture.source.scaleMode = 'nearest';
        loaded.set(file, texture);
    }));
    const frames: SpriteFrames = new Map();
    for (const def of nativeSprites.values()) {
        const source = loaded.get(def.file);
        if (!source) throw new Error(`Sprite source was not loaded: ${def.file}`);
        const { x, y, width, height } = def.rect;
        const texture = new Texture({ source: source.source, frame: new Rectangle(x, y, width, height) });
        const match = /^(.*)\[(\d+)\]$/.exec(def.name);
        const name = match?.[1] ?? def.name;
        const group = frames.get(name) ?? [];
        group[match ? Number(match[2]) : 0] = texture;
        frames.set(name, group);
    }
    return { frames };
}

function colorAlpha(color: number): number {
    return color > 0xffffff ? ((color >>> 24) & 0xff) / 255 : 1;
}

export function sampleDotAnimation(tick: number, tileX: number, tileY: number): { offsetY: number; brightness: number } {
    const frame = tick * animationDefinition('dot-anim').fps / 60 + Math.floor(tileX / 2) + Math.floor(tileY / 2);
    const visual = sampleAnimation('dot-anim', frame)[0];
    return { offsetY: visual.position.y * 0.125, brightness: visual.color[0] };
}

function createSprite(texture: Texture): RenderableSprite {
    const sprite = new Sprite(texture) as RenderableSprite;
    sprite.anchor.set(0.5);
    sprite.roundPixels = true;
    return sprite;
}

export class GameRenderer {
    private readonly app: Application;
    private readonly mount: HTMLDivElement;
    private readonly resizeObserver: ResizeObserver;
    private readonly textures: SpriteAtlas;
    private readonly background = new Graphics();
    private readonly wallLayer = new Container();
    private readonly dotLayer = new Container();
    private readonly actorLayer = new Container();
    private readonly particleLayer = new Container();
    private readonly hudLayer = new Container();
    private readonly overlayLayer = new Container();
    private readonly joystickLayer = new Container();
    private readonly joystickPool: RenderableSprite[] = [];
    private readonly dotPool: RenderableSprite[] = [];
    private readonly actorPool: RenderableSprite[] = [];
    private readonly bubblePool: RenderableSprite[] = [];
    private readonly hudTextPool: RenderableSprite[] = [];
    private readonly pointTextPool: RenderableSprite[] = [];
    private readonly fruitPool: RenderableSprite[] = [];
    private readonly livesPool: RenderableSprite[] = [];
    private readonly overlayTextPool: RenderableSprite[] = [];
    private readonly contextStatus: HTMLDivElement;
    private readonly handleContextLost = (event: Event): void => {
        event.preventDefault();
        this.contextLost = true;
        this.contextStatus.hidden = false;
        this.contextStatus.style.display = 'grid';
        this.contextStatus.textContent = 'Graphics context lost. Waiting for WebGL to recover.';
    };
    private readonly handleContextRestored = (): void => {
        this.contextLost = false;
        this.contextStatus.hidden = true;
        this.contextStatus.style.display = 'none';
        requestAnimationFrame(() => {
            if (!this.destroyed && this.lastView) {
                this.resizeCanvas();
                this.render(this.lastView, this.lastPaused);
            }
        });
    };
    private mazeCache: { id: string; width: number; height: number; colors: string; tiles: Tile[][] } | null = null;
    private layoutWidth = 0;
    private layoutHeight = 0;
    private displayPixelRatio = 0;
    private actorCursor = 0;
    private boardTop = HUD_TOP;
    private lastView: GameView | null = null;
    private lastPaused = false;
    private contextLost = false;
    private destroyed = false;

    private constructor(host: HTMLElement, app: Application, textures: SpriteAtlas) {
        this.app = app;
        this.textures = textures;
        this.mount = document.createElement('div');
        this.mount.style.position = 'relative';
        this.mount.style.width = '100%';
        this.mount.style.height = '100%';
        this.mount.style.overflow = 'hidden';
        this.mount.style.minWidth = '0';
        this.mount.style.minHeight = '0';
        this.mount.appendChild(app.canvas);
        this.contextStatus = document.createElement('div');
        this.contextStatus.hidden = true;
        this.contextStatus.setAttribute('role', 'status');
        this.contextStatus.setAttribute('aria-live', 'assertive');
        this.contextStatus.style.position = 'absolute';
        this.contextStatus.style.inset = '0';
        this.contextStatus.style.display = 'grid';
        this.contextStatus.style.placeItems = 'center';
        this.contextStatus.style.padding = '1rem';
        this.contextStatus.style.boxSizing = 'border-box';
        this.contextStatus.style.color = '#fff';
        this.contextStatus.style.background = 'rgba(0, 0, 0, 0.85)';
        this.contextStatus.style.fontFamily = 'sans-serif';
        this.contextStatus.style.textAlign = 'center';
        this.contextStatus.style.display = 'none';
        this.mount.appendChild(this.contextStatus);
        app.canvas.addEventListener('webglcontextlost', this.handleContextLost);
        app.canvas.addEventListener('webglcontextrestored', this.handleContextRestored);
        app.canvas.style.position = 'absolute';
        app.canvas.style.imageRendering = 'pixelated';
        app.canvas.style.maxWidth = 'none';
        app.canvas.style.maxHeight = 'none';
        app.canvas.style.touchAction = 'none';
        host.appendChild(this.mount);

        this.app.stage.addChild(
            this.background,
            this.wallLayer,
            this.dotLayer,
            this.actorLayer,
            this.particleLayer,
            this.hudLayer,
            this.overlayLayer,
            this.joystickLayer,
        );
        this.resizeObserver = new ResizeObserver(() => {
            this.resizeCanvas();
            if (this.lastView && !this.destroyed && !this.contextLost) this.app.render();
        });
        this.resizeObserver.observe(this.mount);
        this.resizeCanvas();
    }

    static async create(host: HTMLElement): Promise<GameRenderer> {
        const app = new Application();
        try {
            await app.init({
                preference: 'webgl',
                autoStart: false,
                antialias: false,
                autoDensity: false,
                resolution: 1,
                backgroundColor: 0x000000,
            });
        } catch (error) {
            throw new Error('Unable to initialize the Pac renderer: WebGL is required and could not be created.', { cause: error });
        }
        if (!(app.renderer instanceof WebGLRenderer)) {
            app.destroy();
            throw new Error('Unable to initialize the Pac renderer: PixiJS did not create a WebGL renderer.');
        }
        try {
            const textures = await loadSpriteFrames();
            return new GameRenderer(host, app, textures);
        } catch (error) {
            app.destroy();
            throw error;
        }
    }

    render(view: GameView, paused: boolean, joystick: JoystickView | null = null): void {
        if (this.destroyed) return;
        this.lastView = view;
        this.lastPaused = paused;
        if (this.contextLost) return;
        const { maze } = view;
        const boardWidth = maze.width * TILE_PIXELS;
        const boardHeight = maze.height * TILE_PIXELS;
        this.boardTop = HUD_TOP;
        const width = boardWidth;
        const height = HUD_TOP + boardHeight + HUD_BOTTOM;
        if (this.layoutWidth !== width || this.layoutHeight !== height || this.displayPixelRatio !== window.devicePixelRatio) {
            this.layoutWidth = width;
            this.layoutHeight = height;
            this.resizeCanvas();
        }

        const winningFlash = view.state === 'winning' && view.stateTick >= 120 && view.stateTick < 225 && (view.stateTick - 120) % 30 < 15;
        const colorKey = `${maze.backgroundColor}:${maze.fillColor}:${maze.borderColor}:${winningFlash}`;
        const cached = this.mazeCache;
        if (!cached || cached.id !== maze.id || cached.width !== maze.width || cached.height !== maze.height ||
            cached.colors !== colorKey || cached.tiles !== maze.tiles) {
            this.rebuildStaticMaze(maze, colorKey, winningFlash);
        }

        this.wallLayer.visible = view.state !== 'winning' || view.stateTick < 240;
        this.drawDots(view);
        this.drawActors(view);
        this.drawParticles(view);
        this.drawHud(view);
        this.drawOverlay(view, paused);
        this.drawJoystick(view, paused ? null : joystick);
        this.app.render();
    }

    screenToGame(clientX: number, clientY: number): Vec | null {
        if (!this.lastView || this.destroyed || !Number.isFinite(clientX) || !Number.isFinite(clientY)) return null;
        const rect = this.app.canvas.getBoundingClientRect();
        if (rect.width <= 0 || rect.height <= 0) return null;
        const x = (clientX - rect.left) * this.layoutWidth / rect.width;
        const y = (clientY - rect.top) * this.layoutHeight / rect.height - this.boardTop;
        const width = this.lastView.maze.width * TILE_PIXELS;
        const height = this.lastView.maze.height * TILE_PIXELS;
        if (x < 0 || x >= width || y < 0 || y >= height) return null;
        return { x, y };
    }

    destroy(): void {
        if (this.destroyed) return;
        this.destroyed = true;
        this.resizeObserver.disconnect();
        this.app.canvas.removeEventListener('webglcontextlost', this.handleContextLost);
        this.app.canvas.removeEventListener('webglcontextrestored', this.handleContextRestored);
        this.mount.remove();
        this.app.destroy({ removeView: true }, { children: true });
        for (const frames of this.textures.frames.values()) {
            for (const texture of frames) texture.destroy(false);
        }
        this.mazeCache = null;
        this.lastView = null;
    }

    private resizeCanvas(): void {
        if (this.destroyed || this.contextLost || !this.layoutWidth || !this.layoutHeight) return;
        const bounds = this.mount.getBoundingClientRect();
        if (!bounds.width || !bounds.height) return;
        const scale = Math.min(bounds.width / this.layoutWidth, bounds.height / this.layoutHeight);
        const width = this.layoutWidth * scale;
        const height = this.layoutHeight * scale;
        this.displayPixelRatio = window.devicePixelRatio;
        const resolution = scale * this.displayPixelRatio;
        if (this.app.screen.width !== this.layoutWidth || this.app.screen.height !== this.layoutHeight || this.app.renderer.resolution !== resolution) {
            this.app.renderer.resize(this.layoutWidth, this.layoutHeight, resolution);
        }
        this.app.canvas.style.width = `${width}px`;
        this.app.canvas.style.height = `${height}px`;
        this.app.canvas.style.left = `${(bounds.width - width) / 2}px`;
        this.app.canvas.style.top = `${(bounds.height - height) / 2}px`;
    }

    private frames(group: string, name: string): Texture[] {
        return this.textures.frames.get(`${group}.${name}`) ?? [];
    }

    private frame(group: string, name: string, index = 0): Texture | undefined {
        const frames = this.frames(group, name);
        return frames.length ? frames[index % frames.length] : undefined;
    }

    private rebuildStaticMaze(maze: MazeView, colorKey: string, winningFlash: boolean): void {
        this.wallLayer.removeChildren().forEach(child => child.destroy());
        this.background.clear().rect(0, 0, this.layoutWidth, this.layoutHeight).fill(0x000000);
        const floor = new Graphics();
        for (let y = 0; y < maze.height; y++) {
            for (let x = 0; x < maze.width; x++) {
                const tile = tileAt(maze, x, y);
                if (!tile || isOutOfBounds(tile)) continue;
                const left = x * TILE_PIXELS;
                const top = this.boardTop + y * TILE_PIXELS;
                if (tile.kind === 'wall' || tile.kind === 'ghostWall' || tile.kind === 'door') {
                    const spriteIndex = findWallSprite(maze, x, y);
                    const door = tile.kind === 'door';
                    const fillAlpha = winningFlash && !door ? 0 : 1;
                    const borderTint = winningFlash && !door ? WHITE : door ? GHOST_DOOR_COLOR : maze.borderColor & 0xffffff;
                    this.addWallSprite('wall-bg-sprites', spriteIndex, left, top, maze.backgroundColor & 0xffffff, winningFlash ? 0 : 1);
                    this.addWallSprite('wall-fill-sprites', spriteIndex, left, top, door ? GHOST_DOOR_COLOR : winningFlash ? 0 : maze.fillColor & 0xffffff, fillAlpha);
                    this.addWallSprite('wall-border-sprites', spriteIndex, left, top, borderTint);
                } else if (!winningFlash) {
                    floor.rect(left, top, TILE_PIXELS, TILE_PIXELS).fill({
                        color: maze.backgroundColor & 0xffffff,
                        alpha: colorAlpha(maze.backgroundColor),
                    });
                }
            }
        }
        this.wallLayer.addChildAt(floor, 0);
        this.mazeCache = {
            id: maze.id,
            width: maze.width,
            height: maze.height,
            colors: colorKey,
            tiles: maze.tiles,
        };
    }

    private addWallSprite(group: string, index: number, x: number, y: number, tint: number, alpha = 1): void {
        const sprite = createSprite(Texture.EMPTY);
        this.paintSprite(sprite, `${group}.${String(index).padStart(2, '0')}`, { x, y }, { x: 0.125, y: 0.125 });
        sprite.tint = tint & 0xffffff;
        sprite.alpha = alpha;
        this.wallLayer.addChild(sprite);
    }

    private paintSprite(sprite: Sprite, name: string, position: Vec, scale: Vec, rotation = 0, visual?: AnimationVisual): void {
        const def = nativeSprites.get(name);
        const match = /^(.*?)\.(.*?)(?:\[(\d+)\])?$/.exec(name);
        const texture = match ? this.frame(match[1], match[2], Number(match[3] ?? 0)) : undefined;
        if (!def || !texture) throw new Error(`Missing native sprite: ${name}`);
        sprite.texture = texture;
        sprite.anchor.set(def.handle.x / def.rect.width, def.handle.y / def.rect.height);
        const angle = rotation * Math.PI / 180;
        const localX = (visual?.position.x ?? 0) * scale.x;
        const localY = (visual?.position.y ?? 0) * scale.y;
        sprite.position.set(position.x + localX * Math.cos(angle) - localY * Math.sin(angle), position.y + localX * Math.sin(angle) + localY * Math.cos(angle));
        sprite.scale.set(scale.x * (visual?.scale.x ?? 1) * def.scale.x, scale.y * (visual?.scale.y ?? 1) * def.scale.y);
        sprite.rotation = angle + (visual?.rotation ?? 0) * Math.PI / 180;
        const color = visual?.color ?? [1, 1, 1, 1];
        sprite.tint = Math.round(color[0] * 255) << 16 | Math.round(color[1] * 255) << 8 | Math.round(color[2] * 255);
        sprite.alpha = color[3];
    }

    private drawAnimation(name: string, frame: number, position: Vec, scale: Vec, pool: RenderableSprite[], parent: Container, used: number, rotation = 0): number {
        for (const visual of sampleAnimation(name, frame)) {
            this.paintSprite(this.obtainSprite(pool, parent, used++), visual.sprite, position, scale, rotation, visual);
        }
        return used;
    }

    private drawJoystick(view: GameView, joystick: JoystickView | null): void {
        let used = 0;
        if (joystick && joystick.length > 0) {
            const scale = Math.min(3, (joystick.length - 20) / 100 + 1);
            const sprite = this.obtainSprite(this.joystickPool, this.joystickLayer, used++);
            const rotation = Math.atan2(joystick.offset.y, joystick.offset.x) * 180 / Math.PI;
            this.paintSprite(sprite, 'char-sprites.move-arrow',
                this.boardPosition(view.pac.position), { x: scale, y: scale }, rotation);
            sprite.alpha = Math.max(0.25, Math.min(0.75, 1 / scale));
        }
        this.hideSprites(this.joystickPool, used);
    }

    private drawDots(view: GameView): void {
        let used = 0;
        if (view.state !== 'gameover' && !(view.state === 'dead' && view.stateTick >= 20)) {
            for (let y = 0; y < view.maze.height; y++) {
                for (let x = 0; x < view.maze.width; x++) {
                    const tile = tileAt(view.maze, x, y);
                    if (tile?.kind !== 'dot' && tile?.kind !== 'power') continue;
                    const power = tile.kind === 'power';
                    const name = power ? 'power-anim' : 'dot-anim';
                    const animation = animationDefinition(name);
                    const frame = view.animation.dotTicks * animation.fps / 60 + (power ? animation.length / 2 : Math.floor(x / 2) + Math.floor(y / 2));
                    used = this.drawAnimation(name, frame, { x: x * 8 + 4, y: this.boardTop + y * 8 + 4 }, { x: 0.125, y: 0.125 }, this.dotPool, this.dotLayer, used);
                }
            }
        }
        this.hideSprites(this.dotPool, used);
    }

    private drawActors(view: GameView): void {
        this.actorCursor = 0;
        const ghostsVisible = !view.playerReady && ['ready', 'playing', 'caught'].includes(view.state);
        if (ghostsVisible && view.fruit) {
            const sprite = this.obtainSprite(this.actorPool, this.actorLayer, this.actorCursor++);
            const pos = this.boardPosition(view.fruit.position);
            if (view.fruit.moving) pos.y += fruitBounceOffset(view.fruit.position);
            this.paintSprite(sprite, `char-sprites.fruit[${view.fruit.type}]`, pos, { x: 0.125, y: 0.125 });
        }
        if (ghostsVisible) {
            for (const ghost of view.ghosts) {
                if (ghost.state === 'scared' || ghost.state === 'flash') this.drawGhost(view, ghost);
            }
        }
        if (view.pac.visible) {
            const pos = this.boardPosition(view.pac.position);
            const rotation = view.pac.direction === 'up' ? 90 : view.pac.direction === 'down' ? 270 : 0;
            const scale = { x: view.pac.direction === 'right' ? -0.125 : 0.125, y: 0.125 };
            if (view.pac.powered) {
                const frame = view.animation.totalTicks * animationDefinition('power-aura-anim').fps / 60;
                this.actorCursor = this.drawAnimation('power-aura-anim', frame, pos, scale, this.actorPool, this.actorLayer, this.actorCursor, rotation);
            }
            const dying = ['dying', 'dead', 'gameover'].includes(view.state);
            const name = `${view.character}-${dying ? 'death' : view.pac.powered ? 'power' : 'char'}-anim`;
            this.actorCursor = this.drawAnimation(name, dying ? view.animation.deathFrame : view.animation.pacFrame, pos, scale, this.actorPool, this.actorLayer, this.actorCursor, rotation);
        }
        if (ghostsVisible) {
            for (const ghost of view.ghosts) {
                if (ghost.state !== 'scared' && ghost.state !== 'flash') this.drawGhost(view, ghost);
            }
        }
        this.hideSprites(this.actorPool, this.actorCursor);
    }

    private drawGhost(view: GameView, ghost: GameView['ghosts'][number]): void {
        const type = ghost.state === 'scared' ? 'scared' : ghost.state === 'flash' ? 'flash' : ghost.state === 'eyes' ? 'eyes' : 'move';
        const name = `ghost-${ghost.index + 1}-${type}-anim`;
        const pos = this.boardPosition(ghost.position);
        const scale = { x: 0.125, y: 0.125 };
        const frame = view.animation.ghostTicks * animationDefinition(name).fps / 60;
        this.actorCursor = this.drawAnimation(name, frame, pos, scale, this.actorPool, this.actorLayer, this.actorCursor);
        if (type === 'move' || type === 'eyes') {
            const pupils = `ghost-${ghost.index + 1}-pupils-anim`;
            const eye = directionVector(ghost.eyeDirection);
            this.actorCursor = this.drawAnimation(pupils, view.animation.ghostTicks * animationDefinition(pupils).fps / 60,
                { x: pos.x + eye.x, y: pos.y + eye.y }, scale, this.actorPool, this.actorLayer, this.actorCursor);
        }
    }

    private drawParticles(view: GameView): void {
        let used = 0;
        if (!view.playerReady && ['ready', 'playing', 'caught'].includes(view.state)) {
            for (const particle of view.particles) {
                used = this.drawAnimation(`bubble-${particle.animation + 1}-anim`, particle.frame, this.boardPosition(particle.position),
                    { x: particle.scale, y: particle.scale }, this.bubblePool, this.particleLayer, used);
            }
        }
        this.hideSprites(this.bubblePool, used);
    }

    private drawHud(view: GameView): void {
        let hudCount = 0;
        const formatScore = (value: number) => String(Math.min(9_999_999, Math.max(0, Math.trunc(value)))).padStart(7, ' ');
        for (let player = 0; player < view.scores.length; player++) {
            const x = player === 0 ? 0 : this.layoutWidth - 64;
            if (view.player !== player + 1 || view.tick % 30 < 15 || view.state === 'gameover') {
                hudCount = this.drawBitmapText(`${player + 1}UP`, x + (player === 0 ? 24 : 16), 0, 0xdedeff, this.hudTextPool, hudCount);
            }
            hudCount = this.drawBitmapText(view.scores[player] === 0 ? '     00' : formatScore(view.scores[player]), x, 8, 0xdedeff, this.hudTextPool, hudCount);
        }
        hudCount = this.drawBitmapText('HIGH SCORE', 72, 0, 0xdedeff, this.hudTextPool, hudCount);
        hudCount = this.drawBitmapText(view.highScore === 0 ? '     00' : formatScore(view.highScore), 80, 8, 0xdedeff, this.hudTextPool, hudCount);
        this.hideSprites(this.hudTextPool, hudCount);

        const bottom = this.boardTop + view.maze.height * TILE_PIXELS;
        let lifeCount = 0;
        for (let i = 0; i < Math.min(5, Math.max(0, view.lives - (view.playerReady ? 0 : 1))); i++) {
            lifeCount = this.drawAnimation(`${view.character}-char-anim`, 0,
                { x: 24 + i * TILE_PIXELS * 2, y: bottom + HUD_BOTTOM - 8 }, { x: 0.125 * 0.875, y: 0.125 * 0.875 },
                this.livesPool, this.hudLayer, lifeCount);
        }
        this.hideSprites(this.livesPool, lifeCount);

        let fruitCount = 0;
        const history = view.fruitHistory.slice(-8);
        for (let i = history.length - 1; i >= 0; i--) {
            const sprite = this.obtainSprite(this.fruitPool, this.hudLayer, fruitCount++);
            this.paintSprite(sprite, `char-sprites.fruit[${history[i]}]`,
                { x: this.layoutWidth - 24 - (history.length - 1 - i) * TILE_PIXELS * 2, y: bottom + HUD_BOTTOM - 8 },
                { x: 0.125 * 0.875, y: 0.125 * 0.875 });
        }
        this.hideSprites(this.fruitPool, fruitCount);

        let pointCount = 0;
        for (const point of view.state === 'playing' ? view.points : []) {
            const digits = String(Math.max(0, Math.trunc(point.value)));
            const glyphs = [...digits].map(digit => this.frame('small-font-sprites', digit));
            if (glyphs.some(texture => !texture)) continue;
            const glyphWidths = glyphs.map(texture => texture!.frame.width * 0.125);
            const totalWidth = glyphWidths.reduce((sum, width) => sum + width, 0) + Math.max(0, glyphWidths.length - 1);
            const maxHeight = Math.max(...glyphs.map(texture => texture!.frame.height * 0.125));
            let x = Math.floor(point.position.x - totalWidth / 2);
            const top = Math.floor(point.position.y + this.boardTop - maxHeight / 2);
            for (let i = 0; i < glyphs.length; i++) {
                const texture = glyphs[i]!;
                const width = glyphWidths[i];
                const height = texture.frame.height * 0.125;
                const sprite = this.obtainSprite(this.pointTextPool, this.hudLayer, pointCount++);
                sprite.anchor.set(0);
                sprite.texture = texture;
                sprite.position.set(x, top);
                sprite.width = width;
                sprite.height = height;
                sprite.tint = point.color;
                x += width + 1;
            }
        }
        this.hideSprites(this.pointTextPool, pointCount);
    }

    private drawOverlay(view: GameView, paused: boolean): void {
        let text = '';
        if (paused) text = 'PAUSED';
        else if (view.playerReady) text = 'PLAYER ONE';
        else if (view.state === 'ready') text = 'READY!';
        else if (view.state === 'gameover') text = 'GAME OVER';
        const ghostRow = view.maze.tiles.findIndex(row => row.some(tile => tile.kind === 'door')) - 1;
        const y = this.boardTop + (ghostRow + (view.playerReady ? 0 : 6)) * TILE_PIXELS;
        const tint = view.playerReady ? 0x00ffff : view.state === 'gameover' ? 0xff0000 : 0xffff00;
        let count = this.drawBitmapText(text, Math.max(0, (this.layoutWidth - text.length * 8) / 2), y, tint, this.overlayTextPool, 0, this.overlayLayer);
        if (view.playerReady && !paused) {
            const levelText = `LEVEL ${String(view.level).padStart(2, '0')}`;
            count = this.drawBitmapText(levelText, (this.layoutWidth - levelText.length * 8) / 2, y + 48, 0xffff00, this.overlayTextPool, count, this.overlayLayer);
        }
        this.hideSprites(this.overlayTextPool, count);
    }

    private drawBitmapText(text: string, x: number, y: number, tint: number, pool: RenderableSprite[], startIndex = 0, parent: Container = this.hudLayer): number {
        for (const char of text.toLowerCase()) {
            const sprite = this.obtainSprite(pool, parent, startIndex);
            const texture = this.frame('font-sprites', char);
            if (texture) {
                this.paintSprite(sprite, `font-sprites.${char}`, { x, y }, { x: 0.125, y: 0.125 });
                sprite.tint = tint;
                sprite.visible = true;
            } else {
                sprite.visible = false;
            }
            x += 8;
            startIndex++;
        }
        return startIndex;
    }

    private obtainSprite(pool: RenderableSprite[], parent: Container, index: number): RenderableSprite {
        let sprite = pool[index];
        if (!sprite) {
            sprite = createSprite(Texture.EMPTY);
            pool[index] = sprite;
            parent.addChild(sprite);
        }
        sprite.visible = true;
        return sprite;
    }

    private hideSprites(pool: RenderableSprite[], used: number): void {
        for (let i = used; i < pool.length; i++) pool[i].visible = false;
    }

    private boardPosition(position: Vec): Vec {
        return { x: position.x, y: position.y + this.boardTop };
    }
}

function fruitBounceOffset(position: Vec): number {
    const offsets = [2, 1.4142, 0, -1.4142, -2];
    const centerX = Math.round((position.x - TILE_PIXELS / 2) / TILE_PIXELS) * TILE_PIXELS + TILE_PIXELS / 2;
    const centerY = Math.round((position.y - TILE_PIXELS / 2) / TILE_PIXELS) * TILE_PIXELS + TILE_PIXELS / 2;
    const distance = position.x === centerX ? Math.abs(position.y - centerY) : position.y === centerY ? Math.abs(position.x - centerX) : undefined;
    return distance === undefined ? 0 : offsets[Math.trunc(distance) % offsets.length];
}

function directionVector(direction: string): Vec {
    switch (direction) {
        case 'up': return { x: 0, y: -1 };
        case 'down': return { x: 0, y: 1 };
        case 'left': return { x: -1, y: 0 };
        case 'right': return { x: 1, y: 0 };
        default: return { x: 0, y: 0 };
    }
}
