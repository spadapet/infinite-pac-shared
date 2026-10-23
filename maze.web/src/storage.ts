import type { Character, Difficulty, GameOptions } from './types';

export interface Settings extends GameOptions { sound: boolean; vibration: boolean }
export interface HighScore { name: string; score: number; level: number; date: string }
interface SavedData { version: 1; settings: Settings; scores: Record<string, HighScore[]> }
export interface StorageLike { getItem(key: string): string | null; setItem(key: string, value: string): void }
export const defaultSettings: Settings = { character: 'mr', mazeSet: 'mr', difficulty: 'normal', players: 1, sound: true, vibration: true };
const key = 'infinite-pac-web.v1';
const character = (value: unknown): value is Character => value === 'mr' || value === 'ms';
const difficulty = (value: unknown): value is Difficulty => value === 'easy' || value === 'normal' || value === 'hard';
const record = (value: unknown): value is Record<string, unknown> => typeof value === 'object' && value !== null && !Array.isArray(value);
const integer = (value: unknown): value is number => typeof value === 'number' && Number.isSafeInteger(value) && value >= 0;
export const scoreName = (name: string): string => name.toLowerCase().replace(/[^a-z .]/g, '').slice(0, 9);

export class GameStorage {
    private data: SavedData = { version: 1, settings: { ...defaultSettings }, scores: {} };

    constructor(private readonly storage: StorageLike, private readonly onError: (message: string) => void) {
        try {
            const text = storage.getItem(key);
            if (text === null) return;
            const saved: unknown = JSON.parse(text);
            if (!record(saved) || saved.version !== 1 || !record(saved.settings) || !record(saved.scores)) {
                throw new Error('Unsupported save format');
            }
            const options = saved.settings;
            if (!character(options.character) || !character(options.mazeSet) || !difficulty(options.difficulty)
                || (options.players !== 1 && options.players !== 2) || typeof options.sound !== 'boolean' || typeof options.vibration !== 'boolean') {
                throw new Error('Invalid saved options');
            }
            const scores: Record<string, HighScore[]> = {};
            for (const [category, entries] of Object.entries(saved.scores)) {
                if (!Array.isArray(entries) || entries.some((entry: unknown) => !record(entry)
                    || typeof entry.name !== 'string' || entry.name.length > 16 || !integer(entry.score)
                    || !integer(entry.level) || typeof entry.date !== 'string')) {
                    throw new Error('Invalid saved high scores');
                }
                scores[category] = entries.filter((entry: HighScore) => entry.score > 0)
                    .map((entry: HighScore) => ({ ...entry, name: scoreName(entry.name) }))
                    .sort((a, b) => b.score - a.score).slice(0, 10);
            }
            this.data = { version: 1, settings: { character: options.character, mazeSet: options.mazeSet, difficulty: options.difficulty,
                players: options.players, sound: options.sound, vibration: options.vibration }, scores };
        } catch (error) {
            this.onError(`Saved data could not be loaded. Defaults are in use. ${error instanceof Error ? error.message : String(error)}`);
        }
    }

    get settings(): Settings { return { ...this.data.settings }; }

    saveSettings(settings: Settings): void {
        this.data.settings = { ...settings };
        this.save();
    }

    category(options: GameOptions): string { return `${options.mazeSet}-${options.difficulty}`; }

    scores(options: GameOptions): HighScore[] { return (this.data.scores[this.category(options)] ?? []).map(entry => ({ ...entry })); }

    qualifies(options: GameOptions, score: number): boolean {
        const scores = this.scores(options);
        return integer(score) && score > 0 && (scores.length < 10 || score > scores[scores.length - 1].score);
    }

    addScore(options: GameOptions, entry: HighScore): void {
        if (!integer(entry.score) || !integer(entry.level) || typeof entry.name !== 'string' || typeof entry.date !== 'string') {
            throw new Error('Invalid high score');
        }
        if (!this.qualifies(options, entry.score)) return;
        const scores = this.scores(options);
        scores.push({ ...entry, name: scoreName(entry.name) });
        this.data.scores[this.category(options)] = scores.sort((a, b) => b.score - a.score).slice(0, 10);
        this.save();
    }

    private save(): void {
        try {
            this.storage.setItem(key, JSON.stringify(this.data));
        } catch (error) {
            this.onError(`Progress could not be saved on this device. ${error instanceof Error ? error.message : String(error)}`);
        }
    }
}
