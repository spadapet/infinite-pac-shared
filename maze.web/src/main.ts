import './style.css';
import { Game } from './game';
import { GameRenderer } from './renderer';
import { GameAudio } from './audio';
import { GameStorage } from './storage';
import { ScoreScreen, TitleScreen, scoreLetters, titleLabels, titleRows } from './screens';
import { InputController } from './input';
import { GameVibration } from './vibration';
import type { Direction, GameOptions, GameEvent } from './types';

function element<T extends HTMLElement>(id: string, type: { new(): T }): T {
    const value = document.getElementById(id);
    if (!(value instanceof type)) throw new Error(`Missing ${id}`);
    return value;
}
const status = element('status', HTMLElement);
let statusTimer: ReturnType<typeof setTimeout> | undefined;
function setStatus(message: string, error = false): void {
    if (statusTimer !== undefined) clearTimeout(statusTimer);
    statusTimer = undefined;
    status.textContent = message;
    status.classList.toggle('error', error);
    if (message) statusTimer = setTimeout(() => setStatus(''), 8000);
}
function report(message: string): void {
    console.error(message);
    setStatus(message, true);
}
function failure(error: unknown): void { report(error instanceof Error ? error.message : String(error)); }
const storage = new GameStorage({ getItem: key => localStorage.getItem(key), setItem: (key, value) => localStorage.setItem(key, value) }, report);
let settings = storage.settings;
settings.character = settings.mazeSet;
const audio = new GameAudio();
const vibration = new GameVibration(report);
audio.setMuted(!settings.sound);
const host = element('game', HTMLElement);
const menu = element('menu', HTMLElement);
const pauseMenu = element('pause-menu', HTMLElement);
const scoreEntry = element('score-entry', HTMLElement);
const aboutScreen = element('about-screen', HTMLElement);
const aboutContent = element('about-content', HTMLElement);
const aboutBack = element('about-back', HTMLButtonElement);
const aboutActions = Array.from(aboutScreen.querySelectorAll<HTMLAnchorElement | HTMLButtonElement>('a, button'));
const pauseButton = element('pause', HTMLButtonElement);
const soundButton = element('sound', HTMLButtonElement);
const titleButtons = ['start', 'start-ms', 'players', 'difficulty', 'sound', 'vibration', 'fullscreen', 'about'].map(id => element(id, HTMLButtonElement));
let title = new TitleScreen(settings);
let scoreScreen: ScoreScreen | undefined;
let scoreFade = 0;
let scoreClosing = false;
let scoreInterrupted = false;
let selectedGame = settings.mazeSet;
let renderer: GameRenderer;
let game = new Game(settings);
const presentationView = game.view;
let titleScores = storage.scores(settings);
let options: GameOptions = { ...settings };
let mode: 'menu' | 'game' = 'menu';
let paused = false;
let lastTime = 0;
let accumulator = 0;
let soundNeedsGesture = false;
let soundLoading: Promise<void> | undefined;
let soundError: string | undefined;
let starting = false;
let graphicsLost = false;
let presentationDirty = true;
let presentationPixelRatio = 0;
const pendingScores: Extract<GameEvent, { type: 'gameOver' }>[] = [];

function updateSoundButton(): void {
    soundButton.setAttribute('aria-label', settings.sound ? 'Sound on' : 'Sound off');
    soundButton.setAttribute('aria-pressed', String(settings.sound));
}
updateSoundButton();
function unlockSound(): Promise<void> {
    if (!settings.sound) return Promise.resolve();
    if (navigator.userActivation && !navigator.userActivation.hasBeenActive) {
        soundNeedsGesture = true;
        if (!status.classList.contains('error')) setStatus('Click, tap, or press a key to enable sound.');
        return Promise.resolve();
    }
    soundNeedsGesture = false;
    if (!soundLoading) {
        const loading = audio.unlock().then(() => {
            if (soundError && status.textContent === soundError) setStatus('');
            soundError = undefined;
            if (!status.classList.contains('error')) setStatus('');
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
            if (!status.classList.contains('error')) setStatus('');
        }).catch(failure);
    }
});
function saveSettings(): void {
    settings.character = settings.mazeSet;
    storage.saveSettings(settings);
    titleScores = storage.scores(settings);
    updateSoundButton();
}
async function startGame(): Promise<void> {
    if (!renderer || starting || graphicsLost || mode !== 'menu') return;
    starting = true;
    titleButtons.forEach(button => { button.disabled = true; });
    try {
        saveSettings();
        const ready = unlockSound();
        audio.stopAll();
        await Promise.all([ready, audio.setPaused(false)]);
        title.done = true;
    } catch (error) {
        starting = false;
        titleButtons.forEach(button => { button.disabled = false; });
        failure(error);
    }
}
function beginGame(): void {
    options = { ...settings, character: selectedGame, mazeSet: selectedGame };
    game = new Game(options);
    pendingScores.length = 0;
    mode = 'game';
    presentationDirty = true;
    paused = false;
    starting = false;
    accumulator = 0;
    lastTime = 0;
    menu.hidden = true;
    pauseMenu.hidden = true;
    pauseButton.disabled = false;
    pauseButton.hidden = false;
    host.focus({ preventScroll: true });
    if (document.hidden || graphicsLost) setPaused(true);
    handleEvents();
}
function showMenu(): void {
    audio.stopAll();
    vibration.stop();
    void audio.setPaused(false).catch(failure);
    mode = 'menu';
    presentationDirty = true;
    paused = false;
    starting = false;
    pendingScores.length = 0;
    scoreScreen = undefined;
    scoreClosing = false;
    scoreInterrupted = false;
    title = new TitleScreen(settings);
    titleScores = storage.scores(settings);
    selectedGame = settings.mazeSet;
    menu.hidden = false;
    aboutScreen.hidden = true;
    pauseMenu.hidden = true;
    scoreEntry.hidden = true;
    pauseButton.disabled = true;
    pauseButton.hidden = true;
    titleButtons.forEach(button => { button.disabled = false; });
    accumulator = 0;
    host.focus({ preventScroll: true });
}
function setPaused(value: boolean): void {
    if (value && scoreScreen) scoreInterrupted = true;
    if (mode !== 'game' || game.state === 'gameover' || scoreScreen || (!value && graphicsLost)) return;
    paused = value;
    presentationDirty = true;
    if (value) vibration.stop();
    accumulator = 0;
    pauseMenu.hidden = !value;
    pauseButton.hidden = value;
    void audio.setPaused(value).catch(failure);
    if (!value) { void unlockSound().catch(failure); host.focus({ preventScroll: true }); }
}
function togglePause(): void {
    if (mode === 'menu') return;
    setPaused(!paused);
}
function showAbout(): void {
    menu.hidden = true;
    aboutScreen.hidden = false;
    aboutBack.focus({ preventScroll: true });
    aboutContent.scrollTop = 0;
    accumulator = 0;
}
function closeAbout(): void {
    aboutScreen.hidden = true;
    menu.hidden = false;
    accumulator = 0;
    lastTime = 0;
    host.focus({ preventScroll: true });
}
aboutBack.addEventListener('click', closeAbout);
function toggleSound(): void {
    settings.sound = !settings.sound;
    audio.setMuted(!settings.sound);
    storage.saveSettings(settings);
    updateSoundButton();
    if (settings.sound) { void unlockSound().catch(failure); if (paused) void audio.setPaused(true).catch(failure); }
}
const fullscreenGestureMessage = 'Press F to enter fullscreen, or click/tap the Full Screen option.';
function fullscreen(): void {
    if (!document.fullscreenEnabled) { report('Fullscreen is unavailable in this browser.'); return; }
    if (document.fullscreenElement) { void document.exitFullscreen().catch(failure); return; }
    if (navigator.userActivation?.isActive === false) { report(fullscreenGestureMessage); return; }
    void element('app', HTMLElement).requestFullscreen().then(() => {
        if (status.textContent === fullscreenGestureMessage) setStatus('');
    }).catch((error: unknown) => {
        if (error instanceof TypeError || (error instanceof DOMException && ['NotAllowedError', 'SecurityError'].includes(error.name))) report(fullscreenGestureMessage);
        else failure(error);
    });
}
function showNextScore(): void {
    while (pendingScores.length && !storage.qualifies(options, pendingScores[0].score)) pendingScores.shift();
    const entry = pendingScores[0];
    if (!entry) {
        const interrupted = scoreInterrupted;
        scoreInterrupted = false;
        scoreEntry.hidden = true;
        scoreScreen = undefined;
        scoreClosing = false;
        paused = false;
        accumulator = 0;
        lastTime = 0;
        pauseButton.disabled = false;
        pauseButton.hidden = false;
        if (game.gameOverDone) showMenu();
        else if (document.hidden || graphicsLost || interrupted) setPaused(true);
        else { void audio.setPaused(false).catch(failure); void unlockSound().catch(failure); host.focus({ preventScroll: true }); }
        return;
    }
    paused = true;
    vibration.stop();
    void audio.setPaused(true).catch(failure);
    scoreEntry.hidden = false;
    pauseMenu.hidden = true;
    pauseButton.disabled = true;
    pauseButton.hidden = true;
    const slot = storage.scores(options).filter(score => score.score >= entry.score).length;
    scoreScreen = new ScoreScreen(entry.score, slot);
    scoreFade = 0;
    scoreClosing = false;
    host.focus({ preventScroll: true });
}
function handleEvents(): void {
    for (const event of game.drainEvents()) {
        if (event.type === 'gameOver' && storage.qualifies(options, event.score)) pendingScores.push(event);
        else if (event.type === 'vibrate') {
            if (settings.vibration && !paused && !graphicsLost) vibration.play(event);
        } else audio.handle(event);
    }
    if (pendingScores.length && scoreEntry.hidden) showNextScore();
}

async function menuEffect(name: 'eat-dot-1' | 'eat-dot-2' | 'fruit-bounce'): Promise<void> {
    if (document.hidden) return;
    if (settings.vibration && name !== 'eat-dot-2') {
        vibration.play({ type: 'vibrate', duration: 100,
            strongMagnitude: name === 'fruit-bounce' ? 0.5 : 0,
            weakMagnitude: name === 'eat-dot-1' ? 0.5 : 0 });
    }
    if (!settings.sound) return;
    try {
        await unlockSound();
        audio.handle({ type: 'sound', name: `mr-${name}` });
    } catch (error) { failure(error); }
}
async function executeTitle(index: number, left = false): Promise<void> {
    if (mode !== 'menu' || starting || graphicsLost || !aboutScreen.hidden) return;
    if (index < 2) {
        title.selected = index;
        selectedGame = index === 0 ? 'mr' : 'ms';
        settings.character = settings.mazeSet = selectedGame;
        await startGame();
        return;
    }
    if (index === 2) settings.players = settings.players === 1 ? 2 : 1;
    if (index === 3) {
        const difficulties = ['easy', 'normal', 'hard'] as const;
        settings.difficulty = difficulties[(difficulties.indexOf(settings.difficulty) + (left ? 2 : 1)) % 3];
    }
    if (index === 4) toggleSound();
    if (index === 5) { settings.vibration = !settings.vibration; if (!settings.vibration) vibration.stop(); }
    if (index === 6) fullscreen();
    if (index === 7) showAbout();
    saveSettings();
    await menuEffect('fruit-bounce');
}
function navigateScreen(direction: Direction): void {
    if (!aboutScreen.hidden) {
        const selected = aboutActions.findIndex(control => control === document.activeElement);
        const offset = direction === 'up' || direction === 'left' ? -1 : 1;
        aboutActions[(selected + offset + aboutActions.length) % aboutActions.length].focus();
        return;
    }
    if (starting || graphicsLost || scoreClosing) return;
    if (scoreScreen) { scoreScreen.move(direction); return; }
    if (mode !== 'menu') return;
    if (direction === 'left' || direction === 'right') {
        if (title.selected >= 2 && title.selected <= 6) void executeTitle(title.selected, direction === 'left');
    } else if (title.move(direction)) {
        if (title.selected < 2) {
            settings.character = settings.mazeSet = title.selected === 0 ? 'mr' : 'ms';
            saveSettings();
        }
        void menuEffect(title.selected % 2 ? 'eat-dot-2' : 'eat-dot-1');
    }
}
function finishScore(save: boolean): void {
    const entry = pendingScores[0];
    if (!scoreScreen || !entry || scoreClosing) return;
    if (save) storage.addScore(options, { name: scoreScreen.name, score: entry.score, level: entry.level, date: new Date().toISOString() });
    pendingScores.shift();
    scoreClosing = true;
    if (graphicsLost) showNextScore();
}
function action(): void {
    if (!aboutScreen.hidden) {
        const selected = aboutActions.find(control => control === document.activeElement) ?? aboutBack;
        selected.click();
        return;
    }
    if (scoreClosing || graphicsLost) return;
    if (scoreScreen) { if (scoreScreen.execute()) finishScore(true); }
    else if (mode === 'menu') void executeTitle(title.selected);
    else if (paused) setPaused(false);
}
for (const [index, button] of titleButtons.entries()) {
    button.addEventListener('click', () => { void executeTitle(index); });
    button.addEventListener('pointerenter', () => { title.hover = index; });
    button.addEventListener('pointerleave', () => { title.hover = -1; });
}
for (const [index, letter] of scoreLetters.entries()) {
    const button = document.createElement('button');
    button.type = 'button';
    button.setAttribute('aria-label', letter.text === ' ' ? 'Space' : letter.text === 'End' ? 'Save score' : letter.text);
    button.style.left = `${letter.x - 4}px`;
    button.style.top = `${letter.y - 4}px`;
    button.style.width = `${letter.text.length * 8 + 8}px`;
    button.style.height = '16px';
    button.addEventListener('pointerenter', () => { if (scoreScreen) scoreScreen.selected = index; });
    button.addEventListener('click', () => { if (scoreScreen) { scoreScreen.selected = index; action(); } });
    scoreEntry.append(button);
}
pauseButton.addEventListener('click', togglePause);
element('resume', HTMLButtonElement).addEventListener('click', () => setPaused(false));
element('home', HTMLButtonElement).addEventListener('click', showMenu);
document.addEventListener('keydown', event => {
    if (!aboutScreen.hidden) {
        const directions: Record<string, Direction> = { ArrowUp: 'up', ArrowDown: 'down', ArrowLeft: 'left', ArrowRight: 'right' };
        if (event.key === 'Escape' || event.key === 'Enter' || event.key === ' ' || directions[event.key]) {
            event.preventDefault();
            event.stopImmediatePropagation();
            if (directions[event.key]) navigateScreen(directions[event.key]);
            else if (!event.repeat) { if (event.key === 'Escape') closeAbout(); else action(); }
        }
        return;
    }
    if (mode !== 'menu' && !scoreScreen) return;
    if (scoreClosing) { event.preventDefault(); event.stopImmediatePropagation(); return; }
    const directions: Record<string, Direction> = { ArrowUp: 'up', ArrowDown: 'down', ArrowLeft: 'left', ArrowRight: 'right' };
    if (scoreScreen && event.key === 'Escape') {
        event.preventDefault(); event.stopImmediatePropagation(); if (!event.repeat) finishScore(false);
    } else if (directions[event.key]) {
        event.preventDefault(); event.stopImmediatePropagation(); navigateScreen(directions[event.key]);
    } else if (scoreScreen && (event.key === 'Backspace' || /^[a-z .]$/i.test(event.key))) {
        event.preventDefault(); event.stopImmediatePropagation(); scoreScreen.type(event.key);
    } else if (event.key === 'Enter' || (!scoreScreen && event.key === ' ')) {
        event.preventDefault(); event.stopImmediatePropagation(); if (!event.repeat) action();
    }
}, true);
function layoutScreenControls(): void {
    const canvas = host.querySelector('canvas');
    if (!canvas) return;
    const bounds = canvas.getBoundingClientRect();
    const hostBounds = host.getBoundingClientRect();
    const scale = bounds.width / 224;
    for (const controls of [menu, scoreEntry]) {
        controls.style.left = `${bounds.left - hostBounds.left}px`;
        controls.style.top = `${bounds.top - hostBounds.top}px`;
        controls.style.width = '224px';
        controls.style.height = controls === menu ? '248px' : '288px';
        controls.style.transform = `scale(${scale})`;
    }
    const labels = titleLabels(settings, !!document.fullscreenElement);
    titleButtons.forEach((button, index) => {
        button.style.left = '54px'; button.style.top = `${titleRows[index] - 6}px`;
        button.style.width = `${labels[index].length * 8 + 4}px`; button.style.height = '12px';
        if (index >= 2) button.setAttribute('aria-label', labels[index]);
    });
    updateSoundButton();
}
document.addEventListener('visibilitychange', () => { if (document.hidden) { vibration.stop(); setPaused(true); } });
window.addEventListener('blur', () => { vibration.stop(); setPaused(true); });
window.addEventListener('pagehide', () => vibration.stop());
window.addEventListener('gamepaddisconnected', () => vibration.stop());
host.addEventListener('webglcontextlost', () => {
    graphicsLost = true;
    setPaused(true);
    element('resume', HTMLButtonElement).disabled = true;
    if (!status.classList.contains('error')) setStatus('Graphics connection interrupted. Waiting for recovery.');
}, true);
host.addEventListener('webglcontextrestored', () => {
    graphicsLost = false;
    presentationDirty = true;
    element('resume', HTMLButtonElement).disabled = false;
    if (!status.classList.contains('error')) setStatus('Graphics recovered. Resume when you are ready.');
}, true);

async function boot(): Promise<void> {
    renderer = await GameRenderer.create(host);
    new ResizeObserver(() => { presentationDirty = true; }).observe(host);
    const input = new InputController(host, {
        active: () => mode === 'game' && !paused && !graphicsLost && game.state !== 'gameover',
        move: direction => game.press(direction),
        action,
        menuActive: () => mode === 'menu' || !!scoreScreen,
        navigate: navigateScreen,
        cancel: () => { if (!aboutScreen.hidden) closeAbout(); else if (scoreScreen) finishScore(false); else if (mode === 'game') { if (paused) setPaused(false); else showMenu(); } },
        pause: togglePause,
        start: () => { if (mode === 'menu') action(); else togglePause(); },
        mute: toggleSound,
        fullscreen,
        direction: () => game.direction,
    });
    if (!status.classList.contains('error')) setStatus('');
    showMenu();
    function frame(now: number): void {
        try {
            const elapsed = lastTime ? Math.min(now - lastTime, 100) : 0;
            lastTime = now;
            input.poll();
            if (!graphicsLost && !document.hidden && aboutScreen.hidden
                && (mode === 'menu' || scoreScreen || (!paused && !soundLoading))) {
                accumulator += elapsed;
                while (accumulator >= 1000 / 60) {
                    accumulator -= 1000 / 60;
                    if (mode === 'menu') {
                        if (title.done) { beginGame(); break; }
                        title.advance();
                    } else if (scoreScreen) {
                        scoreScreen.advance();
                        scoreFade = Math.max(0, Math.min(0.875, scoreFade + (scoreClosing ? -1 : 1) / 64));
                        if (scoreClosing && scoreFade === 0) { showNextScore(); break; }
                    } else {
                        input.advance();
                        game.advance();
                        handleEvents();
                        if (game.gameOverDone) { showMenu(); break; }
                        if (paused) { accumulator = 0; break; }
                    }
                }
            } else accumulator = 0;
            if (aboutScreen.hidden && !document.hidden && (!paused || scoreScreen || presentationDirty || presentationPixelRatio !== window.devicePixelRatio)) {
                const view = game.readView(presentationView);
                view.highScore = Math.max(view.highScore, storage.topScore(mode === 'game' ? options : settings));
                if (mode === 'menu') renderer.renderScreen(title.view(settings, titleScores, !!document.fullscreenElement));
                else if (scoreScreen) renderer.renderScreen({ ...scoreScreen.view(view), backgroundFade: scoreFade,
                    showForeground: !scoreClosing && scoreFade === 0.875 });
                else renderer.render(view, paused, input.joystick);
                layoutScreenControls();
                element('loading', HTMLElement).hidden = true;
                host.dataset.state = mode === 'menu' ? 'menu' : scoreScreen ? 'score' : paused ? 'paused' : view.state;
                host.dataset.character = mode === 'menu' ? settings.character : options.character;
                host.dataset.mazeSet = mode === 'menu' ? settings.mazeSet : options.mazeSet;
                host.dataset.selection = String(scoreScreen?.selected ?? title.selected);
                host.dataset.name = scoreScreen?.name ?? '';
                host.dataset.score = String(view.score);
                host.dataset.tick = String(view.tick);
                host.dataset.position = `${view.pac.position.x},${view.pac.position.y}`;
                host.dataset.level = String(view.level);
                host.dataset.player = String(view.player);
                presentationDirty = false;
                presentationPixelRatio = window.devicePixelRatio;
            } else if (!aboutScreen.hidden) host.dataset.state = 'about';
            requestAnimationFrame(frame);
        } catch (error) {
            audio.stopAll();
            vibration.stop();
            pauseButton.disabled = true;
            element('loading', HTMLElement).hidden = true;
            failure(error);
        }
    }
    requestAnimationFrame(frame);
}
void boot().catch(error => {
    element('loading', HTMLElement).hidden = true;
    report(`The game could not start. WebGL hardware rendering is required. ${error instanceof Error ? error.message : String(error)}`);
});
