#ifndef BOUNCE_NATIVE_APP_FLOW_H
#define BOUNCE_NATIVE_APP_FLOW_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * BounceAppFlow is the authoritative native application/UI boundary for the
 * splash/menu, explicit action-boundary, bounded Instructions, bounded High
 * Score, bounded Level Selection, and the non-gameplay gameplay-entry
 * boundary. It owns only application state and small menu/list/entry models;
 * it does not own level, player, physics, camera, renderer, or persistence
 * state. BounceGameState remains a synchronized gameplay/lifecycle field.
 */
typedef enum BounceAppState {
    BOUNCE_APP_STATE_MENU = 0,
    BOUNCE_APP_STATE_GAMEPLAY = 1,
    BOUNCE_APP_STATE_PAUSE = 2,
    BOUNCE_APP_STATE_GAME_OVER = 3,
    BOUNCE_APP_STATE_LEVEL_COMPLETE = 4,
    BOUNCE_APP_STATE_SPLASH = 5,
    BOUNCE_APP_STATE_ACTION_BOUNDARY = 6,
    BOUNCE_APP_STATE_INSTRUCTIONS = 7,
    BOUNCE_APP_STATE_HIGH_SCORE = 8,
    BOUNCE_APP_STATE_LEVEL_SELECTION = 9,
    BOUNCE_APP_STATE_GAMEPLAY_ENTRY = 10,
    /* Native integration seam: level parsed and owned, nothing else started. */
    BOUNCE_APP_STATE_LEVEL_LOADED = 11,
    /*
     * NATIVE EXTENSION. The recovered Nokia Bounce source contains no Options,
     * Settings, or Language item: the complete source search in
     * reverse/ui-menu-persistence-localization-audit.md section 4 records "there
     * is no Options, Settings, Language, About, or Help menu item". These two
     * states exist only because the Linux reimplementation was asked for them.
     * Nothing here is attributed to the original.
     */
    BOUNCE_APP_STATE_SETTINGS = 12,
    /* NATIVE EXTENSION, same justification as BOUNCE_APP_STATE_SETTINGS. */
    BOUNCE_APP_STATE_ABOUT = 13,
    /*
     * STEP 12X-P4-B -- the game-complete terminal destination, the counterpart
     * of BounceGame.ShowGameEnd(true) (BounceGame.java:177-197) reached through
     * TODO_ShowInstructions(true) (BounceGame.java:430-435) from e.java:321.
     *
     * It is deliberately NOT BOUNCE_APP_STATE_GAME_OVER. That state models
     * e.java:271 TODO_ShowInstructions(false), the death result: no Continue, one
     * OK command, and a route back to the main menu -- but reached by running out
     * of lives, not by finishing the game. The two forms differ in the original
     * (BounceGame.java:182-186 shows CONGRATS when won is true and GAME_OVER
     * otherwise), so the two states must not be merged.
     *
     * There is no Continue affordance here, matching the original, where
     * commandContinue is attached only inside ShowLevelComplete()
     * (BounceGame.java:209), which e.java:320 skips.
     */
    BOUNCE_APP_STATE_GAME_END = 14
} BounceAppState;

/*
 * NATIVE EXTENSION. Which Settings page is current. BOUNCE_SETTINGS_PAGE_ROOT is
 * the Settings list itself; the other four values are its read-only pages. This
 * enum is a native navigation model and corresponds to no recovered symbol.
 */
typedef enum BounceSettingsPage {
    BOUNCE_SETTINGS_PAGE_ROOT = 0,
    BOUNCE_SETTINGS_PAGE_DISPLAY = 1,
    BOUNCE_SETTINGS_PAGE_INPUT = 2,
    BOUNCE_SETTINGS_PAGE_AUDIO = 3,
    BOUNCE_SETTINGS_PAGE_LANGUAGE = 4,
    /*
     * NATIVE EXTENSION: the temporary runtime UI color profiles. No original
     * Nokia Bounce screen or setting corresponds to this page.
     */
    BOUNCE_SETTINGS_PAGE_THEME = 5,
    /*
     * STEP 38-RESET -- NATIVE ADDITION. The Reset to Default destination, and the
     * confirmation screen it opens.
     *
     * THE VALUE IS NOT ARBITRARY. bounce_app_flow_settings_select() maps a Settings
     * root row onto a page with `(BounceSettingsPage)(selected_index + 1)`, so the
     * page a row opens is fixed by its position: row 0 -> DISPLAY(1), row 1 ->
     * INPUT(2), and so on. Row 5 must therefore be 6 for the mapping to keep working
     * unchanged, which is why this is appended rather than slotted in next to the
     * other setting pages. Inserting it anywhere else would either renumber every
     * page after it or need a parallel lookup table that does not exist.
     *
     * It resets the whole documented fresh-install state, not one setting: the
     * Continue snapshot, the unlocked-level count, the high score, the color
     * profile, the audio gate, the keypad gate and the language. That is why it is a
     * page of its own rather than another two-row toggle -- a toggle cannot express
     * "make this look like a new install".
     */
    BOUNCE_SETTINGS_PAGE_RESET = 6
} BounceSettingsPage;

typedef enum BounceAppEvent {
    BOUNCE_APP_EVENT_START = 0,
    BOUNCE_APP_EVENT_PAUSE = 1,
    BOUNCE_APP_EVENT_RESUME = 2,
    BOUNCE_APP_EVENT_GAME_OVER = 3,
    BOUNCE_APP_EVENT_LEVEL_COMPLETE = 4,
    BOUNCE_APP_EVENT_NEXT_LEVEL = 5,
    BOUNCE_APP_EVENT_MENU = 6,
    BOUNCE_APP_EVENT_SPLASH_DONE = 7,
    /*
     * STEP 12X-P4-B -- the final-level terminal edge.
     *
     * e.java:320-322 is the only branch in the recovered source that leaves
     * level completion for a screen other than ShowLevelComplete():
     *
     *   if (this.level > 11) { this.game.TODO_ShowInstructions(true); }
     *   else { this.H = false; this.game.ShowLevelComplete(); repaint(); }
     *
     * It is accepted only from BOUNCE_APP_STATE_LEVEL_COMPLETE and leads to
     * BOUNCE_APP_STATE_GAME_END, which is NOT BOUNCE_APP_STATE_GAME_OVER: that
     * state is the death result (e.java:271 -> TODO_ShowInstructions(false)),
     * and Step 12X-P4-A established the two must stay distinct.
     */
    BOUNCE_APP_EVENT_GAME_END = 8
} BounceAppEvent;

#define BOUNCE_APP_FIRST_LEVEL_ID 1
#define BOUNCE_APP_LAST_LEVEL_ID 11
/*
 * Documentation of the recovered timer period only. This constant is never
 * used to start a native timer: BounceTimer.java:17 schedules
 * Timer.schedule(this, 0L, 40L).
 */
#define BOUNCE_APP_TICK_MS 40
#define BOUNCE_APP_SPLASH_TIMER_LIMIT 30u
/*
 * Native menu rows. Rows 0..3 are the source-verified original entries in the
 * order LoadMenuStrings() creates them (BounceGame.java:103-108) and the order
 * ShowMainMenu() appends them (BounceGame.java:116-119): Continue, New game,
 * High score, Instructions. Row 4 (Settings) and row 5 (About) are NATIVE
 * EXTENSIONS with no original counterpart. Row 6 (Exit) is the source's
 * Command.EXIT (BounceGame.java:120-121), which the original attaches to the
 * list as a command rather than as a list element; making it a selectable row
 * is a native presentation choice, not a claim about the original list.
 */
#define BOUNCE_MENU_ROW_COUNT 7u
/* First row index that is a native extension rather than a source-backed row. */
#define BOUNCE_MENU_FIRST_NATIVE_ROW 4u
/*
 * STEP 38-RESET. Six rows: the four original native Settings destinations, the
 * Theme page, and Reset to Default. It was five before this stage.
 */
#define BOUNCE_SETTINGS_ROOT_ITEM_COUNT 6u
/*
 * NATIVE EXTENSION: the runtime UI color profiles. Index 0 is the default and is
 * what every fresh launch uses; nothing is persisted. Must stay equal to
 * BOUNCE_UI_COLOR_PROFILE_COUNT: this is the number the Theme page reports as a
 * navigable list, and the two diverging is how a page can offer more rows than
 * there are palettes. 13 of the 15 target profiles are specified; see
 * ui_shell.h.
 */
#define BOUNCE_UI_THEME_COUNT 13u
/*
 * Settings root row indices, in display order. These are row positions, NOT the
 * BounceSettingsPage values: selecting row N opens page N+1. Naming them keeps
 * navigation code from conflating the two, which silently walked onto the wrong
 * row when the Theme page was added.
 */
#define BOUNCE_SETTINGS_ROW_DISPLAY 0u
#define BOUNCE_SETTINGS_ROW_INPUT 1u
#define BOUNCE_SETTINGS_ROW_AUDIO 2u
#define BOUNCE_SETTINGS_ROW_LANGUAGE 3u
#define BOUNCE_SETTINGS_ROW_THEME 4u
/*
 * STEP 38-RESET. Row 5, which is the last root row and therefore the one
 * bounce_app_flow_settings_select() turns into BOUNCE_SETTINGS_PAGE_RESET via its
 * `selected_index + 1` mapping.
 */
#define BOUNCE_SETTINGS_ROW_RESET 5u
/*
 * The Language list length: the four ORIGINAL SOURCE-BACKED recovered resources
 * plus Indonesian, a NATIVE EXTENSION that has no resource file. This is a
 * native list length; it is not a claim that the original offered runtime
 * language switching, which no recovered source line establishes.
 */
#define BOUNCE_LANGUAGE_RESOURCE_COUNT 5u
/* The number of entries that actually have a file in src/main/resources. */
#define BOUNCE_LANGUAGE_RECOVERED_COUNT 4u

/*
 * Source-verified new-game constants passed as literal arguments by
 * BounceGame.java:151 (v.a(level, 0, 3)) and set as the pre-tick countdown by
 * e.java:119 (p = 120).
 */
#define BOUNCE_APP_ENTRY_INITIAL_SCORE 0
#define BOUNCE_APP_ENTRY_INITIAL_LIVES 3
#define BOUNCE_APP_ENTRY_COUNTDOWN 120

/*
 * These are native action boundaries, not recovered Java symbols.  EXIT is
 * intentionally separate from the ordinary menu rows.
 */
typedef enum BounceMenuAction {
    BOUNCE_MENU_ACTION_NONE = 0,
    BOUNCE_MENU_ACTION_CONTINUE = 1,
    BOUNCE_MENU_ACTION_NEW_GAME = 2,
    BOUNCE_MENU_ACTION_HIGH_SCORE = 3,
    BOUNCE_MENU_ACTION_INSTRUCTIONS = 4,
    BOUNCE_MENU_ACTION_EXIT = 5,
    BOUNCE_MENU_ACTION_CONTINUE_UNAVAILABLE = 6,
    BOUNCE_MENU_ACTION_START_LEVEL = 7,
    /* NATIVE EXTENSION: no original counterpart exists. */
    BOUNCE_MENU_ACTION_SETTINGS = 8,
    /* NATIVE EXTENSION: no original counterpart exists. */
    BOUNCE_MENU_ACTION_ABOUT = 9
} BounceMenuAction;

typedef enum BounceMenuDirection {
    BOUNCE_MENU_DIRECTION_UP = 0,
    BOUNCE_MENU_DIRECTION_DOWN = 1
} BounceMenuDirection;

typedef struct BounceMenuState {
    bool continue_available;
    unsigned int selected_index; /* index among enabled rows */
    unsigned int enabled_item_count;
    BounceMenuAction pending_action;
    /*
     * STEP 12X-P4-C-D-B-R2 -- one-shot New Game intent, the native counterpart of
     * BounceGame.a(boolean paramBoolean, int paramInt) (BounceGame.java:148-157),
     * where `paramBoolean` alone decides whether `this.v.a(paramInt, 0, 3)` runs and
     * therefore whether e.score is zeroed (e.java:85).
     *
     * WHY IT LIVES HERE AND NOT IN BounceGameplayEntry: the menu block is the only
     * route-origin state that survives to the activation boundary.
     * bounce_app_flow_begin_gameplay_entry() rewrites the whole gameplay_entry block
     * (app_flow.c:834-845) but never touches flow->menu, and menu.pending_action is
     * still the gate two stages later in app_begin_canonical_gameplay()
     * (vertical_slice.c:5074). Step 13.27 established this placement on that evidence.
     *
     * WHY IT IS NEEDED AT ALL: pending_action cannot carry the intent, because it is
     * both overwritten and then required to equal BOUNCE_MENU_ACTION_START_LEVEL.
     * bounce_app_flow_record_start_level() overwrites it at app_flow.c:83, and both
     * bounce_app_flow_begin_gameplay_entry() (app_flow.c:828) and
     * app_begin_canonical_gameplay() (vertical_slice.c:5074) require
     * START_LEVEL. Verified in Step 13.27.5.
     *
     * SET BY exactly two origins, both of which are the single recovered Java
     * a(true, ...) call: the direct New Game branch of
     * bounce_app_flow_menu_select(), and bounce_app_flow_level_selection_select().
     * NEVER SET BY bounce_app_flow_continue_level(), which is Java's a(false, level).
     *
     * ONE-SHOT by construction: it is read only through
     * bounce_app_flow_consume_new_game_score_reset(), which clears it as it reads, so
     * a stale true can never carry into a later Continue or level transition.
     */
    bool new_game_score_reset_pending;
} BounceMenuState;

typedef struct BounceLevelSelectionState {
    unsigned int available_level_count; /* source-shaped MaxLevels model */
    unsigned int selected_index;         /* zero-based, bounded */
} BounceLevelSelectionState;

/*
 * STEP 38-RESET -- THE DOCUMENTED FRESH-INSTALL STATE.
 *
 * These four numbers are the whole of "what a new install looks like", and they are
 * named rather than written inline at each site because three separate places have
 * to agree on them: bounce_app_flow_init() for a launch with no store,
 * bounce_app_flow_reset_to_default() for the Reset to Default row, and the verifier
 * that asserts a fresh install and a reset produce the same state. A literal in one
 * of them and a constant in the others is how the three drift apart.
 *
 * TWO OF THE FOUR DIFFER FROM WHAT THE RECOVERED SOURCE WOULD PRODUCE, and both
 * differences were asked for explicitly.
 *
 *   max_levels = 2, not 0. BounceGame.java:276-279 shows that when the store holds
 *   no records LoadRecords() writes three zero-filled records and reads none of
 *   them, so MaxLevels keeps its Java default of 0 from :21. With 0 the New Game gate
 *   at :241 (`MaxLevels > 1`) is false, so `a(true, 1)` runs and the first level
 *   starts directly -- the Level Select screen is unreachable on a fresh install, and
 *   a second level is unreachable forever. Two is the smallest count that makes both
 *   levels 1 and 2 selectable while still leaving 3..11 gated behind progress.
 *
 *   t9_input_enabled = true, not false. The native keypad is not a recovered feature
 *   at all (see the field's own comment), so its default is a product decision rather
 *   than a reconstruction, and it is now ON so that a new install drives entirely
 *   from the numeric keypad.
 *
 * high_score = 0 and the color profile's index 0 are not listed here because they
 * are already 0 in every sense that matters: bounce_app_flow_init() zeroes the whole
 * context, and index 0 IS the default profile. They are asserted by the verifier
 * rather than given a constant, so there is no second place for a default to hide.
 */
#define BOUNCE_DEFAULT_UNLOCKED_LEVEL_COUNT 2
#define BOUNCE_DEFAULT_T9_INPUT_ENABLED true
#define BOUNCE_DEFAULT_AUDIO_ENABLED true
#define BOUNCE_DEFAULT_LANGUAGE_INDEX 0u

/*
 * NATIVE EXTENSION state for the Settings destination. It records navigation
 * only: which page is current, which row is selected, and which recovered
 * language resource row is highlighted. It deliberately holds no applied
 * configuration, because no display, input, audio, or localization switch is
 * implemented; nothing here is attributed to the original source.
 */
typedef struct BounceSettingsState {
    BounceSettingsPage page;
    unsigned int selected_index;  /* bounded, non-wrapping */
    /* The row the highlight is on; moving does not change the language. */
    unsigned int language_index;
    /*
     * The language actually in force. It changes only when the highlighted row
     * is selected, which is what "SELECT TO SWITCH" on the Language screen
     * means. It is never persisted: a fresh launch always starts at index 0,
     * which is the default English resource.
     */
    unsigned int applied_language_index;
    /* The highlighted color-profile row; moving does not change the profile. */
    unsigned int theme_index;
    /* The color profile actually in force; never persisted. */
    unsigned int applied_theme_index;
    /*
     * STEP 34-F-K -- NATIVE ADDITION. Whether gameplay audio events are
     * delivered to the audio player. NATIVE SETTING: the original Java has no
     * Audio settings page at all (STEP 34-F-J section 5), so this corresponds
     * to no recovered symbol and is not a reconstruction.
     *
     * The value is the APPLIED one, exactly as applied_language_index and
     * applied_theme_index are. `selected_index` is the highlight on the Audio
     * page and means nothing until SELECT is pressed. Row 0 is ON, row 1 OFF.
     *
     * It is a logical event-delivery gate and nothing else. It does not touch
     * ALSA device selection, synthesis, the backend, or the player lifecycle:
     * the player, its worker thread and the backend all keep running, and the
     * sink is simply detached while OFF. Never persisted -- see
     * bounce_app_flow_settings_audio_enabled().
     */
    bool audio_enabled;
    /*
     * ===================================================================
     * NATIVE ADDITION -- the numeric keypad as a game controller.
     * ===================================================================
     *
     * NOT A RECOVERY. The recovered game has no Settings screen at all and no
     * keypad input: Bounce is driven by getGameAction() (e.java:392-408) plus the
     * raw codes 35, 49, 51, -6 and -7. This is a native setting a player asked
     * for, so it corresponds to no recovered symbol and no Java line.
     *
     * THE VALUE IS THE APPLIED ONE, exactly as audio_enabled is:
     * `selected_index` is only the highlight on the Input page and means nothing
     * until SELECT is pressed. Row 0 is ON, row 1 is OFF.
     *
     * IT IS PERSISTED, and that is the difference from audio_enabled, which is
     * documented as NOT persisted. A player who switches the keypad on has asked
     * for it to stay on, and a setting that forgets itself on every launch is
     * not a setting. It rides the same version-2 container as the color profile,
     * as record 5; see persistence.h.
     *
     * DEFAULT IS FALSE. A fresh install must not silently gain a control mode
     * the player never chose, and OFF is the only value that changes nothing
     * about how the shipped keys behave.
     */
    bool t9_input_enabled;
    /*
     * STEP 38-RESET -- the one-shot "the player confirmed a reset" intent.
     *
     * WHY IT LIVES HERE AND NOT IN THE RESULT. bounce_app_flow_settings_select() only
     * receives a BounceAppFlow*, so it can reset flow state but it cannot touch the
     * store: deleting Record 3 and rewriting records 1, 2, 4 and 5 is I/O, and the
     * flow layer does no I/O anywhere. This is the same split the rest of this file
     * already uses -- bounce_app_flow_menu_set_continue_available() records a fact
     * and the shell acts on it, and bounce_app_flow_consume_new_game_score_reset() is
     * a one-shot flag read and cleared by its consumer.
     *
     * It is a FLAG rather than a return value because the caller cannot act on a
     * return value: bounce_ui_shell_handle_press() collapses every settings action
     * into "handled / not handled", so a "reset happened" signal has to survive until
     * the shell looks for it.
     *
     * ONE-SHOT BY CONSTRUCTION. It is set only by
     * bounce_app_flow_settings_select() on the Confirm row, and it is read only
     * through bounce_app_flow_consume_reset_to_default(), which clears it as it
     * reads. A stale true could therefore never carry into a later press, which
     * matters because the consumer performs deletions.
     */
    bool reset_to_default_pending;
} BounceSettingsState;

/*
 * Where the native gameplay-entry record currently stops. The values name the
 * recovered Java boundary steps, not gameplay phases.
 */
typedef enum BounceGameplayEntryPhase {
    BOUNCE_GAMEPLAY_ENTRY_PHASE_NONE = 0,
    /* START_LEVEL(N) is recorded but no source step has been executed. */
    BOUNCE_GAMEPLAY_ENTRY_PHASE_START_LEVEL_RECORDED = 1,
    /* Source-verified new-game metadata is recorded; still pre-level-load. */
    BOUNCE_GAMEPLAY_ENTRY_PHASE_INIT_METADATA_RECORDED = 2,
    /* Native seam: the level is parsed and owned; no player, timer, or tick. */
    BOUNCE_GAMEPLAY_ENTRY_PHASE_LEVEL_LOADED = 3,
    /* Native seam: player data is initialized and owned; no timer or tick. */
    BOUNCE_GAMEPLAY_ENTRY_PHASE_PLAYER_INITIALIZED = 4,
    /*
     * Native IMPLEMENTATION SEAM, not a recovered Java step. The original
     * mutates its single LevelTiles array in place and has no separate runtime
     * object, so this phase records only that the native mutable tile copy and
     * dynamic-thorn state are now owned. It is deliberately placed after
     * PLAYER_INITIALIZED because the f constructor probe reads the pristine
     * bytes (e.java:127) that this step copies.
     */
    BOUNCE_GAMEPLAY_ENTRY_PHASE_RUNTIME_INITIALIZED = 5,
    /*
     * Native seam: the already-verified camera initialization has run. The
     * original performs l = 0; k = 0 (e.java:130-131) then e() (e.java:132)
     * immediately after `new f(...)` and before any tick, which is exactly
     * where this phase sits. The camera algorithm itself is unchanged.
     */
    BOUNCE_GAMEPLAY_ENTRY_PHASE_CAMERA_INITIALIZED = 6,
    /*
     * Native activation boundary. The original has no activation state at all:
     * BounceGame.java:155 performs `this.display.setCurrent(this.v)`, a MIDP
     * display-focus change, after the init chain returns. The native
     * BOUNCE_APP_STATE_GAMEPLAY is the portable stand-in for "the gameplay
     * canvas is now current". No timer is started here.
     */
    BOUNCE_GAMEPLAY_ENTRY_PHASE_ACTIVATED = 7,
    /*
     * Native timer-scheduling seam. Records that the source-derived 40 ms
     * nominal period (BounceTimer.java:17) has an owning, started timer. The
     * coarse application state stays GAMEPLAY and no Tick body has run.
     */
    BOUNCE_GAMEPLAY_ENTRY_PHASE_TIMER_INITIALIZED = 8
} BounceGameplayEntryPhase;

/*
 * Audit record for BounceGame.a(boolean,int) (BounceGame.java:148-157) and
 * e.a(int,int,int) (e.java:81-90). This is a boundary record only: it owns no
 * level, player, runtime tile, timer, physics, camera, or persistence object,
 * and it deliberately records which recovered source steps were NOT executed.
 *
 * Intended player origin is the source formula initial_X * 12 + 6 and
 * initial_Y * 12 + 6 (e.java:121) from the level header (b.java:229-230).
 * Concrete coordinates are intentionally deferred with level-load integration.
 */
typedef struct BounceGameplayEntry {
    int level_id;
    int initial_score;
    int initial_lives;
    int countdown;
    BounceGameplayEntryPhase phase;
    bool level_loaded;   /* LoadLevelId executed: false at this milestone */
    bool timer_started;  /* StartGameTimer executed: false at this milestone */
    bool player_created; /* new f(...) executed: false at this milestone */
    /*
     * Native implementation seam only: the mutable runtime tile copy and the
     * dynamic-thorn state derived from the level payload are owned. The
     * original has no separate runtime object; this flag records native
     * ownership, never a recovered Java symbol. It is a separate accessor so
     * the verified three-flag entry_step_executed contract is unchanged.
     */
    bool runtime_tiles_created;
    /*
     * Native seam: the already-verified camera initialization has been applied
     * to l/k/v. Separate accessor, so the verified three-flag contract and the
     * runtime-ownership accessor are both unchanged.
     */
    bool camera_initialized;
    /*
     * Native activation: the gameplay surface is now current. Separate accessor,
     * so the verified three-flag contract and the two seam accessors are all
     * unchanged.
     */
    bool gameplay_activated;
} BounceGameplayEntry;

typedef struct BounceAppFlow {
    BounceAppState state;
    int level_id;
    uint32_t splash_timer;
    bool splash_skip_requested;
    int64_t next_tick_ms;
    BounceMenuState menu;
    BounceLevelSelectionState level_selection;
    /* NATIVE EXTENSION: Settings navigation state, empty on the original path. */
    BounceSettingsState settings;
    BounceGameplayEntry gameplay_entry;
    int pending_level_id; /* zero means no pending level action */
    /*
     * INSTRUCTIONS SCROLL -- the first line of the body currently shown.
     *
     * NATIVE EXTENSION, and like every other field in this struct it is UI state
     * with no gameplay effect. It is NOT persisted, NOT derived, and NOT restored
     * on a fresh launch; it is reset to 0 whenever the flow enters the
     * Instructions page, so the page always opens at the top.
     *
     * WHY IT IS STORED AT ALL, WHEN THE THEME PAGE'S SCROLL IS NOT. The Theme
     * page derives its window from the selected row -- theme_first_row() is a
     * pure function of the selection, and app_flow.c carries no field for it. A
     * read-only text page has no selection to derive from: there is nothing to
     * move, only text to move past. The value therefore has to be state, and the
     * flow is where this file keeps every other screen cursor
     * (level_selection.selected_index, settings.page, menu.selected_index).
     *
     * IT IS NOT A RECOVERY. BounceGame.java:167-174 renders Instructions with
     * `new Form(...)`, and MIDP's Form scrolls its own content on the device;
     * there is no scroll offset, scrollbar or scrolling code anywhere in the
     * eleven Java files. This field and the shared scrollbar in ui_shell.c are a
     * rebuilt platform feature, not a transliteration of the source.
     */
    unsigned int instructions_scroll;
    /*
     * STEP 37 PERSIST -- Java HighScore, BounceGame.java:23.
     *
     * Before this milestone the High Score screen rendered a hard-coded "0"
     * (ui_shell.c, draw_native_score_zero) because nothing ever produced a
     * value. The Java rule is WriteToStore() at BounceGame.java:422-426:
     *
     *   if (this.v.score > this.HighScore) {
     *       this.HighScore = this.v.score;
     *       this.q = true;
     *       WriteToStore(2);
     *   }
     *
     * i.e. a running maximum that only ever increases, updated at game over
     * (e.java:269) and at level complete (e.java:319). bounce_app_flow_note_score()
     * is that rule; bounce_app_flow_high_score() is the reader the High Score
     * screen uses.
     *
     * This is session state like every other field here. Making it survive a
     * launch is persistence.c's job, not this struct's: bounce_app_flow_init()
     * still zeroes it, and the load happens in main() afterwards, exactly the
     * way BounceGame.java:95 calls LoadRecords() after the field defaults exist.
     */
    int32_t high_score;
    /*
     * D-07 -- BounceGame.q, the NEW_HIGH_SCORE latch.
     *
     * Java writes it in exactly two places:
     *
     *   BounceGame.java:422-426, the no-argument WriteToStore():
     *       if (this.v.score > this.HighScore) {
     *           this.HighScore = this.v.score;
     *           this.q = true;              <-- the only SETTER
     *           WriteToStore(2);
     *       }
     *
     *   BounceGame.java:150, the first statement inside
     *   a(boolean, int)'s `if (paramBoolean)` branch:
     *       if (paramBoolean) {
     *           this.q = false;             <-- the only CLEARER
     *           this.v.a(paramInt, 0, 3);
     *       }
     *
     * and reads it in exactly one, BounceGame.java:188-190, where both terminal
     * forms append Translation.NEW_HIGH_SCORE when it is set.
     *
     * So it is a one-way session latch: a monotonically "the run has beaten the
     * record at least once" flag, cleared only by starting a new game. It is
     * deliberately NOT cleared by ShowMainMenu(), by a level transition, or by
     * leaving the Game End screen -- none of those touches `q` in the source --
     * which is why it belongs beside high_score and not inside the menu state
     * that bounce_app_flow_enter_menu() resets.
     *
     * IT IS NOT PERSISTED. BounceGame.java:19 declares `public boolean q;` and
     * it appears in no record: LoadRecords() reads no `q` (BounceGame.java:290-330)
     * and WriteToStore(3) writes none (:351-407). A fresh launch therefore
     * always starts with it false, and bounce_app_flow_init() supplies that.
     */
    bool new_high_score;
} BounceAppFlow;

/* Initializes the native shell at the source-backed splash boundary. */
void bounce_app_flow_init(BounceAppFlow *flow);

/*
 * Applies the small application transition table. Invalid transitions return
 * -1 and leave flow unchanged. PAUSE/RESUME and the gameplay result events
 * remain existing temporary/native boundaries; SPLASH_DONE is the minimal
 * startup transition added for this milestone.
 */
int bounce_app_flow_apply(
    BounceAppFlow *flow,
    BounceAppEvent event
);

/* Advance one 40 ms application tick. Returns 1 only on SPLASH -> MENU. */
int bounce_app_flow_tick(BounceAppFlow *flow);

/* Request a splash skip; the next application tick performs the transition. */
int bounce_app_flow_request_splash_skip(BounceAppFlow *flow);

/* Bounded, non-wrapping native menu navigation. */
int bounce_app_flow_menu_move(
    BounceAppFlow *flow,
    BounceMenuDirection direction
);

/* Record a selected action and enter its explicit bounded destination. */
BounceMenuAction bounce_app_flow_menu_select(BounceAppFlow *flow);
/* Legacy menu-only clear; action boundaries must use return_to_menu(). */
void bounce_app_flow_menu_clear_action(BounceAppFlow *flow);

/* Return from an explicit native destination boundary to MAIN_MENU. */
int bounce_app_flow_return_to_menu(BounceAppFlow *flow);

/*
 * NATIVE EXTENSION: enter the Settings or About destination from MAIN_MENU.
 * Neither destination exists in the recovered source; these are recorded as
 * native so no later reader can mistake them for source-backed behavior.
 */
int bounce_app_flow_enter_settings(BounceAppFlow *flow);
int bounce_app_flow_enter_about(BounceAppFlow *flow);

/*
 * NATIVE EXTENSION: bounded, non-wrapping Settings navigation.
 *
 * bounce_app_flow_settings_move() moves the selection inside whichever list is
 * current: the Settings root list, or the language resource list on the
 * Language page. It refuses to move in any other page and never wraps.
 *
 * bounce_app_flow_settings_select() acts on the current list. On the root list
 * it opens the selected page; on the Language page it records the highlighted
 * resource row, which is a selection boundary only: the native build keeps the
 * source-backed English fallback because no localization decoder is
 * implemented, and it does not pretend otherwise.
 *
 * bounce_app_flow_settings_back() steps one level: a page returns to the root
 * list, and the root list returns to MAIN_MENU.
 */
int bounce_app_flow_settings_move(
    BounceAppFlow *flow,
    BounceMenuDirection direction
);
int bounce_app_flow_settings_select(BounceAppFlow *flow);
int bounce_app_flow_settings_back(BounceAppFlow *flow);

/*
 * STEP 38-RESET -- restore the documented fresh-install state.
 *
 * WHAT IT RESETS, and what it deliberately does not:
 *
 *   level_selection.available_level_count  -> BOUNCE_DEFAULT_UNLOCKED_LEVEL_COUNT
 *   level_selection.selected_index         -> 0
 *   high_score                             -> 0
 *   new_high_score                         -> false
 *   settings.applied_theme_index           -> 0   (the default profile)
 *   settings.theme_index                   -> 0   (the highlight, too, so the
 *                                                 Theme page agrees with it)
 *   settings.audio_enabled                 -> BOUNCE_DEFAULT_AUDIO_ENABLED
 *   settings.t9_input_enabled              -> BOUNCE_DEFAULT_T9_INPUT_ENABLED
 *   settings.applied_language_index        -> BOUNCE_DEFAULT_LANGUAGE_INDEX
 *   settings.language_index                -> 0   (highlight, same reason)
 *   menu.continue_available                -> false
 *   menu.selected_index                    -> 0
 *
 * NOT RESET, deliberately: flow->state (the caller decides where the player lands
 * afterwards), level_id, pending_level_id, the gameplay-entry block, the splash
 * countdown and instructions_scroll. None of those is part of "a new install", and
 * clearing level_id in particular would break the staged entry this can be reached
 * from. instructions_scroll is left alone for the same reason it is reset on ENTRY
 * to the Instructions page rather than on leaving it.
 *
 * IT DOES NOT DELETE Record 3. That is the consumer's job, because it is I/O: this
 * function sets reset_to_default_pending and the shell removes the snapshot. A flow
 * that claimed the record was gone while it was still on disk would produce exactly
 * the stale-Continue defect that app_persist_resume_snapshot_for_continue() was added
 * to close.
 *
 * Returns 0, or -1 for a NULL flow or a flow that is not in the Settings
 * destination -- the same two refusals every other settings entry point has, so a
 * caller cannot reset the game from an unexpected screen.
 */
int bounce_app_flow_reset_to_default(BounceAppFlow *flow);

/*
 * STEP 38-RESET -- consume the one-shot reset intent.
 *
 * Returns the flag AND clears it in the same operation, so a reset can be performed
 * at most once per confirmation. There is deliberately no plain getter: a getter
 * would let a caller read the flag without clearing it, and the consumer DELETES a
 * saved snapshot, so a repeated read would delete it twice and a leaked true would
 * delete it on an unrelated press. Returns false for a NULL flow.
 */
bool bounce_app_flow_consume_reset_to_default(BounceAppFlow *flow);

/* Read-only accessors for the native Settings navigation state. */
BounceSettingsPage bounce_app_flow_settings_page(const BounceAppFlow *flow);
unsigned int bounce_app_flow_settings_selected_index(const BounceAppFlow *flow);
unsigned int bounce_app_flow_settings_language_index(const BounceAppFlow *flow);
/* The language actually in force; 0 is the default English resource. */
unsigned int bounce_app_flow_settings_applied_language_index(
    const BounceAppFlow *flow);
unsigned int bounce_app_flow_settings_theme_index(const BounceAppFlow *flow);
/* The color profile actually in force; 0 is the default profile. */
unsigned int bounce_app_flow_settings_applied_theme_index(
    const BounceAppFlow *flow);
unsigned int bounce_app_flow_settings_list_count(const BounceAppFlow *flow);
/*
 * The language resource row, or NULL when the index is out of range or the
 * language has no resource file. Indonesian is a NATIVE EXTENSION and
 * deliberately returns NULL; the four recovered languages return the files that
 * actually ship in src/main/resources.
 */
const char *bounce_app_flow_language_resource_name(unsigned int index);

/* Query helpers keep row mapping explicit and testable. */
unsigned int bounce_app_flow_menu_enabled_item_count(
    const BounceAppFlow *flow
);
bool bounce_app_flow_menu_item_enabled(
    const BounceAppFlow *flow,
    unsigned int row
);
unsigned int bounce_app_flow_menu_row_for_selection(
    const BounceAppFlow *flow,
    unsigned int selection_index
);
unsigned int bounce_app_flow_available_level_count(
    const BounceAppFlow *flow
);
unsigned int bounce_app_flow_level_selection_selected_index(
    const BounceAppFlow *flow
);

/*
 * STEP 34-F-K -- NATIVE ADDITION. The two rows of the Audio settings page:
 * row 0 is ON, row 1 is OFF. There is no third state.
 */
#define BOUNCE_SETTINGS_AUDIO_VALUE_COUNT 2u
#define BOUNCE_SETTINGS_AUDIO_ROW_ON  0u
#define BOUNCE_SETTINGS_AUDIO_ROW_OFF 1u

/*
 * STEP 34-F-K. Whether gameplay audio events should be delivered to the audio
 * player. NATIVE SETTING, not a reconstruction: the original Java has no Audio
 * settings page (STEP 34-F-J section 5).
 *
 * Default is TRUE, and that is derived from current native behaviour rather than
 * from Java: the game already creates the audio player, attaches the Layer B
 * sink and plays, with no setting involved. ON therefore preserves exactly the
 * pre-existing behaviour and is the only value that changes nothing.
 *
 * NOT PERSISTED. The native Settings shell has no persistence path for any
 * setting -- applied_language_index and applied_theme_index are likewise
 * runtime-only and are documented as such at app_flow.h:250-258 -- so a fresh
 * process always starts ON. Introducing a store for this one value alone would
 * be a new persistence system, which this step explicitly does not create.
 *
 * This is a read-only query. It never creates, destroys, opens or closes the
 * player, the backend or the device; it only reports what the user chose.
 * Returns true for a NULL flow, so a caller with no shell attached behaves as
 * "audio allowed" and the pre-existing path is preserved.
 */
bool bounce_app_flow_settings_audio_enabled(
    const BounceAppFlow *flow
);

/*
 * NATIVE ADDITION -- the Input page's two rows: row 0 is ON, row 1 is OFF. There
 * is no third state, exactly as on the Audio page.
 */
#define BOUNCE_SETTINGS_T9_VALUE_COUNT 2u
#define BOUNCE_SETTINGS_T9_ROW_ON  0u
#define BOUNCE_SETTINGS_T9_ROW_OFF 1u

/*
 * STEP 38-RESET -- the Reset to Default confirmation screen's two rows.
 *
 * CONFIRM IS ROW 0, and row 0 is where the highlight starts. Opening the page and
 * pressing SELECT therefore resets immediately, which is the safe direction to make
 * the easy one only because the page cannot be reached by accident: it takes a
 * deliberate walk to the last Settings row and then another press to open it.
 *
 * STEP 38-RESET-UI -- ROW 1 IS NAMED BACK, NOT CANCEL, AND IT IS NO LONGER A NO-OP.
 *
 * It used to be a Cancel row that returned success without doing anything, which read
 * as a dead button: it was highlighted, it accepted SELECT, and the only thing it did
 * was nothing. What the player wanted from that row was the thing every other screen
 * in the shell calls Back, so that is what it now is.
 *
 * The one behavioural consequence is deliberate and worth stating plainly, because
 * this is a confirmation screen and the distinction is the whole point of it: SELECT on
 * BACK navigates and SELECT on CONFIRM destroys. Previously the safe row also had no
 * navigation behaviour of its own, so "leave without resetting" was only ever reachable
 * through the ESC key. A pointer user had no way out of this page at all. Now both rows
 * are clickable, and BACK leaves without resetting.
 *
 * Both routes still land on the Settings root and neither one clears a thing:
 * bounce_app_flow_settings_back() has never touched progress, theme, audio, keypad,
 * language or the store. Renaming the row therefore makes the safe choice clickable
 * without giving the destructive one anything new.
 *
 * The count stays 2. Making this page a single button was considered and rejected: the
 * Back row is what lets a pointer user leave without a keyboard, and a page that can
 * only be left with the key that is deliberately not labelled on it would be worse than
 * the one-row version.
 */
#define BOUNCE_SETTINGS_RESET_VALUE_COUNT 2u
#define BOUNCE_SETTINGS_RESET_ROW_CONFIRM 0u
#define BOUNCE_SETTINGS_RESET_ROW_BACK    1u

/*
 * NATIVE ADDITION. Whether the numeric keypad drives the game. NATIVE SETTING,
 * not a reconstruction: the recovered Java has no Input page and no keypad.
 *
 * Default is FALSE, derived from current native behaviour rather than from Java:
 * every keypad keysym is currently unmapped, so OFF is the only value that
 * changes nothing. Returns false for a NULL flow, so a caller with no shell
 * attached behaves as "no keypad", which is the safe direction.
 */
bool bounce_app_flow_settings_t9_enabled(
    const BounceAppFlow *flow
);

/*
 * NATIVE ADDITION -- apply a persisted keypad setting at launch.
 *
 * This mirrors bounce_app_flow_restore_persisted_theme(), which exists for the
 * same reason: the store has to be read BEFORE the first frame, not after it, or
 * the player sees the default for one frame and then watches it change.
 *
 * It sets the APPLIED value, not the highlight: `selected_index` is navigation
 * state and reset_settings_navigation() owns it. Only a stored ON is honoured --
 * anything absent, zero or refused leaves the flow at its OFF default, so a
 * store written before this feature existed reads correctly untouched.
 */
int bounce_app_flow_restore_persisted_t9(
    BounceAppFlow *flow,
    bool enabled
);

/*
 * STEP 12X-P4-C-D-B-R2: consume the one-shot New Game intent.
 *
 * Returns the flag AND clears it in the same operation, so the reset can be applied at
 * most once per New Game request. There is deliberately no plain getter: a getter would
 * let a caller read the flag without clearing it, and a flag that survives its read would
 * reset the score again on the next Continue within the same session.
 *
 * Returns false for a NULL flow.
 */
bool bounce_app_flow_consume_new_game_score_reset(BounceAppFlow *flow);
int bounce_app_flow_pending_level_id(
    const BounceAppFlow *flow
);

/* Model-only level count seam; it never reads or writes RMS. */
int bounce_app_flow_set_available_level_count(
    BounceAppFlow *flow,
    unsigned int count
);

/*
 * Progression seam for the Java level-completion block: advance the current
 * level and derive the unlocked-level count from it.
 *
 * It deliberately takes no count argument, so it is not a general setter: the
 * resulting count is always min(flow->level_id, BOUNCE_APP_LAST_LEVEL_ID) and
 * can only ever grow by one completion. It is the only production writer of
 * flow->level_id outside the level loaders and the NEXT_LEVEL event.
 */
int bounce_app_flow_complete_level(BounceAppFlow *flow);

/*
 * STEP 12X-P4-B: has the just-completed level been the last playable one?
 *
 * Java writes `if (this.level > 11)` at e.java:320, a literal. It is evaluated
 * immediately after WriteToStore() at e.java:319, which has already set
 *
 *   BounceGame.java:418-419   if (this.v.level > this.MaxLevels) {
 *                                 this.MaxLevels = Math.min(this.v.level, 11);
 *
 * so at that point `this.level > this.MaxLevels` is equivalent to the literal:
 * for level <= 11 MaxLevels equals level and the test is false, and for level 12
 * MaxLevels is 11 and the test is true. This predicate therefore uses the
 * authoritative available-level count -- the native MaxLevels counterpart, which
 * bounce_app_flow_complete_level() has just synchronised -- and introduces no new
 * level number of its own. False for a NULL flow, so a caller that has not
 * established a flow cannot read it as final.
 */
bool bounce_app_flow_level_is_final(const BounceAppFlow *flow);

/*
 * Step 12X-G-P4: the CONTINUE edge. Routes a completed level into the EXISTING
 * gameplay-entry staging for the already-advanced flow->level_id, which is the
 * native counterpart of Java's `this.v.a(paramInt, 0, 3)` at
 * BounceGame.java:151. It adds no state, no event and no enum value: the
 * destination is the same ACTION_BOUNDARY + START_LEVEL that New Game and Level
 * Select already produce. Returns the level that will be started, or -1.
 */
int bounce_app_flow_continue_level(BounceAppFlow *flow);

/*
 * STEP P6 -- write the lives a staged gameplay entry will start with.
 *
 * THE CARRIER IS NOT NEW. This writes the existing
 * gameplay_entry.initial_lives, the same field bounce_app_flow_entry_initial_lives()
 * reads and the same field bounce_game_activate_staged_level() consumes at stage 5
 * (game.c:761). No second lives field is introduced and game->lives is never
 * patched here or afterwards.
 *
 * WHY IT IS NEEDED. bounce_app_flow_begin_gameplay_entry() unconditionally records
 * BOUNCE_APP_ENTRY_INITIAL_LIVES, which is correct for New Game and Level Select
 * because Java initialises those with `a(paramInt, 0, 3)` (BounceGame.java:242 and
 * :218 -> e.java:84). The post-completion CONTINUE route is the one route where Java
 * performs NO lives write at all: the completion block (e.java:313-327) does not
 * write it, ShowLevelComplete() (:199-213) does not write it, the CONTINUE branch
 * (:263-266) does not write it, and InitializeGame() (e.java:115-124) does not
 * write it -- the only clobbering write in the whole source, e.java:84, is
 * positively skipped by the `if (paramBoolean)` guard at BounceGame.java:150 when
 * a(false, ...) is used. So Java carries the completed run's remaining lives into
 * the next level purely by never writing the field.
 *
 * WHY IT IS A SEPARATE CALL AND NOT A PARAMETER. The write must land AFTER
 * bounce_app_flow_begin_gameplay_entry(), which is the call that overwrites the
 * carrier with the constant. bounce_app_flow_continue_level() cannot do it itself,
 * because it stops at ACTION_BOUNDARY and the release of the completed run has to
 * happen between that and begin_gameplay_entry(). This mirrors
 * bounce_app_flow_begin_resume_entry() exactly, which re-applies its `lives`
 * argument after the same call for the same reason (app_flow.c:995-1002).
 *
 * THE VALUE IS STORED VERBATIM, with no clamp and no rejection. Java preserves
 * whatever the death write (f.java:219) or the 1-up write (f.java:612) last left,
 * including a negative value, so rejecting one here would introduce a behaviour
 * change that the source does not have. Refused only for a NULL flow or a flow
 * that is not in the gameplay-entry boundary, which is the state both existing
 * callers are already in.
 */
int bounce_app_flow_set_entry_initial_lives(BounceAppFlow *flow, int lives);

/* Bounded, non-wrapping source-shaped level List navigation. */
int bounce_app_flow_level_selection_move(
    BounceAppFlow *flow,
    BounceMenuDirection direction
);
int bounce_app_flow_level_selection_select(BounceAppFlow *flow);

/*
 * Consume a recorded START_LEVEL(N) action and record only the source-verified
 * new-game metadata, entering the non-gameplay gameplay-entry boundary.
 * This never loads a level, creates a player, starts a timer, or runs a tick.
 */
int bounce_app_flow_begin_gameplay_entry(BounceAppFlow *flow);

/*
 * G-R3-R1 -- record a Record 3 resume entry, e.a(int,int) (e.java:92-113).
 *
 * WHAT THIS IS. The fourth producer of the ordinary START_LEVEL gameplay-entry
 * state, alongside New Game (app_flow.c:376), Level Selection (:909) and terminal
 * Continue (:838). It records the saved level and the saved lives, then hands
 * control to the same bounce_app_flow_begin_gameplay_entry() every other path
 * uses. Nothing here loads a level, creates a player, starts a timer or runs a
 * tick, and nothing here restores gameplay state.
 *
 * WHY initial_lives IS A PARAMETER RATHER THAN A NEW FIELD. R1-E established
 * that stage 5 is the sole canonical writer of game->lives and that it reads
 * gameplay_entry.initial_lives (game.c:761). Writing the saved value into that
 * existing carrier means the carrier and the live field cannot disagree, and it
 * needs no new state. The alternative -- writing game->lives after stage 5 --
 * would leave the entry overlay showing the default 3 while the live field held
 * the restored value.
 *
 * WHY THE SCORE FLAG IS DELIBERATELY NOT SET. new_game_score_reset_pending is
 * set only by New Game (app_flow.c:376) and Level Selection (:909), and consumed
 * once at stage 5 (game.c:785) where it triggers bounce_game_reset_score().
 * Java's resume restores score at e.java:96 and nothing after that point in
 * a(int,int) touches it again, so a resume must not consume that reset. Leaving
 * the flag false is what keeps game->score intact through stage 5; the score
 * itself is restored by the R1 restore window, per the S-C decision.
 *
 * Returns 0 on success, -1 for a NULL flow, a flow not in ACTION_BOUNDARY, an
 * out-of-range level id, or negative lives.
 */
/*
 * D-03/D-04 -- the J == 2 Continue entry, i.e. e.a(level, score, lives)
 * (e.java:81-90), called from BounceGame.java:231-232.
 *
 * NOT THE SAME AS bounce_app_flow_begin_resume_entry() below, which is the J == 1
 * branch and restores a POSITION through e.a(int,int) (e.java:92-114). This one
 * starts the recorded level from the beginning and therefore has to carry the
 * recorded SCORE, which the J == 1 branch does not need and must not have applied
 * to it.
 *
 * Like that function it records START_LEVEL(level) so the staged chain is reused
 * unchanged, and it deliberately does NOT set new_game_score_reset_pending, because
 * stage 5 would consume that and reset the very score this route delivers.
 *
 * Returns 0, or -1 for a NULL flow, a non-MENU state, a level outside
 * 1..BOUNCE_APP_LAST_LEVEL_ID, or a negative score or lives.
 */
int bounce_app_flow_begin_record_continue_entry(
    BounceAppFlow *flow,
    int level_id,
    int score,
    int lives);

int bounce_app_flow_begin_resume_entry(
    BounceAppFlow *flow,
    int level_id,
    int lives
);

/* Read-only accessors for the recorded entry metadata. */
int bounce_app_flow_entry_level_id(const BounceAppFlow *flow);
int bounce_app_flow_entry_initial_score(const BounceAppFlow *flow);
int bounce_app_flow_entry_initial_lives(const BounceAppFlow *flow);
int bounce_app_flow_entry_countdown(const BounceAppFlow *flow);
BounceGameplayEntryPhase bounce_app_flow_entry_phase(const BounceAppFlow *flow);
bool bounce_app_flow_entry_step_executed(
    const BounceAppFlow *flow,
    bool level_loaded,
    bool timer_started,
    bool player_created
);
const char *bounce_app_flow_entry_phase_name(
    BounceGameplayEntryPhase phase
);

/* Native runtime-ownership flag; separate so the 3-flag contract above is kept. */
bool bounce_app_flow_entry_runtime_tiles_created(const BounceAppFlow *flow);

/* Native camera-initialization flag; separate for the same reason. */
bool bounce_app_flow_entry_camera_initialized(const BounceAppFlow *flow);

/* Native activation flag; separate for the same reason. */
bool bounce_app_flow_entry_gameplay_activated(const BounceAppFlow *flow);

/*
 * Record that the level-load step of the gameplay entry completed and enter
 * the native LEVEL_LOADED seam. Requires GAMEPLAY_ENTRY with a recorded
 * START_LEVEL(N) action, so the pending action is consumed exactly once.
 * This records no new gameplay data and starts nothing.
 */
int bounce_app_flow_record_level_loaded(BounceAppFlow *flow);

/*
 * Record that the player-initialization step of the gameplay entry completed.
 * Requires the LEVEL_LOADED boundary with level_loaded already recorded, so it
 * can only follow one successful level load. Sets player_created and leaves
 * timer_started false; no timer, tick, or gameplay is started.
 */
int bounce_app_flow_record_player_initialized(BounceAppFlow *flow);

/*
 * Record that the native runtime-initialization seam completed: the mutable
 * runtime tile copy and the dynamic-thorn state derived from the level payload
 * are now owned. Requires the PLAYER_INITIALIZED boundary with level_loaded and
 * player_created already recorded, so it can only follow one successful player
 * initialization, and refuses to run twice.
 *
 * This is an implementation seam, not a recovered Java lifecycle step: the
 * original mutates one LevelTiles array in place. The coarse application state
 * stays LEVEL_LOADED because no screen, input route, camera, timer, tick, or
 * gameplay transition belongs to this step.
 */
int bounce_app_flow_record_runtime_initialized(BounceAppFlow *flow);

/*
 * Record that the already-verified camera initialization has been applied to
 * l/k/v. Requires the RUNTIME_INITIALIZED boundary with level_loaded,
 * player_created, and runtime_tiles_created already recorded, so it can only
 * follow one successful runtime initialization, and refuses to run twice.
 *
 * This records the step only; the caller performs the camera work. The coarse
 * application state stays LEVEL_LOADED because no screen, input route, timer,
 * tick, repaint, or gameplay transition belongs to this step.
 */
int bounce_app_flow_record_camera_initialized(BounceAppFlow *flow);

/*
 * Record that gameplay activation completed, moving the coarse application
 * state from LEVEL_LOADED to GAMEPLAY. This is the native stand-in for the
 * original's `this.display.setCurrent(this.v)` at BounceGame.java:155, which is
 * the only source-backed action in that post-camera block; the original has no
 * activation state, object, or flag of its own.
 *
 * Requires the CAMERA_INITIALIZED boundary with level_loaded, player_created,
 * runtime_tiles_created, and camera_initialized already recorded, so it can only
 * follow one complete initialization, and refuses to run twice.
 *
 * This starts NO timer: next_tick_ms is left at 0 and timer_started stays false,
 * because BounceGame.java:153 StartGameTimer() is a separate, later milestone.
 * It performs no tick, movement, collision, dyn-thorn update, renderer, repaint,
 * audio, persistence, or input work.
 */
int bounce_app_flow_record_gameplay_activated(BounceAppFlow *flow);

/*
 * Record that the game timer was initialized, mirroring the source's
 * StartGameTimer() boundary (b.java:835-839, scheduled at 40 ms by
 * BounceTimer.java:17). Requires the ACTIVATED boundary with every earlier
 * record present, and refuses to run twice, which matches the original
 * StartGameTimer() idempotence at b.java:836-837.
 *
 * The nominal period is owned by the platform-independent BounceGameTimer, not
 * by this record. next_tick_ms is deliberately left at 0: the original has no
 * deadline field, and adding one would introduce a catch-up/accumulator model
 * that the source does not contain. The coarse application state stays GAMEPLAY,
 * and no Tick body, tick entry, or callback runs here.
 */
int bounce_app_flow_record_timer_initialized(BounceAppFlow *flow);

/*
 * Apply the source's pre-tick countdown step, e.java:258-259:
 *
 *     if (this.p != 0)
 *         this.p--;
 *
 * The native owner of p is gameplay_entry.countdown, which InitializeGame sets
 * to BOUNCE_APP_ENTRY_COUNTDOWN (120) at e.java:119. p is not physics: the paint
 * path reads it at e.java:201 to gate the "LEVEL n" title overlay, so it is
 * pure presentation countdown state.
 *
 * Returns the resulting countdown, or -1 for a NULL flow. This is called only
 * from the Tick dispatch, after the reset and splash gates, exactly as in the
 * source ordering.
 */
int bounce_app_flow_tick_pre_countdown(BounceAppFlow *flow);

/*
 * D-13 -- arm the level-start countdown to BOUNCE_APP_ENTRY_COUNTDOWN, i.e.
 * reproduce e.java:119 `this.p = 120;`.
 *
 * begin_gameplay_entry() already records it for the staged entry routes. This
 * is for the routes that load a level without passing through that metadata
 * step: the af level skip's RESET_GATE arm and the legacy
 * bounce_game_start_level() diagnostic route. It is intentionally not gated on
 * a display state, because a level load -- not an entry state -- is what arms
 * `p` in the source, and the af skip runs while the flow is GAMEPLAY.
 * Returns 0, or -1 only for a NULL flow.
 */
int bounce_app_flow_arm_entry_countdown(BounceAppFlow *flow);

/*
 * Model-only hook for a future verified resume source; this milestone keeps it
 * false.
 *
 * STEP 37 PERSIST re-checked this and left it alone. The hook exists for Java
 * CONTINUE (BounceGame.java:225-235), whose record-3 path needs a full mid-level
 * snapshot -- the mutated LevelTiles grid, the collected-object list, and the
 * dynamic-thorn offsets -- before it can mean anything. STEP 37 persists only
 * records 1 and 2; see persistence.h. Nothing here enables CONTINUE.
 */
int bounce_app_flow_menu_set_continue_available(
    BounceAppFlow *flow,
    bool available
);

/*
 * STEP 37 PERSIST -- the Java HighScore rule, transcribed.
 *
 * BounceGame.java:422-426. Returns 1 when this call raised the record, 0 when
 * it did not, and -1 for a NULL flow. Callers that persist must save only on 1,
 * which is exactly what Java does by writing record 2 only inside the branch.
 */
int bounce_app_flow_note_score(BounceAppFlow *flow, int score);

/*
 * D-07 -- read BounceGame.q, i.e. "this run has beaten the record at least
 * once". BounceGame.java:188-190 gates both terminal forms' NEW_HIGH_SCORE
 * line on it. See the new_high_score field comment for the two writers and the
 * reason it is session state rather than persisted state. Returns false for a
 * NULL flow.
 */
bool bounce_app_flow_new_high_score(const BounceAppFlow *flow);

/* The current HighScore record. Returns 0 for a NULL flow. */
int bounce_app_flow_high_score(const BounceAppFlow *flow);

/*
 * TUGAS 2b -- adopt the persisted color-profile index at startup.
 *
 * THIS IS A SEPARATE ENTRY POINT rather than a fourth parameter to
 * bounce_app_flow_restore_persisted() because the two have different provenance
 * and different failure policies. restore_persisted() takes Java's records and
 * therefore has Java lines to be faithful to; the color profile is native-only
 * (N-03) and has none, so it gets its own function and its own comment rather
 * than being smuggled into a Java-documented one.
 *
 * The index is applied to BOTH applied_theme_index and theme_index. The Settings
 * Theme page shows theme_index as its highlight, so setting only the applied
 * value would leave the page claiming a different profile from the one in use --
 * which is exactly the bug this whole stage exists to fix.
 *
 * NO RANGE CHECK HERE, for the same reason restore_persisted() has none:
 * persistence.c cannot know BOUNCE_UI_COLOR_PROFILE_COUNT, so the bound belongs
 * at the call site. A value that is out of range is simply ignored, leaving the
 * default profile, because a saved setting must never be able to leave the
 * application with no usable palette. Returns -1 for a NULL flow.
 */
int bounce_app_flow_restore_persisted_theme(
    BounceAppFlow *flow,
    int theme_index
);

/*
 * STEP 37 PERSIST -- adopt values read from the native store at startup.
 * Neither validates ranges: persistence.c already does that on load, and a
 * caller-supplied value here is a test or restore path. Returns -1 for NULL.
 */
int bounce_app_flow_restore_persisted(
    BounceAppFlow *flow,
    int max_levels,
    int high_score
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_FLOW_H */
