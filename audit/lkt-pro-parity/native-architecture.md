# Native Architecture — Nokia Bounce Linux Reconstruction

**Project:** `/home/antomay/Projects/nokia-bounce-linux`  
**Language:** C11 (`-std=c11 -Wall -Wextra -Werror -pedantic`)  
**Build:** Single-command Makefile build → `native/app/bounce_vertical_slice`  
**Total source:** ~71,500 lines across 40+ C/H files

---

## 1. High-Level Architecture

The native implementation is a **single-binary, single-process** C application with a layered architecture:

```
┌─────────────────────────────────────────────────────┐
│  vertical_slice.c — Integration layer (main loop,   │
│  X11 event dispatch, frame timing, CLI flags)       │
├─────────────────────────────────────────────────────┤
│  app_flow.c/h — Application state machine (menus,   │
│  splash, settings, level selection, game entry)     │
│  game.c/h — Gameplay context (level, player,       │
│  timer, score, lives, progression)                  │
│  ui_shell.c/h — UI rendering (all screens)         │
│  locale.c/h — Localization (5 languages, ~120 keys) │
│  input_state.c/h — Input handling (35 sources)      │
│  persistence.c/h — Save/load (custom binary format) │
│  audio_*.c/h — Audio system (RTPL decode, synth,   │
│  ALSA/Null backends, player thread)                │
│  debug_tracker.c/h — Debug output (NBB_DEBUG env)  │
├─────────────────────────────────────────────────────┤
│  player.c/h — Player state and physics helpers     │
│  collision_query.c/h — Pure collision predicates    │
│  game_timer.c/h — Game timer (40ms canonical tick) │
├─────────────────────────────────────────────────────┤
│  level.c/h — Level parsing and runtime tiles       │
│  level_loader.c/h — File I/O for levels             │
│  dyn_thorns.c/h — Dynamic thorn animation state     │
├─────────────────────────────────────────────────────┤
│  renderer/ — Rendering pipeline                     │
│  surface.c/h — 128x128 logical surface              │
│  image.c/h — 0xAARRGGBB image data                  │
│  renderer.c/h — Clip, fill_rect, blit primitives    │
│  presentation.c/h — X11 presentation layer          │
│  pipeline.c/h — Render pipeline                     │
│  e_buffer.c/h — E-buffer for tile rendering         │
├─────────────────────────────────────────────────────┤
│  asset.c/h — Asset identity and PNG decode          │
│  visual_assets.c/h — Sprite atlas and tile rendering│
│  asset_inventory.c/h — Asset coverage verification  │
│  script_font.c/h — 5x7 glyph bitmaps (CJK, Thai)    │
│  rtpl_decoder.c/h — Nokia RTPL sound format decoder │
└─────────────────────────────────────────────────────┘
```

---

## 2. Entry Point and Main Loop

### 2.1 Entry Point

`vertical_slice.c:36204` — `int main(int argc, char **argv)`

- Parses CLI flags: `--check`, `--assets`, `--save=DIR`, `--save-path`, `--save-plant`, `--screenshots=DIR`, `--audio-device`, `--camera-diagnostic`, `--vertical-diagnostic`, `--render-trace`, `--live-status`
- Initializes game context via `bounce_game_init()`
- Loads persistence data
- Enters frame loop

### 2.2 Frame Loop

The frame loop drives:
1. X11 event dispatch (keyboard, mouse)
2. Application tick (splash timer, state transitions)
3. Game tick (player physics, collision, camera)
4. Render (UI shell or gameplay scene)

### 2.3 Gameplay Entry Staging

The native implementation uses a **staged gameplay entry** with 9 phases (`app_flow.h:394-434`):

| Phase | Constant | Description |
|---|---|---|
| NONE | `BOUNCE_GAMEPLAY_ENTRY_PHASE_NONE` | No entry started |
| START_LEVEL_RECORDED | `BOUNCE_GAMEPLAY_ENTRY_PHASE_START_LEVEL_RECORDED` | START_LEVEL(N) recorded |
| INIT_METADATA_RECORDED | `BOUNCE_GAMEPLAY_ENTRY_PHASE_INIT_METADATA_RECORDED` | New-game metadata recorded |
| LEVEL_LOADED | `BOUNCE_GAMEPLAY_ENTRY_PHASE_LEVEL_LOADED` | Level parsed and owned |
| PLAYER_INITIALIZED | `BOUNCE_GAMEPLAY_ENTRY_PHASE_PLAYER_INITIALIZED` | Player data initialized |
| RUNTIME_INITIALIZED | `BOUNCE_GAMEPLAY_ENTRY_PHASE_RUNTIME_INITIALIZED` | Mutable tile copy + dyn thorn state owned |
| CAMERA_INITIALIZED | `BOUNCE_GAMEPLAY_ENTRY_PHASE_CAMERA_INITIALIZED` | Camera l/k/v initialized |
| ACTIVATED | `BOUNCE_GAMEPLAY_ENTRY_PHASE_ACTIVATED` | Gameplay surface current |
| TIMER_INITIALIZED | `BOUNCE_GAMEPLAY_ENTRY_PHASE_TIMER_INITIALIZED` | 40ms timer started |

---

## 3. Game State Machine

### 3.1 Application States

`app_flow.h:19-61` — 15 states:

| State | Value | Description |
|---|---|---|
| `BOUNCE_APP_STATE_MENU` | 0 | Main menu |
| `BOUNCE_APP_STATE_GAMEPLAY` | 1 | Active gameplay |
| `BOUNCE_APP_STATE_PAUSE` | 2 | Paused (unreachable — no consumer) |
| `BOUNCE_APP_STATE_GAME_OVER` | 3 | Game over (death) |
| `BOUNCE_APP_STATE_LEVEL_COMPLETE` | 4 | Level complete screen |
| `BOUNCE_APP_STATE_SPLASH` | 5 | Splash screen |
| `BOUNCE_APP_STATE_ACTION_BOUNDARY` | 6 | Explicit action boundary |
| `BOUNCE_APP_STATE_INSTRUCTIONS` | 7 | Instructions screen |
| `BOUNCE_APP_STATE_HIGH_SCORE` | 8 | High score screen |
| `BOUNCE_APP_STATE_LEVEL_SELECTION` | 9 | Level selection |
| `BOUNCE_APP_STATE_GAMEPLAY_ENTRY` | 10 | Gameplay entry staging |
| `BOUNCE_APP_STATE_LEVEL_LOADED` | 11 | Level loaded, nothing else started |
| `BOUNCE_APP_STATE_SETTINGS` | 12 | Settings (native extension) |
| `BOUNCE_APP_STATE_ABOUT` | 13 | About screen (native extension) |
| `BOUNCE_APP_STATE_GAME_END` | 14 | Game complete (win) |

### 3.2 Game States

`game.h:40-77` — Mirrors app states plus gameplay-specific states:

| State | Value | Description |
|---|---|---|
| `BOUNCE_GAME_STATE_SPLASH` | 0 | Splash |
| `BOUNCE_GAME_STATE_MENU` | 1 | Menu |
| `BOUNCE_GAME_STATE_PLAYING` | 2 | Playing |
| `BOUNCE_GAME_STATE_LEVEL_COMPLETE` | 3 | Level complete |
| `BOUNCE_GAME_STATE_DEAD` | 4 | Dead (waiting respawn) |
| `BOUNCE_GAME_STATE_RESPAWN` | 5 | Respawning |
| `BOUNCE_GAME_STATE_GAME_OVER` | 6 | Game over |
| `BOUNCE_GAME_STATE_PAUSED` | 7 | Paused |
| `BOUNCE_GAME_STATE_UNKNOWN` | 8 | Unknown |
| `BOUNCE_GAME_STATE_ACTION_BOUNDARY` | 9 | Action boundary |
| `BOUNCE_GAME_STATE_INSTRUCTIONS` | 10 | Instructions |
| `BOUNCE_GAME_STATE_HIGH_SCORE` | 11 | High score |
| `BOUNCE_GAME_STATE_LEVEL_SELECTION` | 12 | Level selection |
| `BOUNCE_GAME_STATE_GAMEPLAY_ENTRY` | 13 | Gameplay entry |
| `BOUNCE_GAME_STATE_LEVEL_LOADED` | 14 | Level loaded |
| `BOUNCE_GAME_STATE_SETTINGS` | 15 | Settings |
| `BOUNCE_GAME_STATE_ABOUT` | 16 | About |
| `BOUNCE_GAME_STATE_GAME_END` | 17 | Game end (win) |

---

## 4. Player System

### 4.1 Player State

`player.h:57-115` — `BouncePlayer` struct:

| Field | Type | Java Equivalent | Description |
|---|---|---|---|
| `TODO_unkX` | `int32_t` | `f.TODO_unkX` | Buffer X position |
| `TODO_unkY` | `int32_t` | `f.TODO_unkY` | Buffer Y position |
| `ballSize` | `int32_t` | `f.ballSize` | Ball size (12 or 16) |
| `p` | `int32_t` | `f.p` | Jump offset |
| `horizontal_velocity` | `int32_t` | `f.l` | Horizontal velocity |
| `o` | `int32_t` | `f.o` | Vertical velocity |
| `t` | `int32_t` | `f.t` | Gravity-related state |
| `powerUpGravity` | `int32_t` | `f.powerUpGravity` | Gravity power-up counter |
| `TODO_somePowerUp1` | `int32_t` | `f.TODO_somePowerUp1` | Speed bonus counter (tile 38) |
| `TODO_somePowerUp3` | `int32_t` | `f.TODO_somePowerUp3` | Gravity/jump bonus counter |
| `C` | `int32_t` | `f.C` | Unknown |
| `z` | `int32_t` | `f.z` | Death counter |
| `q` | `int32_t` | `f.q` | Unknown |
| `m` | `bool` | `f.m` | Grounded flag |
| `u` | `bool` | `f.u` | Contact flag |
| `v` | `bool` | `f.v` | Rubber-wall latch |
| `current_ball_mode` | `BouncePlayerBallMode` | — | REGULAR or BIG |
| `initialized` | `bool` | — | Player initialized flag |

### 4.2 Player Lifecycle

| Function | Location | Description |
|---|---|---|
| `bounce_player_reset()` | `player.c:19` | Reset player state |
| `bounce_player_tick_entry()` | `player.c:26` | f.java:652-669 prologue |
| `bounce_player_initialize_from_level()` | `player.c:102` | Init from level header |
| `bounce_player_use_big_ball()` | `player.c:979` | f.UseBigBall() 6-direction walk |
| `bounce_player_use_big_ball_level()` | `player.c:1002` | Constructor probe against pristine level |
| `player_apply_slope_response()` | `player.c:236` | f.java:234-270 slope response |

### 4.3 Checkpoint

`game.h:151-153` — Mutable crystal checkpoint:
- `checkpoint_tile_x`, `checkpoint_tile_y`, `checkpoint_ball_size`
- Set at player entry from level spawn
- Replaced by every tile-7 pickup
- Never cleared by death or respawn

---

## 5. Collision System

### 5.1 Pure Query

`collision_query.c` — Pure, mask-backed collision predicates:

| Function | Location | Description |
|---|---|---|
| `bounce_collision_query()` | `collision_query.c:1103` | Complete runtime tile query |
| `bounce_collision_query_level()` | `collision_query.c:1135` | Pristine level query |
| `bounce_collision_tile_overlaps()` | `collision_query.c:811` | Single tile overlap |
| `bounce_collision_tile_shape_overlaps()` | `collision_query.c:896` | f.b() verdict |
| `java_shape_overlaps()` | `collision_query.c:252` | f.java:369-385 shape overlap |
| `java_hoop_shape_overlaps()` | `collision_query.c:300` | f.java:387-421 hoop overlap |
| `slope_mask_overlaps()` | `collision_query.c:363` | Slope mask overlap |
| `wall_mask_overlaps()` | `collision_query.c:172` | Wall mask overlap |
| `java_interaction_collision()` | `collision_query.c:569` | f.java:433-645 verdicts |
| `java_dyn_thorn_overlaps()` | `collision_query.c:503` | f.java:463-473 dyn thorn |
| `java_thorn_rect_overlaps()` | `collision_query.c:131` | f.b() inclusive test |

### 5.2 Ball Masks

- `bounce_regular_collision[12][12]` — 12x12 ball pixel mask
- `bounce_big_ball_collision[16][16]` — 16x16 ball pixel mask
- `bounce_slope_collision[12][12]` — 12x12 slope mask

---

## 6. Level System

### 6.1 Level Structure

`level.h:12-21` — `BounceLevel` (opaque):
- Header bytes 0-7: `initial_X, initial_Y, isBigBall, exit_X, exit_Y, HoopsTotal, width, height`
- Tile region: `width * height` row-major bytes
- Thorn payload: `thorn_count` byte + `8 * thorn_count` bytes

### 6.2 Runtime Tiles

`level.h:215-299` — `BounceRuntimeLevelTiles` (opaque):
- Mutable copy of level tiles
- Dynamic thorn animation state
- `bounce_runtime_level_tiles_get/set()` — read/write cells
- `bounce_runtime_level_tiles_update_dyn_thorns()` — advance animation
- `bounce_runtime_level_tiles_restore_dyn_thorns()` — restore from save

### 6.3 Dynamic Thorns

`dyn_thorns.c` — Dynamic thorn animation:
- 8-byte records: `w_x, w_y, ae_x, ae_y` (all int16)
- `bounce_dyn_thorns_update()` — advance one step
- `bounce_dyn_thorns_test_point_inside()` — point-in-thorn test

---

## 7. Rendering System

### 7.1 Renderer Pipeline

```
BounceSurface (128x128) → BounceRenderer (clip, fill, blit) → BounceImage (0xAARRGGBB)
```

| Component | Location | Description |
|---|---|---|
| `BounceSurface` | `surface.h` | 128x128 logical surface |
| `BounceImage` | `image.h` | 0xAARRGGBB pixel data |
| `BounceRenderer` | `renderer.h` | Clip, fill_rect, blit primitives |
| `BouncePresentation` | `presentation.h` | X11 presentation layer |
| `BouncePipeline` | `pipeline.h` | Render pipeline |
| `BounceEBuffer` | `e_buffer.h` | E-buffer for tile rendering |

### 7.2 Visual Assets

`visual_assets.c` — Sprite atlas and tile rendering:
- `BounceVisualAssets` — atlas + extracted sprites
- `bounce_visual_assets_draw_tile()` — tile rendering by ID
- `bounce_visual_assets_draw_exit_door()` — 24x48 exit door sprite
- `bounce_visual_assets_draw_dyn_thorn()` — 24x24 dynamic thorn composite
- `bounce_visual_assets_draw_hud_score()` — HUD score display

### 7.3 Asset Inventory

`asset_inventory.c` — Asset coverage verification:
- Tracks all resources: levels, images, sounds, languages, manifest
- `bounce_asset_inventory_check()` — verifies every inventoried resource exists

---

## 8. UI Shell

### 8.1 Screens

| Screen | Location | Description |
|---|---|---|
| Splash | `ui_shell.c:2271` | Splash screen rendering |
| Menu | `ui_shell.c:2286` | Main menu rendering |
| Instructions | `ui_shell.c:2563` | Instructions screen |
| High Score | `ui_shell.c:2750` | High score screen |
| Settings | `ui_shell.c:2830` | Settings pages |
| About | `ui_shell.c:3961` | About screen |
| Game End | `ui_shell.c:3495` | Game end (win) screen |
| Level Complete | `ui_shell.c:3737` | Level complete screen |
| Game Over | `ui_shell.c:3860` | Game over screen |
| Level Selection | `ui_shell.c:4086` | Level selection screen |
| Level Banner | `ui_shell.c:4035` | Level name banner |
| Gameplay Entry | `ui_shell.c:4222` | Gameplay entry staging screen |

### 8.2 UI Input

| Function | Location | Description |
|---|---|---|
| `bounce_ui_shell_handle_press()` | `ui_shell.h:186` | Apply UI edge |
| `bounce_ui_shell_handle_click()` | `ui_shell.h:156` | Mouse click → UI edge |
| `bounce_ui_shell_handle_number()` | `ui_shell.h:180` | Numeric key input |

### 8.3 Color Profiles

`ui_shell.h:32-67` — 13 runtime UI color profiles:
- Default, Green LCD, Sepia, Inverted Green LCD, Amber/Orange, Deep Navy/Cream, Forest Green/Pale Mint, Burgundy/Soft Blush, Charcoal/Electric Blue, Classic Nokia Blue, Purple Haze, Arctic White, Dark Teal/Ivory

---

## 9. Audio System

### 9.1 Architecture

```
BounceAudioEvent → BounceAudioPlayer → BounceAudioBackend → ALSA / Null
```

| Component | Location | Description |
|---|---|---|
| `BounceAudioEvent` | `audio_event.h:85-91` | Event identity (NONE, UP, PICKUP, POP) |
| `BounceAudioPlayer` | `audio_player.h:80-206` | Bounded FIFO, dedicated thread |
| `BounceAudioBackend` | `audio_backend.h:67-212` | Function table, S16_LE, 44100Hz |
| `BounceAudioBackendNull` | `audio_backend_null.h:42-85` | Silent backend |
| `BounceAudioBackendALSA` | `audio_backend_alsa.c` | ALSA PCM backend |
| `bounce_synth_render_plan()` | `audio_synth.h:272-337` | Note/rest segment plan |
| `bounce_synth_render_pcm()` | `audio_synth.h:340-352` | PCM synthesis |
| `bounce_synth_prepare()` | `audio_synth.h:354-370` | Identity→resource→decode→plan |

### 9.2 RTPL Decoder

`rtpl_decoder.h:63-311` — Full Smart Messaging Rev 2.0.0 decoder:
- Decodes 3 original `.ott` assets
- Produces PCM for synthesis

---

## 10. Persistence System

### 10.1 Container Format

`persistence.c:1-106` — Custom binary container:
- Magic: "BNCPRST1"
- Version 1: 20 bytes (2 records)
- Version 2: Directory of (record id, payload length) pairs

### 10.2 Records

| Record | ID | Content | Evidence |
|---|---|---|---|
| Max Levels | 1 | `int32_t` | `persistence.c:497` |
| High Score | 2 | `int32_t` | `persistence.c:497` |
| Game State | 3 | `BouncePersistenceRecord3` | `persistence.c:655` |
| Theme Index | 4 | `int32_t` | `persistence.c:933,1053` |
| T9 Input | 5 | `int32_t` | `persistence.c:979,1032` |

### 10.3 Path Resolution

`persistence.c:409-456` — Resolution order:
1. `$BOUNCE_SAVE_DIR/bounce/records.bin` (if override set)
2. `$XDG_DATA_HOME/bounce/records.bin` (standard)
3. `$HOME/.local/share/bounce/records.bin` (default)

---

## 11. Input System

### 11.1 Input Sources

`input_state.h:15-163` — 35 sources:

| Range | Sources | Description |
|---|---|---|
| 0-7 | Arrow keys + WASD | Directional movement |
| 8-17 | NUM0-NUM9 | Numeric keypad digits |
| 18 | PAUSE | Pause (unreachable) |
| 19-21 | CONFIRM_RETURN/KP_ENTER/SPACE | Confirm/select |
| 22 | ESCAPE | Back/escape |
| 23 | QUIT | Quit |
| 24 | HASH | Cheat gravity (af-gated) |
| 25-34 | KP_UP/DOWN/LEFT/RIGHT + diagonals | Keypad navigation |

### 11.2 Alias System

`input_state.c:26-83` — Distant bit aliases:
- Each keypad source is added to an existing action's source set
- `held_sources` bitmask keeps simultaneous aliases independent
- Releasing one alias does not clear another still held

---

## 12. Debug System

### 12.1 Debug Tracker

`debug_tracker.h:14-230` — `NBB_DEBUG` environment variable:

| Level | Value | Output |
|---|---|---|
| Disabled | unset or `0` | Nothing |
| Events | `1` | new_game, level_start/complete, game_over/end, death, respawn, checkpoint, flow_change, tile |
| + State | `2` | score, lives, ball_size, death tick ladder |
| + Verbose | `3` | Per-tick player state line (~25 lines/s) |

### 12.2 CLI Diagnostics

| Flag | Description |
|---|---|
| `--camera-diagnostic` | Camera state visualization |
| `--vertical-diagnostic` | Vertical slice diagnostic |
| `--render-trace` | Render tracing |
| `--live-status` | Live status output |

---

## 13. Safe Integration Points for Future Features

### 13.1 Score System

The score field (`game.h:206`) and `bounce_game_add_score()` (`game.c:1487`) are the integration points for wiring point values:
- Hoop collection: +500
- Gem collection: +200
- Life gem: +1000
- Level exit: +5000

### 13.2 Audio Events

The audio event system (`audio_event.h:85-91`) is the integration point for sound effects:
- `BOUNCE_AUDIO_EVENT_UP` — hoop collected
- `BOUNCE_AUDIO_EVENT_PICKUP` — gem/power-up collected
- `BOUNCE_AUDIO_EVENT_POP` — ball popped

### 13.3 Collision Interaction

The `java_interaction_collision()` function (`collision_query.c:569`) is the integration point for tile interaction behaviors. It currently returns verdicts but does not trigger side effects.

### 13.4 UI Shell

The UI shell (`ui_shell.c`) is the integration point for new screens. Each screen has a dedicated render function following the pattern `bounce_ui_shell_render_*()`.

### 13.5 Persistence

The persistence system (`persistence.c`) is the integration point for new save data. The version-2 container format supports additional records by adding new record IDs.

### 13.6 Input

The input system (`input_state.c`) is the integration point for new input sources. The alias system allows adding new sources that map to existing actions.

### 13.7 Cheat System

The cheat system (`game.h:244-288`) is the integration point for new cheats. The `af` latch and `af_reload_requested` flag provide the pattern for af-gated features.

---

## 14. Key Design Patterns

### 14.1 Staged Gameplay Entry

The native implementation breaks gameplay entry into 9 explicit phases, each with a record function. This allows the UI to show progress and prevents partial initialization.

### 14.2 Pure Collision Query

The collision system is a pure predicate — it returns a boolean verdict without side effects. This makes it testable and reusable.

### 14.3 Immutable Level + Mutable Runtime Tiles

The level is parsed once into an immutable `BounceLevel`. A separate `BounceRuntimeLevelTiles` mutable copy is created for gameplay. This separation allows save/restore without modifying the original.

### 14.4 One-Shot Flags

Many flags are "one-shot" — set by one consumer and cleared by reading. This prevents stale state from carrying into later operations.

### 14.5 Source Aliases

Input sources use a bitmask alias system where multiple physical keys can map to the same logical action while remaining independently held.

### 14.6 Native Extensions Clearly Marked

Features with no recovered source counterpart are clearly marked as "NATIVE EXTENSION" in comments, preventing confusion between reconstruction and invention.
