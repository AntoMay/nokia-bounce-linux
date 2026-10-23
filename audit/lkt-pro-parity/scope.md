# LKT Pro Parity Audit — Scope

> **DOCUMENTATION ONLY — not an active work driver.** This audit is retained as a
> record of a comparison that was performed, nothing more. The LKT Pro mod is **not**
> an active parity target for this repository and is scheduled to be removed from the
> workflow later; nothing below should be treated as an open task or an acceptance
> criterion. The authoritative basis for the native reconstruction remains BASE-001
> (the recovered decompiled Java) under the evidence hierarchy in
> `reverse/EXTERNAL-SOURCES-REGISTRY.md`.

## Repositories

| Role | Path | VCS | Branch | HEAD |
|---|---|---|---|---|
| Native implementation (audit target) | `/home/antomay/Projects/nokia-bounce-linux` | Git | `master` | `3edc558` |
| Reference mod (comparison source) | `/home/antomay/Projects/Bounce_mod_LKT_Pro_final` | Not a Git repo | N/A | N/A |

### Native repository working-tree status

Two modified files at audit time:
- `native/app/vertical_slice.c`
- `native/level/dyn_thorns.c`

These are uncommitted changes that were present before the audit began. They were not created or modified by this audit.

### Reference mod provenance

The reference mod is a **decompiled Java reimplementation** of Nokia Bounce, packaged as `Bounce_mod_LKT_Pro_final.jar` (SHA-256: `a8886145a167b855dcc0fbff8a591f253f6ed3a6a4e1f4cb523ee4d91493d0e5`). It was decompiled with CFR 0.152 and Temurin JRE 17.0.20.1. The package is `com.lktpro.boun` with 9 classes totaling 4,263 lines. It is **not compilable** without MIDP/Nokia API stubs.

The mod is a human-readable reimplementation of the original Nokia Bounce (original package `com.nokia.mid.appl.boun`). It is NOT original Nokia source.

## Audit limitations

1. **The reference mod is a decompilation, not original source.** Behavioral claims are based on decompiled code, which may contain decompiler artifacts.
2. **The native project is explicitly a work in progress.** Its README states: "This project is an ongoing native Linux reconstruction of Nokia Bounce. It is not yet complete."
3. **No runtime verification was performed.** This audit is source-code inspection only. No build or test was executed.
4. **The reference mod cannot be executed.** It requires a Java ME runtime with MIDP/Nokia APIs that are not available.
5. **The native project's uncommitted changes were not analyzed in depth.** They are noted as present but their content was not audited.
6. **Level 11 differs** between the two repositories (one byte at offset 0x1D98: 0x01 in LKT Pro vs 0x61 in native). This is recorded as a finding.
7. **lang.xx differs** between the two repositories (889 bytes in LKT Pro vs 730 bytes in native). The native version is missing settings-related strings that the mod added.
8. **Sound formats differ.** LKT Pro uses `.wav` files; native uses `.ott` (Nokia RTPL) files. The native project decodes RTPL format.
9. **Sprite assets differ.** LKT Pro has 70 individual `Q*.png` sprite files; native uses a single `objects_nm.png` atlas (48x72) and extracts tiles from it.

## What was inspected

### Native project (complete file inventory)

- `native/app/` — 40+ C source and header files (app_flow, game, player, persistence, locale, input_state, collision_query, audio_*, renderer, ui_shell, visual_assets, asset_inventory, debug_tracker, vertical_slice, game_timer, script_font, rtpl_decoder)
- `native/level/` — level.c, level_loader.c, dyn_thorns.c + headers
- `native/renderer/` — surface.c, image.c, renderer.c, presentation.c, pipeline.c, e_buffer.c + headers
- `native/assets/` — asset.c, asset.h
- `native/app/Makefile` — build system with 15+ targets
- `native/app/tools/` — build_appimage.sh, dump_level_maps.py, gen_script_font.py, test_translations.py
- `src/main/java/` — original decompiled Java reference (Bounce.java, BounceGame.java, b.java, e.java, f.java, etc.)
- `src/main/resources/` — levels, sounds, lang files, icons
- `reverse/EXTERNAL-SOURCES-REGISTRY.md` — provenance registry
- `README.md` — project documentation
- `screenshots/` — 15 PNG screenshots of the native build

### Reference mod (complete file inventory)

- `decompiled/com/lktpro/boun/` — 9 Java files: Ball.java (1277 lines), BounceCanvas.java (776), BounceConst.java (262), Bounce.java (39), BounceUI.java (619), GameTimer.java (38), Local.java (207), Profile.java (35), TileCanvas.java (1010)
- `resources/` — sprites/ (70 PNGs), sounds/ (3 WAVs), levels/ (11 binaries), icons/ (3 PNGs), lang.xx, lang.vi-VN, META-INF/MANIFEST.MF
- `README.md` — provenance documentation

## What was NOT inspected

- The native project's uncommitted changes to `vertical_slice.c` and `dyn_thorns.c` (noted but not analyzed)
- The original Java decompilation in `src/main/java/` (used as context, not as the primary comparison target — the LKT Pro mod is the comparison source)
- Runtime behavior of either codebase (no execution performed)
- The AppImage packaging script's runtime behavior
- The `dist/Bounce-x86_64.AppImage` binary

## Audit date

2026-10-11
