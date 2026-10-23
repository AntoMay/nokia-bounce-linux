# Evidence Index — LKT Pro Parity Audit

All file paths are absolute. Line numbers correspond to the state of each repository at audit time (2026-10-11).

---

## 1. Native repository — key evidence paths

### 1.1 Entry point and main loop

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/vertical_slice.c` | 36204–36363 | `main()` — entry point, CLI flags, frame loop |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/vertical_slice.c` | 24322–24326 | `x11_god_mode_sequence_step()` — GodMode sequence detector |

### 1.2 Application flow and state machine

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.h` | 19–61 | `BounceAppState` enum — 15 states |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.h` | 68–98 | `BounceSettingsPage` enum — 7 pages |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.h` | 100–124 | `BounceAppEvent` enum — 9 events |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.h` | 145–178 | Menu row constants — 7 rows |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.h` | 394–434 | `BounceGameplayEntryPhase` enum — 9 phases |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.h` | 446–475 | `BounceGameplayEntry` struct |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.h` | 477–571 | `BounceAppFlow` struct |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 217 | `bounce_app_flow_init()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 267 | `bounce_app_flow_apply()` — transition table |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 348 | `bounce_app_flow_tick()` — splash timer |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 412 | `bounce_app_flow_menu_move()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 434 | `bounce_app_flow_menu_select()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 735 | `bounce_app_flow_settings_move()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 789 | `bounce_app_flow_reset_to_default()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 839 | `bounce_app_flow_settings_select()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 959 | `bounce_app_flow_settings_back()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 1051 | `bounce_app_flow_complete_level()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 1154–1247 | Level selection functions |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/app_flow.c` | 1819 | `bounce_app_flow_note_score()` |

### 1.3 Game state and lifecycle

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.h` | 40–77 | `BounceGameState` enum — 18 states |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.h` | 85–653 | `BounceGame` struct |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.h` | 151–153 | Checkpoint fields |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.h` | 206 | `score` field |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.h` | 211–252 | GodMode cheat |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.h` | 253–288 | `af` cheat latch |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.h` | 321–384 | `af_reload_requested` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.h` | 386–446 | `completion_requested` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 127 | `bounce_game_init()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 284 | `bounce_game_release_level()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 311 | `bounce_game_load_level()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 416 | `bounce_game_activate_loaded_level()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 500 | `bounce_game_load_level_entry()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 626 | `bounce_game_initialize_player_entry()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 709 | `bounce_game_initialize_runtime_entry()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 896 | `bounce_game_tick_dispatch()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 1206 | `bounce_game_mark_dead()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 1217 | `bounce_game_request_respawn()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 1228 | `bounce_game_mark_level_complete()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 1487 | `bounce_game_add_score()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 1495 | `bounce_game_reset_score()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 1503 | `bounce_game_continue_level()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 1599 | `bounce_game_mark_game_over()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/game.c` | 1609 | `bounce_game_shutdown()` |

### 1.4 Player

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.h` | 57–115 | `BouncePlayer` struct |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.h` | 123–132 | `BouncePlayerSpawn` struct |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.h` | 217–222 | `BouncePlayerHazardCallback` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.c` | 19 | `bounce_player_reset()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.c` | 26 | `bounce_player_tick_entry()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.c` | 102 | `bounce_player_initialize_from_level()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.c` | 236 | `player_apply_slope_response()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.c` | 979 | `bounce_player_use_big_ball()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/player.c` | 1002 | `bounce_player_use_big_ball_level()` |

### 1.5 Collision

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.h` | 1–86 | Collision query API |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 17–50 | Ball masks |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 53–66 | Slope masks |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 131 | `java_thorn_rect_overlaps()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 172 | `wall_mask_overlaps()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 252 | `java_shape_overlaps()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 300 | `java_hoop_shape_overlaps()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 363 | `slope_mask_overlaps()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 485–567 | `java_dyn_thorn_overlaps()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 569 | `java_interaction_collision()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 750 | `tile_has_static_collision()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 811 | `bounce_collision_tile_overlaps()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 896 | `bounce_collision_tile_shape_overlaps()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 1103 | `bounce_collision_query()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/collision_query.c` | 1135 | `bounce_collision_query_level()` |

### 1.6 Level system

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.h` | 1–342 | Level API |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.h` | 45–48 | `BouncePlayerBallMode` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.h` | 215–299 | Runtime tiles API |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.c` | 64 | `bounce_level_parse()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.c` | 154 | `bounce_level_destroy()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.c` | 493 | `bounce_runtime_level_tiles_create()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.c` | 532 | `bounce_runtime_level_tiles_destroy()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.c` | 565 | `bounce_runtime_level_tiles_update_dyn_thorns()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level.c` | 584 | `bounce_runtime_level_tiles_restore_dyn_thorns()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/level_loader.c` | 17 | `bounce_level_load_file()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/dyn_thorns.c` | 62 | `bounce_dyn_thorns_test_point_inside()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/dyn_thorns.c` | 92 | `bounce_level_dyn_thorn_record()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/level/dyn_thorns.c` | 264 | `bounce_dyn_thorns_update()` |

### 1.7 Persistence

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.h` | 60–426 | Persistence API |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.h` | 203–254 | `BouncePersistenceRecord3` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.c` | 409–456 | Path resolution |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.c` | 497 | `bounce_persistence_load()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.c` | 655 | `bounce_persistence_load_record3()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.c` | 933, 1053 | Theme index record 4 |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.c` | 979, 1032 | T9 input record 5 |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.c` | 1088 | `bounce_persistence_save_record3()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/persistence.c` | 1155 | `bounce_persistence_delete_record3()` |

### 1.8 UI shell

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.h` | 1–471 | UI shell API |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.h` | 32–67 | `BounceUiPalette`, 13 color profiles |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.h` | 134–183 | UI input handling |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 2271 | `bounce_ui_shell_render_splash()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 2286 | `bounce_ui_shell_render_menu()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 2563 | `bounce_ui_shell_render_instructions()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 2750 | `bounce_ui_shell_render_high_score()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 2830 | `bounce_ui_shell_render_settings()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 3495 | `bounce_ui_shell_render_game_end()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 3737 | `bounce_ui_shell_render_level_complete()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 3860 | `bounce_ui_shell_render_game_over()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 3961 | `bounce_ui_shell_render_about()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/ui_shell.c` | 4086 | `bounce_ui_shell_render_level_selection()` |

### 1.9 Audio

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/audio_event.h` | 85–91 | `BounceAudioEvent` enum |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/audio_event.h` | 102–127 | Event sink |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/audio_synth.h` | 143–206 | Synth config |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/audio_synth.h` | 272–337 | `bounce_synth_render_plan()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/audio_synth.h` | 340–352 | `bounce_synth_render_pcm()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/audio_backend.h` | 67–212 | Backend interface |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/audio_player.h` | 80–206 | Player |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/rtpl_decoder.h` | 63–311 | RTPL decoder |

### 1.10 Input

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/input_state.h` | 15–163 | `BounceInputSource` enum — 35 sources |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/input_state.h` | 165–178 | `BounceInputState` struct |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/input_state.c` | 26–83 | Alias system |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/input_state.c` | 85 | `bounce_input_reset()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/input_state.c` | 92 | `bounce_input_source_is_keypad()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/input_state.c` | 105 | `bounce_input_apply_source()` |

### 1.11 Localization

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/locale.h` | 28–36 | 5 languages |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/locale.h` | 44–282 | ~120 locale keys |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/locale.c` | 164–675 | `locale_text[5][BOUNCE_LOCALE_KEY_COUNT]` table |

### 1.12 Rendering

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/renderer/renderer.h` | 20–92 | `BounceRenderer` |
| `/home/antomay/Projects/nokia-bounce-linux/native/renderer/surface.h` | 1–50 | `BounceSurface` |
| `/home/antomay/Projects/nokia-bounce-linux/native/renderer/image.h` | 1–? | `BounceImage` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/visual_assets.h` | 17–159 | `BounceVisualAssets` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/visual_assets.c` | 495–660 | `visual_build_sprites()` |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/visual_assets.c` | 857–1035 | `bounce_visual_assets_draw_tile()` |

### 1.13 Build and tools

| Path | Content |
|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/Makefile` | Build system — 249 lines, 15+ targets |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/tools/build_appimage.sh` | AppImage packaging |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/tools/dump_level_maps.py` | Level map dumper |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/tools/gen_script_font.py` | Glyph bitmap generator |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/tools/test_translations.py` | Translation verification |

### 1.14 Debug

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/nokia-bounce-linux/native/app/debug_tracker.h` | 14–230 | Debug tracker API |
| `/home/antomay/Projects/nokia-bounce-linux/native/app/debug_tracker.c` | ~400 | Debug tracker implementation |

---

## 2. Reference mod — key evidence paths

### 2.1 MIDlet and UI

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Bounce.java` | 14–38 | MIDlet lifecycle |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 101–116 | UI init |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 118–144 | Main menu |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 146–159 | New game menu |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 192–213 | Game over |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 215–229 | Level complete |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 341–410 | Game data load |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 412–440 | Game data save |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 442–503 | Write save game |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 526–537 | Check data |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 546–565 | Settings menu |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 567–601 | Load settings |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceUI.java` | 603–618 | Save settings |

### 2.2 Constants

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceConst.java` | 1–262 | All game constants |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceConst.java` | 76–129 | Tile IDs |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceConst.java` | 131–198 | Sprite image indices |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceConst.java` | 212–261 | State constants, game constants |

### 2.3 Game canvas and loop

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceCanvas.java` | 62–80 | Canvas init |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceCanvas.java` | 82–113 | `resetGame()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceCanvas.java` | 115–158 | `createNewLevel()`, `createBufferFocused()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceCanvas.java` | 160–201 | `screenFlip()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceCanvas.java` | 234–255 | HUD rendering |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceCanvas.java` | 320–463 | `run()` — main loop |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceCanvas.java` | 465–643 | `keyPressed()` — input + cheats |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/BounceCanvas.java` | 650–679 | Audio loading and playback |

### 2.4 Ball (player)

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 17–49 | Ball fields |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 41–368 | Static mask data |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 424–460 | `collisionDetection()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 462–513 | Enlarge/shrink ball |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 515–527 | `popBall()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 535–578 | `redirectBall()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 580–623 | `squareCollide()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 625–693 | `triangleCollide()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 695–731 | `thinCollide()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 733–773 | `edgeCollide()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 775–1016 | `testTile()` — master tile handler |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 1031–1075 | Gravity |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 1113–1209 | Ramp movement |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 1171–1186 | Horizontal accel and jump |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 1213–1224 | Screen wrap |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Ball.java` | 1228–1276 | `paint()` and `dirtyTiles()` |

### 2.5 Tile canvas (level rendering)

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 24–111 | Canvas fields and init |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 122–170 | `loadLevel()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 172–207 | `manipulateImage()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 209–315 | Moving object creation and update |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 328–651 | `drawTile()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 671–694 | `cleanBuffer()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 696–788 | `scrollBuffer()`, `testScroll()` |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 813–928 | Sprite loading |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/TileCanvas.java` | 964–986 | Exit tile creation and open animation |

### 2.6 Localization

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Local.java` | 12–207 | Full localization framework |

### 2.7 Timer

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/GameTimer.java` | 11–38 | Timer task |

### 2.8 Profile

| Path | Lines | Symbols / content |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/decompiled/com/lktpro/boun/Profile.java` | 6–35 | Profiling fields (all disabled) |

---

## 3. Resource evidence

### 3.1 Level files

| Path | Size | Notes |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/resources/levels/J2MElvl.001` – `J2MElvl.011` | 905–10,195 bytes | 11 levels; J2MElvl.011 differs from native by one byte at offset 0x1D98 |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/levels/J2MElvl.001` – `J2MElvl.011` | 905–10,195 bytes | 11 levels; identical except J2MElvl.011 |

### 3.2 Language files

| Path | Size | Notes |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/resources/lang.xx` | 889 bytes | 30 strings |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/lang.xx` | 730 bytes | Fewer strings; missing settings strings |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/resources/lang.vi-VN` | — | Vietnamese |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/lang.th-TH` | — | Thai |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/lang.zh-CN` | — | Chinese Simplified |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/lang.zh-TW` | — | Chinese Traditional |

### 3.3 Sound files

| Path | Format | Notes |
|---|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/resources/sounds/up.wav` | WAV | Hoop sound |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/resources/sounds/pickup.wav` | WAV | Pickup sound |
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/resources/sounds/pop.wav` | WAV | Pop sound |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/sounds/up.ott` | RTPL | Hoop sound (RTPL format) |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/sounds/pickup.ott` | RTPL | Pickup sound |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/sounds/pop.ott` | RTPL | Pop sound |

### 3.4 Sprite assets

| Path | Notes |
|---|---|
| `/home/antomay/Projects/Bounce_mod_LKT_Pro_final/resources/sprites/` | 70 individual PNG files (Q00–Q66, A, D, L) |
| `/home/antomay/Projects/nokia-bounce-linux/src/main/resources/icons/objects_nm.png` | Single 48x72 atlas; native extracts tiles from it |

### 3.5 Icons

| Path | Notes |
|---|---|
| Both repositories | `icon.png`, `nokiagames.png`, `bouncesplash.png` — identical |

---

## 4. Commands used in audit

```bash
# Git state
git log --oneline -5 && git status --short && git rev-parse HEAD

# File inventory
find . -type f -not -path './.git/*' | head -200

# Line counts
wc -l native/app/*.c native/app/*.h native/level/*.c native/level/*.h native/renderer/*.c native/renderer/*.h

# Level file comparison
cmp -l resources/levels/J2MElvl.011 src/main/resources/levels/J2MElvl.011

# Language file comparison
cmp -l resources/lang.xx src/main/resources/lang.xx

# Sound file comparison
diff -rq resources/sounds/ src/main/resources/sounds/

# Level file comparison (all)
diff -rq resources/levels/ src/main/resources/levels/

# Symbol search
grep -n "god_mode\|GodMode\|cheat\|Cheat" native/app/vertical_slice.c
grep -n "mCheat\|mInvincible\|cheat\|Cheat" decompiled/com/lktpro/boun/BounceCanvas.java
grep -n "QWERTY\|qwerty\|keyboard.*type" decompiled/com/lktpro/boun/BounceCanvas.java
grep -n "FPS\|fps\|mFps\|volume\|Volume\|mVolume" decompiled/com/lktpro/boun/BounceUI.java
```
