# Infinite Pac Shared
Example video of the game in action:
https://www.youtube.com/watch?v=LZImeRnXGvA

This repo contains the full source code for building the game *Infinite Pac Shared*, which is a desktop port of *Infinite Pac* in the Microsoft Store. The game *Infinite Pac* is available for free here: https://www.microsoft.com/store/apps/9nblggh4vbpc

## Starting point
Open the solution maze.sln in Visual Studio 2022 or later. The native C++ game remains in `maze`; the TypeScript browser game is in `maze.web`.

## Web game
The `maze.web` JavaScript/TypeScript project uses PixiJS with WebGL rendering and Web Audio. Install Node.js 22.12+ (24 LTS recommended, available on PATH) and Visual Studio's JavaScript/TypeScript project support. Restart Visual Studio if Node was just installed, reload the solution, set **maze.web** as the startup project, and press **F5**. The development server runs at http://127.0.0.1:5173. The first build restores npm dependencies and the JavaScript project SDK.

Visual Studio starts `npm run dev` during the JavaScript project's **Deploy** phase before opening Edge. Keep both **Build** and **Deploy** checked for `maze.web` in **Build > Configuration Manager** (enabled in the solution's Debug, Profile, and Release configurations). If Edge reports `ERR_CONNECTION_REFUSED`, check **Deploy**, stop debugging, and retry F5; also check the development-server console for startup errors. Reload the solution after pulling changes to its configuration.

The project can also be run from a terminal:

```powershell
Set-Location maze.web
npm ci
npm run dev
```

- **Build:** `npm run build` type-checks and produces a static site in `maze.web\dist`. Deploy the contents of that directory, including `assets`. No application server or external service is required. Use `npm run preview` to serve the production build locally at http://127.0.0.1:4173; opening `index.html` directly with a `file:` URL is not supported.
- **Tests:** `npm test` runs deterministic gameplay, resource, audio, and storage tests. After building, `npm run test:browser` runs desktop and touch-screen smoke tests in installed Microsoft Edge through Playwright. The headless tests use software WebGL so they can run without a physical GPU; the game itself requests WebGL normally.
- **Controls:** Arrow keys/WASD, standard gamepad stick/D-pad, swipe/tap on the maze, or the touch direction buttons. P/Escape pauses, M toggles sound, and F toggles fullscreen. A gamepad's A button starts/resumes and Start pauses. Losing focus or hiding the tab pauses the game without accumulating simulation time.
- **Content:** Both character graphics and maze sets, three difficulties, alternating two-player games, fruit, ghost modes, extra lives, high scores, and the original artwork/sounds. Menus are adapted for browser and touch use rather than duplicating Windows dialogs. Browser fullscreen, audio, and vibration follow device permissions/capabilities; audio starts after interaction.
- **Assets:** `predev` and `prebuild` copy PNG, WAV, JSON, and icon files directly from `maze\assets` into the ignored `maze.web\public\assets` directory. Do not edit the generated copies. The native resource definitions remain the source of truth for maze/difficulty data and sprite/animation metadata.
- **Saves:** Versioned settings and high-score tables are stored locally per browser/site, not in the native Windows save format. Clearing site data removes them. No online leaderboard or account is required.

The simulation advances at 60 fixed updates per second, independently of display refresh rate. This is a TypeScript recreation, not a WebAssembly build of the C++ code; automated tests cover key behavior, but exhaustive tick-for-tick equivalence with the native game has not been established.

### Web components
`main.ts` connects the HTML menus, input, gameplay, renderer, audio, and local storage. `game.ts` owns simulation state and emits events; `renderer.ts` consumes a `GameView` and draws PixiJS/WebGL sprites, while `audio.ts` handles sound events using Web Audio. The browser's `requestAnimationFrame` drives presentation; an accumulator advances gameplay in fixed 1/60-second steps, clamping frame elapsed time to 100 ms. Rendering uses the latest state without position interpolation.

`native-animation.ts` loads all 32 original character/effect/ghost animations from `Assets.Anim.res.json` and `Assets.GhostAnim.res.json`, expanding scoped `res:values`, templates, and references on module initialization. Vite bundles the JSON; no generated replacement animation format is maintained. The player implements the interpolation, sprite tracks, layered visuals, rotation, color/opacity, and looping/bounds used by these resources. Track interpolation follows the native engine: a visual's `method: spline` does not change its key tracks' default linear interpolation. The 286 sprite frames retain their original PNG rectangles, repeat offsets, handles, and signed scales from `Assets.Sprites.res.json`.

Dot motion remains a subtle +/-1 logical pixel with integer tile phases. Ghost loops use their original lengths, including the third ghost's independently rotating arms; pupils look toward the pending direction with a one-pixel offset. Mouth animation freezes at walls rather than restarting, and death rotation uses continuous eighth-frame progress after the original delay. Actor/effect visibility follows the ready, caught, ghost-eaten, dying, dead, and winning transitions. The HUD includes both players' scores and the high score, original life/fruit icons, and source-colored point digits.

Eating dots, power pellets, fruit, and ghosts emits independent, non-looping bubbles through that same animation player, using `bubble.png` and its original handle. Instances retain their spawn positions with randomized size, playback speed, and upward drift; the native parent scale is approximately 1, not the maze sprite scale of 0.125. Each bubble grows, rises, fades, and expires instead of being drawn as a solid circle or attached to Pac.

Audio references, volume, speed, and loop flags come from `Assets.Audio.res.json`. On a trusted gesture, Web Audio requests an interactive/low-latency context and decodes each unique WAV once. With sound enabled, Play waits for decoding before starting; gameplay also waits during sound enable/resume, with no catch-up updates. Ghost/fruit effects call `AudioBufferSourceNode.start()` synchronously in the same simulation update that emits the event, before drawing the result. Timed effects are never queued for late playback; premature audible playback is an explicit error. The first player's intro and original ready delay are restored. Same-named one-shots restart; background loops change only when gameplay requests a different loop. Sound off mutes the master gain; pause suspends the context. Gamepad-only startup may require a click/key to unlock browser audio, and the page displays that requirement. Browser, OS and output-device buffering (particularly Bluetooth) can still add latency; a latency hint cannot remove hardware latency.

Keyboard state is sampled as simultaneous held axes, with native Up-over-Down and Left-over-Right priority. Releasing one key immediately restores other held keys (including independent arrow/WASD aliases); releases and focus loss clear stale buffered turns. Touch controls retain their deliberate swipe/tap direction latch.

Browser menus, score entry, pause panels, and touch controls remain intentional HTML adaptations rather than copies of the native title-screen attract animation and Windows UI. Consequently the native title-screen fruit-bounce/navigation sounds are not recreated as an attract sequence; gameplay uses the original sound resources. PNG/WAV assets have not been recompressed or trimmed, avoiding a new lossy conversion during the fidelity work.

## Copyright
Copyright 2025 Peter Spada