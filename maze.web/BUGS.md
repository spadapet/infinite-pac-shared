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
