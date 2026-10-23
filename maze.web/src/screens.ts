import { Game, parseMazeResource } from './game';
import { animationDefinition } from './native-animation';
import type { HighScore, Settings } from './storage';
import type { Direction, GameView, MazeView, Vec } from './types';

export interface ScreenText {
    text: string;
    x: number;
    y: number;
    color: number;
    alpha?: number;
}

export interface NativeScreenView {
    kind: 'title' | 'score';
    maze: MazeView;
    background?: GameView;
    backgroundFade?: number;
    showForeground?: boolean;
    texts: ScreenText[];
    pac: { x: number; y: number; character: 'mr' | 'ms'; frame: number };
    fade: number;
}

export const formatScore = (score: number): string => score === 0 ? '     00' : String(Math.min(9999999, Math.trunc(score))).padStart(7, ' ');
export const titleRows = [32, 44, 68, 80, 92, 104, 116, 128];
export const titleLabels = (settings: Settings, fullscreen: boolean): string[] => [
    'MR.PAC', 'MS.LILA', `PLAYERS:${settings.players === 1 ? 'ONE' : 'TWO'}`,
    `DIFFICULTY:${settings.difficulty.toUpperCase()}`, `SOUND:${settings.sound ? 'ON' : 'OFF'}`,
    `VIBRATE:${settings.vibration ? 'ON' : 'OFF'}`, `FULL SCREEN:${fullscreen ? 'ON' : 'OFF'}`, 'ABOUT',
];

export class TitleScreen {
    readonly maze = parseMazeResource('title-maze-front');
    readonly background = Game.titleBackground();
    private readonly backgroundView = this.background.view;
    private readonly textPool: ScreenText[] = [];
    private readonly presentation: NativeScreenView = { kind: 'title', maze: this.maze, texts: [],
        pac: { x: 44, y: 32, character: 'mr', frame: 0 }, fade: 1 };
    private labelSettings?: Settings;
    private fullscreen = false;
    private labels: string[] = [];
    private readonly scoreValues: number[] = [];
    private readonly scoreNames: string[] = [];
    private readonly scoreLines: string[] = [];
    selected: number;
    hover = -1;
    readonly hoverOpacity = titleRows.map(() => 0);
    ticks = 0;
    fade = 1;
    done = false;

    constructor(settings: Settings) { this.selected = settings.mazeSet === 'ms' ? 1 : 0; }

    advance(): void {
        this.background.advance();
        this.background.drainEvents();
        this.ticks++;
        this.fade = Math.max(0, this.fade - 1 / 64);
        for (let i = 0; i < this.hoverOpacity.length; i++) {
            this.hoverOpacity[i] = i === this.hover ? 1 : Math.max(0, this.hoverOpacity[i] - 1 / 24);
        }
    }

    move(direction: Direction): boolean {
        const previous = this.selected;
        if (direction === 'up') this.selected = Math.max(0, this.selected - 1);
        if (direction === 'down') this.selected = Math.min(titleRows.length - 1, this.selected + 1);
        if (previous !== this.selected) { this.hoverOpacity.fill(0); this.hover = -1; }
        return previous !== this.selected;
    }

    hit(position: Vec, labels: string[]): number {
        return labels.findIndex((label, index) => position.x >= 54 && position.x < 58 + label.length * 8
            && position.y >= titleRows[index] - 6 && position.y < titleRows[index] + 6);
    }

    menuLabels(settings: Settings, fullscreen: boolean): string[] {
        const previous = this.labelSettings;
        if (!previous || previous.players !== settings.players || previous.difficulty !== settings.difficulty
            || previous.sound !== settings.sound || previous.vibration !== settings.vibration || this.fullscreen !== fullscreen) {
            this.labels = titleLabels(settings, fullscreen);
            this.labelSettings = { ...settings };
            this.fullscreen = fullscreen;
        }
        return this.labels;
    }

    view(settings: Settings, scores: HighScore[], fullscreen: boolean): NativeScreenView {
        const screen = this.presentation;
        const labels = this.menuLabels(settings, fullscreen);
        for (let i = 0; i < 10; i++) {
            const score = scores[i]?.score ?? 0, name = scores[i]?.name || '---';
            if (this.scoreValues[i] !== score || this.scoreNames[i] !== name) {
                this.scoreValues[i] = score;
                this.scoreNames[i] = name;
                this.scoreLines[i] = `${formatScore(score)}  ${name}`;
            }
        }
        let used = 0;
        for (let i = 0; i < labels.length; i++) {
            writeText(screen.texts, this.textPool, used++, labels[i], 56, titleRows[i] - 4, 0xffffff);
            if (this.hoverOpacity[i] > 0) writeText(screen.texts, this.textPool, used++, labels[i], 56, titleRows[i] - 4, 0x597ccf, this.hoverOpacity[i]);
        }
        writeText(screen.texts, this.textPool, used++, ' START GAME:', 48, 16, 0xbf0000);
        if (this.selected < 2) {
            writeText(screen.texts, this.textPool, used++, 'HIGH SCORES:', 48, 140, 0xbf0000);
            for (let i = 0; i < 10; i++) writeText(screen.texts, this.textPool, used++, this.scoreLines[i], 48, 152 + i * 8, 0xffffff);
        }
        screen.texts.length = used;
        screen.background = this.background.readView(this.backgroundView);
        screen.pac.y = titleRows[this.selected];
        screen.pac.character = this.selected === 1 ? 'ms' : 'mr';
        screen.pac.frame = Math.max(0, this.ticks - 1) * animationDefinition(`${screen.pac.character}-char-anim`).fps / 60;
        screen.fade = this.fade;
        return screen;
    }
}

export const scoreLetters = [...'abcdefghijklmnopqrstuvwxyz', ' ', '.', 'Back', 'End'].map((text, index) => ({
    text, x: index < 28 ? 60 + index % 7 * 16 : index === 28 ? 60 : 140,
    y: index < 28 ? 184 + Math.floor(index / 7) * 16 : 248,
}));

export class ScoreScreen {
    readonly maze = parseMazeResource('high-score-maze');
    selected = 0;
    name = '';
    ticks = 0;
    private readonly textPool: ScreenText[] = [];
    private readonly presentation: NativeScreenView = { kind: 'score', maze: this.maze, texts: [],
        pac: { x: 0, y: 0, character: 'mr', frame: 0 }, fade: 0 };
    private readonly scoreText: string;
    private readonly slotText: string;

    constructor(readonly score: number, readonly slot: number) {
        this.scoreText = `      ${formatScore(score)}`;
        this.slotText = ` New high score ${String(slot + 1).padStart(2, '0')}!`;
    }

    advance(): void { this.ticks++; }

    hit(pos: Vec): number {
        return scoreLetters.findIndex(letter => pos.x >= letter.x - 4 && pos.x < letter.x + (letter.text.length - 1) * 8 + 12
            && pos.y >= letter.y - 4 && pos.y < letter.y + 12);
    }

    move(direction: Direction): void {
        const pos = scoreLetters[this.selected];
        const dx = direction === 'left' ? -16 : direction === 'right' ? 16 : 0;
        const dy = direction === 'up' ? -16 : direction === 'down' ? 16 : 0;
        let distance = Infinity;
        let next = this.selected;
        scoreLetters.forEach((letter, index) => {
            if ((!dy && letter.y !== pos.y) || (!dx && letter.x !== pos.x)
                || (dx > 0 && letter.x <= pos.x) || (dx < 0 && letter.x >= pos.x)
                || (dy > 0 && letter.y <= pos.y) || (dy < 0 && letter.y >= pos.y)) return;
            const candidate = (letter.x - pos.x - dx) ** 2 + (letter.y - pos.y - dy) ** 2;
            if (candidate < distance) { distance = candidate; next = index; }
        });
        if (next === this.selected && dy > 0) next = this.selected <= 22 ? 28 : 29;
        this.selected = next;
    }

    execute(): boolean {
        if (this.selected === 29) return true;
        if (this.selected === 28) this.name = this.name.slice(0, -1);
        else if (this.name.length < 9) this.name += scoreLetters[this.selected].text;
        return false;
    }

    type(key: string): boolean {
        const index = key === 'Backspace' ? 28 : 'abcdefghijklmnopqrstuvwxyz .'.indexOf(key.toLowerCase());
        if (index < 0) return false;
        this.selected = index;
        this.execute();
        this.selected = 29;
        return true;
    }

    view(background: GameView): NativeScreenView {
        const screen = this.presentation;
        const texts = screen.texts;
        let used = 0;
        for (const entry of scoreLetters) writeText(texts, this.textPool, used++, entry.text, entry.x, entry.y, 0xffffff);
        const letter = scoreLetters[this.selected];
        writeText(texts, this.textPool, used++, letter.text, letter.x - 1, letter.y - 1, 0xff00ff);
        writeText(texts, this.textPool, used++, this.scoreText, 32, 56, 0xffffff);
        writeText(texts, this.textPool, used++, this.slotText, 32, 88, 0xffffff);
        writeText(texts, this.textPool, used++, '    Enter name:', 32, 104, 0xffffff);
        writeText(texts, this.textPool, used++, this.name, 72, 144, 0xffffff);
        const blink = (Math.max(0, this.ticks - 1) % 60) < 30 && this.selected < 28;
        const dashes = blink ? 8 : 9;
        if (this.name.length < dashes) writeText(texts, this.textPool, used++, DASHES[dashes - this.name.length],
            (9 + 9 - dashes + this.name.length) * 8, 144, 0xffffff);
        if (this.name.length < 9 && blink) writeText(texts, this.textPool, used++, letter.text, 72 + this.name.length * 8, 144, 0xff00ff);
        texts.length = used;
        screen.background = background;
        screen.pac.x = letter.x + 4;
        screen.pac.y = letter.y + 4;
        screen.pac.frame = Math.max(0, this.ticks - 1) * animationDefinition('mr-char-anim').fps / 60;
        return screen;
    }
}

const DASHES = Array.from({ length: 10 }, (_, length) => '-'.repeat(length));

function writeText(texts: ScreenText[], pool: ScreenText[], index: number, text: string, x: number, y: number, color: number, alpha?: number): void {
    const entry = pool[index] ?? (pool[index] = { text, x, y, color });
    entry.text = text;
    entry.x = x;
    entry.y = y;
    entry.color = color;
    if (alpha === undefined) delete entry.alpha;
    else entry.alpha = alpha;
    texts[index] = entry;
}
