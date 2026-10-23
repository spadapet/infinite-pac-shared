import type { Direction, JoystickView, Vec } from './types';

export interface InputActions {
    active(): boolean;
    move(direction: Direction | Vec | null): void;
    action(): void;
    pause(): void;
    mute(): void;
    fullscreen(): void;
    direction(): Direction;
}
const keys: Record<string, Direction> = { ArrowUp: 'up', ArrowLeft: 'left', ArrowDown: 'down', ArrowRight: 'right',
    w: 'up', a: 'left', s: 'down', d: 'right' };

export class InputController {
    private pointer?: { id: number; origin: Vec; position: Vec };
    private touchPress: Vec | null = null;
    private readonly buttons = new Map<number, { start: boolean; action: boolean; directions: Set<Direction> }>();
    private readonly heldKeys = new Map<string, Direction>();
    private readonly heldGamepad = new Set<Direction>();
    private gamepadMoving = false;

    constructor(private readonly host: Pick<HTMLElement, 'addEventListener' | 'focus' | 'setPointerCapture' | 'hasPointerCapture' | 'releasePointerCapture'>, private readonly actions: InputActions) {
        window.addEventListener('keydown', event => {
            const target = event.target;
            if (target instanceof HTMLElement && target.closest('input, select, textarea')) return;
            const key = event.key.length === 1 ? event.key.toLowerCase() : event.key;
            const direction = keys[key];
            if (direction && actions.active()) {
                event.preventDefault();
                this.heldKeys.set(event.code || key, direction);
                this.advance();
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
                if (actions.active() && (this.heldKeys.size || this.heldGamepad.size || this.pointer || this.touchPress)) this.advance();
                else actions.move(null);
            }
        });
        const clearHeld = () => {
            const held = this.heldKeys.size > 0 || this.gamepadMoving || !!this.pointer || !!this.touchPress;
            this.heldKeys.clear();
            this.heldGamepad.clear();
            for (const button of this.buttons.values()) button.directions.clear();
            this.gamepadMoving = false;
            this.cancelPointer();
            if (held) actions.move(null);
        };
        window.addEventListener('blur', clearHeld);
        document.addEventListener('visibilitychange', () => { if (document.hidden) clearHeld(); });
        host.addEventListener('pointerdown', event => {
            if (!actions.active() || event.button !== 0 || this.pointer) return;
            event.preventDefault();
            host.focus({ preventScroll: true });
            this.pointer = { id: event.pointerId, origin: { x: event.clientX, y: event.clientY }, position: { x: event.clientX, y: event.clientY } };
            host.setPointerCapture(event.pointerId);
            this.advance();
        });
        host.addEventListener('pointermove', event => {
            if (!this.pointer || this.pointer.id !== event.pointerId || !actions.active()) return;
            event.preventDefault();
            this.pointer.position = { x: event.clientX, y: event.clientY };
            this.advance();
        });
        host.addEventListener('pointerup', event => {
            if (this.pointer?.id !== event.pointerId) return;
            this.pointer.position = { x: event.clientX, y: event.clientY };
            this.advance();
            this.pointer = undefined;
            if (host.hasPointerCapture(event.pointerId)) host.releasePointerCapture(event.pointerId);
            this.advance();
        });
        const cancelPointer = (event: PointerEvent) => {
            if (this.pointer?.id !== event.pointerId) return;
            this.cancelPointer();
            actions.move(null);
            if (actions.active()) this.advance();
        };
        host.addEventListener('pointercancel', cancelPointer);
        host.addEventListener('lostpointercapture', cancelPointer);
        document.querySelectorAll<HTMLButtonElement>('[data-direction]').forEach(button => {
            button.addEventListener('pointerdown', event => {
                event.preventDefault();
                const direction = button.dataset.direction;
                if (actions.active() && (direction === 'up' || direction === 'left' || direction === 'down' || direction === 'right')) {
                    this.touchPress = directionVector(direction);
                    this.advance();
                }
            });
            button.addEventListener('click', () => {
                const direction = button.dataset.direction;
                if (actions.active() && (direction === 'up' || direction === 'left' || direction === 'down' || direction === 'right')) {
                    this.touchPress = directionVector(direction);
                    this.advance();
                }
            });
        });
    }

    get joystick(): JoystickView | null {
        if (!this.pointer || !this.actions.active()) return null;
        const offset = { x: this.pointer.position.x - this.pointer.origin.x, y: this.pointer.position.y - this.pointer.origin.y };
        return { offset, length: Math.hypot(offset.x, offset.y) * window.devicePixelRatio };
    }

    private cancelPointer(): void {
        const id = this.pointer?.id;
        this.pointer = undefined;
        this.touchPress = null;
        if (id !== undefined && this.host.hasPointerCapture(id)) this.host.releasePointerCapture(id);
    }

    advance(): void {
        if (!this.actions.active()) return;
        const held = new Set([...this.heldKeys.values(), ...this.heldGamepad]);
        const x = held.has('left') ? -1 : held.has('right') ? 1 : 0;
        const y = held.has('up') ? -1 : held.has('down') ? 1 : 0;
        if (x || y) {
            this.touchPress = null;
            this.actions.move({ x, y });
        } else if (this.pointer) {
            const joystick = this.joystick;
            if (!joystick) return;
            this.touchPress = joystick.length >= 20 ? pointerDirection(joystick.offset) : { x: 0, y: 0 };
            this.actions.move({ ...this.touchPress });
        } else if (this.touchPress) {
            // Native release keeps a buffered turn only until Pac takes that direction.
            const direction = directionVector(this.actions.direction());
            if (direction.x === this.touchPress.x) this.touchPress.x = 0;
            if (direction.y === this.touchPress.y) this.touchPress.y = 0;
            this.actions.move({ ...this.touchPress });
        } else if (this.gamepadMoving) this.actions.move(null);
    }

    poll(): void {
        if (document.hidden) return;
        this.heldGamepad.clear();
        const connected = new Set<number>();
        for (const pad of navigator.getGamepads?.() ?? []) {
            if (!pad || !pad.connected || (pad.mapping !== 'standard' && !isXboxGamepad(pad))) continue;
            connected.add(pad.index);
            const previous = this.buttons.get(pad.index);
            const start = pad.buttons[9]?.pressed ?? false;
            const action = pad.buttons[0]?.pressed ?? false;
            if (start && !previous?.start) this.actions.pause();
            if (action && !previous?.action) this.actions.action();
            const x = pad.axes[0] ?? 0;
            const y = pad.axes[1] ?? 0;
            const values: [Direction, number, number][] = [['up', -y, 12], ['down', y, 13], ['left', -x, 14], ['right', x, 15]];
            const directions = new Set<Direction>();
            for (const [direction, value, button] of values) {
                // Match gamepad_device's separate press/release thresholds on each stick direction.
                const stickHeld = value >= 0.5625 || (value >= 0.5 && !!previous?.directions.has(direction));
                if (stickHeld) directions.add(direction);
                if (stickHeld || pad.buttons[button]?.pressed) this.heldGamepad.add(direction);
            }
            this.buttons.set(pad.index, { start, action, directions });
        }
        for (const index of this.buttons.keys()) if (!connected.has(index)) this.buttons.delete(index);
        if (!this.actions.active() && (this.pointer || this.touchPress)) {
            this.cancelPointer();
            this.actions.move(null);
        }
        this.advance();
        if (!this.actions.active() && this.gamepadMoving && !this.heldKeys.size && !this.heldGamepad.size) this.actions.move(null);
        this.gamepadMoving = this.heldGamepad.size > 0;
    }
}

function isXboxGamepad(pad: Gamepad): boolean {
    return /x-?box|xinput/i.test(pad.id);
}

function directionVector(direction: Direction): Vec {
    switch (direction) {
        case 'up': return { x: 0, y: -1 };
        case 'down': return { x: 0, y: 1 };
        case 'left': return { x: -1, y: 0 };
        case 'right': return { x: 1, y: 0 };
    }
}

function pointerDirection(offset: Vec): Vec {
    const slice = Math.trunc(Math.atan2(offset.y, offset.x) * 8 / Math.PI);
    switch (slice) {
        case 0: return { x: 1, y: 0 };
        case 1: case 2: return { x: 1, y: 1 };
        case 3: case 4: return { x: 0, y: 1 };
        case 5: case 6: return { x: -1, y: 1 };
        case 7: case -7: case 8: case -8: return { x: -1, y: 0 };
        case -6: case -5: return { x: -1, y: -1 };
        case -4: case -3: return { x: 0, y: -1 };
        case -2: case -1: return { x: 1, y: -1 };
        default: throw new Error(`Invalid pointer direction slice: ${slice}`);
    }
}
