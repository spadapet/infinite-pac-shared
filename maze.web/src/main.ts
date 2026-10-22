import './style.css';
import { Game } from './game';
import { GameRenderer } from './renderer';
import { GameAudio } from './audio';
import { GameStorage } from './storage';
import { InputController } from './input';
import type { GameOptions, GameEvent } from './types';

function element<T extends HTMLElement>(id: string, type: { new(): T }): T {
    const value = document.getElementById(id);
    if (!(value instanceof type)) throw new Error(`Missing ${id}`);
    return value;
}
const status = element('status', HTMLElement);
function report(message: string): void {
    console.error(message);
    status.textContent = message;
    status.classList.add('error');
}
function failure(error: unknown): void { report(error instanceof Error ? error.message : String(error)); }
const storage = new GameStorage({ getItem: key => localStorage.getItem(key), setItem: (key, value) => localStorage.setItem(key, value) }, report);
let settings = storage.settings;
const audio = new GameAudio();
audio.setMuted(!settings.sound);
const host = element('game', HTMLElement);
const menu = element('menu', HTMLElement);
const pauseMenu = element('pause-menu', HTMLElement);
const scoresPanel = element('scores', HTMLElement);
const scoreEntry = element('score-entry', HTMLElement);
const pauseButton = element('pause', HTMLButtonElement);
const soundButton = element('sound', HTMLButtonElement);
const characterSelect = element('character', HTMLSelectElement);
const mazeSelect = element('mazeSet', HTMLSelectElement);
const difficultySelect = element('difficulty', HTMLSelectElement);
const playersSelect = element('players', HTMLSelectElement);
const vibrationInput = element('vibration', HTMLInputElement);
characterSelect.value = settings.character;
mazeSelect.value = settings.mazeSet;
difficultySelect.value = settings.difficulty;
playersSelect.value = String(settings.players);
vibrationInput.checked = settings.vibration;
let renderer: GameRenderer;
let game = new Game(settings);
let options: GameOptions = { ...settings };
let mode: 'menu' | 'game' = 'menu';
let paused = false;
let lastTime = 0;
let accumulator = 0;
let endedAt = 0;
let soundNeedsGesture = false;
let soundLoading: Promise<void> | undefined;
let soundError: string | undefined;
let starting = false;
let graphicsLost = false;
const pendingScores: Extract<GameEvent, { type: 'gameOver' }>[] = [];

function updateSoundButton(): void {
    soundButton.textContent = settings.sound ? 'Sound on' : 'Sound off';
    soundButton.setAttribute('aria-pressed', String(!settings.sound));
}
updateSoundButton();
function unlockSound(): Promise<void> {
    if (!settings.sound) return Promise.resolve();
    if (navigator.userActivation && !navigator.userActivation.hasBeenActive) {
        soundNeedsGesture = true;
        if (!status.classList.contains('error')) status.textContent = 'Click, tap, or press a key to enable sound.';
        return Promise.resolve();
    }
    soundNeedsGesture = false;
    if (!soundLoading) {
        const loading = audio.unlock().then(() => {
            if (soundError && status.textContent === soundError) {
                status.classList.remove('error');
                status.textContent = 'Original graphics & sound. WebGL powered.';
            }
            soundError = undefined;
        }).catch((error: unknown) => {
            soundError = error instanceof Error ? error.message : String(error);
            if (mode === 'game') setPaused(true);
            throw error;
        }).finally(() => {
            if (soundLoading === loading) soundLoading = undefined;
            accumulator = 0;
            lastTime = 0;
        });
        soundLoading = loading;
    }
    return soundLoading;
}
for (const event of ['pointerdown', 'keydown']) document.addEventListener(event, () => {
    if (soundNeedsGesture) {
        void unlockSound().then(() => {
            if (!status.classList.contains('error')) status.textContent = 'Original graphics & sound. WebGL powered.';
        }).catch(failure);
    }
});
function readOptions(): void {
    const character = characterSelect.value;
    const mazeSet = mazeSelect.value;
    const difficulty = difficultySelect.value;
    const players = Number(playersSelect.value);
    if ((character !== 'mr' && character !== 'ms') || (mazeSet !== 'mr' && mazeSet !== 'ms')
        || (difficulty !== 'easy' && difficulty !== 'normal' && difficulty !== 'hard') || (players !== 1 && players !== 2)) {
        throw new Error('Invalid game options');
    }
    settings = { ...settings, character, mazeSet, difficulty, players, vibration: vibrationInput.checked };
    storage.saveSettings(settings);
}
async function startGame(): Promise<void> {
    if (!renderer || starting || graphicsLost || !scoreEntry.hidden || !scoresPanel.hidden) return;
    starting = true;
    const startButton = element('start', HTMLButtonElement);
    startButton.disabled = true;
    startButton.textContent = settings.sound ? 'Loading sound...' : 'Starting...';
    try {
        readOptions();
        const ready = unlockSound();
        audio.stopAll();
        await Promise.all([ready, audio.setPaused(false)]);
        options = { ...settings };
        game = new Game(options);
        pendingScores.length = 0;
        mode = 'game';
        paused = false;
        endedAt = 0;
        accumulator = 0;
        lastTime = 0;
        menu.hidden = true;
        pauseMenu.hidden = true;
        pauseButton.disabled = false;
        pauseButton.textContent = 'Pause';
        host.focus({ preventScroll: true });
        if (document.hidden || graphicsLost) setPaused(true);
        handleEvents();
    } catch (error) {
        failure(error);
    } finally {
        starting = false;
        startButton.disabled = false;
        startButton.textContent = 'Play game';
    }
}
function showMenu(): void {
    audio.stopAll();
    mode = 'menu';
    paused = false;
    endedAt = 0;
    pendingScores.length = 0;
    menu.hidden = false;
    pauseMenu.hidden = true;
    scoreEntry.hidden = true;
    scoresPanel.hidden = true;
    pauseButton.disabled = true;
    pauseButton.textContent = 'Pause';
    game = new Game(settings);
    element('start', HTMLButtonElement).focus({ preventScroll: true });
}
function setPaused(value: boolean): void {
    if (mode !== 'game' || game.view.state === 'gameover' || !scoreEntry.hidden || (!value && graphicsLost)) return;
    paused = value;
    accumulator = 0;
    pauseMenu.hidden = !value;
    pauseButton.textContent = value ? 'Resume' : 'Pause';
    void audio.setPaused(value).catch(failure);
    if (value) element('resume', HTMLButtonElement).focus({ preventScroll: true });
    else { void unlockSound().catch(failure); host.focus({ preventScroll: true }); }
}
function togglePause(): void {
    if (!scoresPanel.hidden) { scoresPanel.hidden = true; element('scores-button', HTMLButtonElement).focus(); return; }
    setPaused(!paused);
}
function toggleSound(): void {
    settings.sound = !settings.sound;
    audio.setMuted(!settings.sound);
    storage.saveSettings(settings);
    updateSoundButton();
    if (settings.sound) { void unlockSound().catch(failure); if (paused) void audio.setPaused(true).catch(failure); }
}
function fullscreen(): void {
    if (!document.fullscreenEnabled) { report('Fullscreen is unavailable in this browser.'); return; }
    if (document.fullscreenElement) void document.exitFullscreen().catch(failure);
    else void element('app', HTMLElement).requestFullscreen().catch(failure);
}
function showNextScore(): void {
    const entry = pendingScores[0];
    if (!entry) {
        scoreEntry.hidden = true;
        paused = false;
        accumulator = 0;
        if (game.view.state === 'gameover') showMenu();
        else { void audio.setPaused(false).catch(failure); void unlockSound().catch(failure); host.focus({ preventScroll: true }); }
        return;
    }
    paused = true;
    void audio.setPaused(true).catch(failure);
    scoreEntry.hidden = false;
    pauseMenu.hidden = true;
    element('score-summary', HTMLElement).textContent = `Player ${entry.player}: ${entry.score.toLocaleString()} points - Level ${entry.level}`;
    const name = element('score-name', HTMLInputElement);
    name.value = `PLAYER ${entry.player}`;
    name.focus({ preventScroll: true });
    name.select();
}
function handleEvents(): void {
    for (const event of game.drainEvents()) {
        if (event.type === 'gameOver' && storage.qualifies(options, event.score)) pendingScores.push(event);
        else if (event.type === 'vibrate') {
            if (settings.vibration && navigator.vibrate) navigator.vibrate(Math.min(event.duration, 1000));
        } else audio.handle(event);
    }
    if (pendingScores.length && scoreEntry.hidden) showNextScore();
}

element('options', HTMLFormElement).addEventListener('submit', event => { event.preventDefault(); void startGame(); });
element('options', HTMLFormElement).addEventListener('change', () => {
    readOptions();
    if (mode === 'menu') game = new Game(settings);
});
pauseButton.addEventListener('click', togglePause);
soundButton.addEventListener('click', toggleSound);
element('fullscreen', HTMLButtonElement).addEventListener('click', fullscreen);
element('resume', HTMLButtonElement).addEventListener('click', () => setPaused(false));
element('home', HTMLButtonElement).addEventListener('click', showMenu);
element('scores-button', HTMLButtonElement).addEventListener('click', () => {
    const list = element('score-list', HTMLOListElement);
    list.replaceChildren();
    const scores = storage.scores(settings);
    for (const score of scores) {
        const item = document.createElement('li');
        item.append(document.createTextNode(score.name));
        const points = document.createElement('span');
        points.textContent = score.score.toLocaleString();
        item.append(points);
        list.append(item);
    }
    if (!scores.length) { const empty = document.createElement('li'); empty.textContent = 'No scores yet. Your maze awaits.'; list.append(empty); }
    element('score-category', HTMLElement).textContent = `${settings.mazeSet === 'mr' ? 'Mr. Pac' : 'Ms. Pac'} mazes - ${settings.difficulty}`;
    scoresPanel.hidden = false;
    element('scores-close', HTMLButtonElement).focus({ preventScroll: true });
});
element('scores-close', HTMLButtonElement).addEventListener('click', () => { scoresPanel.hidden = true; element('scores-button', HTMLButtonElement).focus(); });
element('score-form', HTMLFormElement).addEventListener('submit', event => {
    event.preventDefault();
    const entry = pendingScores[0];
    const name = element('score-name', HTMLInputElement).value.trim();
    if (!entry || !name) return;
    storage.addScore(options, { name, score: entry.score, level: entry.level, date: new Date().toISOString() });
    pendingScores.shift();
    showNextScore();
});
document.addEventListener('visibilitychange', () => { if (document.hidden) setPaused(true); });
window.addEventListener('blur', () => setPaused(true));
host.addEventListener('webglcontextlost', () => {
    graphicsLost = true;
    setPaused(true);
    element('resume', HTMLButtonElement).disabled = true;
    if (!status.classList.contains('error')) status.textContent = 'Graphics connection interrupted. Waiting for recovery.';
}, true);
host.addEventListener('webglcontextrestored', () => {
    graphicsLost = false;
    element('resume', HTMLButtonElement).disabled = false;
    if (!status.classList.contains('error')) status.textContent = 'Graphics recovered. Resume when you are ready.';
}, true);

async function boot(): Promise<void> {
    renderer = await GameRenderer.create(host);
    const input = new InputController(host, {
        active: () => mode === 'game' && !paused && !graphicsLost && game.view.state !== 'gameover',
        move: direction => game.press(direction),
        action: () => { if (mode === 'menu') void startGame(); else if (paused && scoreEntry.hidden) setPaused(false); },
        pause: togglePause,
        mute: toggleSound,
        fullscreen,
        point: (x, y) => renderer.screenToGame(x, y),
        pac: () => game.view.pac.position,
    });
    if (!status.classList.contains('error')) status.textContent = 'Original graphics & sound. WebGL powered.';
    showMenu();
    function frame(now: number): void {
        try {
            const elapsed = lastTime ? Math.min(now - lastTime, 100) : 0;
            lastTime = now;
            input.poll();
            if (mode === 'game' && !paused && !soundLoading && !graphicsLost && !document.hidden) {
                accumulator += elapsed;
                while (accumulator >= 1000 / 60) {
                    game.advance();
                    accumulator -= 1000 / 60;
                    handleEvents();
                    if (paused) { accumulator = 0; break; }
                }
                if (game.view.state === 'gameover') {
                    if (!endedAt) endedAt = now;
                    else if (now - endedAt > 2500) showMenu();
                }
            } else accumulator = 0;
            const view = game.view;
            view.highScore = Math.max(view.highScore, storage.scores(mode === 'game' ? options : settings)[0]?.score ?? 0);
            renderer.render(view, paused);
            host.dataset.state = mode === 'menu' ? 'menu' : paused ? 'paused' : view.state;
            host.dataset.score = String(view.score);
            host.dataset.tick = String(view.tick);
            host.dataset.position = `${view.pac.position.x},${view.pac.position.y}`;
            host.dataset.level = String(view.level);
            host.dataset.player = String(view.player);
            requestAnimationFrame(frame);
        } catch (error) {
            audio.stopAll();
            pauseButton.disabled = true;
            failure(error);
        }
    }
    requestAnimationFrame(frame);
}
void boot().catch(error => report(`The game could not start. WebGL hardware rendering is required. ${error instanceof Error ? error.message : String(error)}`));
