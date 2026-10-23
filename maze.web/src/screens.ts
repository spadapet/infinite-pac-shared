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

    view(settings: Settings, scores: HighScore[], fullscreen: boolean): NativeScreenView {
        const texts: ScreenText[] = titleLabels(settings, fullscreen).flatMap((text, i) => [
            { text, x: 56, y: titleRows[i] - 4, color: 0xffffff },
            ...this.hoverOpacity[i] > 0 ? [{ text, x: 56, y: titleRows[i] - 4, color: 0x597ccf, alpha: this.hoverOpacity[i] }] : [],
        ]);
        texts.push({ text: ' START GAME:', x: 48, y: 16, color: 0xbf0000 });
        if (this.selected < 2) {
            texts.push({ text: 'HIGH SCORES:', x: 48, y: 140, color: 0xbf0000 });
            for (let i = 0; i < 10; i++) texts.push({
                text: `${formatScore(scores[i]?.score ?? 0)}  ${scores[i]?.name || '---'}`, x: 48, y: 152 + i * 8, color: 0xffffff,
            });
        }
        const character = this.selected === 1 ? 'ms' : 'mr';
        return { kind: 'title', maze: this.maze, background: this.background.readView(this.backgroundView), texts,
            pac: { x: 44, y: titleRows[this.selected], character, frame: Math.max(0, this.ticks - 1) * animationDefinition(`${character}-char-anim`).fps / 60 }, fade: this.fade };
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

    constructor(readonly score: number, readonly slot: number) {}

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
        const index = key === 'Backspace' ? 28 : [...'abcdefghijklmnopqrstuvwxyz .'].indexOf(key.toLowerCase());
        if (index < 0) return false;
        this.selected = index;
        this.execute();
        this.selected = 29;
        return true;
    }

    view(background: GameView): NativeScreenView {
        const texts: ScreenText[] = scoreLetters.map(letter => ({ ...letter, color: 0xffffff }));
        const letter = scoreLetters[this.selected];
        texts.push({ text: letter.text, x: letter.x - 1, y: letter.y - 1, color: 0xff00ff });
        texts.push({ text: `      ${formatScore(this.score)}`, x: 32, y: 56, color: 0xffffff },
            { text: ` New high score ${String(this.slot + 1).padStart(2, '0')}!`, x: 32, y: 88, color: 0xffffff },
            { text: '    Enter name:', x: 32, y: 104, color: 0xffffff },
            { text: this.name, x: 72, y: 144, color: 0xffffff });
        const blink = (Math.max(0, this.ticks - 1) % 60) < 30 && this.selected < 28;
        const dashes = blink ? 8 : 9;
        if (this.name.length < dashes) texts.push({ text: '-'.repeat(dashes - this.name.length),
            x: (9 + 9 - dashes + this.name.length) * 8, y: 144, color: 0xffffff });
        if (this.name.length < 9 && blink) texts.push({ text: letter.text, x: 72 + this.name.length * 8, y: 144, color: 0xff00ff });
        return { kind: 'score', maze: this.maze, background, texts,
            pac: { x: letter.x + 4, y: letter.y + 4, character: 'mr', frame: Math.max(0, this.ticks - 1) * animationDefinition('mr-char-anim').fps / 60 }, fade: 0 };
    }
}
