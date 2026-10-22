import type { Direction, Vec } from './types';

export interface InputActions {
    active(): boolean;
    move(direction: Direction | Vec | null): void;
    action(): void;
    pause(): void;
    mute(): void;
    fullscreen(): void;
    point(clientX: number, clientY: number): Vec | null;
    pac(): Vec;
}
const keys: Record<string, Direction> = { ArrowUp: 'up', ArrowLeft: 'left', ArrowDown: 'down', ArrowRight: 'right',
    w: 'up', a: 'left', s: 'down', d: 'right' };

export class InputController {
    private pointer?: { id: number; x: number; y: number };
    private readonly buttons = new Map<number, { start: boolean; action: boolean }>();
    private readonly heldKeys = new Map<string, Direction>();
    private gamepadMoving = false;

    constructor(host: HTMLElement, private readonly actions: InputActions) {
        window.addEventListener('keydown', event => {
            const target = event.target;
            if (target instanceof HTMLElement && target.closest('input, select, textarea')) return;
            const key = event.key.length === 1 ? event.key.toLowerCase() : event.key;
            const direction = keys[key];
            if (direction && actions.active()) {
                event.preventDefault();
                this.heldKeys.set(event.code || key, direction);
                this.moveKeyboard();
            }
            else if (!event.repeat && (key === 'p' || key === 'Escape')) { event.preventDefault(); actions.pause(); }
            else if (!event.repeat && key === 'm') actions.mute();
            else if (!event.repeat && key === 'f') actions.fullscreen();
            else if (!event.repeat && (key === 'Enter' || key === ' ') && !(target instanceof HTMLButtonElement)) {
                event.preventDefault(); actions.action();
            }
        });
        window.addEventListener('keyup', event => {
            const key = event.key.length === 1 ? event.key.toLowerCase() : event.key;
            if (this.heldKeys.delete(event.code || key)) {
                if (actions.active()) this.moveKeyboard();
                else actions.move(null);
            }
        });
        const clearHeld = () => {
            const held = this.heldKeys.size > 0 || this.gamepadMoving;
            this.heldKeys.clear();
            this.gamepadMoving = false;
            this.pointer = undefined;
            if (held) actions.move(null);
        };
        window.addEventListener('blur', clearHeld);
        document.addEventListener('visibilitychange', () => { if (document.hidden) clearHeld(); });
        host.addEventListener('pointerdown', event => {
            if (!actions.active() || event.button !== 0) return;
            event.preventDefault();
            host.focus({ preventScroll: true });
            host.setPointerCapture(event.pointerId);
            this.pointer = { id: event.pointerId, x: event.clientX, y: event.clientY };
            const point = actions.point(event.clientX, event.clientY);
            if (point) {
                const pac = actions.pac();
                this.moveByDelta(point.x - pac.x, point.y - pac.y);
            }
        });
        host.addEventListener('pointermove', event => {
            if (!this.pointer || this.pointer.id !== event.pointerId || !actions.active()) return;
            const dx = event.clientX - this.pointer.x;
            const dy = event.clientY - this.pointer.y;
            if (Math.max(Math.abs(dx), Math.abs(dy)) >= 10) {
                this.moveByDelta(dx, dy);
                this.pointer.x = event.clientX;
                this.pointer.y = event.clientY;
            }
        });
        const endPointer = (event: PointerEvent) => { if (this.pointer?.id === event.pointerId) this.pointer = undefined; };
        host.addEventListener('pointerup', endPointer);
        host.addEventListener('pointercancel', endPointer);
        host.addEventListener('lostpointercapture', endPointer);
        document.querySelectorAll<HTMLButtonElement>('[data-direction]').forEach(button => {
            button.addEventListener('pointerdown', event => {
                event.preventDefault();
                const direction = button.dataset.direction;
                if (actions.active() && (direction === 'up' || direction === 'left' || direction === 'down' || direction === 'right')) {
                    actions.move(direction);
                }
            });
            button.addEventListener('click', () => {
                const direction = button.dataset.direction;
                if (actions.active() && (direction === 'up' || direction === 'left' || direction === 'down' || direction === 'right')) actions.move(direction);
            });
        });
    }

    private moveKeyboard(): void {
        const held = new Set(this.heldKeys.values());
        const x = held.has('left') ? -1 : held.has('right') ? 1 : 0;
        const y = held.has('up') ? -1 : held.has('down') ? 1 : 0;
        this.actions.move(x || y ? { x, y } : null);
    }

    poll(): void {
        if (document.hidden) return;
        if (this.heldKeys.size && this.actions.active()) this.moveKeyboard();
        if (!navigator.getGamepads) return;
        let direction: Direction | undefined;
        const connected = new Set<number>();
        for (const pad of navigator.getGamepads()) {
            if (!pad || !pad.connected) continue;
            connected.add(pad.index);
            if (pad.mapping !== 'standard') continue;
            const previous = this.buttons.get(pad.index);
            const start = pad.buttons[9]?.pressed ?? false;
            const action = pad.buttons[0]?.pressed ?? false;
            if (start && !previous?.start) this.actions.pause();
            if (action && !previous?.action) this.actions.action();
            this.buttons.set(pad.index, { start, action });
            const x = pad.axes[0] ?? 0;
            const y = pad.axes[1] ?? 0;
            if (pad.buttons[12]?.pressed) direction = 'up';
            else if (pad.buttons[13]?.pressed) direction = 'down';
            else if (pad.buttons[14]?.pressed) direction = 'left';
            else if (pad.buttons[15]?.pressed) direction = 'right';
            else if (Math.max(Math.abs(x), Math.abs(y)) > 0.35) direction = Math.abs(x) > Math.abs(y) ? (x < 0 ? 'left' : 'right') : (y < 0 ? 'up' : 'down');
        }
        for (const index of this.buttons.keys()) if (!connected.has(index)) this.buttons.delete(index);
        if (!this.heldKeys.size) {
            if (direction && this.actions.active()) this.actions.move(direction);
            else if (!direction && this.gamepadMoving) this.actions.move(null);
        }
        this.gamepadMoving = direction !== undefined;
    }

    private moveByDelta(x: number, y: number): void {
        if (Math.max(Math.abs(x), Math.abs(y)) < 1) return;
        this.actions.move(Math.abs(x) > Math.abs(y) ? (x < 0 ? 'left' : 'right') : (y < 0 ? 'up' : 'down'));
    }
}
