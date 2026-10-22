export interface Vec { x: number; y: number }
export type Direction = 'up' | 'left' | 'down' | 'right';
export type Character = 'mr' | 'ms';
export type Difficulty = 'easy' | 'normal' | 'hard';
export interface GameOptions {
    character: Character;
    mazeSet: Character;
    difficulty: Difficulty;
    players: number;
}
export type TileKind = 'wall' | 'ghostWall' | 'door' | 'dot' | 'power' | 'path' | 'void';
export interface Tile { kind: TileKind; zone: string }
export interface MazeView {
    id: string;
    width: number;
    height: number;
    tiles: Tile[][];
    borderColor: number;
    fillColor: number;
    backgroundColor: number;
}
export type GameState = 'ready' | 'playing' | 'caught' | 'dying' | 'dead' | 'winning' | 'gameover';
export type GhostState = 'chase' | 'scatter' | 'scared' | 'flash' | 'eyes';
export interface PacView { position: Vec; direction: Direction; powered: boolean; moving: boolean; visible: boolean }
export interface GhostView { index: number; position: Vec; direction: Direction; eyeDirection: Direction; state: GhostState }
export interface FruitView { position: Vec; type: number; moving: boolean }
export interface PointView { position: Vec; value: number; remaining: number; color: number }
export type BubbleAnimation = 0 | 1 | 2;
export interface ParticleView { position: Vec; animation: BubbleAnimation; frame: number; scale: number }
export interface GameView {
    state: GameState;
    tick: number;
    stateTick: number;
    playerReady: boolean;
    animation: { pacFrame: number; deathFrame: number; ghostTicks: number; dotTicks: number; totalTicks: number };
    level: number;
    score: number;
    scores: number[];
    highScore: number;
    lives: number;
    player: number;
    playerCount: number;
    character: Character;
    maze: MazeView;
    pac: PacView;
    ghosts: GhostView[];
    fruit: FruitView | null;
    points: PointView[];
    particles: ParticleView[];
    fruitHistory: number[];
}
export type GameEvent =
    | { type: 'sound'; name: string }
    | { type: 'stopSound'; name: string }
    | { type: 'stopSounds' }
    | { type: 'gameOver'; player: number; score: number; level: number }
    | { type: 'vibrate'; duration: number };
