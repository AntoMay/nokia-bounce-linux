# Implementation Backlog — LKT Pro Parity Gaps

> **DOCUMENTATION ONLY — this is not an active backlog.** The BL-xxx items below
> record gaps found by a comparison against the LKT Pro mod. Per project decision the
> mod is not an active parity target and will be dropped from the workflow later, so
> these entries are kept as documentation of the audit's findings, not as a queue to
> work through. Do not treat them as tasks; BL-005 (FPS) was already formally
> cancelled (`BL-005-cancellation-report.md`) and BL-004/BL-006 (volume) shipped
> before this status change.

Ordered by impact, dependency, and risk. Each task is independently verifiable.

---

## Priority 1 — High Impact, Low Risk

### BL-001: Wire scoring events to score field

**Gap:** Hoop (+500), gem (+200), life gem (+1000), and level exit (+5000) point values are not verified as wired to the score field.

**Evidence:**
- Reference: `Ball.java:530` (ring +500), `Ball.java:811` (gem +200), `Ball.java:933` (life +1000), `BounceCanvas.java:452` (exit +5000)
- Native: `game.h:206` (score field exists), `game.c:1487` (`bounce_game_add_score()` exists), but no gameplay caller verified

**Acceptance criteria:**
 Collecting a hoop, gem, life gem, and exiting a level each add the correct points to `game->score`.

**Dependencies:** None

**Verification:** Use `NBB_DEBUG=2` to observe score changes during gameplay.

---

### BL-002: Implement max lives cap (5)

**Gap:** The mod caps lives at 5 (`Ball.java:934`); native has no explicit cap.

**Evidence:**
- Reference: `Ball.java:934` — `if (numLives < 5)`
- Native: `game.h:182` (lives field), no cap found

**Acceptance criteria:** Collecting a life gem when lives is 5 does not increment lives beyond 5.

**Dependencies:** None

**Verification:** Unit test or debug tracker observation.

---

### BL-003: Wire sound pickup events in collision query

**Gap:** `sound = this.n.soundPickup` is not implemented in the collision query.

**Evidence:**
- Reference: `Ball.java:485` — `sound = this.n.soundPickup` on tile-7 collection
- Native: `collision_query.c:653,676` — documented as not implemented

**Acceptance criteria:** Collecting a crystal (tile 7) or gem (tile 29) triggers the pickup sound event.

**Dependencies:** BL-001 (scoring wiring)

**Verification:** Audio event check or runtime observation.

---

## Priority 2 — Medium Impact, Low Risk

### BL-004: Implement invincibility cheat

**Gap:** The mod has an invincibility cheat (7-8-*-#-8-* sequence → `mInvincible`); native does not.

**Evidence:**
- Reference: `BounceCanvas.java:545-550` — sequence → `mInvincible = true`; `Ball.java:516` — `if (!this.mCanvas.mInvincible)` guard
- Native: No invincibility flag or cheat sequence

**Acceptance criteria:** Entering the invincibility cheat sequence prevents ball death from hazards.

**Dependencies:** None

**Verification:** Enter cheat sequence, touch spike, verify no death.

---

### BL-005: Implement FPS setting

**Gap:** The mod has 10-60 FPS adjustment; native has fixed 40ms tick.

**Evidence:**
- Reference: `BounceUI.java:552` — TextField 10-60; `GameTimer.java:19` — `1000/mFps` ms
- Native: `game_timer.c` — fixed 40ms canonical tick; no FPS setting

**Acceptance criteria:** Changing FPS setting in settings menu changes the game tick rate.

**Dependencies:** None

**Verification:** Measure tick rate at different FPS settings.

---

### BL-006: Implement volume control (0-100)

**Gap:** The mod has 0-100 volume control; native has on/off gate only.

**Evidence:**
- Reference: `BounceUI.java:553` — Gauge 0-100; `BounceCanvas.java:667-669` — `VolumeControl.setLevel()`
- Native: `app_flow.h:324-339` — `audio_enabled` boolean gate; no volume level

**Acceptance criteria:** Volume setting in settings menu controls audio output level.

**Dependencies:** None

**Verification:** Measure audio output at different volume settings.

---

### BL-007: Persist language setting

**Gap:** The mod persists language in settings RMS; native does not persist language.

**Evidence:**
- Reference: `BounceUI.java:607` — language saved in settings record
- Native: `app_flow.h:318` — `applied_language_index` documented as "never persisted"

**Acceptance criteria:** Changing language and restarting the game preserves the language choice.

**Dependencies:** None

**Verification:** Change language, restart, verify language is preserved.

---

## Priority 3 — Medium Impact, Medium Risk

### BL-008: Implement profiling system

**Gap:** The mod has a profiling system with 10 timing fields; native does not.

**Evidence:**
- Reference: `Profile.java` — 10 static timing fields for cleanBuffer, drawTile, paint2Buffer, run, paint, testTile, update, collisionDetection
- Native: No profiling system

**Acceptance criteria:** Profiling data is collected and can be reported.

**Dependencies:** None

**Verification:** Enable profiling, verify timing data is collected.

---

### BL-009: Implement pop animation frames

**Gap:** The mod has a 5-frame pop animation; native does not explicitly model pop frames.

**Evidence:**
- Reference: `Ball.java:515-527` — 5-frame pop animation with `popCntr`
- Native: `game.c:1206` — `bounce_game_mark_dead()` but no pop animation frames

**Acceptance criteria:** Ball pop animation plays for 5 frames before respawn.

**Dependencies:** None

**Verification:** Visual observation or render trace.

---

### BL-010: Wire pause functionality

**Gap:** Native pause flag has no consumer; mod has no pause state but has a pause constant.

**Evidence:**
- Reference: `BounceConst.java:254` — `STATE_GAME_PLAYING` (no explicit pause state)
- Native: `input_state.h:171` — pause flag exists but has no consumer; `app_flow.h:22` — BOUNCE_APP_STATE_PAUSE exists

**Acceptance criteria:** Pressing pause during gameplay pauses the game and shows a pause screen.

**Dependencies:** None

**Verification:** Press pause, verify game stops and pause screen appears.

---

## Priority 4 — Low Impact, Low Risk

### BL-011: Align GodMode cheat sequence

**Gap:** GodMode sequences differ: mod uses 7-8-7-8-9-9, native uses 7-8-7-8-9-8.

**Evidence:**
- Reference: `BounceCanvas.java:537-571` — 7-8-7-8-9-9 → `mCheat`
- Native: `game.h:244-251` — 7-8-7-8-9-8 → `god_mode`

**Acceptance criteria:** GodMode sequence matches the mod's sequence.

**Dependencies:** None

**Verification:** Enter mod's sequence, verify GodMode activates.

**Note:** This is a behavioral difference, not necessarily a bug. The native sequence may be intentional. Confirm with project owner before changing.

---

### BL-012: Add QWERTY keyboard mapping

**Gap:** The mod has full QWERTY→keypad mapping; native uses key aliases but with a different approach.

**Evidence:**
- Reference: `BounceCanvas.java:471-508` — full QWERTY→keypad mapping
- Native: `input_state.h:92-162` — KP_UP/DOWN/LEFT/RRIGHT + diagonals

**Acceptance criteria:** QWERTY keys (r/t/y/f/g/h/v/b/n/u/j/m) map to the same actions as the mod.

**Dependencies:** None

**Verification:** Press QWERTY keys, verify correct actions.

**Note:** The native approach is different but may be functionally equivalent. Verify before implementing.

---

### BL-013: Investigate Level 11 byte difference

**Gap:** Level 11 files differ by one byte at offset 0x1D98 (0x01 in LKT Pro vs 0x61 in native).

**Evidence:**
- Reference: `resources/levels/J2MElvl.011` — byte 0x1D98 = 0x01
- Native: `src/main/resources/levels/J2MElvl.011` — byte 0x1D98 = 0x61

**Acceptance criteria:** Determine which version is correct and align.

**Dependencies:** None

**Verification:** Compare level maps and determine which byte is correct.

---

### BL-014: Investigate lang.xx content difference

**Gap:** lang.xx files differ (889 bytes in LKT Pro vs 730 bytes in native). Native is missing settings-related strings.

**Evidence:**
- Reference: `resources/lang.xx` — 889 bytes, 30 strings including settings
- Native: `src/main/resources/lang.xx` — 730 bytes, fewer strings

**Acceptance criteria:** Determine if native lang.xx should include the missing strings.

**Dependencies:** None

**Verification:** Compare lang.xx contents and determine if strings should be added.

**Note:** The native project has its own settings strings in `locale.c` that are not in the lang.xx file. This may be intentional.

---

## Priority 5 — Low Impact, Medium Risk

### BL-015: Implement ShowLevelComplete Form

**Gap:** The Java ShowLevelComplete() Form is not fully reproduced.

**Evidence:**
- Reference: `BounceUI.java:215-229` — `displayLevelComplete()` Form
- Native: `game.c:1302` — documented as deferred

**Acceptance criteria:** Level complete screen matches the mod's Form.

**Dependencies:** None

**Verification:** Visual comparison.

---

### BL-016: Implement RMS write in completion block

**Gap:** WriteToStore() inside the completion block is elided.

**Evidence:**
- Reference: `BounceCanvas.java:453` — `checkData()` → `saveGameData(1/2)`
- Native: `game.c:1303` — documented as deferred

**Acceptance criteria:** Level completion triggers save data write.

**Dependencies:** None

**Verification:** Complete a level, verify save data is written.

---

## Summary

| Priority | Tasks | Estimated Effort |
|---|---|---|
| 1 — High Impact, Low Risk | BL-001, BL-002, BL-003 | Small |
| 2 — Medium Impact, Low Risk | BL-004, BL-005, BL-006, BL-007 | Small-Medium |
| 3 — Medium Impact, Medium Risk | BL-008, BL-009, BL-010 | Medium |
| 4 — Low Impact, Low Risk | BL-011, BL-012, BL-013, BL-014 | Small |
| 5 — Low Impact, Medium Risk | BL-015, BL-016 | Medium |

**Recommended first task:** BL-001 (wire scoring events to score field) — it is the highest impact, lowest risk, and unblocks verification of other scoring-related features.
