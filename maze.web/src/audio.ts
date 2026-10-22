import definitions from '../../maze/assets/Assets.Audio.res.json';
import type { GameEvent } from './types';

interface Effect { file: string; volume: number; speed: number; loop: boolean }
const isRecord = (value: unknown): value is Record<string, unknown> => typeof value === 'object' && value !== null;

function effects(): Map<string, Effect> {
    const resources: Record<string, unknown> = definitions;
    const result = new Map<string, Effect>();
    for (const [name, resource] of Object.entries(resources)) {
        if (!isRecord(resource) || resource['res:type'] !== 'effect') continue;
        if (typeof resource.file !== 'string' || !resource.file.startsWith('ref:')) throw new Error(`Invalid audio reference: ${name}`);
        const fileResource = resources[resource.file.slice(4)];
        if (!isRecord(fileResource) || typeof fileResource.file !== 'string' || !fileResource.file.startsWith('file:')) {
            throw new Error(`Missing audio file: ${name}`);
        }
        result.set(name, { file: fileResource.file.slice(5), volume: typeof resource.volume === 'number' ? resource.volume : 1,
            speed: typeof resource.speed === 'number' ? resource.speed : 1, loop: resource.loop_count === -1 });
    }
    return result;
}

export class GameAudio {
    private context?: AudioContext;
    private master?: GainNode;
    private loading?: Promise<void>;
    private readonly effects = effects();
    private readonly buffers = new Map<string, AudioBuffer>();
    private readonly active = new Map<string, AudioBufferSourceNode>();
    private muted = false;
    private paused = false;
    private desiredBackground?: string;
    private playingBackground?: string;

    async unlock(): Promise<void> {
        // AudioContext creation/resume must occur synchronously inside a user gesture.
        if (!this.context) {
            this.context = new AudioContext({ latencyHint: 'interactive' });
            this.master = this.context.createGain();
            this.master.gain.value = this.muted ? 0 : 1;
            this.master.connect(this.context.destination);
        }
        const resumed = this.context.resume();
        if (!this.loading) {
            this.loading = this.load().catch((error: unknown) => { this.loading = undefined; throw error; });
        }
        await Promise.all([resumed, this.loading]);
        if (this.paused) await this.context.suspend();
        else if (this.desiredBackground) this.playBackground(this.desiredBackground);
    }

    setMuted(value: boolean): void {
        this.muted = value;
        if (this.master && this.context) this.master.gain.setValueAtTime(value ? 0 : 1, this.context.currentTime);
    }

    async setPaused(value: boolean): Promise<void> {
        this.paused = value;
        if (!this.context) return;
        if (value) await this.context.suspend();
        else {
            await this.context.resume();
            if (this.desiredBackground) this.playBackground(this.desiredBackground);
        }
    }

    handle(event: GameEvent): void {
        if (event.type === 'sound') {
            const effect = this.effects.get(event.name);
            if (!effect) throw new Error(`Unknown sound effect: ${event.name}`);
            if (effect.loop) {
                this.desiredBackground = event.name;
                this.playBackground(event.name);
                return;
            }
            if (!this.context || (this.muted && !this.buffers.has(effect.file))) return;
            if (!this.buffers.has(effect.file)) throw new Error(`Sound requested before decoding completed: ${event.name}`);
            this.play(event.name);
        } else if (event.type === 'stopSound') {
            this.stop(event.name);
            if (this.desiredBackground === event.name) this.desiredBackground = undefined;
            if (this.playingBackground === event.name) this.playingBackground = undefined;
        } else if (event.type === 'stopSounds') this.stopAll();
    }

    stopAll(): void {
        for (const name of this.active.keys()) this.stop(name);
        this.desiredBackground = undefined;
        this.playingBackground = undefined;
    }

    private async load(): Promise<void> {
        const context = this.context;
        if (!context) throw new Error('Audio has not been initialized');
        await Promise.all([...new Set([...this.effects.values()].map(effect => effect.file))].map(async file => {
            const response = await fetch(`${import.meta.env?.BASE_URL ?? './'}assets/${file}`);
            if (!response.ok) throw new Error(`Could not load sound ${file}: HTTP ${response.status}`);
            this.buffers.set(file, await context.decodeAudioData(await response.arrayBuffer()));
        }));
    }

    private playBackground(name: string): void {
        const effect = this.effects.get(name);
        if (!this.context || !effect || !this.buffers.has(effect.file)) return;
        if (this.playingBackground && this.playingBackground !== name) this.stop(this.playingBackground);
        this.playingBackground = name;
        this.play(name);
    }

    private play(name: string): void {
        const context = this.context;
        const effect = this.effects.get(name);
        if (!context || !this.master || !effect) return;
        const buffer = this.buffers.get(effect.file);
        if (!buffer) return;
        if (effect.loop && this.active.has(name)) return;
        this.stop(name);
        const source = context.createBufferSource();
        source.buffer = buffer;
        source.loop = effect.loop;
        source.playbackRate.value = effect.speed;
        const gain = context.createGain();
        gain.gain.value = effect.volume;
        source.connect(gain);
        gain.connect(this.master);
        source.onended = () => {
            if (this.active.get(name) === source) this.active.delete(name);
            source.disconnect();
            gain.disconnect();
        };
        this.active.set(name, source);
        source.start();
    }

    private stop(name: string): void {
        const source = this.active.get(name);
        if (source) { source.stop(); this.active.delete(name); }
    }
}
