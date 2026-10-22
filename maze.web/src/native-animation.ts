import characterResources from '../../maze/assets/Assets.Anim.res.json';
import ghostResources from '../../maze/assets/Assets.GhostAnim.res.json';
import spriteResources from '../../maze/assets/Assets.Sprites.res.json';
import type { Vec } from './types';

export interface NativeSprite {
    name: string;
    file: string;
    rect: { x: number; y: number; width: number; height: number };
    handle: Vec;
    scale: Vec;
}
type Bounds = boolean | 'clamp';
type KeyValue = number | number[] | string | string[];
interface Key { frame: number; value: KeyValue }
interface Track { values: Key[]; start: number; length: number; loop: Bounds; spline: boolean; tension: number }
interface Visual { visual: string; position?: string; scale?: string; rotate?: string; color?: string }
export interface NativeAnimation { length: number; fps: number; loop: Bounds; keys: ReadonlyMap<string, Track>; visuals: readonly Visual[] }
export interface AnimationVisual { sprite: string; position: Vec; scale: Vec; rotation: number; color: readonly number[] }

function object(value: unknown, context: string): Record<string, unknown> {
    if (!value || typeof value !== 'object' || Array.isArray(value)) throw new Error(`Expected an object: ${context}`);
    return value as Record<string, unknown>;
}
function number(value: unknown, context: string): number {
    if (typeof value !== 'number' || !Number.isFinite(value)) throw new Error(`Expected a finite number: ${context}`);
    return value;
}
function string(value: unknown, context: string): string {
    if (typeof value !== 'string') throw new Error(`Expected a string: ${context}`);
    return value;
}
function vector(value: unknown, context: string): number[] {
    if (!Array.isArray(value) || !value.every(item => typeof item === 'number' && Number.isFinite(item))) throw new Error(`Expected numeric vector: ${context}`);
    return value;
}
function bounds(value: unknown): Bounds {
    if (value === undefined) return false;
    if (typeof value === 'boolean' || value === 'clamp') return value;
    throw new Error(`Unsupported animation loop mode: ${String(value)}`);
}

export const nativeSprites: ReadonlyMap<string, NativeSprite> = (() => {
    const result = new Map<string, NativeSprite>();
    for (const [groupName, value] of Object.entries(spriteResources)) {
        if (!value || typeof value !== 'object' || !('sprites' in value)) continue;
        for (const [name, raw] of Object.entries(object(value.sprites, groupName))) {
            const def = object(raw, name);
            const pos = vector(def.pos, name);
            const size = def.size ? vector(def.size, name) : [pos[2] - pos[0], pos[3] - pos[1]];
            const handle = def.handle ? vector(def.handle, name) : [0, 0];
            const scale = def.scale ? vector(def.scale, name) : [1, 1];
            const offset = def.offset ? vector(def.offset, name) : [size[0], 0];
            const count = number(def.repeat ?? 1, name);
            if (size.length !== 2 || !size.every(item => item > 0) || !Number.isInteger(count) || count < 1) throw new Error(`Invalid sprite dimensions: ${name}`);
            for (let index = 0; index < count; index++) {
                const spriteName = `${groupName}.${name}${count > 1 ? `[${index}]` : ''}`;
                const file = string(def.file, name);
                if (!file.startsWith('file:')) throw new Error(`Invalid sprite file reference: ${name}`);
                result.set(spriteName, { name: spriteName, file: file.slice(5),
                    rect: { x: pos[0] + index * offset[0], y: pos[1] + index * offset[1], width: size[0], height: size[1] },
                    handle: { x: handle[0], y: handle[1] }, scale: { x: scale[0], y: scale[1] } });
            }
        }
    }
    return result;
})();

function expand(value: unknown, variables: Record<string, unknown>, stack: readonly string[] = []): unknown {
    if (typeof value === 'string' && value.startsWith('res:')) {
        const name = value.slice(4);
        if (!Object.hasOwn(variables, name) || stack.includes(name)) throw new Error(`Missing or cyclic animation variable: ${name}`);
        return expand(variables[name], variables, [...stack, name]);
    }
    if (Array.isArray(value)) return value.map(item => expand(item, variables, stack));
    if (value && typeof value === 'object') {
        const def = object(value, 'animation resource');
        const scoped = { ...variables, ...(def['res:values'] ? object(def['res:values'], 'res:values') : {}) };
        if (def['res:template']) {
            const name = string(def['res:template'], 'res:template');
            const template = object(expand(`res:${name}`, scoped, stack), name);
            const overrides = Object.fromEntries(Object.entries(def).filter(([key]) => !key.startsWith('res:')));
            return { ...template, ...object(expand(overrides, scoped, stack), name) };
        }
        return Object.fromEntries(Object.entries(def).filter(([key]) => key !== 'res:values').map(([key, item]) => [key, expand(item, scoped, stack)]));
    }
    return value;
}

function compile(raw: unknown, name: string): NativeAnimation {
    const def = object(raw, name);
    const length = number(def.length, name);
    const fps = number(def.fps, name);
    if (length <= 0 || fps < 0 || !Array.isArray(def.visuals)) throw new Error(`Invalid animation: ${name}`);
    const keys = new Map<string, Track>();
    for (const [keyName, rawTrack] of Object.entries(object(def.keys, name))) {
        const track = object(rawTrack, keyName);
        if (!Array.isArray(track.values) || track.values.length === 0) throw new Error(`Empty animation track: ${name}.${keyName}`);
        const values = track.values.map(rawKey => {
            const key = object(rawKey, keyName);
            const value: unknown = key.value;
            if (!(typeof value === 'number' || typeof value === 'string' || (Array.isArray(value) && (value.every(item => typeof item === 'number') || value.every(item => typeof item === 'string'))))) {
                throw new Error(`Unsupported animation key: ${name}.${keyName}`);
            }
            return { frame: number(key.frame, keyName), value: value as KeyValue };
        }).sort((a, b) => a.frame - b.frame);
        if (values.some((key, index) => index > 0 && key.frame === values[index - 1].frame)) throw new Error(`Duplicate animation frame: ${name}.${keyName}`);
        for (const { value } of values) {
            const references = typeof value === 'string' ? [value] : Array.isArray(value) && typeof value[0] === 'string' ? value : [];
            for (const ref of references) {
                if (typeof ref !== 'string' || !ref.startsWith('ref:') || !nativeSprites.has(ref.slice(4))) throw new Error(`Missing animation sprite: ${String(ref)}`);
            }
        }
        if (track.method !== undefined && track.method !== 'linear' && track.method !== 'spline') throw new Error(`Unsupported key interpolation: ${keyName}`);
        keys.set(keyName, { values, start: number(track.start ?? values[0].frame, keyName), length: number(track.length ?? values[values.length - 1].frame, keyName),
            loop: bounds(track.loop), spline: track.method === 'spline', tension: (1 - number(track.tension ?? 0, keyName)) / 2 });
    }
    const visuals = def.visuals.map(rawVisual => {
        const value = object(rawVisual, name);
        const visual: Visual = { visual: string(value.visual, name) };
        for (const key of ['position', 'scale', 'rotate', 'color'] as const) {
            if (value[key] !== undefined) visual[key] = string(value[key], name);
        }
        for (const key of Object.values(visual)) if (!keys.has(key)) throw new Error(`Missing animation track: ${name}.${key}`);
        return visual;
    });
    return { length, fps, loop: bounds(def.loop), keys, visuals };
}

export const nativeAnimations: ReadonlyMap<string, NativeAnimation> = (() => {
    const result = new Map<string, NativeAnimation>();
    for (const resources of [characterResources, ghostResources]) {
        const scope = object(resources['res:values'], 'animation variables');
        for (const [name, value] of Object.entries(resources)) {
            if (name.startsWith('res:')) continue;
            result.set(name, compile(expand(value, scope), name));
        }
    }
    return result;
})();

export function animationDefinition(name: string): NativeAnimation {
    const animation = nativeAnimations.get(name);
    if (!animation) throw new Error(`Unknown native animation: ${name}`);
    return animation;
}

function adjustFrame(frame: number, start: number, length: number, loop: Bounds): number | null {
    if (frame >= start && frame < start + length) return frame;
    if (loop === 'clamp') return Math.max(start, Math.min(start + length, frame));
    if (loop) return length ? ((frame - start) % length + length) % length + start : start;
    return null;
}

function sampleTrack(track: Track, frame: number): KeyValue | null {
    const adjusted = adjustFrame(frame, track.start, track.length, track.loop);
    if (adjusted === null) return null;
    const keys = track.values;
    let index = keys.findIndex(key => key.frame >= adjusted);
    if (index < 0) return keys[keys.length - 1].value;
    if (index === 0 || keys[index].frame === adjusted) return keys[index].value;
    index--;
    const current = keys[index];
    const next = keys[index + 1];
    const t = (adjusted - current.frame) / (next.frame - current.frame);
    if (typeof current.value === 'string' || (Array.isArray(current.value) && typeof current.value[0] === 'string')) return current.value;
    const component = (value: KeyValue, i: number): number => typeof value === 'number' ? value : number(value[i], 'interpolated component');
    const interpolate = (i: number): number => {
        const a = component(current.value, i), b = component(next.value, i);
        if (!track.spline) return a + (b - a) * t;
        const before = keys[index ? index - 1 : keys.length - 1];
        const after = keys[index + 2 < keys.length ? index + 2 : 0];
        const tangentA = track.tension * (b - component(before.value, i));
        const tangentB = track.tension * (component(after.value, i) - a);
        return (2 * t ** 3 - 3 * t ** 2 + 1) * a + (-2 * t ** 3 + 3 * t ** 2) * b + (t ** 3 - 2 * t ** 2 + t) * tangentA + (t ** 3 - t ** 2) * tangentB;
    };
    return typeof current.value === 'number' ? interpolate(0) : current.value.map((_, i) => interpolate(i));
}

export function sampleAnimation(name: string, frame: number): AnimationVisual[] {
    const animation = animationDefinition(name);
    const adjusted = adjustFrame(frame, 0, animation.length, animation.loop);
    if (adjusted === null) return [];
    const sample = (key: string | undefined): KeyValue | null => {
        if (!key) return null;
        const track = animation.keys.get(key);
        if (!track) throw new Error(`Missing animation track: ${name}.${key}`);
        return sampleTrack(track, adjusted);
    };
    const result: AnimationVisual[] = [];
    for (const visual of animation.visuals) {
        const references = sample(visual.visual);
        if (!references) continue;
        const position = sample(visual.position), scale = sample(visual.scale), rotate = sample(visual.rotate), color = sample(visual.color);
        const p = position ? vector(position, name) : [0, 0];
        const s = scale ? vector(scale, name) : [1, 1];
        const c = color ? vector(color, name) : [1, 1, 1, 1];
        for (const ref of typeof references === 'string' ? [references] : Array.isArray(references) ? references : []) {
            result.push({ sprite: string(ref, name).slice(4), position: { x: p[0], y: p[1] }, scale: { x: s[0], y: s[1] }, rotation: rotate === null ? 0 : number(rotate, name), color: c });
        }
    }
    return result;
}
