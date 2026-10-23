# maze.web vs. native maze: known differences

Findings from a side-by-side read of the native C++ `maze` project and `maze.web`
(`game.ts`, `renderer.ts`, `main.ts`, `input.ts`). Nothing here has been fixed yet, and the
review did not run either game. The native code (`maze\core\*.cpp`, `maze\states\PacApplication.cpp`,
`maze\assets\*.res.json`) is the source of truth.

Each entry has a **Fix** note when the change is known. Entries marked **Unverified** need a
check against the native source before changing code. Per `.github\instructions\maze-web.instructions.md`,
every fix should come with a source-derived regression test.

Intentional HTML adaptations (menus, score entry, pause panels, touch controls, no attract
animation) are not listed.

## Frame / update timing

### 1. Point popups disappear one update early
- Native `PointActor::AdvanceCountdown` returns true when the counter decrements to 0 and erases
  the actor on the *next* call. A popup lives countdown + 1 updates: 55 for ghost points and 121
  for fruit points.
- Web removes it as soon as `--remaining <= 0`, after exactly 54 / 120 updates. The text vanishes
  on the same update the eaten-pause ends, one update before native.
- **Fix:** keep the popup for one more update. Remove it when the counter is already 0 at the start
  of the update, not when it reaches 0. Add a test that checks a popup's lifetime is 55 / 121 updates.

### 2. Bubble expiry is off by one
- Native removes a bubble when `_frame > frame_length`. Web removes it at `frame >= length`.
- **Fix:** change the comparison to `>`. Verify the exact frame counter semantics in the native
  bubble code first (the web player may count from a different base).

### 3. Final "GAME OVER" screen length is wall-clock based
- Native: when the last player is out, `_isGameOver = true` and `_gameOverCounter = 1`. The game
  keeps updating, and `IsGameOver()` becomes true when the counter exceeds 120. That shows
  "GAME OVER" for 120 more updates before returning to the title.
- Web: `main.ts` waits `now - endedAt > 2500` ms of real time, which is about 150 updates.
  It also depends on display timing and is not tied to simulation updates.
- **Fix:** count updates in `Game`. After `gameOver` becomes true, increment a counter each
  `advance()` and expose `view.gameOverDone` when it exceeds 120. In `main.ts`, call `showMenu()`
  on that flag instead of the `endedAt` timer, and drop `endedAt`. Update the docs and tests that
  assume 2.5 s.

### 4. Press direction accepted while Pac is inactive
- Native `PlayingActor::SetPressDir` ignores input unless the actor is `_active`.
- Web `Game.press()` (`game.ts`, around line 479) always stores the press and sets `canTurn = true`.
- Effect is at most one update, since input is resampled each update.
- **Fix:** early-return in `press()` when `!run.pac.active`.

### 5. Extra input sampling between updates
- Native samples input once per update (`HandlePressing` in `PacApplication::HandleInputEvents`).
- Web `InputController` also calls `advance()` from keydown, keyup and pointer events, as well as
  before every fixed update.
- The end state is the same on the next update. A press and release inside one frame can be seen
  differently from native.
- **Unverified / maybe intentional.** The docs say input is "sampled before every fixed update".
  Decide whether the event-driven calls should go. Removing them would match native exactly but
  can add up to one frame of latency.

### 6. Accumulator and catch-up behaviour
- Native runs exactly one update per fixed tick.
- Web uses a requestAnimationFrame accumulator on a `1000 / 60` float step, clamped to 100 ms
  per frame. The accumulator is zeroed on pause, blur, sound loading and `handleEvents()` pauses.
  Simulation time is dropped, not caught up. This is documented as intentional.
- No fix proposed. Listed only because the update count per real second can differ from native.

## Rules / configuration hard-coded in `game.ts`

### 7. Lives and free-life rules
- Native reads `lives`, `firstLife`, `repeatLife` and `maxFreeLives` per difficulty from
  `maze\assets\Values.Mazes.res.json`. Current values: 3 lives; first life at 10000; repeat at 10000
  (easy / normal) or 20000 (hard); max free lives 3 (easy) or 2 (normal / hard).
- Web hard-codes the same numbers (`game.ts` around lines 342-344, plus the starting lives).
  They match today but will drift if the JSON changes.
- **Fix:** load these values from the copied `Values.Mazes.res.json` (the same way the maze and
  difficulty data are read), keyed by maze set and difficulty.

### 8. Ms. Pac random-scatter condition
- Native: `HasRandomGhostMovement` depends on `Maze->GetCharType()`, which comes from each maze's
  `type` field in `Values.Mazes.res.json`.
- Web uses `options.mazeSet === 'ms'`.
- **Unverified:** equal only if every maze in the `ms` set has `type: "ms"` and every maze in the
  `mr` set has `type: "mr"`.
- **Fix (if they can differ):** read the per-maze `type` from the resource instead of the option.

### 9. Pac animation rate hard-coded
- Web uses `animation.pacTicks * 45 / 60` (`game.ts`, around line 468). Native uses the animation
  resource's `frames_per_second()`. The Pac animations found use 45, so this is correct today.
- **Fix:** read the fps from the animation data in `native-animation.ts` instead of hard-coding it.

### 10. Random numbers
- Web uses `Math.random`; native uses its own RNG. Fruit timing and the scared / scatter random
  choices are statistically equal but cannot be replayed identically. The docs already say this.
- No fix proposed.

## HUD / rendering

### 11. Status bar is 8 px taller than native
- Native `_nStatusTiles = 2`, so the total height is `(mazeH + 5) * 8`.
- Web `HUD_BOTTOM = 24` (3 tiles), so the canvas is `(mazeH + 6) * 8`. Life and fruit icons appear
  about 8 px lower.
- **Fix:** reduce `HUD_BOTTOM` to 16 in `renderer.ts` and re-derive the icon y positions from the
  native `PlayingGame::Render`. Then check the canvas size and layout in the browser tests.

### 12. In-canvas "PAUSED" does not match native
- Native: a half-transparent black fade over the whole screen, then "PAUSED" at 2x scale in red
  (1, 0, 0, 0.5) with black and white +/-0.5 px shadows, placed at the ghost start tile + (-5, 7).
- Web: 1x text centred on screen in the default yellow tint, no fade and no shadows.
- The HTML pause panel is an intentional adaptation, but the in-canvas text is not a faithful copy.
- **Fix:** in `renderer.ts`, draw a `(0, 0, 0, 0.5)` full-screen rectangle, then the text at 2x with
  the red tint and the two shadow offsets, at the native position.

### 13. Point popup digit layout
- Native uses `DrawSmallNumber` (`RenderText.cpp`). Web lays digits out ad hoc (1 px spacing,
  centred, floored positions).
- **Unverified:** compare spacing, centring and rounding with `DrawSmallNumber`.

### 14. High-score display
- Native HUD shows `max(player scores, Stats::highScores[0])` for the maze set and difficulty.
- Web does the same in `main.ts` using `storage.scores(...)[0]`.
- **Unverified:** this relies on the web table being keyed the same way as native `Stats::Get(mazesID)`.
  The docs say it is (maze set + difficulty), so this is probably fine.

## Audio

### 15. Background loop and restart timing
- Native `SoundEffects::PlayBG` restarts only when the effect changes and respects the sound option.
  Web emits events that `audio.ts` turns into loops. `audio.ts` was not reviewed.
- **Unverified:** compare loop start / stop timing against native.
- Power-pellet sound is *not* a difference: native `EFFECT_EAT_POWER1/2` map to the same
  `eat-dot-1/2` resources.

## Minor / performance

### 16. Repeated work in hot paths
- `difficultyFor()` is re-parsed on several per-tick paths in `game.ts`.
- `finishDeath` rebuilds the level and re-parses the maze (`mazeSerial++`) on every death.
- **Fix:** cache the difficulty row per run and reuse the parsed maze when restarting after a death.

## Verified equivalent (no action needed)

- Update order in `AdvanceActors`.
- 16.16 fixed-point movement and tile conversion.
- Pac turning, cornering and stuck logic.
- Ghost house, leaving and eyes logic.
- Ghost decisions and chase targets, including the orange ghost's 8192 threshold.
- Elroy thresholds and all speeds.
- Scatter, chase and scared timers.
- Dot release counters.
- Fruit spawn, path and brain.
- Tunnel wrap and the collision threshold.
- The 54-update eaten pause and the 200 / 400 / 800 / 1600 chain.
- State timings and the 120-update game-over freeze.
- Free-life logic, player switching and level progression.
- Animation counters and the death frame formula.
- Winning flash timing and dot hiding.
- HUD positions, colours and the 1UP blink.
- The 8-slice touch map and the 20 px dead zone.

## Performance improvement backlog (2026-10-23)

This section records a separate review of the current web implementation. The older parity
findings above are historical and were not revalidated by this performance review. These are
optimization candidates, not claims that the game currently misses its frame budget. Status
notes below track the first allocation-focused implementation pass; unmarked entries remain open.

### Baseline and design constraints

- Instrumentation of the production build in headless Edge, using WebGL2/software rendering
  at a 1280 x 720 viewport, measured 3 draw calls per ordinary gameplay frame, 3 while paused,
  and 5 on the title screen. About still submitted 5 behind its opaque HTML screen. These are
  submission counts, not hardware GPU timings; other devices and game states can differ.
- The initial Mr. Pac board has 510 wall/door tiles and 278 dots/power pellets. Each wall/door
  uses three sprites (background, fill, border), totaling 1,530 wall sprites, not draw calls.
- Wall backgrounds share `edges-bg.png`; fills and borders share `edges.png`. PixiJS batches
  compatible sprites across ordinary containers and texture frames, including different texture
  sources within the device's batch limit. Tint and alpha differences do not inherently split
  sprite batches. The gameplay capture had a background draw, a floor-geometry draw, and one
  large sprite batch. Reducing draw calls is not the first priority.
- Keep sprite pooling, shared texture sources, decoded audio-buffer reuse, and the static-maze
  cache. The 36 sprite-source images contain about 1.76 MiB of RGBA8 base-level pixel data;
  this excludes framebuffer, driver, and other memory overhead.
- Preserve the fixed 60 Hz simulation, native animation curves, drawing order, ghost tie-breaking,
  random-number consumption, input semantics, graphics recovery, and browser lifecycle behavior.
- Measure CPU preparation time, allocation/GC behavior, draw calls, buffer uploads, and memory
  before/after changes. Use actual hardware for GPU timing. Do not add an ECS, workers, or a
  custom renderer without evidence that the complexity is warranted.

### P1. Skip completely covered or unchanged canvas rendering

- **Status: implemented for About, hidden tabs, and unchanged paused gameplay.** Controller
  polling remains active; resize, DPR changes, pause/resume, and graphics recovery invalidate
  frozen presentation. Browser tests count actual WebGL submissions and exercise invalidation.
  Avoiding duplicate active-scene preparation on high-refresh displays remains open.
- **Where:** `main.ts` frame loop; `renderer.ts` `render()` and `renderScreen()`.
- **Issue:** About freezes simulation but continues preparing/rendering the covered title.
  Paused gameplay also repaints unchanged dots, actors, HUD, and overlays every animation frame.
- **Fix idea:** Keep controller polling independent of rendering. Skip canvas preparation while
  covered; redraw paused scenes only when presentation is dirty. At high display refresh rates,
  avoid duplicate presentation work when no simulation or UI state has changed.
- **Watch out:** Resize, DPR changes, context recovery, pause/resume, visibility changes, and
  returning from About must invalidate presentation. Keep input, status messages, and browser
  shortcuts responsive. Do not change simulation timing to reduce rendering frequency.
- **Validate:** Count submissions while covered/paused; verify redraw after every invalidation,
  controller-only return, high-refresh displays, and no visible stale frames.

### P2. Share dot animation samples by phase

- **Status: implemented with exact integer-offset groups and reusable AnimationSampler buffers.**
  The initial Mr. Pac board now needs 29 samples per changed dot tick instead of 278 (about 90%
  fewer); unchanged frames reuse prior samples. Keep full tile offsets rather than reducing them
  modulo four: regression tests found that modulo grouping changes floating-point rounding of
  eight-bit tint values. The proposed five-sample variant below is therefore not used.
  All actor/effect samplers also reuse their output array, visual objects, vectors, and colors
  after warmup. The allocating `sampleAnimation()` convenience API still returns an independent
  sample; rendering uses retained samplers instead. Resource references/tracks resolve once per
  sampler, and sprite identities survive one-shot expiry/restart.
- **Where:** `renderer.ts` `drawMazeDots()`; `native-animation.ts` `sampleAnimation()`.
- **Issue:** Every visible dot independently searches/interpolates the same animation tracks and
  allocates sampling results. Ordinary dots have four phase groups; power pellets share one phase.
- **Fix idea:** Sample the four ordinary phases and one power phase once per relevant render/tick,
  then reuse the read-only results. The initial board can use 5 samples instead of 278.
- **Watch out:** Derive grouping from the native loop length and tile phase; preserve fractional
  frames, offsets, brightness, scale, rotation, and ready/death/winning visibility behavior.
  Shared sampled values must not be mutated by individual sprites.
- **Validate:** Compare sampled visuals over a complete animation cycle and all phase groups;
  measure sample counts and allocation reduction for gameplay and foreground screen dots.

### P3. Avoid full GameView snapshots for scalar queries

- **Status: implemented.** Input and completion checks use scalar `state`, `direction`, and
  `gameOverDone` accessors. Gameplay and title-background rendering use caller-owned
  `readView(target)` buffers that overwrite existing arrays, actors, positions, and animation
  values. `game.view` still returns an independent snapshot for callers needing that ownership.
  Reuse does not cache simulation state, so there is no revision-invalidation scheme to maintain;
  every read resets presentation-only high-score overrides. New/returning entities may allocate
  when an output list grows; particle spawn/despawn pooling remains P11 work.
- **Where:** `game.ts` `get view()`; `main.ts` input callbacks and frame loop.
- **Issue:** The getter creates actor/position objects, arrays for scores, particles and points,
  and a fruit-history copy. It is also called merely to read state, direction, or completion.
- **Fix idea:** Expose cheap scalar queries and create one presentation snapshot when needed.
  Consider revision-based snapshot reuse only if simpler accessors do not remove enough work.
- **Watch out:** Invalidate any cached snapshot on all observable mutations, including input and
  player/level transitions. `main.ts` currently assigns `view.highScore`; do not accidentally
  persist that mutation in a shared snapshot. Preserve test and renderer ownership expectations.
- **Validate:** Count snapshots/allocations per update and render; run multiplayer, score-entry,
  pause/interruption, progression, and input regressions.

### P4. Make DOM layout, labels, and diagnostics change-driven

- **Where:** `main.ts` `layoutScreenControls()` and frame-loop `dataset` writes.
- **Issue:** Every frame reads layout bounds and writes mostly unchanged positions, transforms,
  accessibility labels, sound attributes, and diagnostic strings.
- **Fix idea:** Recompute hit-region geometry on resize/screen changes, update labels on settings
  changes, and avoid writing unchanged values. Consider an explicit diagnostic mode for rapidly
  changing telemetry rather than unconditional production DOM updates.
- **Watch out:** Batch layout reads before writes. Preserve alignment after fullscreen, DPR,
  orientation, and canvas-size changes. Existing browser tests depend on `data-*` diagnostics;
  provide deliberate test wiring if gating them. Do not remove accessibility updates.
- **Validate:** Measure DOM mutations/layout work and verify hit targets, focus, fullscreen,
  mobile orientation, settings labels, and diagnostic-dependent tests.

### P5. Retain wall geometry through winning flashes

- **Where:** `renderer.ts` maze cache keys, `rebuildStaticMaze()`, and `renderMazeLayer()`.
- **Issue:** Winning-flash transitions invalidate the cache, destroy/recreate wall sprites, and
  rerun wall-pattern matching even though geometry has not changed.
- **Fix idea:** Separate geometry invalidation from palette/flash changes. Retain wall sprites
  and update tint, alpha, visibility, and floor appearance without rebuilding the maze.
- **Watch out:** Doors have special color/alpha rules. Preserve background/fill transparency,
  flash timing, final wall hiding, board offsets, and separate foreground/background caches.
- **Validate:** Assert stable sprite identity/count across flashes, compare rendered flash states,
  and verify real maze changes and multiplayer switches still rebuild correctly.

### P6. Pre-resolve sprite rendering descriptors

- **Status: implemented.** A renderer-lifetime descriptor cache resolves each encountered sprite
  once; warm paints do not parse names or recalculate anchors/base scales. Scalar transform
  arguments remove temporary position/scale objects from dots, actors, bubbles, cursor, icons,
  and bitmap glyphs. Shared defaults/direction data avoid tiny repeated arrays/objects; missing
  glyph lookup no longer allocates empty arrays. Caches are cleared on renderer destruction.
- **Where:** `renderer.ts` `paintSprite()` and `drawAnimation()`.
- **Issue:** Repeated name regex parsing, map lookups, anchor calculations, and trigonometry occur
  while painting dots, glyphs, and actors.
- **Fix idea:** Resolve names to reusable descriptors containing texture, anchor, and base scale.
  Reuse rotation calculations where appropriate, especially for unrotated sprites.
- **Watch out:** Keep animation-local offsets, mirrored scales, rotation composition, and native
  sprite handles exact. Cache lifetime must follow texture ownership and renderer destruction;
  unknown sprite references must continue to produce explicit errors.
- **Validate:** Compare representative rotated/mirrored actors, glyphs, dots, bubbles, and joystick
  arrows; measure painting CPU time and allocations rather than assuming map caching helps.

### P7. Give dots stable tile-associated sprites

- **Where:** `renderer.ts` `drawMazeDots()` and dot pools.
- **Issue:** All maze tiles are scanned each render and surviving dots are packed into sequential
  pool slots. Consuming an early dot can shift many later sprite assignments.
- **Fix idea:** Keep tile-associated sprites or a live-dot list with precomputed positions/phases;
  hide consumed dots and update only surviving dots' animated properties.
- **Watch out:** Handle two players' independent boards, level replacement, title/score mazes,
  global dot hiding, and cache reset without resurrecting consumed pellets. Avoid an unbounded
  cache of past levels. This is lower priority than phase-shared animation sampling.
- **Validate:** Consume dots at the beginning/middle/end of the list, switch players, die/restart,
  complete levels, and verify stable identities and correct visible counts.

### P8. Update persistent text and score snapshots only when changed

- **Status: partially implemented.** `topScore()` avoids copying tables for the HUD; the title
  takes a score snapshot on settings changes/return from gameplay instead of every frame. Fruit
  history rendering no longer slices its array, and popup digit layout uses two bounded passes
  instead of constructing glyph/width arrays. Persistent text/glyph caching and change-driven
  title/score-screen text construction remain open and are priority allocation follow-ups.
- **Where:** `renderer.ts` HUD/bitmap text; `screens.ts` screen view construction;
  `storage.ts` `scores()`; `main.ts` high-score lookup.
- **Issue:** Static glyphs are repainted and score strings are formatted repeatedly. `scores()`
  copies the full in-memory table, sometimes twice per menu frame; it is not reading localStorage.
- **Fix idea:** Retain static labels and formatted values until changed. Add a cheap top-score
  query or cache immutable score snapshots by revision; update blinking/fading elements separately.
- **Watch out:** Preserve score-table encapsulation, sorting/category changes, ties, failed-write
  behavior, player blink, hover fade, score-entry edits, and incoming/outgoing screen fades.
- **Validate:** Count formatting/copy work; run score persistence/ranking and screen-transition
  regressions, including unchanged labels with changing alpha.

### P9. Reuse simple background and fade geometry

- **Where:** `renderer.ts` `renderScreen()` and `drawOverlay()`.
- **Issue:** Background rectangles, screen washes, and pause fades are cleared/reconstructed
  repeatedly although only their alpha, visibility, or dimensions may change.
- **Fix idea:** Retain rectangles or solid-color sprites and update only changed properties.
- **Watch out:** Preserve blend behavior, drawing order, exact pause/score dimming, title fade,
  canvas resizing, and WebGL context recovery. A different primitive can change batch boundaries.
- **Validate:** Compare fade endpoints/intermediate frames and draw-call counts after replacement.

### P10. Benchmark native/capped render resolution or logical-resolution static caching

- **Where:** `renderer.ts` `resizeCanvas()` and static maze layers.
- **Issue:** Drawing-buffer size follows display scale times DPR, increasing rasterization and
  framebuffer memory on large/high-DPI displays despite the low-resolution source art.
- **Fix idea:** Experiment with native/capped-resolution rendering plus nearest-neighbor display
  scaling, or cache only static walls/floor at logical resolution.
- **Watch out:** This is an experiment, not a predetermined improvement. Extra render targets,
  passes, and compositing may cost more than they save when gameplay already takes 3 draw calls.
  Caching adds texture memory. Preserve pixel alignment, fractional scaling, shadows, sprite
  rounding, fullscreen behavior, and context recovery; avoid screen-resolution maze caches.
- **Validate:** Benchmark real low/high-DPI hardware, inspect screenshots at integer/fractional
  scales, and compare GPU time, framebuffer/texture memory, and total preparation cost.

### P11. Reduce smaller input, AI, and particle allocations if profiling warrants it

- **Where:** `input.ts` `advance()`/`poll()`; `game.ts` ghost decisions and particle updates.
- **Issue:** Small Sets/arrays/points are created for input and movement decisions; particles are
  allocated individually and removed using `splice()`.
- **Fix idea:** Reuse input state, calculate candidate distances with fewer temporary points,
  and consider stable particle compaction or a bounded reusable pool.
- **Watch out:** Only four ghosts and small particle populations are involved. Avoid complexity
  for negligible gains. Preserve direction priority, stick hysteresis, button edges/repeat,
  ghost tie-breaking, random-number consumption, particle ordering, and exact expiry timing.
  Do not use unordered removal where it changes rendering or gameplay behavior.
- **Validate:** Profile before optimizing; run deterministic movement/ghost/particle regressions
  and keyboard, touch, and multi-controller input tests. Check pool growth during a long session.
