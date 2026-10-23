# EXT-005 L8 Unseal — Implementation Report

**Source:** EXT-005 cross-audit (bounce-rl, https://github.com/IlyaasK/bounce-rl) against the recovered
decompiled Java (BASE-001) and the shipped level data. Registered in
`reverse/EXTERNAL-SOURCES-REGISTRY.md` (EXT-005 entry).

**Status:** implemented, self-tested, negative-controlled, verified green.

---

## 1. The defect

The shipped `src/main/resources/levels/J2MElvl.008` (byte-identical to the LKT Pro mod's copy)
**seals the spawn pocket from the rest of the level**. With the shipped bytes the ball can never
leave the spawn chamber, so level 8 cannot be played at all.

Geometry (verified against the local file, width=156, height=36, spawn=(99,1)):

| Element | Evidence |
|---|---|
| Spawn pocket | chamber around the spawn, rows 0..7, cols 90..111, walls on all sides |
| Baffle rows 3..6 | carry the chamber's **only** vertical shafts, at columns **91, 97, 102, 108** (empty cells, rows 1..6) |
| Row 7 (the seal) | exactly those four cells are solid slope tiles, raw values **33 / 32 / 33 / 32** |
| Row 7 openings | sit **offset**, at x = **92 / 96 / 103 / 107** — under solid tiles, not under the shafts |
| No escape | the decompiled game has no tile-break mechanic (BASE-001); a ball dropping down shaft 91 lands on solid tile (91,7) |

bounce-rl (EXT-005) measured the consequence on the same bytes: **0 of 11 rings reachable from
spawn until the four row-7 tiles are cleared; 11 of 11 after.** Their fix clears the same four
cells (`7 * LV8_W + {91,97,102,108}`) in an in-memory copy, leaving `levels.h` (their pristine
APK data) untouched.

## 2. Implementation

| File | Change |
|---|---|
| `native/level/level.h` | declaration + full rationale for `bounce_level_patch_l8_unseal()` |
| `native/level/level.c` | `bounce_level_patch_l8_unseal()` — clears the four row-7 cells to EMPTY (0); guards: NULL → -1, non-156x36 → -1 (rejected **before** any byte is written; `BounceLevel` carries no level id, so the fix keys off the dimensions) |
| `native/app/game.c` | patch applied in **both** loaders: `bounce_game_load_level()` (all-in-one: deferred-init reload, legacy diagnostic route) and `bounce_game_load_level_entry()` (staged production route: New Game, Level Select, post-completion Continue) |
| `native/app/vertical_slice.c` | `verify_l8_unseal()` fixture + `l8_unseal_diff_count()` helper, registered in the `--check` suite |
| `README.md` | EXT-005 cross-reference line |
| `reverse/EXTERNAL-SOURCES-REGISTRY.md` | EXT-005 source entry, cross-checked / not-taken lists, recorded conflicts |

### Design decisions

1. **Patch at the game-load seam, NOT in `bounce_level_parse()` / `bounce_level_load_file()`.**
   Those two stay byte-pristine on purpose. Many `--check` fixtures assert the recovered file
   bytes directly (e.g. the (103,3)==0x20 slope witness in the collision fixture, the (110,15)/(110,16)
   hoop-predicate fixture, the asset inventory) — mutating on parse would silently change the
   meaning of every one of them. The game loaders are the only funnel every *gameplay* route goes
   through, so patching there covers all of them while the file on disk and every direct probe keep
   the recovered bytes.

2. **Patch BEFORE the runtime tile copy.** `bounce_runtime_level_tiles_create()` copies from the
   level struct, so placing the call before it makes the mutable gameplay grid inherit the unsealed
   bytes: in `bounce_game_load_level()` directly before the copy; in `bounce_game_load_level_entry()`
   before the ownership swap, which the later `bounce_game_initialize_runtime_entry()` copies from.

3. **In-memory only; the resource file is never rewritten.** The deviation lives in code and is
   documented (level.h, this report, the registry) — same discipline as bounce-rl, and the same as
   the project's provenance rules: recovered data stays as recovered.

4. **Both loaders, not just the production one.** An earlier version of the working tree patched
   only `bounce_game_load_level_entry()`, leaving the deferred-init/diagnostic reload route
   (`bounce_game_load_level()`) able to install the sealed level again.

## 3. Verification

### Self-test — `verify_l8_unseal()` in the `--check` suite

| # | Assertion |
|---|---|
| A | the recovered file stays **pristine**: `bounce_level_load_file()` still sees the shipped seal (all four row-7 cells non-zero) |
| B1 | the staged-entry loader installs the fix: **exactly 4** tile differences vs the recovered bytes — the four documented shaft cells, each changed to EMPTY |
| B2 | the all-in-one loader does the same |
| C | the four shafts are **continuous** from the pocket interior: rows 1..7 of each shaft column are EMPTY in the installed level |
| D | guards are total and level-8-only: NULL → -1, level-1 parse → -1 **without writing a byte** (re-read is byte-identical), and a full level-1 load shows 0 tile differences |

`l8_unseal_diff_count()` polices the *shape* of any difference: a differing cell outside the four
documented coordinates, or a shaft cell not EMPTY afterwards, returns `(size_t)-1` — the tests fail
closed on a wrong-shaped difference rather than on a wrong count only.

### Results

| Check | Result |
|---|---|
| `make -C native/app` | clean build, `-std=c11 -Wall -Wextra -Werror -pedantic`, no warnings |
| `make -C native/app check` | **rc=0, stderr empty (0 bytes)** — the documented contract |
| new fixture output | `L8UNSEAL: shaft cells (91/97/102/108, row 7) cleared on both loaders; 4 of 5616 level-8 bytes changed to EMPTY; level 1 untouched` |
| **negative control** | with both patch calls disabled: **rc=1**, stderr `level 8 unseal (EXT-005) verification failed` — the fixture is not vacuous; it fails exactly when the fix is absent, and passes again when restored |

### What was NOT changed

- `src/main/resources/levels/J2MElvl.008` — pristine recovered data.
- All direct level probes in `--check` (they still assert recovered bytes).
- Levels 1-7 and 9-11 — the patch rejects any non-156x36 level before writing.
- No persistence impact: record-3 tile deltas cover only special tiles (7, 29, 13-16, 21-24);
  the four slope cells (32/33) are not in the delta set, so a resumed session reloads the level
  through a game loader and re-applies the same fix.
