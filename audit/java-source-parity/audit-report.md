# Java Source Parity Audit — Native C vs Decompiled Java ME

**Status:** Read-only audit. Evidence = decompiled Java source (BASE-001) vs current native tree.
**Date:** 2026-10-23
**Tree at audit time:** clean, HEAD `ec8717d`

**Scope:** full native port audited against `src/main/java/com/nokia/mid/appl/boun/`
(Bounce, BounceGame, BounceTimer, b, e, f, d, TileIDs, Translation, Vec2S, Main — 2 938 lines)
across three passes: (1) gameplay/physics/collision/scoring, (2) app flow/menus/persistence/localization/input, (3) rendering/audio/HUD/timer/debug.

**Status legend**

| Status | Meaning |
|---|---|
| IMPLEMENTED | Present, behaviorally equivalent to Java, with native evidence |
| PARTIAL | Present but incomplete / state not fully modeled |
| NOT IMPLEMENTED | Absent from native |
| DEVIATES | Present but behavior differs from Java (deliberate or bug) |
| DIFFERENT | Native-only extension (superset) — not a parity gap |

---

## Executive summary

**Verdict: the port is exceptionally faithful.** Physics, collision, tile dispatch, scoring,
death/respawn, hoops, persistence and the state machine are line-by-line transcriptions with
inline Java line citations. Several comments in native code claiming features are "missing"
are **stale** — the code has since gained them (score wiring, boost bar, GodMode, volume, pause).

### Implemented and verified (highlights)

| Area | Evidence |
|---|---|
| Full movement physics (accel/friction/clamps, 4 gravity variants, jump, ceiling/landing bounces, corner slides, substep loop, damping) | `vertical_slice.c:4466-5375` = `f.java:670-855` |
| Collision subsystem (12×12/16×16 masks, slope mask, thin/edge/shape/hoop/dyn-thorn queries, OOB-blocked) | `collision_query.c:17-1101` = `f.java:48-473,858` |
| **Tile dispatch: every Java tile ID handled** (1,2,3-6,7,9,10,13-28,29,30-37,38,39-42,43-46,47-50,51-54, water 0x40) | matrix below §3 |
| Power-ups (300-tick durations, decrements, KillBall clear, HUD boost bar) | `vertical_slice.c:3842-5049, 6537-6557` |
| Death/respawn (KillBall q=7/z=2, GodMode guard, lives-- , cap-5 on 1-up, checkpoint snapshot incl. big ball, game-over persist) | `vertical_slice.c:2044-3059` |
| Hoops: +500, HoopsScored++, 8 hoop ID pairs, big-ball refusal 13/14/15/16, all-collected → exit unlock, door animation (offset+=4, open at 24) | `vertical_slice.c:1745-1969, 3324-3337, 6193-6257` |
| **Score wiring: all four verified** — hoop +500 (`vs:1937`), crystal +200 (`vs:3093`), 1-up +1000 (`vs:3167`), exit +5000 (`game.c:1454`) | resolves old backlog BL-001 |
| Max-lives cap 5 on extra-life | `vertical_slice.c:3168-3171` — resolves old BL-002 |
| Pickup/POP/UP sounds wired from all gameplay sites (9 emission sites via central sink) | resolves old BL-003 |
| Rubber-pad charge (`v` latch → `t += ±10` on landing → jump `o = ±67 + t`) | `vertical_slice.c:4732-4739, 5122-5128` = `f.java:754-828` |
| Level 8 unseal patch called on **both** load routes + verifier | `level.c:204-225`, `game.c:349`, `game.c:554`, `verify_l8_unseal vs:15051` |
| Persistence: 3 Java records (best level, high score, resume snapshot w/ tile deltas + dyn thorn offsets), save triggers (death/exit/shutdown), J==1 vs J==2 continue | `persistence.c/h`, `vertical_slice.c:2784-28187` |
| Cheats: GodMode `7-8-7-8-9-8` and af `7-8-7-8-9-9` **match Java digit-for-digit** (`e.java:349-381`); `#` gravity cheat af-gated | `vertical_slice.c:27131, 27715-27728` |
| Timer: fixed 40 ms tick = `BounceTimer.java:17`; stop on death/complete/menu | `game_timer.h:38`, `game.c:999,1162,284` |
| Splash skip, level banner, 120-tick entry countdown, HUD geometry/colors (band 0x0853aa, lives y=99, rings y=112, score (64,100), boost bar) | `app_flow.h:196`, `vertical_slice.c:6377-6557` |
| Pause (**native extension**) wired end-to-end on `P`: producer → flow GAMEPLAY→PAUSE → ticks stop → overlay → resume | `vertical_slice.c:28500-28512`, `app_flow.c:284-293` |
| Volume 0–100, **persisted** (record 6), runtime gain synced per frame | `persistence.c:1082,1140`, `vertical_slice.c:2688-2696` |
| FPS setting: **cancelled** (Java never had one; added then reverted in `e24f2ae`) | correct outcome, documented in `BL-005-cancellation-report.md` |

### Gaps — not implemented / deviates (the work list)

| # | Gap | Status | Evidence |
|---|---|---|---|
| G1 | **`K==1` live-session Continue** — Java Continue restarts the in-progress level when a live session exists; native only continues from persisted snapshot | NOT IMPLEMENTED | `BounceGame.java:116,223-224`; structural note `vertical_slice.c:8949-9015` |
| G2 | ~~Second splash `nokiagames.png` never rendered~~ — **no effective gap**: the source is single-stage. The `e` constructor sets `SplashId = 1` (e.java:72), so only `bouncesplash.png` ever displays for 31 ticks (`splashTimer > 30`, e.java:231) and any key skips (e.java:331-332); the `case 0` stage at e.java:235-243 is **dead code** (no assignment of `SplashId = 0` anywhere in the tree). Native matches exactly (single stage, `>30` timing verified, key-skip). Drawing nokiagames would DEVIATE; the asset stays inventory-verified only, mirroring Java never loading SPLASHES[0] at runtime | RESOLVED (analysis; dead code deliberately not implemented) | `e.java:59,72,227-253,331-332`; `app_flow.c:355-366`, `ui_shell.c:2510-2536`, verify timing `vertical_slice.c:9983-9988` |
| G3 | **Missing post-ball front-tile pass** — Java repaints tiles 13–28 (`d.b` rims) *after* the ball; native render order grid→door→thorns→ball→HUD → ball paints over near hoop rim | **RESOLVED** (`1ccd833`) | `e.java:172` → `b.java:577-600`; `vertical_slice.c:7299+` (`draw_hoop_rims_over_ball`), `visual_assets.c:1106+` |
| G4 | ~~No locale autodetect / language never persisted~~ — **RESOLVED** (owner decision: detect + persist). Boot now composes: persisted `records.bin` record 7 (present-flag loader, so stored EN is distinguishable from absent) > environment detection `LC_ALL`→`LC_MESSAGES`→`LANG` (`bounce_locale_language_from_string`: exact normalized zh-CN/zh-TW/th-TH/id-ID/en, bare-code convenience, `/lang.xx`-style EN fallback) > default EN. A Settings language change commits on the same press edge as theme/T9/volume (`app_commit_language_change`); reset-to-defaults writes 0; every other record writer preserves record 7. Java parity note: the detection half mirrors the per-launch `microedition.locale` derivation; the persisted record is a native extension, since Java never persists a language | IMPLEMENTED | `Translation.java:39,55,57`; `locale.c` (detect), `persistence.c` record 7, `app_flow.c restore_persisted_language`, `vs app_restore_persisted_settings`; verifies `LANG-DETECT/PERSIST/BOOT` |
| G5 | ~~Cheat keys `1`/`3` (prev/next level)~~ — **RESOLVED** (2026-10-23). Java writes `this.d = true` unconditionally on both keys (`e.java:340/:345`) and only the level warp sits behind `if (this.af)` (`e.java:341/:346`); `d`'s single reader (`e.java:222`) reloads the CURRENT level when it did not move, so without af the keys are an instant restart. Native hoists the latch (`level_reload_requested`, renamed from `af_reload_requested`) out of the af gate — request unconditional, arithmetic af-gated — verified end to end (`G5-LEVELKEYS`: request-only without af; warp + wrap + pre-tick accumulate with af armed through the real 7-8-7-8-9-9 sequence; one-shot RESET_GATE reload clearing the latch) | RESOLVED | `e.java:339-348`; `vs` (the `1`/`3` consumer, `verify_cheat_level_keys`, `verify_tick_dispatch_boundary`); `game.h` `level_reload_requested` |
| G6 | **Fresh-install unlock count** — native defaults to 2 unlocked levels; Java starts `MaxLevels=0` (Continue hidden, no level select until first win) | DEVIATES — **accepted** (keep 2; owner decision 2026-10-23) | `BounceGame.java:270-286`; `app_flow.h:295`, `app_flow.c:235` |
| G7 | **Mouse-slider volume edits not persisted** — slider click writes `flow->settings.volume` with no commit; `q`, window-close, `app_persist_records` also omit record 6. Only keyboard LEFT/RIGHT commits | **RESOLVED** (`f895aaf`) | `ui_shell.c:1983-1989`; commit edges `x11_dispatch_event` (Button1, `XK_q`, ClientMessage), `verify_volume_slider_pointer` store assertions |
| G8 | **Big-ball settle walk side effects** — state-write tiles 38/47-50/51-54 now reported through the hazard callback and dispatched to shared producer helpers (STEP 38-S, `app_activate_gravity/jump/speed_powerup`); all walks reproduce Java's walk-side effects: death respawn, pumper growth, pumper recursion, and — constructor sub-item closed 2026-10-23 — the level-start walk too, moved from `bounce_game_initialize_player` (pristine shadow view, effects discarded) into `bounce_game_initialize_runtime_entry`, which runs `bounce_player_use_big_ball` against the fresh runtime copy through the production consumer `app_pumper_walk_hazard` between copy installation and phase record, mirroring `f.java:126-131` (`CreateTiles` then `UseBigBall`); `bounce_player_use_big_ball_level` and `player_probe_constructor` removed — one walk and one consumer, the source's single `f.a()` | RESOLVED | `player.c`/`player.h` (38-S arm, single runtime probe), `game.c` `initialize_runtime_entry`, `vertical_slice.c` (consumer, entry-boundary verifies with `BIGBALL ... RUNTIME_INITIALIZED` prints) |
| G9 | **Dirty-tile incremental repaint / E-buffer staging / out-of-level BRICK border** — `0x80` bits written+masked but never drive incremental redraw (full redraw each frame); E parallax double-draw and border stamp absent | NOT IMPLEMENTED (output-equivalent, deliberate) | `b.java:401-404,609-623`, `e.java:162-170`; `vertical_slice.c:7047-7088` |
| G10 | **Placeholder 5×7 fonts** instead of `Font.getFont(32,0,8)` device glyphs (layout/anchors match, shapes do not) | DEVIATES | `e.java:45`; `visual_assets.c:1288,1352` |
| G11 | **HUD dirty flag `y`** has no counterpart; HUD redrawn every frame (state-incomplete, output-equivalent). Some native comments claiming score/boostbar missing are **stale** | PARTIAL (deliberate) | `f.java:223,613`, `e.java:174,188,306-307`; `vertical_slice.c:6304-6317` (stale) |
| G12 | **Timer stop deferred to tick end** on level-complete/game-over (Java stops synchronously inside tick) — same-frame observable, documented | DEVIATES (documented) | `game.c:1302-1312`, `vertical_slice.c:2826-2846` |
| G13 | **Escape-from-PAUSE stale-runtime case** documented but not fixed; separate `input.pause` flag has no consumer (only `P` fires PAUSE) | PARTIAL | `vertical_slice.c:28340-28342`, `input_state.h:132`, `input_state.c:228` |
| G14 | Hidden **"Read RMS"/"Write RMS" debug menu rows** | NOT IMPLEMENTED (low impact) | `BounceGame.java:248-253` |

---

## 1. Gameplay / Physics / Collision / Scoring

### 1.1 Movement physics — all IMPLEMENTED

| Feature | Java | Native |
|---|---|---|
| Accel +6 / friction −4 (unclamped, zero-crossing) | `f.java:806-814` | `vertical_slice.c:5054-5072` |
| H-limits 50 / 100 speed / 150 clamp | `f.java:711-720,800-805` | `vs:5023,5044-5052` |
| Substep loop, live rereads, damping `-(l>>1)` | `f.java:829-855` | `vs:5213-5375` |
| Gravity variants 80/4, 38/3, water 42/6, −30/−2 | `f.java:670-686` | `vs:4466-4482` |
| Integration `o += j`, `m` gate, j==−2 case | `f.java:781-799` | `vs:4989-5007` |
| Gravity inversion + decrement/expiry | `f.java:687-698` | `vs:4483-4499` |
| `o` clamp ±150, low-positive normalize 10 | `f.java:711-722` | `vs:4519-4528` |
| Ceiling bounce (normal `−o>>1`, inverted −20, no break) | `f.java:773-778` | `vs:4749-4757, 4977-4984` |
| Landing bounce `−o/2`, min ±10 | `f.java:751-772` | `vs:4680-4747, 4937-4974` |
| Corner-slide probes | `f.java:739-750` | `vs:4637-4678, 4896-4934` |
| Jump `o = ±67 + t`; big-ball ramp `t += ±5`; jump powerup ±80 | `f.java:815-828, 699-707` | `vs:5112-5128, 4500-4512` |
| Slope redirect (30–33 half, 34–37 full), `!m` gated | `f.java:234-270,360-361` | `vs:1628-1708`, `player.c:236-311` |
| Camera clamp `0..width*12−156` | `e.java:136-143` | `vs:5640-5677` |

Note: there is **no ball-position wrap** in Java — "wrap" is camera clamp + E-parallax double-draw.
Native reproduces camera clamp exactly; E-image double-draw absent (rendering scope, G9).

### 1.2 Collision — all IMPLEMENTED

Masks bit-exact (`collision_query.c:17-66` = `f.java:48-95`); query traversal (`cqc:1030-1101`);
square (`cqc:172-245`), slope (`cqc:363-460`), shape ±4 (`cqc:252-290`), hoop-core ±6/±11 with
x+p vs x+p−1 asymmetry kept (`cqc:300-361`), dyn-thorn ±1 (`cqc:503-567`, `dyn_thorns.c:62,264`),
tile-id masking 0x40/0x80 (`cqc:838-840`), OOB = blocked (`cqc:1010-1015`).

### 1.3 Tile dispatch completeness — every Java tile ID covered

| Tiles | Java behavior | Native |
|---|---|---|
| 1 BRICK / 2 RUBBER | block (+u / +v latch) | IMPLEMENTED `vs:3793,3724,4240` |
| 3–6 THORNS, 10 dyn thorn | KillBall | IMPLEMENTED `vs:3768-3790` |
| 7 CRYSTAL | +200, checkpoint set, tile→0x88 | IMPLEMENTED `vs:3084-3136` |
| 9 EXIT | M-gated finish else block | IMPLEMENTED `vs:3594-3689` |
| 13–16, 21–28 hoops | collect/block matrix, big-ball refusal 13/14/15/16 | IMPLEMENTED `vs:1745-1848,3324-3337,4267-4279`, `cqc:617-732` |
| 29 EXTRA LIFE | +1000, `lives<5` cap, consume | IMPLEMENTED `vs:3158-3191` |
| 30–37 slopes | redirect | IMPLEMENTED `vs:1628-1708,3795` |
| 38 SPEED | `powerup=300`, block, no consume | IMPLEMENTED `vs:4025-4046,5044-5052` |
| 39–42 DEFLATORS | shrink if big | IMPLEMENTED `vs:4104-4110`, `player.c:602-614` |
| 43–46 PUMPERS | grow if 12, `b()`-gated, re-entrant walk | IMPLEMENTED `vs:4203-4211`, `player.c:615-687,806-968` |
| 47–50 GRAVITY / 51–54 JUMP | powerup=300 | IMPLEMENTED `vs:3841-3875,3949-3978` |
| water 0x40 bit | 4 gravity variants | IMPLEMENTED `vs:4450-4475,4864-4891` |

### 1.4 Death / lives / hoops / score — IMPLEMENTED

KillBall body + GodMode guard (`vs:2044-2113`, verifier `:17355`); pop = `q=7` + static popped
sprite (source has **no 5-frame popCntr** — native matches source); game-over at lives<0
(`vs:2808-2923`); respawn with checkpoint snapshot (`vs:2798-3059`); hoops +500 & unlock
(`vs:1850-1969, 1958-1960`); **score: +500/+200/+1000/+5000 all wired** (see summary).

### 1.5 Level 8 unseal + pad charge — IMPLEMENTED

Patch `level.c:204-225` called from both load routes; verifier `vs:15051-15124`.
Rubber charge: `v` latch producers (`vs:3724` first-blocker), A1 landing `t += ∓10`
(`vs:4732-4739`, `4959-4966`), jump consumes (`vs:5122-5128`) — matches `f.java:444,754-763,821-828`.

### 1.6 Top gameplay deviations

1. **G8** — **CLOSED** (2026-10-23). Runtime walks (death respawn, pumper growth, pumper recursion) reproduce Java's walk-side effects including the power-up state writes (STEP 38-S, owner decision reversed 2026-10-23), and the level-start constructor walk now runs inside `bounce_game_initialize_runtime_entry` against the fresh runtime copy with the production consumer `app_pumper_walk_hazard`, between copy installation and phase record — the native spelling of `f.java:126-131` (`CreateTiles` then `UseBigBall`). The inert pristine-shadow form (`bounce_player_use_big_ball_level`) is gone. Reachable cases covered: crystal near L6 pumper, thorn near L8 pumper, gravity/jump/speed during growth (`vs:4186-4190`), plus level-start walks on big-ball levels 3/5/11 (`BIGBALL ... phase=RUNTIME_INITIALIZED ... runtime=PRESENT`).
2. **G11** — HUD dirty flag `y` not modeled; stale native comments claim score/boostbar missing (they exist).
3. **G9** — E-buffer background wrap absent (rendering scope).
4. **G12** — deferred timer stop (documented, same-frame equivalent).

---

## 2. App flow / menus / forms / persistence / localization / input

### 2.1 Lifecycle & state machine — IMPLEMENTED

Splash 30-tick + key skip (`app_flow.c:351-376`), shutdown save on Exit/Escape/window-close
(`vs:28227-28284, 28398-28406, 28626-28627`), focus-loss = `hideNotify` (`vs:27376`),
transition table (`app_flow.c:270-349` — superset adds SETTINGS/ABOUT/GAME_END = DIFFERENT),
terminal-form capture→persist→refresh→release order (`vs:28152-28190` = `BounceGame.java:256-266`).

### 2.2 Menus & progression

| Feature | Status | Evidence |
|---|---|---|
| 4 original rows (Continue/New Game/High Score/Instructions) | IMPLEMENTED | `ui_shell.c:32-35,510-519` |
| Settings/About/Exit rows | DIFFERENT (extension) | `app_flow.h:136-147` |
| Continue gate `J==1/J==2` legs | IMPLEMENTED | `vs:8804-8855`, `app_flow.c:1379,1404` |
| **`K==1` live-session Continue** | **NOT IMPLEMENTED (G1)** | `vs:8949-9015` |
| Level select gate `>1` levels, unlock progression | IMPLEMENTED | `app_flow.c:487,1051` |
| **Fresh-install unlock = 2 vs Java 0** | **DEVIATES (G6, accepted)** | `app_flow.h:295` vs `BounceGame.java:270-286` |
| Hidden Read/Write RMS rows | NOT IMPLEMENTED (G14) | `BounceGame.java:248-253` |

### 2.3 Forms — IMPLEMENTED

High score (`ui_shell.c:2988`), Instructions (`:2801`), Game end (`:3864`) incl. NEW_HIGH_SCORE
row (`:3930,4278`), Game over split state (DIFFERENT), Level complete (`:4106` + OK `vs:27877`),
Level select (`:4455` + digit shortcuts = extension), Settings/About = extensions,
HUD zero-padding (`vs:6395-6540` = `e.java:436-454`).

### 2.4 Persistence — IMPLEMENTED (native superset)

3 Java records with matching semantics (record-1 max level, record-2 high score,
record-3 resume snapshot incl. tile deltas `persistence.h:172-176` and dyn-thorn offsets
`:183-188`; `b1` encoding matches Java `K`); save on death (`vs:2920`), exit/W1
(`:3627, :6932`), shutdown; continue J==1 live / J==2 record (`app_flow.c:1404/1379`).
Volume record 6 persisted (`persistence.c:1082,1140`) — **keyboard, pointer-slider, `q` and window-close edges (G7 resolved, `f895aaf`)**.
Language persisted (record 7) + autodetect (**G4 resolved**). FPS setting: cancelled, absent (correct).

### 2.5 Localization

Java: derives `microedition.locale` each launch, `lang.<locale>` → `lang.xx` fallback
(`Translation.java:39,55,57`). Native (since **G4 resolved**): boot detection from the POSIX
locale chain (`LC_ALL`→`LC_MESSAGES`→`LANG`, `bounce_locale_language_from_string`) mirrors the
per-launch derivation, and a persisted choice (`records.bin` record 7) overrides it — the
persistence is a native extension, since Java never persists a language. Native adds 5
languages (EN, zh-CN, zh-TW, th-TH, ID) + runtime switch (`app_flow.c:884-906`) = DIFFERENT;
placeholder substitution implemented (`ui_shell.c:1196,4051`).

### 2.6 Input & cheats

Directions/GAME_A/T9 all mapped (`vs:15472-15517, 27823-27826, 27606-27610`);
**GodMode `7-8-7-8-9-8` and af `7-8-7-8-9-9` match Java exactly** (`e.java:349-381`,
`vs:27131`); `#` af-gated gravity cheat (`vs:27726-27728`); W1 completion key (XK_c, af-gated);
**G5 resolved**: `1`/`3` level keys now match Java — the reload request is unconditional
(`d=true`), only the level warp is af-gated (`G5-LEVELKEYS` verify, end to end).
Escape/menu with persist superset (`vs:28299-28498`). QWERTY/mouse/digits/`q` = extensions.

### 2.7 Timer & high score — IMPLEMENTED

40 ms fixed tick (`game_timer.h:38` = `BounceTimer.java:17`), stop on death/complete/menu
(`game.c:999,1162,284`), start on new/continue (`game.c:909`); `checkData` → `note_score`
(`app_flow.c:1873-1891`), new-high-score flag set/read/cleared (`:1887,:1899,:1267`).

### Top flow gaps

**G1** K==1 Continue · (G4 resolved: detect + persist · G5 resolved: `1`/`3` request unconditional, warp af-gated · G6 decided: keep 2 · extensions: extra menu rows/settings/about — not gaps)

---

## 3. Rendering / audio / HUD / input device / debug

### 3.1 Rendering

| Feature | Status | Evidence |
|---|---|---|
| Clip 128×96 game / 128×128 HUD, camera bounds & scroll, world→screen | IMPLEMENTED | `vs:100-101, 5785, 6007, 7006` |
| 6 sprite transforms, mirror ball, exit palette, thorn 24×24 composite, tile draw incl. water theme, front/back sprites | IMPLEMENTED | `visual_assets.c:38-46,289,335-408,865-1079,1468` |
| Exit door animation (+4 → 24 → open flag) | IMPLEMENTED | `vs:6193-6257,6695-6699`, `visual_assets.c:1435` |
| HUD band/lives/rings/score/boost geometry + colors exact | IMPLEMENTED | `vs:6395-6557` (`e.java:175-187`) |
| ~~Second splash `nokiagames.png` never drawn; single 30-tick stage~~ | **NO GAP (G2)** — source is effectively single-stage: `SplashId = 1` at e.java:72 makes the case-0 stage dead code; native single-stage 31-tick + key-skip matches | `e.java:72,227-253,331-332`; `app_flow.c:355-366`, `ui_shell.c:2510-2536` |
| **Post-ball front-tile (hoop rim) pass** | **IMPLEMENTED (G3 resolved, `1ccd833`)** | `vs:7299+` vs `e.java:172` → `b.java:577-600` |
| Dirty-tile incremental redraw, E staging, BRICK border | NOT IMPLEMENTED, deliberate (G9) | `vs:7047-7088` |
| Device font glyphs | DEVIATES → placeholder 5×7 (G10) | `visual_assets.c:1288,1352` |

### 3.2 Audio — IMPLEMENTED

All 3 sounds triggered from gameplay: UP (`vs:2244,2304,3379,4317,27689`),
PICKUP (`vs:3134,3189,3687,3874,3977,4045`), POP (`vs:2104,22247,22286,27711`);
central sink + per-frame gate (`vs:37757-37771, 37836-37851`); byte-exact RTPL/.ott decode
(`rtpl_decoder.h`, `audio_synth.c:1076,1503`); ALSA/Null backends; runtime gain sync
(`vs:2734-2750`, `audio_player.c:331-354`). G7 (slider/quit/close volume commits)
resolved in `f895aaf`.

### 3.3 Pause — wired (native extension)

`P` → PAUSE event → flow/game state → ticks stop (`vs:8064-8069`) → overlay (`vs:7694,7980`) →
`P` resumes (`vs:28500-28512`). **G13:** `input.pause` flag has no consumer; Escape-from-pause
stale-runtime documented (`vs:28340-28342`). Java has no pause at all (`Bounce.java:20` stub).

### 3.4 Input device — IMPLEMENTED

Arrows/WASD, top-row + keypad digits both NumLock states, Enter/KP_Enter/Space confirm,
auto-repeat filter preserving press-per-press semantics (`vs:28686-28689`), focus loss.
Extensions: mouse UI, `Escape`, `q`, T9 controller mode.

### 3.5 Debug tooling — native-only extensions

`NBB_DEBUG` 0–3 tracker, camera diagnostic, render trace, hidden input panel, screenshots mode,
`--check` self-test harnesses. No Java counterpart (Java debug flags all `false`).

### Top rendering/audio gaps

**G9** dirty-tile/E-buffer/border · **G10** fonts.
*(G2 resolved as no-effective-gap: `e.java:72` makes the case-0 stage dead code. G3 and G7 resolved: `1ccd833`, `f895aaf`.)*

---

## 4. Consolidated work list (priority order)

| Priority | ID | Gap | Effort |
|---|---|---|---|
| 2 | G2 | ~~Render `nokiagames.png` splash as first stage~~ — **no effective gap**: source is single-stage (`SplashId = 1`, case-0 dead code); closed by analysis, anti-regression comment added | done (analysis) |
| 2 | G1 | `K==1` live-session Continue route | Medium |
| 2 | G4 | ~~Locale autodetect (+ optional language persistence)~~ — **done**: detect + persist (owner decision 2026-10-23), record 7 + boot precedence + `LANG-DETECT/PERSIST/BOOT` verifies | done |
| 3 | G5 | ~~Un-gate `1`/`3` keys to match Java (`d=true` unconditional)~~ — **done** (2026-10-23): request unconditional, warp af-gated, latch renamed `level_reload_requested`, `verify_cheat_level_keys` proves it end to end | done |
| 3 | G6 | ~~Fresh-install unlock count 2 → 0~~ — **decided: keep 2** (accepted deviation) | done (decision) |
| 3 | G8 | Settle-walk side effects — **done**: runtime walks (STEP 38-S: state-write tiles 38/47-54 reported → shared producer helpers) and the constructor sub-item (level-start walk moved to the runtime-init seam with the production consumer; one walk/one consumer; entry-boundary verifies restructured to assert pre-walk at PLAYER_INITIALIZED and post-walk at RUNTIME_INITIALIZED) | done |
| 4 | G13 | Escape-from-pause runtime case; wire `input.pause` consumer | Small |
| 4 | G10 | Font glyph fidelity (device font recreation) | Large, cosmetic |
| 5 | G9, G11, G12, G14 | Deliberate/structural deviations — document, likely accept | — |

**Resolved since previous backlog (verified this audit):** BL-001 score wiring ✅,
BL-002 max-lives cap ✅, BL-003 pickup sound ✅, BL-005 FPS setting → cancelled (correct, Java never had it) ✅,
BL-006 volume 0–100 added + persisted ✅, BL-010 pause → wired via P ✅,
BL-011 GodMode sequence → matches original Java digit-for-digit ✅ (old note compared against LKT Pro mod, not BASE).
**Resolved in this pass:** G3 post-ball hoop-rim pass ✅ (`1ccd833`),
G7 volume commit on slider/quit/close edges ✅ (`f895aaf`).

**Notes**
- Old `audit/lkt-pro-parity/` compares against the *LKT Pro mod* reference; this audit compares against **BASE-001 original Java**. Differences called out there (invincibility cheat, profiling, QWERTY mapping, FPS, 156×96 buffer) are mod/platform matters, not original-source parity gaps.
- Several native comments claiming score/boostbar/"no native counterpart" are stale (`vs:6304-6317`) — code implements them; comment cleanup is worthwhile.
