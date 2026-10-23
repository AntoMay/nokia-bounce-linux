# Feature Matrix — Native C vs LKT Pro Mod

**Status legend:** MATCH = feature present and behaviorally equivalent · PARTIAL = feature present but incomplete or behaviorally different · MISSING = feature absent from native · DIFFERENT = feature exists but behaves differently · UNKNOWN = insufficient evidence to determine

**Reachability:** REACHABLE = feature is reachable through normal gameplay · UNREACHABLE = feature exists in code but cannot be reached · UNVERIFIED = reachability not confirmed

---

## 1. Startup / Initialization / Shutdown

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Entry point | `Bounce.java:14-38` — MIDlet lifecycle | `vertical_slice.c:36204` — `main()` | DIFFERENT | REACHABLE | Native uses CLI args; mod uses MIDlet |
| App initialization | `BounceUI.java:101-116` — loads settings, language, game data, creates canvas | `game.c:127` — `bounce_game_init()` + `app_flow.c:217` — `bounce_app_flow_init()` | MATCH | REACHABLE | Both load settings + game data at startup |
| Settings load | `BounceUI.java:567-601` — reads `bounceSettingsRMS` (keyboard, fps, volume, language) | `persistence.c:497` — `bounce_persistence_load()` + theme/T9 records | PARTIAL | REACHABLE | Native persists theme + T9 but NOT fps/volume/language |
| Game data load | `BounceUI.java:341-410` — reads `bounceRMS` (3 records) | `persistence.c:655` — `bounce_persistence_load_record3()` | MATCH | REACHABLE | Both load best level, best score, save snapshot |
| Splash screen | `BounceCanvas.java:62-80` — 2 splashes (Nokia → Bounce), 30 frames each | `app_flow.c:348` — 30-tick splash → menu | MATCH | REACHABLE | Both show splash then menu |
| Splash skip | `BounceCanvas.java:466-467` — any key skips | `app_flow.c:366` — `bounce_app_flow_request_splash_skip()` | MATCH | REACHABLE | |
| Shutdown / destroy | `Bounce.java:31-38` — `destroyApp()` saves game data, stops timer | `game.c:1609` — `bounce_game_shutdown()` | MATCH | REACHABLE | Both save on exit |
| Error handling | Java exceptions → catch-all, game continues | C return codes → `-1` on failure, `EXIT_FAILURE` | DIFFERENT | REACHABLE | Different paradigms, same intent |

## 2. Game State Machine

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| State constants | `BounceConst.java:212-258` — 6 states (INIT, READY, GAME_PLAYING, START, GAME_OVER, GAME_STARTING) | `app_flow.h:19-61` — 15 states (MENU, GAMEPLAY, PAUSE, GAME_OVER, LEVEL_COMPLETE, SPLASH, ACTION_BOUNDARY, INSTRUCTIONS, HIGH_SCORE, LEVEL_SELECTION, GAMEPLAY_ENTRY, LEVEL_LOADED, SETTINGS, ABOUT, GAME_END) | DIFFERENT | REACHABLE | Native has more states (Settings, About, Game End are native extensions) |
| State transitions | `BounceUI.java:161-168, 240-251, 260-262, 284-285` | `app_flow.c:267` — `bounce_app_flow_apply()` transition table | MATCH | REACHABLE | Both use explicit transition tables |
| Main loop | `BounceCanvas.java:320-463` — `run()` with splash, gameplay, scroll, render | `vertical_slice.c` — frame loop with tick dispatch | MATCH | REACHABLE | |
| Pause | `BounceConst.java:254` — STATE_GAME_PLAYING; no explicit pause state | `app_flow.h:22` — BOUNCE_APP_STATE_PAUSE; `input_state.h:171` — pause flag | PARTIAL | UNREACHABLE | Native pause flag has no consumer; mod has no pause state |
| Game over (death) | `BounceCanvas.java:368-371` — `numLives < 0` → `gameOver(false)` | `game.c:1599` — `bounce_game_mark_game_over()` | MATCH | REACHABLE | |
| Game over (win) | `BounceCanvas.java:454-455` — `mLevelNum > 11` → `gameOver(true)` | `app_flow.h:60` — BOUNCE_APP_STATE_GAME_END | MATCH | REACHABLE | |
| Level complete | `BounceCanvas.java:447-461` — `mLeaveGame=true`, `mLevelNum++`, +5000 | `game.c:1228` — `bounce_game_mark_level_complete()` | MATCH | REACHABLE | |
| Continue from level complete | `BounceUI.java:284-285` — `displayGame(false, 0)` | `game.c:1503` — `bounce_game_continue_level()` | MATCH | REACHABLE | |

## 3. Player / Ball

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Ball position | `Ball.java:19-20` — `xPos, yPos` (buffer), `globalBallX/Y` (tile) | `player.h:58-59` — `TODO_unkX, TODO_unkY` | MATCH | REACHABLE | |
| Ball speed | `Ball.java:24-25` — `xSpeed, ySpeed` clamped ±150 | `player.h:62-63` — `horizontal_velocity, o` | MATCH | REACHABLE | |
| Direction mask | `Ball.java:26` — bitmask 8=UP, 4=DOWN, 2=RIGHT, 1=LEFT | `input_state.h:165-178` — left/right/up/down booleans | DIFFERENT | REACHABLE | Different representation, same semantics |
| Horizontal accel | `Ball.java:1171-1179` — ±6 accel, ±4 friction, max 50 (100 with bonus) | `player.h:85` — `TODO_somePowerUp1` (speed bonus) | PARTIAL | REACHABLE | Speed bonus consumer exists; accel/friction in vertical_slice.c |
| Gravity | `Ball.java:1031-1075` — normal acc=4 max=80; large acc=3 max=38; water variants | `player.h:64-65` — `t`, `powerUpGravity` | PARTIAL | REACHABLE | Gravity values in vertical_slice.c |
| Gravity flip (upside-down) | `Ball.java:1064-1074` — inverts gravity | `player.h:86` — `TODO_somePowerUp3` | PARTIAL | REACHABLE | Producer exists; consumer in vertical_slice.c |
| Jump | `Ball.java:1183-1186` — `ySpeed = -67 + jumpOffset` | `player.h:61` — `p` (jump offset) | PARTIAL | REACHABLE | Jump logic in vertical_slice.c |
| Jump bonus | `Ball.java:1180-1182` — -80 with jump bonus | `player.h:86` — `TODO_somePowerUp3` | PARTIAL | REACHABLE | |
| Ramp movement | `Ball.java:1113-1124, 1196-1209` — diagonal slide | `player.c:236` — `player_apply_slope_response()` | MATCH | REACHABLE | |
| Rubber bounce | `Ball.java:1128-1131` — `jumpOffset += ±10` | `player.h:110` — `v` (rubber-wall latch) | MATCH | REACHABLE | |
| Screen wrap | `Ball.java:1213-1224` — wrap at edges | `vertical_slice.c` — wrap logic | MATCH | REACHABLE | |
| Ball states (normal/dead/popped) | `BounceConst.java:23-25` — 3 states | `player.h:88` — `z` (death counter), `player.h:111` — `current_ball_mode` | DIFFERENT | REACHABLE | Different state representation |
| Pop animation | `Ball.java:515-527` — 5-frame pop, decrements lives | `game.c:1206` — `bounce_game_mark_dead()` | PARTIAL | REACHABLE | Pop animation frames not explicitly modeled |
| Ball sizes (12/16) | `BounceConst.java:17,21` — NORMAL=12, ENLARGED=16 | `level.h:45-48` — REGULAR=12, BIG=16 | MATCH | REACHABLE | |
| Big-ball growth | `Ball.java:462-501` — 8-direction push out of walls | `player.c:979` — `bounce_player_use_big_ball()` | MATCH | REACHABLE | |
| Big-ball constructor probe | `Ball.java:168-207` — `UseBigBall()` | `player.c:1002` — `bounce_player_use_big_ball_level()` | MATCH | REACHABLE | |
| Ball rendering | `Ball.java:1228-1241` — `paint()` with wrap | `visual_assets.c` — ball sprite rendering | MATCH | REACHABLE | |
| Dirty tile marking | `Ball.java:1243-1276` — `dirtyTiles()` | `vertical_slice.c` — tile dirty tracking | MATCH | REACHABLE | |

## 4. Collision

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Collision query | `Ball.java:424-460` — `collisionDetection()` | `collision_query.c:1103` — `bounce_collision_query()` | MATCH | REACHABLE | |
| Square tile collision | `Ball.java:580-623` — `squareCollide()` | `collision_query.c:172` — `wall_mask_overlaps()` | MATCH | REACHABLE | |
| Triangle collision | `Ball.java:625-693` — `triangleCollide()` | `collision_query.c:363` — `slope_mask_overlaps()` | MATCH | REACHABLE | |
| Thin tile collision | `Ball.java:695-731` — `thinCollide()` | `collision_query.c:131` — `java_thorn_rect_overlaps()` | MATCH | REACHABLE | |
| Edge/ramp collision | `Ball.java:733-773` — `edgeCollide()` | `collision_query.c:252` — `java_shape_overlaps()` | MATCH | REACHABLE | |
| Master tile handler | `Ball.java:775-1016` — `testTile()` dispatches by tile ID | `collision_query.c:569` — `java_interaction_collision()` | MATCH | REACHABLE | |
| Triangle bounce redirect | `Ball.java:535-578` — `redirectBall()` | `player.c:236` — `player_apply_slope_response()` | MATCH | REACHABLE | |
| Dynamic thorn collision | `Ball.java:829-835` — rect collide with spike | `collision_query.c:485-567` — `java_dyn_thorn_overlaps()` | MATCH | REACHABLE | |
| Hoop shape overlap | `Ball.java:387-421` — `f.b()` verdict | `collision_query.c:300` — `java_hoop_shape_overlaps()` | MATCH | REACHABLE | |
| Ball masks (12x12, 16x16) | `Ball.java:42-368` — `SMALL_BALL_DATA`, `LARGE_BALL_DATA` | `collision_query.c:17-50` — `bounce_regular_collision[12][12]`, `bounce_big_ball_collision[16][16]` | MATCH | REACHABLE | |
| Slope masks | `Ball.java:41-142` — `TRI_TILE_DATA` | `collision_query.c:53-66` — `bounce_slope_collision[12][12]` | MATCH | REACHABLE | |

## 5. Enemies / Hazards

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Static spikes (tiles 3-6) | `Ball.java:695-731` — `thinCollide()` → `popBall()` | `collision_query.c:770-781` — dispatch by tile ID | MATCH | REACHABLE | |
| Moving spikes (tile 10) | `TileCanvas.java:209-315` — `createMovingObj()`, `updateMovingSpikeObj()` | `dyn_thorns.c:264` — `bounce_dyn_thorns_update()` | MATCH | REACHABLE | |
| Spike image composition | `TileCanvas.java:228-233` — 24x24 from 4 transforms | `visual_assets.c` — `bounce_visual_assets_draw_dyn_thorn()` | MATCH | REACHABLE | |
| Spike save/restore | `BounceCanvas.java:691-703` — `resetSpikes()` | `level.c:584` — `bounce_runtime_level_tiles_restore_dyn_thorns()` | MATCH | REACHABLE | |
| Death (KillBall) | `Ball.java:215-226` — `KillBall()` | `game.c:1206` — `bounce_game_mark_dead()` | MATCH | REACHABLE | |
| GodMode cheat | `Ball.java:516` — `if (!this.n.GodMode)` | `game.h:244-251` — `god_mode` suppresses KillBall | MATCH | REACHABLE | |
| Invincibility cheat | `Ball.java:516` — `if (!this.mCanvas.mInvincible)` | Not implemented | MISSING | UNREACHABLE | Mod has invincibility cheat (7-8-*-#-8-*); native does not |
| Respawn | `BounceCanvas.java:367-381` — recreate buffer at respawn | `game.c:1217` — `bounce_game_request_respawn()` | MATCH | REACHABLE | |

## 6. Collectibles / Power-ups

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Respawn gem (tile 7) | `Ball.java:810-816` — +200 pts, sets respawn | `game.h:151-153` — `checkpoint_tile_x/y/ball_size` | MATCH | REACHABLE | |
| Extra life gem (tile 29) | `Ball.java:933` — +1000 pts, +1 life (max 5) | `player.h:85-86` — power-up fields exist | PARTIAL | REACHABLE | Producer exists; max-5 life cap not verified |
| Speed boost (tile 38) | `Ball.java:642` — `speedBonusCntr = 300` | `player.h:85` — `TODO_somePowerUp1` | MATCH | REACHABLE | |
| Gravity flip (tiles 47-50) | `Ball.java:1064-1074` — `gravBonusCntr = 300` | `player.h:86` — `TODO_somePowerUp3` | MATCH | REACHABLE | |
| Jump boost (tiles 51-54) | `Ball.java:1180-1182` — `jumpBonusCntr = 300` | `player.h:86` — `TODO_somePowerUp3` | MATCH | REACHABLE | |
| Deflators (tiles 39-42) | `Ball.java:503-513` — shrink ball | `collision_query.c` — interaction dispatch | PARTIAL | REACHABLE | Shrink logic in vertical_slice.c |
| Inflators (tiles 43-46) | `Ball.java:462-501` — enlarge ball | `player.c:979` — `bounce_player_use_big_ball()` | MATCH | REACHABLE | |
| Power-up duration (300 frames) | `BounceConst.java:27` — `BONUS_DURATION = 300` | `player.h:85-86` — counter fields | MATCH | REACHABLE | |
| Power-up counter decrement | `Ball.java:802` — per-tick decrement | `vertical_slice.c` — counter decrement | MATCH | REACHABLE | |

## 7. Levels

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Level loading | `TileCanvas.java:122-170` — `loadLevel()` | `level.c:64` — `bounce_level_parse()` + `level_loader.c:17` — `bounce_level_load_file()` | MATCH | REACHABLE | |
| Level file format | Binary: header + tiles + thorn payload | Same format | MATCH | REACHABLE | |
| Level count | 11 levels (`BounceConst.java:202`) | 11 levels (`app_flow.h:127`) | MATCH | REACHABLE | |
| Level transitions | `BounceCanvas.java:447-461` — `mLevelNum++`, +5000 | `game.c:1228` — `bounce_game_mark_level_complete()` | MATCH | REACHABLE | |
| Level display timer | `BounceCanvas.java:119` — `mLevelDisCntr = 120` | `app_flow.h:196` — `BOUNCE_APP_ENTRY_COUNTDOWN = 120` | MATCH | REACHABLE | |
| Level name display | `BounceCanvas.java:312-317` — `TEXT_FONT_LARGE` centered | `ui_shell.c:4035` — `bounce_ui_shell_render_level_banner()` | MATCH | REACHABLE | |
| Level selection | `BounceUI.java:146-159` — `displayNewGameMenu()` | `app_flow.c:1154-1247` — `bounce_app_flow_level_selection_move/select()` | MATCH | REACHABLE | |
| Level unlock progression | `BounceUI.java:527-529` — `mBestLevel = min(level, 11)` | `app_flow.c:1051` — `bounce_app_flow_complete_level()` | MATCH | REACHABLE | |
| Default unlock count | 0 (fresh install) | 2 (`BOUNCE_DEFAULT_UNLOCKED_LEVEL_COUNT`) | DIFFERENT | REACHABLE | Native chose 2 so level select is reachable |
| Checkpoint (respawn gem) | `Ball.java:810-816` — updates respawn point | `game.h:151-153` — `checkpoint_tile_x/y/ball_size` | MATCH | REACHABLE | |
| Exit tile | `TileCanvas.java:964-986` — `createExitTileObject()`, `openExit()` | `visual_assets.c` — `bounce_visual_assets_draw_exit_door()` | MATCH | REACHABLE | |
| Exit door animation | `TileCanvas.java:979-986` — `mImageOffset += 4`, `mOpenFlag = true` at 24 | `game.h:599-600` — `door_image_offset`, `door_open_flag` | MATCH | REACHABLE | |
| Moving spike objects | `TileCanvas.java:209-315` — full creation, update, render | `dyn_thorns.c` — full animation state | MATCH | REACHABLE | |
| Level 11 byte difference | LKT Pro: 0x01 at offset 0x1D98 | Native: 0x61 at offset 0x1D98 | DIFFERENT | N/A | One byte differs; likely a level data fix in one version |

## 8. Scoring / Lives / Progression

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Score field | `BounceCanvas.java:38` — `mScore` | `game.h:206` — `int32_t score` | MATCH | REACHABLE | |
| Add score | `BounceCanvas.java:203-206` — `add2Score()` | `game.c:1487` — `bounce_game_add_score()` | MATCH | REACHABLE | |
| Reset score | `BounceCanvas.java:86` — `mScore = score` (param) | `game.c:1495` — `bounce_game_reset_score()` | MATCH | REACHABLE | |
| Ring points (500) | `Ball.java:530` — `add2Score(500)` | `game.h:206` — score field exists | PARTIAL | UNVERIFIED | Hoop scoring not verified as wired |
| Gem points (200) | `Ball.java:811` — `add2Score(200)` | `game.h:206` — score field exists | PARTIAL | UNVERIFIED | Gem scoring not verified as wired |
| Life points (1000) | `Ball.java:933` — `add2Score(1000)` | `game.h:206` — score field exists | PARTIAL | UNVERIFIED | Life scoring not verified as wired |
| Exit points (5000) | `BounceCanvas.java:452` — `add2Score(5000)` | `game.c:1228` — level complete | PARTIAL | UNVERIFIED | Exit bonus not verified as wired |
| Lives | `BounceCanvas.java:37` — `numLives` | `game.h:182` — `lives` | MATCH | REACHABLE | |
| Initial lives | 3 (`BounceConst.java:261`) | 3 (`app_flow.h:195`) | MATCH | REACHABLE | |
| Max lives | 5 (`Ball.java:934`) | Not explicitly capped | PARTIAL | UNVERIFIED | Max-5 cap not verified |
| Life decrement | `Ball.java:520` — `--numLives` on pop | `game.c:1206` — death path | MATCH | REACHABLE | |
| Game over on lives < 0 | `BounceCanvas.java:368` | `game.c:1599` | MATCH | REACHABLE | |
| High score | `BounceUI.java:48` — `mBestScore` | `app_flow.h:535` — `high_score` | MATCH | REACHABLE | |
| New high score flag | `BounceUI.java:49` — `mNewBestScore` | `app_flow.h:570` — `new_high_score` | MATCH | REACHABLE | |
| High score check | `BounceUI.java:526-537` — `checkData()` | `app_flow.c:1819` — `bounce_app_flow_note_score()` | MATCH | REACHABLE | |
| Hoops scored | `BounceCanvas.java:36` — `numRings` | `game.h:183` — `hoops_scored` | MATCH | REACHABLE | |
| Hoops total | `TileCanvas.java:65` — `mTotalNumRings` | `level.h:92` — `bounce_level_hoops_total()` | MATCH | REACHABLE | |

## 9. Input

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Arrow key input | `BounceCanvas.java:574-579` — `getGameAction()` UP/LEFT/RIGHT/DOWN | `input_state.h:16-23` — LEFT/RIGHT/UP/DOWN arrows | MATCH | REACHABLE | |
| WASD input | Not supported | `input_state.h:17-23` — A/D/W/S keys | DIFFERENT | REACHABLE | Native extension |
| T9 keypad input | `BounceCanvas.java:471-508` — key codes 49-57, 48, 42, 35 | `input_state.h:24-33` — NUM0-NUM9 | MATCH | REACHABLE | |
| QWERTY keyboard mode | `BounceCanvas.java:471-508` — full QWERTY→keypad mapping | `input_state.h:92-162` — KP_UP/DOWN/LEFT/RIGHT + diagonals | DIFFERENT | REACHABLE | Native uses key aliases; mod uses key code mapping |
| Pause key | Not supported (no pause in mod) | `input_state.h:34` — PAUSE source | PARTIAL | UNREACHABLE | Native pause flag has no consumer |
| Confirm/Select | `BounceCanvas.java:574-579` — GAME_A (8) | `input_state.h:35-37` — CONFIRM_RETURN/KP_ENTER/SPACE | MATCH | REACHABLE | |
| Escape/Back | Not supported | `input_state.h:38` — ESCAPE | DIFFERENT | REACHABLE | Native extension |
| Quit | Not supported | `input_state.h:39` — QUIT | DIFFERENT | REACHABLE | Native extension |
| Hash key ('#') | `BounceCanvas.java:518-521` — cheat gravity | `input_state.h:50` — HASH source | MATCH | REACHABLE | |
| GodMode sequence | `BounceCanvas.java:537-571` — 7-8-7-8-9-9 | `game.h:244-251` — 7-8-7-8-9-8 | DIFFERENT | REACHABLE | Different sequences! Mod: 7-8-7-8-9-9; Native: 7-8-7-8-9-8 |
| Cheat activation | `BounceCanvas.java:537-571` — `mCheat = true` | `game.h:253-288` — `af = true` | MATCH | REACHABLE | |
| Cheat: gravity ('#') | `BounceCanvas.java:519-521` — `gravBonusCntr = 300` | `game.h:284` — af-gated '#' | PARTIAL | REACHABLE | Producer exists; consumer in vertical_slice.c |
| Cheat: prev level ('1') | `BounceCanvas.java:523-528` — `--mLevelNum` | `game.h:321-384` — `af_reload_requested` | MATCH | REACHABLE | |
| Cheat: next level ('3') | `BounceCanvas.java:530-535` — `++mLevelNum` | `game.h:321-384` — `af_reload_requested` | MATCH | REACHABLE | |
| Invincibility cheat | `BounceCanvas.java:545-550` — 7-8-*-#-8-* → `mInvincible` | Not implemented | MISSING | UNREACHABLE | |

## 10. UI / Menus

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Main menu | `BounceUI.java:118-144` — 5 items (Continue, New Game, High Score, Instructions, Settings) | `app_flow.h:145` — 7 rows (Continue, New Game, High Score, Instructions, Settings, About, Exit) | DIFFERENT | REACHABLE | Native adds About + Exit as native extensions |
| Continue | `BounceUI.java:123-124` — shown if playing or has save | `app_flow.h:223` — `continue_available` flag | MATCH | REACHABLE | |
| New game / level select | `BounceUI.java:146-159` — level list | `app_flow.c:1154-1247` — level selection | MATCH | REACHABLE | |
| High score display | `BounceUI.java:170-176` — `displayHighScore()` | `ui_shell.c:2750` — `bounce_ui_shell_render_high_score()` | MATCH | REACHABLE | |
| Instructions | `BounceUI.java:178-190` — `displayInstructions()` | `ui_shell.c:2563` — `bounce_ui_shell_render_instructions()` | MATCH | REACHABLE | |
| Settings menu | `BounceUI.java:546-565` — Language, Keyboard, FPS, Volume | `app_flow.h:68-98` — DISPLAY, INPUT, AUDIO, LANGUAGE, THEME, RESET pages | DIFFERABLE | REACHABLE | Native has more settings pages (Theme, Reset) |
| About screen | Not supported | `app_flow.h:43` — BOUNCE_APP_STATE_ABOUT | DIFFERENT | REACHABLE | Native extension |
| Game over screen | `BounceUI.java:192-213` — `displayGameOver(isWin)` | `ui_shell.c:3860` — `bounce_ui_shell_render_game_over()` | MATCH | REACHABLE | |
| Level complete screen | `BounceUI.java:215-229` — `displayLevelComplete()` | `ui_shell.c:3737` — `bounce_ui_shell_render_level_complete()` | MATCH | REACHABLE | |
| Game end screen (win) | `BounceUI.java:192-213` — `displayGameOver(true)` | `ui_shell.c:3495` — `bounce_ui_shell_render_game_end()` | MATCH | REACHABLE | |
| HUD: lives display | `BounceCanvas.java:238-241` — life icons at y=99 | `visual_assets.c` — HUD ball rendering | MATCH | REACHABLE | |
| HUD: rings display | `BounceCanvas.java:242-246` — ring icons at y=112 | `visual_assets.c` — HUD hoop rendering | MATCH | REACHABLE | |
| HUD: score display | `BounceCanvas.java:234-255` — score at (64, 100) | `visual_assets.c` — `bounce_visual_assets_draw_hud_score()` | MATCH | REACHABLE | |
| HUD: bonus bar | `BounceCanvas.java:234-255` — bonus bar at bottom-left | `visual_assets.c` — bonus rendering | MATCH | REACHABLE | |
| Splash screens | `BounceCanvas.java:56` — 2 splashes (Nokia, Bounce) | `ui_shell.c:2271` — `bounce_ui_shell_render_splash()` | MATCH | REACHABLE | |
| Mouse click support | Not supported | `ui_shell.h:156-160` — `bounce_ui_shell_handle_click()` | DIFFERENT | REACHABLE | Native extension |
| Numeric key input | Not supported | `ui_shell.h:180-183` — `bounce_ui_shell_handle_number()` | DIFFERENT | REACHABLE | Native extension |

## 11. Audio

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Sound effects | 3 WAV files: up.wav, pickup.wav, pop.wav | 3 RTPL files: up.ott, pickup.ott, pop.ott | DIFFERENT | REACHABLE | Different formats; same sounds |
| Hoop sound | `BounceCanvas.java:65` — `mSoundHoop` | `audio_event.h:88` — BOUNCE_AUDIO_EVENT_UP | MATCH | REACHABLE | |
| Pickup sound | `BounceCanvas.java:66` — `mSoundPickup` | `audio_event.h:89` — BOUNCE_AUDIO_EVENT_PICKUP | MATCH | REACHABLE | |
| Pop sound | `BounceCanvas.java:67` — `mSoundPop` | `audio_event.h:90` — BOUNCE_AUDIO_EVENT_POP | MATCH | REACHABLE | |
| Volume control | `BounceUI.java:553` — Gauge 0-100 | `app_flow.h:324-339` — `audio_enabled` gate | PARTIAL | REACHABLE | Native has on/off gate; mod has 0-100 volume |
| Volume persistence | `BounceUI.java:607` — saved in settings RMS | Not persisted | PARTIAL | REACHABLE | Native audio gate not persisted |
| Audio backend | Java ME `Manager.createPlayer()` | ALSA + Null backend | DIFFERABLE | REACHABLE | Different backends |
| Audio synthesis | Java ME media | RTPL decoder + sine synth | DIFFERENT | REACHABLE | Native synthesizes from RTPL; mod plays WAV |
| Background music | Not supported | Not supported | MATCH | N/A | Neither has music |

## 12. Persistence

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Save format | RMS (Java ME RecordStore) | Custom binary container ("BNCPRST1") | DIFFERENT | REACHABLE | Different formats; same semantics |
| Best level record | `BounceUI.java:342` — record 1 (1 byte) | `persistence.c:497` — record 1 (int32) | MATCH | REACHABLE | |
| Best score record | `BounceUI.java:343` — record 2 (4 bytes) | `persistence.c:497` — record 2 (int32) | MATCH | REACHABLE | |
| Game state record | `BounceUI.java:344` — record 3 (255 bytes) | `persistence.c:655` — record 3 (variable) | MATCH | REACHABLE | |
| Settings record | `BounceUI.java:570` — `bounceSettingsRMS` (4 bytes) | `persistence.c:933,979` — records 4 (theme) + 5 (T9) | DIFFERENT | REACHABLE | Native splits settings across records |
| Save on destroy | `Bounce.java:33` — `saveGameData(3)` | `vertical_slice.c` — save on exit | MATCH | REACHABLE | |
| Save triggers | Level complete, game over, settings change | Level complete, game over, settings change | MATCH | REACHABLE | |
| Continue from save | `BounceUI.java:240-251` — J==1 (position) or J==2 (level restart) | `app_flow.c:992-1002` — `bounce_app_flow_begin_record_continue_entry()` | MATCH | REACHABLE | |
| Resume snapshot | `BounceUI.java:442-503` — full state (position, speed, bonuses, tiles, spikes) | `persistence.h:203-254` — `BouncePersistenceRecord3` | MATCH | REACHABLE | |
| Tile delta save | `BounceUI.java:468-493` — special tiles (7, 29, 13-16, 21-24) | `persistence.h:172-176` — `BouncePersistenceTileDelta` | MATCH | REACHABLE | |
| Dyn thorn save | `BounceUI.java:494-502` — spike offsets + directions | `persistence.h:183-188` — `BouncePersistenceDynThorn` | MATCH | REACHABLE | |
| Path resolution | N/A (RMS is internal) | `persistence.c:409-456` — BOUNCE_SAVE_DIR → XDG_DATA_HOME → HOME | DIFFERENT | REACHABLE | Native uses filesystem paths |
| Reset to Default | Not supported | `app_flow.c:789` — `bounce_app_flow_reset_to_default()` | DIFFERENT | REACHABLE | Native extension |

## 13. Rendering

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Game buffer | 156x96 (`TileCanvas.java:96`) | 128x128 surface (`surface.h`) | DIFFERENT | REACHABLE | Different buffer sizes |
| Full-screen buffer | 128x128 (`BounceCanvas.java:69`) | 128x128 surface | MATCH | REACHABLE | |
| Scaling | `BounceCanvas.java:258-290` — nearest-neighbor | `presentation.c` — X11 presentation | DIFFERENT | REACHABLE | Different scaling approaches |
| Scrolling | `TileCanvas.java:696-788` — `scrollBuffer()`, `testScroll()` | `vertical_slice.c` — camera logic | MATCH | REACHABLE | |
| Screen flip | `BounceCanvas.java:160-201` — vertical/horizontal transition | `vertical_slice.c` — screen flip logic | MATCH | REACHABLE | |
| Dirty tile system | `Ball.java:1243-1276` — `dirtyTiles()` with 0x80 flag | `vertical_slice.c` — tile dirty tracking | MATCH | REACHABLE | |
| Tile rendering | `TileCanvas.java:328-651` — `drawTile()` with dark theme | `visual_assets.c` — `bounce_visual_assets_draw_tile()` | MATCH | REACHABLE | |
| Sprite loading | `TileCanvas.java:831-928` — 67 Q*.png + fallback to atlas | `visual_assets.c` — atlas extraction from `objects_nm.png` | DIFFERENT | REACHABLE | Mod loads individual PNGs; native extracts from atlas |
| Sprite transforms | `TileCanvas.java:172-207` — 6 transform modes | `visual_assets.c` — `visual_source_coordinate()` | MATCH | REACHABLE | |
| Dark theme (water) | `TileCanvas.java:328-651` — water color 1073328 | `visual_assets.c` — water background 0xFF1060B0 | MATCH | REACHABLE | |
| Hoop animation | `TileCanvas.java:424-454` — hoop image list overlay | `visual_assets.c` — hoop rendering | MATCH | REACHABLE | |
| Exit door rendering | `TileCanvas.java:813-829` — 24x48 sprite | `visual_assets.c` — `bounce_visual_assets_draw_exit_door()` | MATCH | REACHABLE | |
| Ball rendering | `Ball.java:1228-1241` — `paint()` with wrap | `visual_assets.c` — ball sprite rendering | MATCH | REACHABLE | |
| HUD rendering | `BounceCanvas.java:234-255` — lives, rings, score, bonus | `visual_assets.c` — HUD rendering | MATCH | REACHABLE | |
| Frame timing | `GameTimer.java:19-24` — 1000/mFps ms | `game_timer.c` — 40ms canonical tick | MATCH | REACHABLE | |
| FPS setting | `BounceUI.java:552` — TextField 10-60, default 25 | Not implemented (fixed 40ms) | PARTIAL | UNREACHABLE | Native has no FPS setting |

## 14. Debug / Cheat Features

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Debug flags | 8 flags, all `false` (`BounceConst.java:7-9,56,214-215`, `Bounce.java:17`, `Profile.java:7`, `Ball.java:18`) | `NBB_DEBUG` env var — 4 levels | DIFFERENT | REACHABLE | Different debug systems |
| Profiling | `Profile.java` — 10 timing fields, all disabled | Not implemented | MISSING | UNREACHABLE | |
| Cheat: GodMode | `BounceCanvas.java:537-571` — 7-8-7-8-9-9 → `mCheat` | `game.h:244-251` — 7-8-7-8-9-8 → `god_mode` | DIFFERENT | REACHABLE | Different sequences |
| Cheat: invincibility | `BounceCanvas.java:545-550` — 7-8-*-#-8-* → `mInvincible` | Not implemented | MISSING | UNREACHABLE | |
| Cheat: gravity ('#') | `BounceCanvas.java:519-521` — `gravBonusCntr = 300` | `game.h:284` — af-gated '#' | PARTIAL | REACHABLE | |
| Cheat: prev level ('1') | `BounceCanvas.java:523-528` — `--mLevelNum` | `game.h:321-384` — `af_reload_requested` | MATCH | REACHABLE | |
| Cheat: next level ('3') | `BounceCanvas.java:530-535` — `++mLevelNum` | `game.h:321-384` — `af_reload_requested` | MATCH | REACHABLE | |
| Cheat: skip level ('0') | `BounceCanvas.java:595-596` — `mLeaveGame = true` | `game.h:386-446` — `completion_requested` (XK_c) | DIFFERENT | REACHABLE | Different key bindings |
| Debug tracker | Not supported | `debug_tracker.h` — NBB_DEBUG env var | DIFFERENT | REACHABLE | Native extension |
| Camera diagnostic | Not supported | `vertical_slice.c --camera-diagnostic` | DIFFERENT | REACHABLE | Native extension |
| Render trace | Not supported | `vertical_slice.c --render-trace` | DIFFERENT | REACHABLE | Native extension |

## 15. Localization

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Language count | 2 (xx, vi-VN) | 5 (EN, ZH_CN, ZH_TW, TH_TH, ID) | DIFFERENT | REACHABLE | Native has more languages |
| String count | 30 strings | ~120 keys | DIFFERENT | REACHABLE | Native has more strings (settings, themes, etc.) |
| Language selection | `BounceUI.java:548-549` — ChoiceGroup (Auto, VI) | `app_flow.h:185` — 5 languages | DIFFERENT | REACHABLE | |
| Language persistence | `BounceUI.java:607` — saved in settings RMS | Not persisted (runtime only) | PARTIAL | REACHABLE | Native language not persisted |
| lang.xx format | Binary: offset table + UTF strings | Same format | MATCH | REACHABLE | |
| lang.xx content | 889 bytes, 30 strings | 730 bytes, fewer strings | DIFFERENT | N/A | Native missing settings strings |
| Instructions text | `Local.java:14-19` — 6 parts | `locale.h:156-158` — body + prefix | MATCH | REACHABLE | |
| Parameter substitution | `Local.java:53-66` — `%U`, `%0U`, `%1U` | `ui_shell.c:1642` — placeholder substitution | MATCH | REACHABLE | |

## 16. Build / Platform

| Feature | Reference Mod | Native C | Status | Reachability | Verification |
|---|---|---|---|---|---|
| Platform | Java ME (MIDP 2.0, CLDC 1.0) | Linux (X11, ALSA, libpng) | DIFFERENT | N/A | Different platforms |
| Build system | javac + jar | Makefile + cc | DIFFERENT | N/A | |
| Dependencies | Java ME APIs | X11, libpng, ALSA (optional), pthread, -lm | DIFFERENT | N/A | |
| Packaging | JAR | AppImage | DIFFERENT | N/A | |
| Windows executable | Not supported | Not supported (Linux-only) | MATCH | N/A | Neither supports Windows |

---

## Summary of confirmed gaps

### MISSING features (not implemented in native)

1. **Invincibility cheat** — Mod has 7-8-*-#-8-* sequence → `mInvincible`; native does not
2. **Profiling system** — Mod has `Profile.java` with 10 timing fields; native does not
3. **FPS setting** — Mod has 10-60 FPS adjustment; native has fixed 40ms tick
4. **Volume control (0-100)** — Mod has 0-100 volume; native has on/off gate only
5. **QWERTY keyboard mapping** — Mod has full QWERTY→keypad; native has key aliases but different approach

### PARTIAL features (incomplete or behaviorally different)

1. **Scoring wiring** — Hoop/gem/life/exit point values not verified as wired to score field
2. **Max lives cap (5)** — Not explicitly verified in native
3. **Pause** — Native pause flag has no consumer; mod has no pause state
4. **Audio volume** — Native has on/off gate; mod has 0-100 volume
5. **Language persistence** — Native language not persisted across launches
6. **Cheat sequences** — GodMode: mod 7-8-7-8-9-9 vs native 7-8-7-8-9-8
7. **Pop animation** — 5-frame pop animation not explicitly modeled in native
8. **Sound pickup wiring** — `sound = this.n.soundPickup` not implemented in collision query

### DIFFERENT features (exist but behave differently)

1. **Platform** — Java ME vs Linux native
2. **Save format** — RMS vs custom binary container
3. **Sound format** — WAV vs RTPL
4. **Sprite loading** — Individual PNGs vs atlas extraction
5. **Buffer size** — 156x96 vs 128x128
6. **Default unlock count** — 0 vs 2
7. **Language count** — 2 vs 5
8. **String count** — 30 vs ~120
9. **Menu items** — 5 vs 7
10. **Settings pages** — 4 vs 6
11. **Cheat sequences** — Different GodMode sequences
12. **Level 11 data** — One byte differs
13. **lang.xx content** — Different sizes and content
