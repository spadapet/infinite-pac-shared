import type { GameEvent } from './types';

type VibrationEvent = Extract<GameEvent, { type: 'vibrate' }>;
interface Pulse { end: number; strongMagnitude: number; weakMagnitude: number }

export class GameVibration {
    private readonly pulses: Pulse[] = [];
    private readonly failed = new WeakSet<GamepadHapticActuator>();
    private controller?: { index: number; id: string; actuator: GamepadHapticActuator };
    private timer?: ReturnType<typeof setTimeout>;
    private deviceVibrating = false;

    constructor(private readonly report: (message: string) => void) {}

    play(event: VibrationEvent): void {
        if (document.hidden) return;
        const pad = Array.from(navigator.getGamepads?.() ?? []).find(pad =>
            pad?.connected && typeof pad.vibrationActuator?.playEffect === 'function'
                && typeof pad.vibrationActuator.reset === 'function' && !this.failed.has(pad.vibrationActuator));
        if (!pad) {
            this.stop();
            this.deviceVibrating = navigator.vibrate?.(event.duration) ?? false;
            return;
        }
        if (this.controller?.index !== pad.index || this.controller.id !== pad.id) {
            this.stop();
            this.controller = { index: pad.index, id: pad.id, actuator: pad.vibrationActuator };
        }
        this.pulses.push({ end: performance.now() + event.duration,
            strongMagnitude: event.strongMagnitude, weakMagnitude: event.weakMagnitude });
        this.update();
    }

    stop(): void {
        if (this.timer !== undefined) clearTimeout(this.timer);
        this.timer = undefined;
        this.pulses.length = 0;
        const controller = this.controller;
        this.controller = undefined;
        if (controller) void this.reset(controller.actuator);
        if (this.deviceVibrating) navigator.vibrate?.(0);
        this.deviceVibrating = false;
    }

    private update(): void {
        if (this.timer !== undefined) clearTimeout(this.timer);
        this.timer = undefined;
        const controller = this.controller;
        if (!controller) return;
        const pad = navigator.getGamepads?.()[controller.index];
        if (document.hidden || !pad?.connected || pad.id !== controller.id) {
            this.stop();
            return;
        }
        const now = performance.now();
        for (let index = this.pulses.length - 1; index >= 0; index--) {
            if (this.pulses[index].end <= now) this.pulses.splice(index, 1);
        }
        if (!this.pulses.length) {
            this.stop();
            return;
        }
        // Native XInput keeps the strongest unexpired pulse independently for each motor.
        const strongMagnitude = Math.max(...this.pulses.map(pulse => pulse.strongMagnitude));
        const weakMagnitude = Math.max(...this.pulses.map(pulse => pulse.weakMagnitude));
        const duration = Math.min(...this.pulses.map(pulse => pulse.end)) - now;
        this.timer = setTimeout(() => this.update(), duration);
        void this.playEffect(controller.actuator, { startDelay: 0, duration, strongMagnitude, weakMagnitude });
    }

    private async playEffect(actuator: GamepadHapticActuator, parameters: GamepadEffectParameters): Promise<void> {
        try {
            const result = await actuator.playEffect('dual-rumble', parameters);
            if (result !== 'complete' && result !== 'preempted') throw new Error(`Unexpected haptics result: ${String(result)}`);
        } catch (error) {
            this.failed.add(actuator);
            if (this.controller?.actuator === actuator) this.stop();
            this.report(`Controller rumble could not play: ${error instanceof Error ? error.message : String(error)}`);
        }
    }

    private async reset(actuator: GamepadHapticActuator): Promise<void> {
        try {
            await actuator.reset();
        } catch (error) {
            this.report(`Controller rumble could not stop: ${error instanceof Error ? error.message : String(error)}`);
        }
    }
}
