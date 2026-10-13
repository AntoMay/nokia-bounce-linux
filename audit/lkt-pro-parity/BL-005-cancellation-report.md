# BL-005 Cancellation Report

**Status:** BL-005 (configurable FPS settings) is **cancelled in full**.
**Cancellation commit:** `e24f2ae35a0b2e0eca2cbd786608a3ce11e460c9`
(`revert: remove BL-005 FPS settings`)
**Branch at time of writing:** `master`, **ahead 1 / behind 0** relative to
`origin/master` — committed locally, **not pushed**.

This report supersedes
[`BL-005-fps-report.md`](BL-005-fps-report.md) for any question about the
current state of the code. That file is retained as a historical record of the
implementation as it existed before cancellation. Its original body is
**unmodified**; only a `SUPERSEDED` banner was prepended to its top, which
changes no finding below it.

---

## 1. Decision summary

BL-005 was cancelled completely. The FPS settings feature does not exist in the
current code: not its UI, not its input handling, not its state or API, not its
persistence record, not its locale strings, and not its render-pacing path.

The cancellation was performed as a surgical revert rather than by reverting
whole commits, because the commits that carried BL-005 also carried BL-004 work
that had to survive. `c2901b9`, for example, contains both the BL-004 volume
reachability fix and the BL-005 FPS field work, so reverting it outright would
have destroyed working volume code.

Every commit in the BL-005 lineage below contributed BL-005 hunks that were
individually identified and removed, while BL-004 hunks in the same commits were
retained.

## 2. Historical commits that carried BL-005

Verified against the repository history:

| Commit | Subject | BL-005 content removed |
|--------|---------|------------------------|
| `749116c` | `feat(video): add configurable 10-180 FPS target` | The original feature: `target_fps` state, `BOUNCE_FPS_MIN/MAX/DEFAULT`, persistence record 7, Display page FPS row, `sleep_frame_fps()` |
| `c90b9ac` | `fix(video): complete FPS settings declarations and locales` | Remaining declarations, `BOUNCE_LOCALE_TARGET_FPS` and its strings in all 5 languages |
| `d56b08e` | `fix(audio,video): wire volume and FPS to runtime and match LKT Pro parity` | Startup FPS restore, `app_persist_fps()` / `app_commit_fps_change()`, `verify_fps_range_normalization()`, `verify_display_fps_row_is_painted()`, Display page `list_count` / `select` branches, the `region_digest()` helper |
| `c2901b9` | `fix(ui): make the volume control reachable and the FPS field editable` | Display FPS click handler, digit/Backspace keyboard block, `SELECT`-opens-field, FPS row rendering and caret, `XK_BackSpace` mapping, digit/Backspace dispatcher arms, `BOUNCE_INPUT_SOURCE_BACKSPACE`, `BOUNCE_UI_INPUT_DIGIT0..9` / `BACKSPACE` |
| *(working tree)* | — | The uncommitted BL-005 parity work: commit-time clamping, empty-field handling, the X11 digit-dispatch fall-through fix and its verifier |

`56a0d0f` (`feat(audio): add persistent 0-100 volume control`) is the parent of
`749116c` and was used as the authoritative reference for "BL-004 without
BL-005" when reconstructing pre-BL-005 forms.

Note that `d56b08e` and `c2901b9` each also contain **BL-004** work. Those
hunks were **not** reverted.

## 3. Scope of the cancellation

| Area | What was removed |
|------|------------------|
| **UI** | FPS row on the Display page (label, value, selection band), the FPS caret/typed-buffer rendering, the Display-page click handler for the FPS field, `BOUNCE_UI_DISPLAY_FPS_*` geometry constants |
| **Input** | `BOUNCE_UI_INPUT_DIGIT0..9` and `BOUNCE_UI_INPUT_BACKSPACE`; `BOUNCE_INPUT_SOURCE_BACKSPACE`; the `XK_BackSpace` keysym mapping; the digit and Backspace arms in `app_ui_input_from_source()` |
| **State / API** | `target_fps`, `fps_edit_active`, `fps_edit_len`, `fps_edit_digits` from `BounceSettingsState`; `BOUNCE_FPS_MIN/MAX/DEFAULT`; `BOUNCE_SETTINGS_FPS_STEP`; `BOUNCE_SETTINGS_DISPLAY_VALUE_COUNT/ROW_FPS`; `bounce_app_flow_settings_fps()`, `bounce_app_flow_restore_persisted_fps()` and the seven FPS-field edit functions |
| **Persistence** | `BOUNCE_PERSISTENCE_RECORD_FPS` (record 7), `bounce_persistence_load_fps()`, `bounce_persistence_save_fps()`, the record-7 write block in `write_container()`, and the FPS arguments at its call sites |
| **Locale** | `BOUNCE_LOCALE_TARGET_FPS` and its strings in all five languages (EN, ZH_CN, ZH_TW, TH_TH, ID) |
| **Frame pacing** | `sleep_frame_fps()`; the render loop calls plain `sleep_frame()` again. All six `write_container()` argument lists were restored byte-for-byte to their pre-BL-005 forms |
| **Verification** | The five BL-005 verifiers and their driver calls (see section 6) |

Files touched by the cancellation commit: `app_flow.c`, `app_flow.h`,
`input_state.h`, `locale.c`, `locale.h`, `persistence.c`, `persistence.h`,
`ui_shell.c`, `ui_shell.h`, `vertical_slice.c` — **+41 / −1796 lines**.

### What was deliberately kept

- **BL-004 volume in full.** See section 5.
- **The LEFT/RIGHT mapping arms** in `app_ui_input_from_source()`. Although
  introduced in a BL-005-flavoured commit, they are required by the BL-004
  volume slider. Their comment was retitled from `BL-004/BL-005` to `BL-004`
  and the wording narrowed to volume only.
- **The `&marker` NUL fix** in `ui_shell.c` — a general correctness bug, neither
  BL-004 nor BL-005.
- **The Input-page click handler** — not BL-005.
- **`region_digest()` was removed even though it carried a "BL-004" comment**,
  because it had no BL-004 caller: only `verify_display_fps_row_is_painted()`
  used it. Leaving it would have tripped `-Werror` as an unused static function.

## 4. Current state of the affected subsystems

- **Display page** is a read-only fact page again: three size facts (logical
  resolution, gameplay viewport, UI HUD region), zero navigable rows, and UP /
  DOWN / SELECT all refused by the flow.
- **Simulation tick is untouched.** `BOUNCE_GAME_TIMER_PERIOD_MS` remains 40 ms
  (25 Hz). BL-005 only ever paced rendering, never physics or gameplay speed.
- **Render loop** calls `sleep_frame()` — a fixed 16 ms `nanosleep`, identical
  to the pre-BL-005 implementation.
- **Persistence container** is unchanged: records 1–6 only, record 7 gone, and
  the container version untouched.

## 5. BL-004 volume is retained

BL-004 was explicitly excluded from the cancellation. Verified present in the
committed tree after `e24f2ae`:

| Symbol | References |
|--------|-----------|
| `BOUNCE_AUDIO_VOLUME_STEP` | 15 |
| `bounce_app_flow_settings_volume` | 38 |
| `BOUNCE_SETTINGS_AUDIO_ROW_VOLUME` | 11 |
| `BOUNCE_PERSISTENCE_RECORD_VOLUME` | 4 |
| `app_sync_audio_volume` | 12 |
| `app_commit_volume_change` | 4 |
| `bounce_audio_player_set_volume` | 12 |
| `bounce_audio_player_null_peak` | 9 |
| `draw_volume_slider` | 3 |
| `bounce_ui_shell_volume_from_track_x` | 12 |
| `BOUNCE_UI_VOLUME_SLIDER_X` | 6 |
| `BOUNCE_LOCALE_VOLUME` | 7 |
| `stored_volume` (0 = mute persists) | 5 |

This covers volume, mute, the slider, volume persistence (record 6) and the
audio player. The five BL-004 checks still run and pass: `AUDIO-VOLUME-RUNTIME`,
`VOLUME-ZERO`, `AUDIO-PEAK-RESET`, `VOLUME-INPUT`, `VOLUME-SLIDER`.

## 6. Verification results

Recorded on the tree **after** commit `e24f2ae`:

| Command | Result |
|---------|--------|
| `make -C native/app` | exit 0 — no warnings under `-Wall -Wextra -Werror -pedantic`, including a full rebuild from `make clean` |
| `make -C native/app check` | exit 0 — **126 PASS / 0 FAIL** |
| `make -C native/app check-audio-player` | exit 0 — PASS (0 failures) |
| `make -C native/app check-audio-runtime` | exit 0 — PASS (0 failures) |
| `git diff --cached --check` (before commit) | exit 0 |

A full-text search of the committed source for `fps`, `target_fps`,
`BOUNCE_FPS`, `DISPLAY_FPS`, `TARGET_FPS`, `RECORD_FPS`, `fps_edit`,
`BOUNCE_UI_INPUT_DIGIT`, `BOUNCE_UI_INPUT_BACKSPACE`,
`BOUNCE_INPUT_SOURCE_BACKSPACE`, `sleep_frame_fps`, `load_fps`, `save_fps`,
`restore_persisted_fps` and `settings_fps` returns **zero matches**.

### The five BL-005 verifiers are no longer invoked

`FPS-RANGE`, `FPS-FIELD`, `FPS-FIELD-PERSISTENCE`, `DISPLAY-FPS-ROW` and
`FRAME-PACING` were removed together with the feature and no longer appear
anywhere in the source. The check count consequently fell from 131 at `c2901b9`
to 126; a set comparison of check identifiers confirmed that exactly those five
disappeared, that nothing was added, and that no BL-004 check was lost.

A mutation test was run to confirm the remaining suite still guards the BL-004
path: reintroducing the LEFT/RIGHT fall-through bug made `make check` exit
non-zero, and the suite passed again once the mutation was reverted.

## 7. Limitations and caveats

1. **Stale FPS records are not cleaned up.** A version-2 container written by an
   earlier build may still contain a record-7 FPS entry. The loader no longer
   reads record 7 and `write_container()` no longer writes it, so the entry is
   **inert** — but it is not garbage-collected. Any future writer that
   reconstructs the directory will simply omit it.

2. **Committed locally, not pushed.** As of this report `master` is ahead 1 and
   behind 0. The cancellation exists only in the local repository.

3. **`audit/` is untracked.** This directory, including both BL-005 reports, is
   not under version control and is not part of commit `e24f2ae`.

4. **The historical report is not a description of current code.** See
   `BL-005-fps-report.md` and its superseded banner. In particular its
   "Features Verified" list contains claims that later audits found inaccurate
   (the FPS value was never actually painted on screen, among others).

5. **ALSA hardware remains unproven.** The real device cannot be opened in this
   environment, so only the Null backend audio path is measured. This is
   unchanged by the cancellation.

6. **No full parity with the reference for volume.** BL-004 volume commits on
   input change rather than on OK, and BACK does not discard the pending value
   as `BounceUI.java:308` does. This deviation predates the cancellation and is
   unaffected by it.

## 8. Relationship to the historical report

| | `BL-005-fps-report.md` | This report |
|---|---|---|
| Describes | The feature as implemented, before cancellation | The repository as it stands now |
| Status | Superseded, historical only | Authoritative for current state |
| Edited during cancellation? | **No** — body preserved verbatim; a banner was prepended | New file |
| Claims accurate? | Partly; several disproved by later audits | Verified against the committed tree |

The historical report was deliberately **not** rewritten. It records what was
believed and built at the time, which is part of the audit trail; correcting it
in place would have erased that record and would have made it appear the
implementation never existed.
