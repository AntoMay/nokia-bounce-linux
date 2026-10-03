#ifndef BOUNCE_NATIVE_APP_GAME_H
#define BOUNCE_NATIVE_APP_GAME_H

#include <stdbool.h>
#include <stdint.h>

#include "app_flow.h"
#include "input_state.h"
#include "persistence.h"
#include "player.h"
#include "../level/level.h"

#include "game_timer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BounceVisualAssets BounceVisualAssets;

/* These are temporary diagnostic owners implemented by the integration shell. */
typedef struct CameraDiagnostic CameraDiagnostic;
typedef struct VerticalDiagnostic VerticalDiagnostic;
typedef struct RenderTrace RenderTrace;

/*
 * STEP 13E -- the terminal debug tracker. NATIVE TOOLING ONLY: a NULL pointer
 * disables it entirely, and nothing in the game reads or writes it. See
 * debug_tracker.h for the full contract.
 */
typedef struct BounceDebugTracker BounceDebugTracker;

/*
 * BounceAppFlow is the authoritative application/UI owner. BounceGameState
 * remains a synchronized gameplay/lifecycle mirror for the existing player,
 * level, temporary native test boundaries, and explicit UI destinations; it
 * does not own menu/list selection, text, splash timing, rendering, or
 * persistence.
 */
typedef enum BounceGameState {
    BOUNCE_GAME_STATE_SPLASH = 0,
    BOUNCE_GAME_STATE_MENU = 1,
    BOUNCE_GAME_STATE_PLAYING = 2,
    BOUNCE_GAME_STATE_LEVEL_COMPLETE = 3,
    BOUNCE_GAME_STATE_DEAD = 4,
    BOUNCE_GAME_STATE_RESPAWN = 5,
    BOUNCE_GAME_STATE_GAME_OVER = 6,
    BOUNCE_GAME_STATE_PAUSED = 7,
    BOUNCE_GAME_STATE_UNKNOWN = 8,
    BOUNCE_GAME_STATE_ACTION_BOUNDARY = 9,
    BOUNCE_GAME_STATE_INSTRUCTIONS = 10,
    BOUNCE_GAME_STATE_HIGH_SCORE = 11,
    BOUNCE_GAME_STATE_LEVEL_SELECTION = 12,
    /*
     * UI 6 boundary mirror: START_LEVEL(N) plus source-verified new-game
     * metadata, with no level, player, timer, or tick. The metadata itself
     * stays owned by BounceAppFlow.
     */
    BOUNCE_GAME_STATE_GAMEPLAY_ENTRY = 13,
    /* Native integration seam mirror; the loaded level is owned by BounceGame. */
    BOUNCE_GAME_STATE_LEVEL_LOADED = 14,
    /*
     * NATIVE EXTENSION mirrors. These name native-only UI destinations that have
     * no recovered-source counterpart. The mirror stays a pure name for the
     * authoritative BounceAppFlow state; it owns no level, player, timer, tick,
     * or configuration.
     */
    /*
     * STEP 12X-P4-B -- the gameplay mirror of BOUNCE_APP_STATE_GAME_END, the
     * game-complete terminal destination (BounceGame.java:430-435 -> :177-197
     * through e.java:321). Deliberately distinct from
     * BOUNCE_GAME_STATE_GAME_OVER, which is the death result from e.java:271.
     */
    BOUNCE_GAME_STATE_GAME_END = 17,
    BOUNCE_GAME_STATE_SETTINGS = 15,
    BOUNCE_GAME_STATE_ABOUT = 16
} BounceGameState;

/*
 * The gameplay context is the owner of the parsed level, its independent
 * mutable tile copy, and the player lifecycle.  Rendering, camera, input
 * presentation, and temporary diagnostics remain fields on the integration
 * shell so this milestone does not refactor those verified systems.
 */
typedef struct BounceGame {
    BounceAppFlow flow;
    BounceGameState state;

    int level_id;
    char *level_path;
    BounceLevel *level;
    BounceRuntimeLevelTiles *runtime;

    BounceVisualAssets *visuals;
    /* Both header modes initialize; the existing physics seam is regular-only. */
    BouncePlayer player;
    BouncePlayerSpawn spawn;
    /*
     * STEP 12X-P4-C-D-C-B4-C -- the mutable CRYSTAL CHECKPOINT, Java f.d / f.c / f.b.
     *
     * f.java:480-486, the tile-7 case:
     *
     *     case TileIDs.CRYSTAL -> {
     *         this.n.AddScore(200);
     *         this.n.LevelTiles[this.c][this.d] = 0x80 | TileIDs.EMPTY;
     *         a(tileX, tileY);
     *         this.n.LevelTiles[tileY][tileX] = 0x80 | TileIDs.CRYSTAL_ACTIVE;
     *         sound = this.n.soundPickup;
     *     }
     *
     * and the setter it calls, f.java:134-138:
     *
     *     public void a(int tileX, int tileY) {
     *         this.d = tileX;
     *         this.c = tileY;
     *         this.b = this.ballSize;
     *     }
     *
     * So the source's checkpoint is three ints: column, row, and a VALUE SNAPSHOT of the
     * ball size at the instant of collection. `b` is a snapshot and not a reference, which
     * is why it is copied here rather than read from the player at respawn time.
     *
     * THESE ARE NOT `spawn`. `spawn` is the level's IMMUTABLE initial spawn record, derived
     * from the level header, and it is what the source's InitializeGame() uses to establish
     * the FIRST checkpoint (e.java:122). Roughly twenty existing assertions require spawn to
     * remain exactly that value forever -- see the memcmp(&app.spawn, &spawn_before, ...)
     * harnesses in vertical_slice.c and the initial_x / constructor_x checks -- so the
     * mutable checkpoint is a separate field and spawn is read exactly once, at
     * initialization.
     *
     * NO VALIDITY FLAG, deliberately. The source has none either, because Java's
     * InitializeGame() establishes the checkpoint before play begins, so "no checkpoint" is
     * unreachable there. Native has the identical structure: these three are written in
     * bounce_game_initialize_player_entry(), which the staging chain in
     * app_begin_canonical_gameplay() always runs before any tile walk. Adding a flag would
     * introduce a state the source does not have and that no code path can reach.
     *
     * LIFETIME: per level. Initialized from the level's spawn at player entry; replaced by
     * every tile-7 pickup; never cleared by death or by the respawn that consumes it.
     * A level transition, a New Game and a Continue all re-run player entry, which is what
     * resets it -- the same reason Java's e.java:122 resets it, since Java's Continue is
     * always followed by the next level's InitializeGame().
     *
     * NOT REPRODUCED, deliberately: f.java:613 `this.n.y = true` (a HUD dirty flag with no
     * native equivalent -- BounceGame has no such field). Neither is invented here.
     * f.java:485 `sound = this.n.soundPickup` IS reproducible and the soundPickup
     * identity exists, but this particular statement is still absent: the tile-8
     * case in app_vertical_collision_info() awards nothing to sound. See that
     * branch's comment, which records why.
     */
    int32_t checkpoint_tile_x;
    int32_t checkpoint_tile_y;
    int32_t checkpoint_ball_size;
    BounceInputState input;

    /*
     * Game timer, owned by value like the player. It holds the source-derived
     * nominal period and the Tick-entry observation count only: no deadline, no
     * accumulator, no delta state, and no catch-up model, because the original
     * has none. See game_timer.h.
     */
    BounceGameTimer game_timer;
    /*
     * Backend deadline for the canonical 40 ms timer, owned by the frame loop
     * that drives it (BounceTimer.java:17 fixed delay). It is deliberately NOT
     * part of BounceGameTimer, which stays free of any deadline or accumulator,
     * and it is separate from the legacy next_player_tick_ms, which
     * app_run_player_ticks() still owns unchanged.
     */
    int64_t canonical_next_tick_ms;
    /*
     * Pure telemetry for the double-physics protection check: counts production
     * frames whose gameplay update was routed to the legacy app_update_game()
     * path while the game state was PLAYING, i.e. the only frames in which
     * app_run_player_ticks() was permitted to run player ticks. It is never read
     * for control flow, so it cannot alter behavior. It must remain 0 across a
     * canonical gameplay window.
     */
    int64_t legacy_player_tick_route_count;

    /* New-game counters have source evidence; later mutation is a TODO. */
    int lives;
    int hoops_scored;
    /*
     * STEP 12X-P4-C-D-B -- the native score core.
     *
     * e.java:33 `public int score;`, owned by the single per-session `e` object.
     * `int32_t` is the exact signed 32-bit counterpart of a Java `int`, and the
     * only arithmetic performed on it is bounce_game_add_score(), which wraps
     * through the project's existing java_int32_add() helper so that C's
     * undefined signed-overflow behaviour can never be invoked.
     *
     * The score is deliberately NOT reset by a level load, a level transition, a
     * death, a respawn or a menu visit. Only a New Game zeroes it, matching
     * BounceGame.a(boolean, int) at BounceGame.java:151, where only the
     * `paramBoolean == true` branch calls e.a(level, 0, 3). e.java:115-126
     * InitializeGame() never touches score, which is why a level change cannot
     * reset it.
     *
     * No gameplay event is connected to this field yet. The hoop (+500), the
     * checkpoint crystal (+200), the crystal ball (+1000) and the level
     * completion bonus (+5000) integrations are deliberately deferred, as is
     * every read of this field: neither the HUD nor the Game End form reads it
     * yet.
     */
    int32_t score;
    bool level_complete;
    bool dead;
    bool respawn_requested;
    bool game_over;
    /*
     * STEP 45A -- e.java:55, GodMode, the death-suppression latch.
     *
     * `public boolean GodMode = false;`, declared on the single per-session `e`
     * gameplay canvas, which is the object `f` holds as its back-pointer
     * (`f.java:97 public e n;`, assigned at `f.java:114`). That back-pointer is
     * why the reader is a player-to-canvas query rather than a local.
     *
     * The field has exactly three references in the whole Java tree:
     *   e.java:55   declaration, default false
     *   e.java:363  the ONLY writer, `this.GodMode = true;`
     *   f.java:216  the ONLY reader, `if (!this.n.GodMode) {`
     * so it guards the whole body of f.KillBall() and nothing else anywhere.
     *
     * ONE-WAY AND SESSION-SCOPED, both properties preserved here. There is no
     * `GodMode = false` after the initialiser anywhere in the source, so this is
     * a monotonic false->true latch with no deactivation path. It is also absent
     * from BounceGame.LoadRecords()/WriteToStore() (BounceGame.java:269-300
     * carries only RecordLives, RecordHoopsScored, RecordLevel, RecordScore and
     * LastRecordTimestampMillis), so it is deliberately NOT persisted, and it is
     * not reset by a level load, a death, or a respawn -- matching the source,
     * where only the whole `e` object ending can clear it.
     *
     * IT IS NOT A NOCLIP FLAG. In BOTH Java call sites the movement block is
     * assigned before and independently of KillBall():
     *   f.java:469  bool = false;  KillBall();   (dynamic thorns, tile 10)
     *   f.java:476  bool = false;  KillBall();   (THORNS_UP/RIGHT/DOWN/LEFT)
     * so GodMode suppresses the death and the life loss while the collision
     * stays fully blocking. The native already mirrors that ordering, with
     * `info->blocked = true` set at vertical_slice.c:3087 outside both
     * app_kill_ball() calls; the guard in app_kill_ball() must never move that
     * assignment inside itself.
     *
     * REACHABLE. The only writer is the 7 8 7 8 9 8 sequence detector at
     * e.java:350-380, and STEP 46G wired it to the keyboard: an ordered press
     * stream reaches it through x11_dispatch_event() -> x11_god_mode_sequence_step()
     * (vertical_slice.c:24322), which sets this field at :24326. The numeric
     * keypad's 7/8/9 feed that same detector, so top-row, keypad and mixed runs
     * all arm it. Until the player types the sequence this field is false and the
     * guarded body runs, which is exactly the pre-45A behaviour.
     */
    bool god_mode;
    /*
     * STEP 64-AF -- e.af, the developer cheat latch (e.java:53).
     *
     *     private boolean af = false;              // e.java:53
     *
     * PRODUCER, e.java:374-378, inside keyPressed's `case 57:` ('9') arm:
     *
     *     if (this.g == 4) { this.g++; break; }
     *     if (this.g == 5) {                      // e.java:375
     *         this.soundPop.play(1);
     *         this.af = true;                     // e.java:376
     *         this.g = 0;
     *         break;
     *     }
     *     this.g = 0;
     *
     * i.e. the digit sequence 7 8 7 8 9 9. It is the SAME `g` automaton that
     * arms god_mode, and it is one step away from it: the 7 8 7 8 9 8 sequence
     * closes the cycle through e.java:363 (GodMode = true), while 7 8 7 8 9 9
     * closes it through e.java:376 (af = true). The two are DISTINCT variables
     * with distinct writers and are never conflated; arming one does not arm
     * the other.
     *
     * LIFECYCLE. The memset in bounce_game_init (game.c:132) supplies the
     * e.java:53 initial value, so a new game starts with af == false and this
     * step adds no initialiser. Java has exactly two writers and no reset:
     * the declaration above and e.java:376. No clearing writer is therefore
     * reproduced, and none is invented -- notably, resetting af when god_mode
     * is reset would be unsupported, because Java's GodMode is one-way
     * (e.java:363) and af has no counterpart path at all.
     *
     * The four e.java consumers are e.java:341 ('1'), :346 ('3'), :383 ('#')
     * and :406 (getGameAction case 8). See the STEP 64-AF report for which are
     * reproduced natively and which are not, and why.
     */
    bool af;
    /*
     * STEP 12X-G-P3 -- b.d, the deferred-initialization latch.
     *
     * Java declares it on the base canvas (b.java:129 public boolean d) and
     * reads it in exactly one place, the first statement of Tick():
     *
     *   e.java:222-226   if (this.d) { InitializeGame(); repaint(); return; }
     *
     * It is written in only three places: false in the b constructor
     * (b.java:202), false at the head of LoadLevelId (b.java:213), and true in
     * the level-completion block (e.java:316, immediately before the
     * `this.level++` at :317). The two remaining `this.d = true` writes at
     * e.java:340 and :345 belong to the af-gated debug level-skip keys and have
     * no native counterpart.
     *
     * So it is a one-shot latch with a single producer and a single consumer:
     * completion arms it, the next tick that runs after control returns to the
     * canvas consumes it, and the level load inside that consumption disarms it
     * again. It is explicitly NOT "level complete", "next level requested",
     * "continue available" or "gameplay active"; those are level_complete, the
     * P2 progression fields, the absent continue_available writer, and the
     * existing game state respectively.
     *
     * NOT the BOUNCE_GAMEPLAY_ENTRY_PHASE_* enum. That phase is a staged-start
     * progress marker that only advances while the flow is in GAMEPLAY_ENTRY or
     * LEVEL_LOADED and is discarded by the staging driver; this latch is armed
     * from LEVEL_COMPLETE, survives bounce_game_release_level(), and is cleared
     * by a level load rather than by a stage transition. No phase value is legal
     * in LEVEL_COMPLETE, and this step adds no enum value.
     */
    bool deferred_init;
    /*
     * STEP 65-B2 -- af_reload_requested, the e.java:340/:345 half of b.d.
     *
     * game.h:297-299 above records, for deferred_init, that "the two remaining
     * `this.d = true` writes at e.java:340 and :345 belong to the af-gated
     * debug level-skip keys and have no native counterpart". This field is the
     * missing half of that statement. Java writes the SAME variable from all
     * three producers:
     *
     *   e.java:316   this.d = true;   level completion          -> deferred_init
     *   e.java:340   this.d = true;   key '1'                   -> this field
     *   e.java:345   this.d = true;   key '3'                   -> this field
     *
     * and reads it in one place, the first statement of Tick() (e.java:222).
     * The native reader cannot be shared: deferred_init is consumed by the
     * pre-guard branch at game.c:897, which is deliberately placed before the
     * gameplay guard (game.c:847-853) so it also fires while LEVEL_COMPLETE is
     * still on screen, and which calls bounce_game_timer_stop() at game.c:901.
     * That stop is correct for a finished run and fatal for a live one: the
     * reload it performs leaves runtime == NULL and the player uninitialised
     * (bounce_game_load_level_entry writes only level, level_path, level_id,
     * flow.level_id and deferred_init -- game.c:516-519, :545), so the next
     * tick fails the guard at game.c:907, :908 and :914 and returns -1, which
     * the frame loop turns into EXIT_FAILURE. There is no stop-then-restart
     * path either: bounce_game_timer_start() has exactly one caller,
     * bounce_game_initialize_game_timer (game.c:811), which requires
     * entry_phase == BOUNCE_GAMEPLAY_ENTRY_PHASE_ACTIVATED (game.c:799-800)
     * and that phase is not re-established by a reload. So this is a SEPARATE
     * latch, not a repurposing of that one, and its name says which producer it
     * serves.
     *
     * THE CONSUMPTION POINT ALREADY EXISTED. bounce_game_tick_dispatch has
     * carried a `reset_pending` parameter for this exact position since before
     * the reload existed: game.c:917-922 documents the slot as "e.java:222-226.
     * The source runs InitializeGame() and repaint() here and returns", and
     * game.c:923-927 already returns BOUNCE_TICK_DISPATCH_RESET_GATE with rc 0
     * before timer_record_dispatch (:970), before player_tick_entry (:982) and
     * before the physics body (:1001). Only the producer was missing: the sole
     * production caller passed a literal false (vertical_slice.c:5626).
     *
     * ONE-SHOT, like its Java counterpart. Armed by the af-gated '1'/'3'
     * consumer, consumed by exactly one reset-gate dispatch, and cleared by
     * that consumer once the reload and the camera have both succeeded. It is
     * cleared AFTER the work rather than before, because the clear inside
     * bounce_game_load_level()'s own lifecycle does not exist: that function
     * touches neither this field nor the timer, and adding a clear to
     * bounce_game_release_level() would have meant changing game.c for a latch
     * that never survives a level release anyway. The memset in
     * bounce_game_init (game.c:132) supplies the false initial value, the same
     * way it supplies af's, so no initialiser is added.
     *
     * IT CARRIES NO LEVEL. The target is Java's `this.level`, which the key
     * handler mutates in place (e.java:341, :346) and which InitializeGame()
     * then passes to LoadLevelId (e.java:117). The native counterpart is the
     * existing game->flow.level_id -- already written by
     * bounce_game_load_level at game.c:363 and already read as the level to
     * load by the deferred branch at game.c:898 -- so no second field and no
     * pending-level structure is introduced.
     *
     * NOT a menu action, not a flow state, and not reachable outside gameplay.
     * Its only writer sits inside the existing
     * `app->flow.state == BOUNCE_APP_STATE_GAMEPLAY` gate in
     * x11_process_events, and its only consumer is inside the gameplay tick.
     */
    bool af_reload_requested;
    /*
     * STEP 65-H -- completion_requested, the e.java:407 half of e.e.
     *
     * THE FOURTH WRITER OF JAVA'S `e`, AND THE ONLY ONE WITH NO PRODUCER YET.
     * game.h:297-299 above records that "the two remaining `this.d = true`
     * writes at e.java:340 and :345 belong to the af-gated debug level-skip
     * keys and have no native counterpart". Those two became af_reload_requested
     * in STEP 65-B2. This field is the fourth `e` writer that remains:
     *
     *   e.java:407   if (this.af) this.e = true;    <- W1, getGameAction == 8
     *
     * which is a different variable from `d` entirely. e.e (e.java:39) has three
     * writers -- e.java:407, f.java:602 and f.java:663 -- and one reader, the
     * completion pipeline at e.java:313-327. Two of those three writers already
     * reach the native pipeline: f.java:602 is the tile-9 + M collision that
     * calls bounce_game_mark_level_complete() at vertical_slice.c:2952, and
     * f.java:663 is a dead store that no tick can ever consume (STEP 65-F).
     * So the exit-door route needs no request flag because native calls the
     * pipeline synchronously at the trigger site, and the last-life route needs
     * no flag at all because Java never consumes it. W1 is the one writer whose
     * timing genuinely differs from its trigger, so it is the one writer that
     * needs state.
     *
     * WHY NOT af_reload_requested. That field is the same SHAPE -- written from
     * the input callback, consumed by the next tick, cleared after consumption
     * -- but a different MEANING, and STEP 65-G classified it B rather than A for
     * exactly this reason. It carries a target level id in flow.level_id and its
     * consumer performs a level reload. This field carries no payload and its
     * consumer performs the completion pipeline. Sharing one flag would force
     * either two meanings onto one boolean or the reload path to run completion
     * as well. The timings also differ and must not be merged: af_reload_requested
     * is consumed BEFORE that tick's physics (the RESET_GATE arm returns from
     * bounce_game_tick_dispatch at game.c:926, ahead of timer_record_dispatch at
     * :970 and the body at :1001), whereas this field is consumed AFTER physics,
     * because e.java:313 is the last statement block of Tick().
     *
     * ONE-SHOT, like its Java counterpart. e.java:314 clears `e` before any
     * effect runs, and the pipeline terminates the run anyway
     * (game.c:1266-1267 set level_complete and the LEVEL_COMPLETE states), so a
     * second consumption is not reachable. This field is cleared by its consumer
     * only after bounce_game_mark_level_complete() has succeeded.
     *
     * THE PHYSICAL EVENT IS NOT SOURCE-DETERMINED. What arms e.java:407 is
     * getGameAction(keyCode) == 8, which is MIDP's Canvas.FIRE. That mapping is
     * NOT RECOVERABLE from this source: Canvas.getGameAction delegates to KeyMap
     * and then to ITUKeyMap, whose keyToGameActionMap is built by ACC_NATIVE
     * methods whose JNI library is absent from the repository (STEP 65-C,
     * STEP 65-D -- EXTERNAL-MAPPING-NOT-RECOVERED).
     *
     * BIND ANYWAY, LABELLED AS A BINDING. STEP 65-H bound XK_c to this field
     * (app_w1_completion_key at vertical_slice.c:24018, which also requires af
     * per e.java:406, and the call site at :24447). That is a NATIVE BINDING, NOT
     * A RECOVERY: XK_c is chosen, not derived, and no claim is made that the
     * handset's FIRE key was 'c'. The field is therefore reachable, but reaching
     * it does not recover the mapping it stands in for.
     *
     * The memset in bounce_game_init (game.c:132) supplies the false initial
     * value, the same path that supplies af and af_reload_requested, and
     * bounce_game_release_level clears level_complete beside it, so no stale
     * request can survive into a new level and no initialiser is added.
     */
    bool completion_requested;
    /*
     * G-R3-T2 -- resume_record3, the mid-level session that the main menu
     * outlives, and the reason it can be written at all.
     *
     * WHY THIS FIELD EXISTS. Java's snapshot is not taken while gameplay is
     * running. BounceGame.java:352-353 guards WriteToStore(3) with
     * `if (this.v == null || this.v.aq == null) return;`, but `this.v` is built
     * in the BounceGame constructor (BounceGame.java:96, `new e(this, 1)`) and
     * `this.v.aq` in the e constructor (e.java:127, `new f(...)`). Neither can
     * ever be null after construction, so the guard NEVER trips and
     * WriteToStore(3) writes a record on EVERY destroyApp. "No snapshot" is not
     * represented by absence in Java; it is represented by b1 == 0, because
     * b1 encodes K (BounceGame.java:354-359) and a fresh install has K == 2
     * (:19), and BounceGame.java:225 only resumes when J != 0.
     *
     * Java can do this because ShowMainMenu() (BounceGame.java:110-133) never
     * destroys `this.v` -- it only calls v.StopGameTimer() at :131 and shows a
     * separate List at :132. The canvas outlives the menu, so destroyApp() still
     * finds a populated session.
     *
     * Native CANNOT do that, and this field is the measured reason. Native
     * fuses the canvas into the flow and destroys the session when the flow
     * leaves gameplay: app_end_canonical_gameplay() at vertical_slice.c:19263
     * (Escape) and :19186 (game end) call bounce_game_release_level(), which
     * destroys the runtime tiles and the level, resets the player and zeroes
     * the camera (game.c:284-309). A live instrumented run measured the
     * consequence at the menu-EXIT site itself: after really playing level 1
     * with lives == 3, every field Record 3 would serialise was already
     * NULL/zero there. Native therefore has nothing to serialise unless the
     * session is captured BEFORE that release and held here.
     *
     * WHY THE CAPTURE IS AT THE GAMEPLAY EDGE AND THE WRITE IS AT THE MENU
     * EDGE. Those are the two edges Java itself has, and separating them keeps
     * each one narrow:
     *   capture  the instant gameplay ends, before the release, from the live
     *            session -- the only moment the values still exist;
     *   write    the explicit main-menu Exit, which is the only Record 3
     *            writer in the Java tree (Bounce.java:25, from destroyApp).
     * The capture performs NO I/O, so a gameplay-to-menu transition still never
     * touches the store, which is what the T2 decision requires.
     *
     * Nothing here is consumed yet. There is no resume path, no Continue
     * availability flag and no menu handler; G-R3-R1 and the Continue wiring
     * own those. bounce_game_init's memset (game.c:132) supplies the zeroed
     * initial value, and bounce_game_release_level deliberately does NOT clear
     * it: the capture runs immediately before that call, and clearing it here
     * would destroy the very state the field exists to hold.
     */
    BouncePersistenceRecord3 resume_record3;
    /*
     * G-R3-R1 -- resume_requested, the one-shot latch that marks this entry as a
     * Record 3 resume rather than a New Game, Level Selection or terminal
     * Continue.
     *
     * WHY A FLAG AT ALL. R1-E established that new_game_score_reset_pending
     * separates {New Game, Level Selection} from {Continue, Resume}, so it
     * cannot tell a resume from a terminal Continue. The gameplay-entry phase and
     * pending_action fields cannot either: all four paths deliberately reach stage
     * 1 through the identical START_LEVEL guard chain (R1-B), which is what makes
     * the chain reusable. This flag is the discriminator, and it is consumed once,
     * inside the restore window, before the first tick.
     *
     * IT DOES NOT CARRY THE PAYLOAD. The payload is resume_record3 above, and the
     * level and lives were already recorded into the flow by
     * bounce_app_flow_record_resume_entry(). This is a marker, not a second copy
     * of the snapshot.
     *
     * The memset in bounce_game_init supplies false, and it is cleared by the
     * restore window once consumed, so a stale request cannot survive into a later
     * New Game.
     */
    bool resume_requested;
    /*
     * D-03/D-04 -- continue_record_pending, the one-shot latch that marks this
     * entry as Java's J == 2 branch rather than J == 1.
     *
     * JAVA EVIDENCE. BounceGame.java:225-233 offers two different resumes behind
     * the single Continue row:
     *
     *     } else if (this.J != 0) {
     *         this.display.setCurrent(this.v);
     *         if (this.J == 1) {
     *             this.v.a(this.y, this.M);          <-- restore position
     *         } else {
     *             this.v.a(this.RecordLevel, this.RecordScore,
     *                      this.RecordLives);         <-- RESTART the level
     *         }
     *         this.u = null;
     *         this.v.StartGameTimer();
     *         this.K = 1;
     *     }
     *
     * The two targets are genuinely different operations. e.a(int,int) at
     * e.java:92-114 is the mid-level restore: it calls k() and AddScore() and
     * puts the ball back on its recorded tile. e.a(int,int,int) at e.java:81-90 is
     * a fresh start of the recorded level -- it sets HoopsScored = 0, e = false,
     * TODO_ExitUnlocked = false and then calls InitializeGame(), which resets the
     * countdown and spawns the ball from the level's own init point. Nothing in
     * the J == 2 branch touches the position.
     *
     * SO THE EXISTING resume_requested FLAG CANNOT COVER IT. That flag means "run
     * app_resume_restore_payload()", i.e. the J == 1 branch, and its window
     * restores the tile, the power-ups, the thorns and the ball. Running it for a
     * J == 2 record would put the player back where they finished the level
     * instead of starting it, which is the opposite of what the source does.
     *
     * IT DOES NOT CARRY THE PAYLOAD either, for the same reason
     * resume_requested does not: the score lives in resume_record3 above, and the
     * level and lives were recorded into the flow by
     * bounce_app_flow_begin_record_continue_entry(). This is a marker, not a
     * second copy of anything. bounce_game_init's memset supplies false and the
     * window clears it once consumed.
     */
    bool continue_record_pending;
    /*
     * e.TODO_ExitUnlocked - set once HoopsScored reaches HoopsTotal
     * (e.java:284-285). It is NOT completion: the source keeps four further
     * gates between this flag and the completion pipeline (e.java:286 window
     * plus b.z, the b.h() door animation, f.java:601 tile 9 gated on M, then
     * e.java:313). Only the flag and its producer exist here; none of those
     * later stages is reproduced. Kept distinct from level_complete, which
     * stands for the terminal e.java:313 handler and whose equivalence to the
     * original e flag is UNRESOLVED.
     */
    bool TODO_ExitUnlocked;

    /*
     * D-09 -- door_animation_active IS GONE, and the note that used to be here
     * said:
     *
     *     CORRECTION (STEP 63). ... Source: door_animation_active is the
     *     `else h()` arm of the gate (e.java:291), i.e. it is Java's `!M`;
     *     b.z is a different variable.
     *
     * THE SECOND HALF OF THAT NOTE WAS THE BUG. e.java:286 does not test M in its
     * CONDITION; it tests three operands and then BRANCHES on M inside the body:
     *
     *     286:  if (this.TODO_ExitUnlocked && this.z && ...) {
     *     288:      if (this.M) {
     *     289:          this.z = false;
     *     290:          this.TODO_ExitUnlocked = false;
     *     291:      } else {
     *     292:          h();
     *
     * So "Java's `!M`" is not an operand at all, and the native gate that used
     * `!door_open_flag` to stand for it turned the whole block off the instant the
     * animation finished -- which made the e.java:289-290 one-shot clear
     * unreachable on every tick of every run. The field existed only to carry that
     * wrong reading, so it is removed rather than left written and never read.
     * door_image_offset is b.b, door_open_flag is b.M, and b_z below is b.z; that
     * is the whole of the source's door state.
     */
    int door_image_offset;
    bool door_open_flag;

    /*
     * STEP 63 — b.z, the fourth operand of the finish gate.
     *
     * Java b.z (b.java:113, `protected boolean z`) is a LATCH, and it is not
     * door state. Its only producer is b.java:461, one statement inside
     * `case 9:` of CreateTiles(int,int,int,int) — that is, it is set TRUE as a
     * side effect of RASTERISING a tile-9 cell, the exit door:
     *
     *     455:  case 9 -> {
     *     ...
     *     461:      this.z = true;
     *     462:  }
     *
     * Java's three writers, exhaustively (STEP 58 §3):
     *   W1  b.java:205  constructor      unconditional  false
     *   W2  b.java:461  tile 9 rasterised             true    <-- the producer
     *   W3  e.java:288  gate M-arm, when M is true     false
     *
     * It has exactly ONE reader in the whole tree: e.java:286,
     *
     *     286:  if (this.TODO_ExitUnlocked && this.z && (this.exitX+1)*12 > m()
     *           && this.exitX*12 < g()) {
     *
     * so it is consumed as a bare boolean, POSITIVE polarity, no negation.
     * It is distinct from f.z (an int death counter, f.java:107) and from
     * BounceGame.z (an int read from a record); only this field is b.z.
     *
     * LIFECYCLE, matching Java exactly. Like door_open_flag above, this field
     * is zeroed by the memset in bounce_game_init (game.c:132) and that is the
     * whole of the initialisation, corresponding to Java's W1. It is
     * deliberately NOT reset on level load, because Java's LoadLevelId
     * (b.java:210-254) assigns no z either (STEP 58 §3). The producer at
     * vertical_slice.c:5055 only ever sets it true; W3's clear is not
     * reproduced, because the gate's own `!door_open_flag` operand already makes
     * the gate permanently inoperative once the door has fully opened, which is
     * exactly STEP 52's "M-arm body is self-restoring and inert" finding,
     * carried forward unchanged.
     */
    bool b_z;

    /* Existing recovered camera state; lifecycle code only resets it on load. */
    int32_t l;
    int32_t k;
    int32_t v; /* Java horizontal tile-alignment residual */

    CameraDiagnostic *camera_diagnostic;
    VerticalDiagnostic *vertical_diagnostic;
    RenderTrace *render_trace;
    /* STEP 13E: NULL unless NBB_DEBUG selects a level. Observational only. */
    BounceDebugTracker *debug_tracker;
    int64_t next_player_tick_ms;
} BounceGame;

#define BOUNCE_GAME_LEVEL_ID_UNKNOWN 0

/* Infer 1..11 only from the original J2MElvl.NNN filename; otherwise fallback. */
int bounce_game_level_id_from_path(const char *level_path, int fallback);

/* Initialize the empty context at the explicit splash boundary. */
int bounce_game_init(BounceGame *game);

/* Synchronize the gameplay mirror with native UI-only application states. */
int bounce_game_sync_ui_state(BounceGame *game);
/* Finish the synchronous native splash boundary and enter MENU. */
int bounce_game_enter_menu(BounceGame *game);

/*
 * Load an arbitrary explicit path, parse it, create independent runtime tiles,
 * and initialize player/spawn from the level header.  Replacement is atomic:
 * a failed load leaves the previously owned level and runtime state intact.
 */
int bounce_game_load_level(
    BounceGame *game,
    const char *level_path,
    int level_id
);

/* Activate an already loaded level after the menu -> PLAYING transition. */
int bounce_game_activate_loaded_level(BounceGame *game, int lives);

/* Load a path and enter PLAYING as one lifecycle operation. */
int bounce_game_start_level(
    BounceGame *game,
    const char *level_path,
    int level_id,
    int lives
);

/*
 * Build the verified native resource path for a level id into buffer.
 * Returns 0 on success, -1 for a NULL/short buffer or an id outside the
 * verified 1..11 resource set. No filesystem access occurs.
 */
int bounce_game_level_resource_path(
    int level_id,
    char *buffer,
    size_t buffer_size
);

/*
 * Level-load-only step of the gameplay entry. Loads exactly one level through
 * the verified bounce_level_load_file seam into this context's existing
 * single-level ownership, replacing a previously owned level with a targeted
 * level destroy and path release.
 *
 * It intentionally performs no runtime-tile creation, no player or spawn
 * initialization, no camera update, no counter reset, and no tick scheduling.
 * It refuses to run while a runtime tile copy is owned so level ownership can
 * never be split, and refuses ids outside 1..11 before any file access. A
 * failed load leaves the context unchanged.
 */
int bounce_game_load_level_entry(BounceGame *game, int level_id);

/*
 * Player-initialization step of the gameplay entry. Consumes only the already
 * owned BounceLevel through the existing bounce_player_initialize_from_level
 * seam and installs the derived player and spawn data into this context's
 * single embedded player owner.
 *
 * It requires the LEVEL_LOADED boundary, an owned level, runtime == NULL, and
 * a not-yet-initialized player, so a repeated call is rejected and the existing
 * player is preserved. It builds no runtime tiles or images, performs no
 * collision probe, and starts no timer or tick, and it writes no camera,
 * counter, level, or tick-deadline field.
 *
 * The native player is embedded by value, so the equivalent of "the player
 * exists" is player.initialized == true rather than a non-NULL pointer.
 */
int bounce_game_initialize_player_entry(BounceGame *game);

/*
 * Runtime-initialization step of the gameplay entry: the native mutable
 * runtime tile copy and the dynamic-thorn state derived from the level payload.
 *
 * The original has NO separate runtime object: b.java mutates its single
 * LevelTiles array in place (b.java:384, :482-484, :293). This seam is a native
 * implementation boundary only, placed after PLAYER_INITIALIZED because the f
 * constructor's UseBigBall() probe (e.java:127, f.java:130) reads the pristine
 * bytes that this step copies, and before the camera boundary, which the
 * original orders at e.java:132.
 *
 * It requires the PLAYER_INITIALIZED boundary, an owned level, an initialized
 * player, and runtime == NULL, so a repeated call is rejected and the existing
 * runtime state is preserved. It copies tiles, decodes the 8-byte dynamic-thorn
 * records into independent mutable state, advances nothing, and writes no
 * player, spawn, camera, counter, level, flow, or tick-deadline field. It does
 * not start a timer, enter PLAYING, or call UpdateDynThorns.
 *
 * Ownership: this context's existing single runtime slot owns the copy, and
 * bounce_game_release_level() already destroys it before the BounceLevel. The
 * pristine BounceLevel is never modified.
 */
int bounce_game_initialize_runtime_entry(BounceGame *game);

/*
 * Gameplay-activation step of the staged gameplay entry: the native stand-in
 * for the original's post-camera `this.display.setCurrent(this.v)`
 * (BounceGame.java:155), which is the only source-backed action in that block.
 *
 * The original has NO activation state, object, or flag. BounceGame.java:148-157
 * runs, after the init chain returns: `this.q = false` (a NEW_HIGH_SCORE
 * persistence flag), `StartGameTimer()` (already running since e.java:78),
 * `this.v.aq.a()` (`f.java:150-152`, `w &= 0xFFFFFFF0`, inert because a fresh
 * player already has w == 0 per f.java:125), `display.setCurrent(this.v)`, and
 * `this.K = 1` (menu bookkeeping). The native lifecycle needs a state for that
 * display-focus change because BOUNCE_APP_STATE_GAMEPLAY is what the existing
 * tick and renderer dispatch already gate on, so this is a portability boundary
 * rather than a recovered Java symbol.
 *
 * Only `display.setCurrent` is modeled. It requires the CAMERA_INITIALIZED
 * boundary, an owned level, an initialized player, a non-NULL runtime,
 * camera_initialized, state == LEVEL_LOADED, and gameplay_activated == false, so
 * it can only follow one complete initialization and a repeated call is rejected
 * with everything preserved.
 *
 * It preserves player, spawn, runtime tiles, dynamic-thorn state, camera l/k/v,
 * counters, and the pristine BounceLevel byte-for-byte. It starts NO timer and
 * performs no tick, movement, collision, dyn-thorn update, renderer, repaint,
 * audio, persistence, or input work.
 */
int bounce_game_activate_staged_level(BounceGame *game);

/*
 * Game-timer initialization step of the staged gameplay entry: the native
 * stand-in for the source's StartGameTimer() boundary (b.java:835-839), whose
 * schedule period is 40 ms (BounceTimer.java:17).
 *
 * The original may already have started this timer from its constructor
 * lifecycle (e.java:78, BounceGame.java:97), and StartGameTimer() is idempotent
 * (b.java:836-837); this seam is likewise idempotent and refuses a second call.
 *
 * It requires the ACTIVATED boundary, an owned level, a non-NULL runtime, an
 * initialized player, camera_initialized, gameplay_activated, and an unstarted
 * timer. It starts the platform-independent BounceGameTimer and records the
 * step; it does NOT read a clock, set a deadline, fire a callback, or execute
 * anything from the Tick body.
 *
 * It preserves player, spawn, runtime tiles, dynamic-thorn state, camera l/k/v,
 * all counters, and the pristine BounceLevel byte-for-byte, and starts no
 * renderer, repaint, audio, persistence, or input work.
 */
int bounce_game_initialize_game_timer(BounceGame *game);

/*
 * The branch e.Tick() (e.java:221-266) selects, recorded in source order.
 */
typedef enum BounceTickDispatchBranch {
    /* e.java:222-226: `if (this.d) { InitializeGame(); repaint(); return; }` */
    BOUNCE_TICK_DISPATCH_RESET_GATE = 0,
    /* e.java:227-256: `if (this.SplashId != -1) { ...; repaint(); return; }` */
    BOUNCE_TICK_DISPATCH_SPLASH_GATE = 1,
    /* e.java:262-263: the camera condition held, so `e()` would run. */
    BOUNCE_TICK_DISPATCH_CAMERA = 2,
    /* e.java:264-265: the else branch, so `this.aq.b()` would run. */
    BOUNCE_TICK_DISPATCH_PLAYER_TICK = 3
} BounceTickDispatchBranch;

/*
 * The physics continuation invoked for the PLAYER_TICK branch.
 *
 * It is a continuation, not a reimplementation: the supplied body delegates to
 * the already verified native physics that the previously working
 * app_run_player_ticks() path called, namely app_step_vertical() (f.b() from
 * f.java:670), app_step_horizontal(), and app_update_camera_horizontal()
 * (e.java:310 LoadSpriteSheet hand-off), in that order. Those functions are
 * file-local to the integration layer and are neither modified nor copied.
 *
 * The body must not call app_run_player_ticks() or app_update_game(): the new
 * 40 ms timer path bypasses the legacy scheduler entirely.
 */
typedef int (*BouncePlayerTickBody)(BounceGame *game, void *context);

/*
 * Tick-dispatch boundary: the beginning of e.Tick(), from entry through the
 * pre-dispatch gates and the camera gate, stopping immediately BEFORE the
 * player physics body.
 *
 * Source order, all ORIGINAL VERIFIED:
 *   e.java:222  reset gate        (`this.d`)
 *   e.java:227  splash gate       (`this.SplashId != -1`)
 *   e.java:258  p--               (`if (this.p != 0) this.p--;`)
 *   e.java:261  synchronized(aq)  NATIVE SINGLE-THREAD BOUNDARY, see below
 *   e.java:262  camera gate
 *   e.java:263  e()      | e.java:265  this.aq.b()
 *
 * Every source condition is an explicit parameter rather than a deleted check, so
 * the reset and splash early returns remain representable and testable:
 *   reset_pending      -> e.java:222 this.d
 *   splash_active      -> e.java:227 this.SplashId != -1
 *   camera_gate_needed -> the e.java:262 condition, supplied by the caller from
 *                         the already-verified native gate so that no camera
 *                         formula is duplicated here
 *
 * The caller supplies reset_pending and splash_active because the native staged
 * lifecycle has no counterpart for either. They are NATIVE PRECONDITIONS rather
 * than deleted behavior: `d` is cleared by LoadLevelId (b.java:213) and only set
 * at e.java:316, :340, :345, none reachable here; SplashId is set to -1 at
 * e.java:246 before the menu opens, and the native splash is the already-verified
 * BOUNCE_APP_STATE_SPLASH application path.
 *
 * The PLAYER_TICK branch enters f.b() (f.java:652) through its pre-physics
 * prologue f.java:653-669, then, when body is non-NULL, invokes that
 * continuation to run the existing native physics from f.java:670 onward. A
 * NULL body stops after the prologue, which is the pre-integration behavior.
 *
 * The CAMERA arm (e.java:263) runs no work here either. It records
 * BOUNCE_TICK_DISPATCH_CAMERA and returns, and the caller supplies e() through
 * the layer that owns the file-local app_update_camera(), so the gate stays a
 * pure selection and the two arms remain mutually exclusive.
 *
 * `synchronized (this.aq)` is a NATIVE SINGLE-THREAD BOUNDARY: the native core
 * runs one event-loop thread, and the player is embedded by value in this
 * context, so there is no separate object whose monitor could be contended. No
 * mutex is introduced.
 *
 * This function itself executes no physics. For the RESET and SPLASH branches it
 * records the early return without running InitializeGame, repaint, or the
 * splash UI. For the PLAYER_TICK branch it performs the f.java:653-669 prologue
 * and then, only when body is non-NULL, delegates f.java:670 onward to that
 * continuation; the continuation is the owner of the physics, collision, and
 * camera-horizontal calls, and this function adds no game step of its own. It
 * never calls the legacy scheduler, a timer callback, or a dynamic-thorn update.
 */
int bounce_game_tick_dispatch(
    BounceGame *game,
    bool reset_pending,
    bool splash_active,
    bool camera_gate_needed,
    BouncePlayerTickBody body,
    void *body_context,
    BounceTickDispatchBranch *branch_out
);

/* Release only level/runtime/player-owned state; retain presentation assets. */
void bounce_game_release_level(BounceGame *game);

/* Release all gameplay state and return the context to its empty boundary. */
void bounce_game_shutdown(BounceGame *game);

/* Apply an existing native flow event and keep BounceGameState synchronized. */
int bounce_game_apply_event(BounceGame *game, BounceAppEvent event);

/*
 * Explicit lifecycle boundaries.  These do not invent a death, respawn,
 * level-complete, or progression policy; callers must establish the relevant
 * source event before requesting the boundary.
 */
int bounce_game_mark_dead(BounceGame *game);
int bounce_game_request_respawn(BounceGame *game);
int bounce_game_mark_level_complete(BounceGame *game);

/*
 * STEP 12X-G-P4 -- the Level Complete CONTINUE command
 * (BounceGame.java:203-204, :209, :263-266).
 *
 * Routes the already-advanced flow->level_id into the EXISTING gameplay-entry
 * staging after releasing the completed run's resources, and returns the level
 * that will be started. Refuses unless the flow is in
 * BOUNCE_APP_STATE_LEVEL_COMPLETE and the Step 12X-G-P3 deferred_init latch is
 * armed, so it can never be reached from any other state.
 *
 * It adds no state, no event and no enum value, and it does not increment
 * flow->level_id: e.java:317 already did that in the completion block. The
 * staging itself is performed by the unchanged
 * app_begin_canonical_gameplay() driver in the next frame.
 */
int bounce_game_continue_level(BounceGame *game);

/*
 * STEP 12X-P4-C-D-B -- the minimal score core.
 *
 * bounce_game_add_score() is the native counterpart of e.java:153-156
 *
 *     public void AddScore(int paramInt) {
 *         this.score += paramInt;
 *         this.y = true;
 *     }
 *
 * reduced to the score mutation alone, because the Java `y = true` side effect
 * is the HUD dirty flag, and the HUD does not render a score yet. There is
 * deliberately no validation, no clamp, no saturation, no multiplier, no event
 * and no persistence: the function has no observable effect other than the
 * addition. No gameplay caller exists yet.
 */
int bounce_game_add_score(BounceGame *game, int32_t amount);

/*
 * STEP 12X-P4-C-D-B: zero the score for a new game session. Called from the
 * single point at which the flow records a New Game decision, so that a level
 * load, a level transition, a death, a respawn and a menu visit all leave the
 * running total intact.
 */
int bounce_game_reset_score(BounceGame *game);
int bounce_game_mark_game_over(BounceGame *game);

const char *bounce_game_state_name(BounceGameState state);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_GAME_H */
