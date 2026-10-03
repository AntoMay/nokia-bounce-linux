#include "app_flow.h"
#include "locale.h"

#include <stddef.h>

static void bounce_app_flow_reset_menu(BounceMenuState *menu)
{
    if (menu == NULL)
        return;
    menu->continue_available = false;
    /*
     * D-05 -- `selected_index` is DELIBERATELY NOT RESET HERE. It is
     * BounceGame.N.
     *
     * JAVA EVIDENCE. N is declared at BounceGame.java:89 and is written in
     * exactly one place, :221 inside commandAction():
     *
     *     if (displayable == this.currentList) {
     *         a(true, this.currentList.getSelectedIndex() + 1);
     *     } else {
     *         String str = this.c.getString(this.c.getSelectedIndex());
     *         this.N = this.c.getSelectedIndex();
     *
     * i.e. every SELECT on the MAIN menu remembers the highlighted row, and N is
     * a plain field on BounceGame that nothing else touches. It is read in
     * exactly one place too, ShowMainMenu() at :126-130:
     *
     *     if (this.K == 1 || this.J == 1 || this.J == 2) {
     *         this.c.setSelectedIndex(0, true);
     *     } else {
     *         this.c.setSelectedIndex(this.N, true);
     *     }
     *
     * So the highlight SURVIVES a round trip through Instructions, High Score
     * or the level List, and comes back on the row the player left from. Wiping
     * it on menu entry is the D-05 divergence this comment replaces.
     *
     * bounce_app_flow_init() sets it to 0 explicitly, which is Java's own
     * default for an uninitialised `int` field, so a fresh launch still starts
     * on row 0 and this function's two callers -- enter_menu() and init() --
     * remain the only two places that decide the entry highlight.
     *
     * THE INDEX SPACE IS THE SAME IN BOTH. Java's N indexes the appended items,
     * whose first entry is the Continue row when a session exists. Native's
     * selected_index is a POSITION in the enabled rows and
     * bounce_app_flow_menu_row_for_selection() adds 1 exactly when Continue is
     * absent, so position 0 is New Game without Continue and Continue with it --
     * the same element Java's index 0 names in each case.
     */
    menu->enabled_item_count = BOUNCE_MENU_ROW_COUNT - 1u;
    menu->pending_action = BOUNCE_MENU_ACTION_NONE;
    /* STEP 12X-P4-C-D-B-R2: the one-shot New Game intent starts cleared, so a flow
     * reset can never leave a pending score reset behind. */
    menu->new_game_score_reset_pending = false;
}

/* NATIVE EXTENSION: full reset of the Settings model, language included. */
static void bounce_app_flow_reset_settings(BounceSettingsState *settings)
{
    if (settings == NULL)
        return;
    settings->page = BOUNCE_SETTINGS_PAGE_ROOT;
    settings->selected_index = 0u;
    settings->language_index = 0u;
    /* 0 is the default English resource; a fresh flow is always English. */
    settings->applied_language_index = 0u;
    settings->theme_index = 0u;
    /* 0 is the default color profile; a fresh flow is always the default. */
    settings->applied_theme_index = 0u;
    /* STEP 34-F-K. Default ON: matches the pre-existing no-setting behaviour. */
    settings->audio_enabled = true;
    /* NATIVE ADDITION: the keypad is off until a player asks for it. */
    /*
     * STEP 38-RESET. ON, not off -- the keypad default is a product decision, and
     * this is the other half of it besides the level count above. bounce_persistence
     * still treats an ABSENT record as off, which is a different question: absence
     * means "a store written before the setting existed", while this is the value a
     * launch with no store at all starts from.
     */
    settings->t9_input_enabled = BOUNCE_DEFAULT_T9_INPUT_ENABLED;
}

/*
 * Reset only the Settings navigation, keeping the language the user chose.
 *
 * Returning to MAIN_MENU, or re-entering Settings, must not silently undo a
 * language or color-profile selection made in this session, so those entry
 * points use this. Neither selection is still persisted:
 * bounce_app_flow_init() performs the full reset, so a fresh application launch
 * always starts on the default English resource.
 *
 * STEP 34-F-K: the Audio ON/OFF choice is preserved here for the same reason
 * and on the same terms -- it survives navigating away from the Audio page
 * within the session, and is still not persisted across a launch.
 */
static void bounce_app_flow_reset_settings_navigation(
    BounceSettingsState *settings
)
{
    unsigned int applied;
    unsigned int applied_theme;
    bool audio_enabled;
    bool t9_input_enabled;

    if (settings == NULL)
        return;
    applied = settings->applied_language_index;
    applied_theme = settings->applied_theme_index;
    audio_enabled = settings->audio_enabled;
    t9_input_enabled = settings->t9_input_enabled;
    bounce_app_flow_reset_settings(settings);
    settings->applied_language_index = applied;
    settings->applied_theme_index = applied_theme;
    settings->audio_enabled = audio_enabled;
    settings->t9_input_enabled = t9_input_enabled;
}

static void bounce_app_flow_reset_gameplay_entry(BounceGameplayEntry *entry)
{
    if (entry == NULL)
        return;
    entry->level_id = 0;
    entry->initial_score = 0;
    entry->initial_lives = 0;
    entry->countdown = 0;
    entry->phase = BOUNCE_GAMEPLAY_ENTRY_PHASE_NONE;
    entry->level_loaded = false;
    entry->timer_started = false;
    entry->player_created = false;
    entry->runtime_tiles_created = false;
    entry->camera_initialized = false;
    entry->gameplay_activated = false;
}

/*
 * Record the UI 5 START_LEVEL(N) action and the matching source-anchored entry
 * level. This is the UI 5 stop point; no source initialization step runs here.
 */
static void bounce_app_flow_record_start_level(
    BounceAppFlow *flow,
    int level_id
)
{
    flow->pending_level_id = level_id;
    flow->menu.pending_action = BOUNCE_MENU_ACTION_START_LEVEL;
    flow->gameplay_entry.level_id = level_id;
    flow->gameplay_entry.phase =
        BOUNCE_GAMEPLAY_ENTRY_PHASE_START_LEVEL_RECORDED;
}

static int bounce_app_flow_enter_menu(BounceAppFlow *flow){
    /*
     * D-05 -- the live session predicate is SAMPLED BEFORE the menu reset,
     * because bounce_app_flow_reset_menu() below clears continue_available.
     *
     * JAVA EVIDENCE. ShowMainMenu() uses ONE expression twice:
     *
     *   BounceGame.java:116   if (this.K == 1 || this.J == 1 || this.J == 2)
     *                            this.c.append(this.menuStrings[0], null);
     *
     *   BounceGame.java:126   if (this.K == 1 || this.J == 1 || this.J == 2) {
     *                            this.c.setSelectedIndex(0, true);
     *                        } else {
     *                            this.c.setSelectedIndex(this.N, true);
     *                        }
     *
     * K and J are live session fields, read at the moment the menu is built, so
     * the row's PRESENCE and the highlight INDEX are decided from one snapshot.
     * Native splits the first half out (it is
     * bounce_app_flow_menu_set_continue_available(), which owns the row count),
     * so this function has to sample the flag before the reset to reproduce the
     * second half -- otherwise the predicate would always read false here and the
     * :126 branch would be unreachable.
     *
     * Native's continue_available is the reconciled form of that expression (see
     * STEP G-R3-R2: derived from the store as present && b1 == B1_IN_LEVEL), so
     * it is the same fact under a narrower name. It is sampled, not recomputed:
     * re-deriving it here would duplicate the store read that
     * app_refresh_continue_availability() owns and is not part of this fix.
     */
    bool session_available;

    if (flow == NULL)
        return -1;
    session_available = flow->menu.continue_available;
    flow->state = BOUNCE_APP_STATE_MENU;
    flow->splash_timer = 0u;
    flow->splash_skip_requested = false;
    flow->next_tick_ms = 0;
    flow->level_selection.selected_index = 0u;
    flow->pending_level_id = 0;
    bounce_app_flow_reset_gameplay_entry(&flow->gameplay_entry);
    bounce_app_flow_reset_menu(&flow->menu);
    bounce_app_flow_reset_settings_navigation(&flow->settings);
    /*
     * :127 `setSelectedIndex(0, true)` when a session exists. In the else branch
     * N is kept, which is now simply the absence of an assignment: reset_menu()
     * above deliberately left selected_index alone.
     */
    if (session_available)
        flow->menu.selected_index = 0u;
    /*
     * Java passes N straight to MIDP's setSelectedIndex, where an index past the
     * end of the List is undefined, and the List's length itself varies with the
     * predicate above. Native bounds it instead, which is the strictly safer
     * reading of the same intent and cannot contradict the source: the clamp only
     * bites in the case the source leaves undefined. It reuses the bound
     * bounce_app_flow_menu_set_continue_available() already enforces.
     */
    if (flow->menu.enabled_item_count == 0u)
        flow->menu.selected_index = 0u;
    else if (flow->menu.selected_index >= flow->menu.enabled_item_count)
        flow->menu.selected_index = flow->menu.enabled_item_count - 1u;
    return 0;
}

void bounce_app_flow_init(BounceAppFlow *flow)
{
    if (flow == NULL)
        return;
    flow->state = BOUNCE_APP_STATE_SPLASH;
    flow->level_id = BOUNCE_APP_FIRST_LEVEL_ID;
    flow->splash_timer = 0u;
    flow->splash_skip_requested = false;
    flow->next_tick_ms = 0;
    /*
     * STEP 38-RESET. Two, not the source's zero -- see the
     * BOUNCE_DEFAULT_UNLOCKED_LEVEL_COUNT comment in app_flow.h for why a fresh
     * install is given levels 1 and 2 rather than none.
     */
    flow->level_selection.available_level_count =
        (unsigned int)BOUNCE_DEFAULT_UNLOCKED_LEVEL_COUNT;
    flow->level_selection.selected_index = 0u;
    flow->pending_level_id = 0;
    /* STEP 37 PERSIST -- Java HighScore default. BounceGame.java declares it at
       :23 and LoadRecords() leaves it 0 on a fresh install (:276-279), because
       the fresh branch creates the records and never reads them back. */
    flow->high_score = 0;
    /*
     * D-07 -- BounceGame.q. `public boolean q;` (BounceGame.java:19) is a plain
     * field with no explicit initialiser, so Java gives it false. It is absent
     * from every record, so a fresh launch starts with it false as well.
     *
     * It is set explicitly here rather than being left to the caller: a caller
     * that declares BounceAppFlow on the stack and calls only this initialiser
     * would otherwise read an indeterminate value. bounce_game_init() memsets the
     * whole context first, so production was never exposed -- but the shell
     * verifiers do exactly that, and a verifier must not be able to pass or fail
     * on stack garbage.
     */
    flow->new_high_score = false;
    /*
     * D-05 -- BounceGame.N's initial value. `private int N;`
     * (BounceGame.java:89) is a plain field with no initialiser, so Java gives
     * it 0, and no record carries it either (LoadRecords() at
     * BounceGame.java:290-330 reads no N). bounce_app_flow_reset_menu() below no
     * longer sets selected_index -- it is N, and N persists -- so a fresh launch
     * gets its 0 here instead.
     */
    flow->menu.selected_index = 0u;
    flow->instructions_scroll = 0u;
    bounce_app_flow_reset_gameplay_entry(&flow->gameplay_entry);
    bounce_app_flow_reset_menu(&flow->menu);
    bounce_app_flow_reset_settings(&flow->settings);
}

int bounce_app_flow_apply(
    BounceAppFlow *flow,
    BounceAppEvent event
)
{
    if (flow == NULL)
        return -1;

    switch (event) {
        case BOUNCE_APP_EVENT_START:
            if (flow->state != BOUNCE_APP_STATE_MENU)
                return -1;
            flow->state = BOUNCE_APP_STATE_GAMEPLAY;
            return 0;
        case BOUNCE_APP_EVENT_PAUSE:
            if (flow->state != BOUNCE_APP_STATE_GAMEPLAY)
                return -1;
            flow->state = BOUNCE_APP_STATE_PAUSE;
            return 0;
        case BOUNCE_APP_EVENT_RESUME:
            if (flow->state != BOUNCE_APP_STATE_PAUSE)
                return -1;
            flow->state = BOUNCE_APP_STATE_GAMEPLAY;
            return 0;
        case BOUNCE_APP_EVENT_GAME_OVER:
            if (flow->state != BOUNCE_APP_STATE_GAMEPLAY)
                return -1;
            flow->state = BOUNCE_APP_STATE_GAME_OVER;
            return 0;
        case BOUNCE_APP_EVENT_LEVEL_COMPLETE:
            if (flow->state != BOUNCE_APP_STATE_GAMEPLAY)
                return -1;
            flow->state = BOUNCE_APP_STATE_LEVEL_COMPLETE;
            return 0;
        case BOUNCE_APP_EVENT_GAME_END:
            /*
             * STEP 12X-P4-B. The e.java:320 branch. Accepted only from the
             * completion state, and it moves to the terminal destination rather
             * than back into gameplay.
             *
             * The guard deliberately does NOT consult the level bounds. The
             * caller decides that the level just completed was the last playable
             * one, via bounce_app_flow_level_is_final(); duplicating that test
             * here would give the two sites independent definitions of "final".
             */
            if (flow->state != BOUNCE_APP_STATE_LEVEL_COMPLETE)
                return -1;
            flow->state = BOUNCE_APP_STATE_GAME_END;
            return 0;
        case BOUNCE_APP_EVENT_NEXT_LEVEL:
            if (flow->state != BOUNCE_APP_STATE_LEVEL_COMPLETE
                || flow->level_id >= BOUNCE_APP_LAST_LEVEL_ID)
                return -1;
            ++flow->level_id;
            flow->state = BOUNCE_APP_STATE_GAMEPLAY;
            return 0;
        case BOUNCE_APP_EVENT_MENU:
            if (flow->state != BOUNCE_APP_STATE_GAMEPLAY
                && flow->state != BOUNCE_APP_STATE_PAUSE
                && flow->state != BOUNCE_APP_STATE_GAME_OVER
                && flow->state != BOUNCE_APP_STATE_LEVEL_COMPLETE
                /* STEP 12X-P4-B: the terminal destination's only exit. */
                && flow->state != BOUNCE_APP_STATE_GAME_END
                && flow->state != BOUNCE_APP_STATE_ACTION_BOUNDARY
                && flow->state != BOUNCE_APP_STATE_INSTRUCTIONS
                && flow->state != BOUNCE_APP_STATE_HIGH_SCORE
                && flow->state != BOUNCE_APP_STATE_LEVEL_SELECTION
                && flow->state != BOUNCE_APP_STATE_GAMEPLAY_ENTRY
                && flow->state != BOUNCE_APP_STATE_LEVEL_LOADED)
                return -1;
            return bounce_app_flow_enter_menu(flow);
        case BOUNCE_APP_EVENT_SPLASH_DONE:
            if (flow->state == BOUNCE_APP_STATE_SPLASH)
                return bounce_app_flow_enter_menu(flow);
            if (flow->state == BOUNCE_APP_STATE_MENU)
                return 0;
            return -1;
    }
    return -1;
}

int bounce_app_flow_tick(BounceAppFlow *flow)
{
    if (flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_SPLASH)
        return 0;

    if (flow->splash_skip_requested
        || flow->splash_timer > BOUNCE_APP_SPLASH_TIMER_LIMIT) {
        if (bounce_app_flow_enter_menu(flow) != 0)
            return -1;
        return 1;
    }

    ++flow->splash_timer;
    return 0;
}

int bounce_app_flow_request_splash_skip(BounceAppFlow *flow)
{
    if (flow == NULL || flow->state != BOUNCE_APP_STATE_SPLASH)
        return -1;
    flow->splash_skip_requested = true;
    return 0;
}

unsigned int bounce_app_flow_menu_enabled_item_count(
    const BounceAppFlow *flow
)
{
    if (flow == NULL)
        return 0u;
    return flow->menu.enabled_item_count;
}

bool bounce_app_flow_menu_item_enabled(
    const BounceAppFlow *flow,
    unsigned int row
)
{
    if (flow == NULL || row >= BOUNCE_MENU_ROW_COUNT)
        return false;
    if (row == 0u)
        return flow->menu.continue_available;
    return true;
}

unsigned int bounce_app_flow_menu_row_for_selection(
    const BounceAppFlow *flow,
    unsigned int selection_index
)
{
    unsigned int count;

    if (flow == NULL)
        return 0u;
    count = flow->menu.enabled_item_count;
    if (selection_index >= count)
        return count;
    if (flow->menu.continue_available)
        return selection_index;
    return selection_index + 1u;
}

int bounce_app_flow_menu_move(
    BounceAppFlow *flow,
    BounceMenuDirection direction
)
{
    if (flow == NULL || flow->state != BOUNCE_APP_STATE_MENU)
        return -1;
    if (direction != BOUNCE_MENU_DIRECTION_UP
        && direction != BOUNCE_MENU_DIRECTION_DOWN)
        return -1;

    if (direction == BOUNCE_MENU_DIRECTION_UP) {
        if (flow->menu.selected_index > 0u)
            --flow->menu.selected_index;
    } else if (flow->menu.selected_index + 1u
        < flow->menu.enabled_item_count) {
        ++flow->menu.selected_index;
    }
    flow->menu.pending_action = BOUNCE_MENU_ACTION_NONE;
    return 0;
}

BounceMenuAction bounce_app_flow_menu_select(BounceAppFlow *flow)
{
    unsigned int row;
    BounceMenuAction action;

    if (flow == NULL || flow->state != BOUNCE_APP_STATE_MENU)
        return BOUNCE_MENU_ACTION_NONE;
    row = bounce_app_flow_menu_row_for_selection(
        flow,
        flow->menu.selected_index
    );
    switch (row) {
        case 0u:
            action = flow->menu.continue_available
                ? BOUNCE_MENU_ACTION_CONTINUE
                : BOUNCE_MENU_ACTION_CONTINUE_UNAVAILABLE;
            break;
        case 1u:
            action = BOUNCE_MENU_ACTION_NEW_GAME;
            break;
        case 2u:
            action = BOUNCE_MENU_ACTION_HIGH_SCORE;
            break;
        case 3u:
            action = BOUNCE_MENU_ACTION_INSTRUCTIONS;
            break;
        /*
         * NATIVE EXTENSIONS. Rows 4 and 5 have no recovered-source counterpart;
         * see the BOUNCE_MENU_ROW_COUNT comment in app_flow.h.
         */
        case 4u:
            action = BOUNCE_MENU_ACTION_SETTINGS;
            break;
        case 5u:
            action = BOUNCE_MENU_ACTION_ABOUT;
            break;
        /*
         * The original attaches Exit as Command.EXIT rather than a list element
         * (BounceGame.java:120-121). It is a selectable row here purely so the
         * native menu can be exited with the keyboard.
         */
        case 6u:
            action = BOUNCE_MENU_ACTION_EXIT;
            break;
        default:
            action = BOUNCE_MENU_ACTION_NONE;
            break;
    }
    flow->menu.pending_action = action;
    if (action == BOUNCE_MENU_ACTION_NEW_GAME) {
        if (flow->level_selection.available_level_count > 1u) {
            flow->state = BOUNCE_APP_STATE_LEVEL_SELECTION;
            flow->level_selection.selected_index = 0u;
            flow->pending_level_id = 0;
            flow->gameplay_entry.level_id = 0;
            flow->gameplay_entry.phase =
                BOUNCE_GAMEPLAY_ENTRY_PHASE_NONE;
        } else {
            /* BounceGame.java:241-242 sets K = 4 then calls a(true, 1). */
            flow->state = BOUNCE_APP_STATE_ACTION_BOUNDARY;
            bounce_app_flow_record_start_level(
                flow,
                BOUNCE_APP_FIRST_LEVEL_ID
            );
            /*
             * STEP 12X-P4-C-D-B-R2: this branch is Java's a(true, 1), which zeroes
             * the score through e.a(level, 0, 3). The flag is recorded AFTER
             * record_start_level() on purpose: that function is what overwrites
             * menu.pending_action, so recording the intent before it would put the
             * write in the path that overwrite clears. Ordering verified in
             * Step 13.27.5.
             */
            flow->menu.new_game_score_reset_pending = true;
        }
        flow->next_tick_ms = 0;
    } else if (action == BOUNCE_MENU_ACTION_INSTRUCTIONS) {
        flow->state = BOUNCE_APP_STATE_INSTRUCTIONS;
        flow->pending_level_id = 0;
        flow->next_tick_ms = 0;
        /*
         * The Instructions page always OPENS AT THE TOP. A reader who scrolled
         * halfway down the Chinese text, left, and came back must not land in the
         * middle of it with no way to tell that the text above was skipped.
         *
         * It is reset here, at the transition, rather than when the reader leaves,
         * so every route into the page gets it: the menu row, and the reset_menu()
         * path that returns to the menu from a sub-screen.
         */
        flow->instructions_scroll = 0u;
    } else if (action == BOUNCE_MENU_ACTION_HIGH_SCORE) {
        flow->state = BOUNCE_APP_STATE_HIGH_SCORE;
        flow->pending_level_id = 0;
        flow->next_tick_ms = 0;
    } else if (action == BOUNCE_MENU_ACTION_SETTINGS) {
        /* NATIVE EXTENSION destination. Navigation resets; the language does not. */
        flow->state = BOUNCE_APP_STATE_SETTINGS;
        bounce_app_flow_reset_settings_navigation(&flow->settings);
        flow->pending_level_id = 0;
        flow->next_tick_ms = 0;
    } else if (action == BOUNCE_MENU_ACTION_ABOUT) {
        /* NATIVE EXTENSION destination. */
        flow->state = BOUNCE_APP_STATE_ABOUT;
        flow->pending_level_id = 0;
        flow->next_tick_ms = 0;
    } else if (action == BOUNCE_MENU_ACTION_CONTINUE) {
        /*
         * STEP G-R3-R2 -- the Record 3 resume is initiated by the APP SHELL, not
         * here, and this branch deliberately does NOT transition.
         *
         * Every other destination in this chain moves the flow itself, because
         * each one needs nothing from outside the flow. CONTINUE needs two values
         * that only the shell has: the decoded Record 3's level and lives. The
         * fallback arm below would send it to ACTION_BOUNDARY with
         * pending_level_id == 0, which bounce_app_flow_begin_gameplay_entry()
         * rejects -- so the flow would stall at the boundary instead of resuming.
         *
         * Leaving the state at MENU with pending_action == CONTINUE is what the
         * shell keys on. It then calls bounce_app_flow_begin_resume_entry(), which
         * performs the START_LEVEL recording and the ACTION_BOUNDARY transition
         * itself, using Java's own a(false, level) shape
         * (BounceGame.java:223-224).
         */
        flow->next_tick_ms = 0;
    } else if (action != BOUNCE_MENU_ACTION_NONE
        && action != BOUNCE_MENU_ACTION_EXIT) {
        flow->state = BOUNCE_APP_STATE_ACTION_BOUNDARY;
        flow->pending_level_id = 0;
        flow->next_tick_ms = 0;
    }
    return action;
}

int bounce_app_flow_return_to_menu(BounceAppFlow *flow)
{
    if (flow == NULL
        || (flow->state != BOUNCE_APP_STATE_ACTION_BOUNDARY
            && flow->state != BOUNCE_APP_STATE_INSTRUCTIONS
            && flow->state != BOUNCE_APP_STATE_HIGH_SCORE
            && flow->state != BOUNCE_APP_STATE_LEVEL_SELECTION
            && flow->state != BOUNCE_APP_STATE_GAMEPLAY_ENTRY
            && flow->state != BOUNCE_APP_STATE_LEVEL_LOADED
            /* NATIVE EXTENSION destinations may also return to the menu. */
            && flow->state != BOUNCE_APP_STATE_SETTINGS
            && flow->state != BOUNCE_APP_STATE_ABOUT))
        return -1;
    return bounce_app_flow_enter_menu(flow);
}

/* ------------------------------------------------------------------ */
/* NATIVE EXTENSION: Settings and About.                                */
/*                                                                     */
/* Neither destination exists in the recovered Nokia Bounce source.    */
/* The complete source search recorded in                             */
/* reverse/ui-menu-persistence-localization-audit.md section 4 states  */
/* there is no Options, Settings, Language, About, or Help menu item. */
/* Everything below is a native Linux-reimplementation addition.        */
/* ------------------------------------------------------------------ */

int bounce_app_flow_enter_settings(BounceAppFlow *flow)
{
    if (flow == NULL || flow->state != BOUNCE_APP_STATE_MENU)
        return -1;
    flow->state = BOUNCE_APP_STATE_SETTINGS;
    /* Navigation resets; a language chosen earlier in the session is kept. */
    bounce_app_flow_reset_settings_navigation(&flow->settings);
    return 0;
}

int bounce_app_flow_enter_about(BounceAppFlow *flow)
{
    if (flow == NULL || flow->state != BOUNCE_APP_STATE_MENU)
        return -1;
    flow->state = BOUNCE_APP_STATE_ABOUT;
    return 0;
}

BounceSettingsPage bounce_app_flow_settings_page(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return BOUNCE_SETTINGS_PAGE_ROOT;
    return flow->settings.page;
}

unsigned int bounce_app_flow_settings_selected_index(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0u;
    return flow->settings.selected_index;
}

unsigned int bounce_app_flow_settings_language_index(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0u;
    return flow->settings.language_index;
}

unsigned int bounce_app_flow_settings_applied_language_index(
    const BounceAppFlow *flow
)
{
    if (flow == NULL)
        return 0u;
    return flow->settings.applied_language_index;
}

unsigned int bounce_app_flow_settings_theme_index(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0u;
    return flow->settings.theme_index;
}

unsigned int bounce_app_flow_settings_applied_theme_index(
    const BounceAppFlow *flow
)
{
    if (flow == NULL)
        return 0u;
    return flow->settings.applied_theme_index;
}

/*
 * STEP 34-F-K. Read-only. See app_flow.h for the full contract: native setting,
 * default ON, not persisted, and NULL means "allowed" so a caller without a
 * shell keeps the pre-existing behaviour.
 */
int bounce_app_flow_restore_persisted_t9(
    BounceAppFlow *flow,
    bool enabled
)
{
    if (flow == NULL)
        return -1;
    flow->settings.t9_input_enabled = enabled;
    return 0;
}

bool bounce_app_flow_settings_t9_enabled(
    const BounceAppFlow *flow
)
{
    if (flow == NULL)
        return false;
    return flow->settings.t9_input_enabled;
}

bool bounce_app_flow_settings_audio_enabled(
    const BounceAppFlow *flow
)
{
    if (flow == NULL)
        return true;
    return flow->settings.audio_enabled;
}

/*
 * Only the Settings root list and the Language page are navigable lists. The
 * Display, Input, and Audio pages are read-only fact pages, so they report no
 * list and refuse movement.
 */
unsigned int bounce_app_flow_settings_list_count(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0u;
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_ROOT)
        return BOUNCE_SETTINGS_ROOT_ITEM_COUNT;
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_LANGUAGE)
        return BOUNCE_LANGUAGE_RESOURCE_COUNT;
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_THEME)
        return BOUNCE_UI_THEME_COUNT;
    /*
     * STEP 34-F-K. The Audio page is no longer a read-only fact page: it has
     * exactly two rows, ON and OFF. It is still a single setting with no
     * device row, no volume, no mute and no test control.
     */
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_AUDIO)
        return BOUNCE_SETTINGS_AUDIO_VALUE_COUNT;
    /*
     * NATIVE ADDITION. The Input page used to report no rows and was a read-only
     * fact table, so UP, DOWN and SELECT on it all returned 0. It now offers the
     * one setting it has, in the same two-row shape as the Audio page.
     */
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_INPUT)
        return BOUNCE_SETTINGS_T9_VALUE_COUNT;
    /*
     * STEP 38-RESET. Two rows, Confirm and Cancel, which is the same shape as the
     * Audio and Input pages: a highlight plus a SELECT that acts.
     */
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_RESET)
        return BOUNCE_SETTINGS_RESET_VALUE_COUNT;
    return 0u;
}

const char *bounce_app_flow_language_resource_name(unsigned int index)
{
    if (index >= BOUNCE_LANGUAGE_RESOURCE_COUNT)
        return NULL;
    return bounce_locale_resource_name((BounceLanguage)index);
}

int bounce_app_flow_settings_move(
    BounceAppFlow *flow,
    BounceMenuDirection direction
)
{
    unsigned int count;

    if (flow == NULL || flow->state != BOUNCE_APP_STATE_SETTINGS)
        return -1;
    if (direction != BOUNCE_MENU_DIRECTION_UP
        && direction != BOUNCE_MENU_DIRECTION_DOWN)
        return -1;
    count = bounce_app_flow_settings_list_count(flow);
    if (count == 0u)
        return -1;

    /*
     * Bounded and non-wrapping, matching the main menu and the level list. The
     * original device List wrap behavior is UNKNOWN (audit section 4,
     * "Navigation unknowns"), so wrapping is not invented here.
     */
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_THEME) {
        if (direction == BOUNCE_MENU_DIRECTION_UP) {
            if (flow->settings.theme_index > 0u)
                --flow->settings.theme_index;
        } else if (flow->settings.theme_index + 1u < count) {
            ++flow->settings.theme_index;
        }
        return 0;
    }
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_LANGUAGE) {
        if (direction == BOUNCE_MENU_DIRECTION_UP) {
            if (flow->settings.language_index > 0u)
                --flow->settings.language_index;
        } else if (flow->settings.language_index + 1u < count) {
            ++flow->settings.language_index;
        }
        return 0;
    }
    if (direction == BOUNCE_MENU_DIRECTION_UP) {
        if (flow->settings.selected_index > 0u)
            --flow->settings.selected_index;
    } else if (flow->settings.selected_index + 1u < count) {
        ++flow->settings.selected_index;
    }
    return 0;
}

/*
 * STEP 38-RESET -- the Reset to Default action, and the consume half of its
 * one-shot intent. bounce_app_flow_reset_to_default() carries the reasoning for
 * what each field becomes and, more importantly, for the three it leaves alone;
 * see app_flow.h.
 */
int bounce_app_flow_reset_to_default(BounceAppFlow *flow)
{
    if (flow == NULL || flow->state != BOUNCE_APP_STATE_SETTINGS)
        return -1;

    flow->level_selection.available_level_count =
        (unsigned int)BOUNCE_DEFAULT_UNLOCKED_LEVEL_COUNT;
    flow->level_selection.selected_index = 0u;
    flow->high_score = 0;
    /* The "new high score" notice is a claim about the run that just ended. */
    flow->new_high_score = false;

    flow->settings.applied_theme_index = 0u;
    /* The highlight follows the applied value, or the Theme page would claim a
     * profile the game is not using -- the same defect the restore path documents. */
    flow->settings.theme_index = 0u;
    flow->settings.audio_enabled = BOUNCE_DEFAULT_AUDIO_ENABLED;
    flow->settings.t9_input_enabled = BOUNCE_DEFAULT_T9_INPUT_ENABLED;
    flow->settings.applied_language_index = BOUNCE_DEFAULT_LANGUAGE_INDEX;
    flow->settings.language_index = 0u;

    flow->menu.continue_available = false;
    flow->menu.selected_index = 0u;
    /*
     * Leave the player on the page they confirmed from, with the highlight on
     * Confirm, so the screen they are looking at still describes what they just did
     * rather than silently jumping somewhere else mid-press.
     */
    flow->settings.page = BOUNCE_SETTINGS_PAGE_RESET;
    flow->settings.selected_index = BOUNCE_SETTINGS_RESET_ROW_CONFIRM;
    /*
     * Raised for the shell, which is the only layer that can delete Record 3 and
     * rewrite records 1, 2, 4 and 5. See the field's comment in app_flow.h.
     */
    flow->settings.reset_to_default_pending = true;
    return 0;
}

bool bounce_app_flow_consume_reset_to_default(BounceAppFlow *flow)
{
    bool pending;

    if (flow == NULL)
        return false;
    pending = flow->settings.reset_to_default_pending;
    /* Cleared as it is read, so the consumer's deletions cannot run twice. */
    flow->settings.reset_to_default_pending = false;
    return pending;
}

int bounce_app_flow_settings_select(BounceAppFlow *flow)
{
    unsigned int count;

    if (flow == NULL || flow->state != BOUNCE_APP_STATE_SETTINGS)
        return -1;
    count = bounce_app_flow_settings_list_count(flow);

    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_LANGUAGE) {
        /*
         * Record the chosen language. The flow stays the owner; the application
         * mirrors this value into the active language each frame.
         *
         * A recovered Chinese or Thai resource is a real file, but no
         * localization decoder exists, so selecting one resolves to the same
         * source-backed English text. That is not a partial switch: the whole
         * table falls back together, and the Language screen labels those rows
         * "RESOURCE ONLY" so the state is never mistaken for a working
         * translation.
         */
        if (flow->settings.language_index >= count)
            return -1;
        flow->settings.applied_language_index = flow->settings.language_index;
        return 0;
    }
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_THEME) {
        /* The color profile is applied only when its row is selected. */
        if (flow->settings.theme_index >= count)
            return -1;
        flow->settings.applied_theme_index = flow->settings.theme_index;
        return 0;
    }
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_AUDIO) {
        /*
         * STEP 34-F-K. The Audio page reuses `selected_index` as its highlight,
         * exactly as the root list does, because it has only two rows. The
         * applied value is separate, so moving the highlight does not change
         * the setting -- SELECT does, which is the "SELECT TO SWITCH"
         * convention the other settings pages already use.
         */
        if (flow->settings.selected_index >= count)
            return -1;
        flow->settings.audio_enabled
            = (flow->settings.selected_index == BOUNCE_SETTINGS_AUDIO_ROW_ON);
        return 0;
    }
    /*
     * NATIVE ADDITION. The Input page's two rows are its highlight and its
     * setting in one, exactly as the Audio page does it: moving does not change
     * the setting, SELECT does.
     */
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_INPUT) {
        if (flow->settings.selected_index >= count)
            return -1;
        flow->settings.t9_input_enabled
            = (flow->settings.selected_index == BOUNCE_SETTINGS_T9_ROW_ON);
        return 0;
    }
    /*
     * STEP 38-RESET-UI. Two rows, and they now do genuinely different things: CONFIRM
     * destroys, BACK navigates.
     *
     * BACK DELEGATES TO bounce_app_flow_settings_back() rather than re-implementing
     * "leave this page". That is the point of naming the row Back: the ESC key, the
     * clickable band along the bottom of the screen and this row are then three inputs
     * into one function, so none of them can drift. The old row was a bare `return 0`,
     * which is why it read as a dead button -- it was not wired to the one function
     * that already did the job.
     *
     * It is safe to delegate even though this is a confirmation screen, because
     * bounce_app_flow_settings_back() is pure navigation. It leaves the page, the
     * highlight and the Settings root; it does not restore anything, clear anything,
     * or touch the store. The reset is reachable from CONFIRM and from nowhere else,
     * which is unchanged.
     *
     * CONFIRM delegates to bounce_app_flow_reset_to_default() so there is still exactly
     * one definition of the fresh-install state rather than two.
     */
    if (flow->settings.page == BOUNCE_SETTINGS_PAGE_RESET) {
        if (flow->settings.selected_index >= count)
            return -1;
        if (flow->settings.selected_index == BOUNCE_SETTINGS_RESET_ROW_CONFIRM)
            return bounce_app_flow_reset_to_default(flow);
        return bounce_app_flow_settings_back(flow);
    }
    if (flow->settings.page != BOUNCE_SETTINGS_PAGE_ROOT)
        return -1;
    if (flow->settings.selected_index >= BOUNCE_SETTINGS_ROOT_ITEM_COUNT)
        return -1;

    /* Root rows map in order onto the page enum values 1..5. */
    flow->settings.page = (BounceSettingsPage)(
        (int)flow->settings.selected_index + 1
    );
    /*
     * STEP 38-RESET -- AND THE HIGHLIGHT IS CLAMPED INTO THE PAGE JUST OPENED.
     *
     * What this fixes: selected_index is the ROOT row number, and the root has six
     * rows while the pages it opens have two or none. Carrying the root number into
     * the page left the highlight off the end of the new list, which is not cosmetic
     * -- bounce_app_flow_settings_select() rejects any index at or past the page's
     * row count, so Audio (root row 2 of two) had an unusable page and Reset (root
     * row 5 of two) would have had one too. The player would have navigated to the
     * setting, seen no row highlighted, and had SELECT do nothing.
     *
     * Why it is a clamp to 0 and not a clamp to the last row: the Audio, Input and
     * Reset pages both present their list as "ON then OFF" or "Confirm then Cancel",
     * so row 0 is the value a reset or a fresh visit should land on. It is also what
     * bounce_app_flow_settings_back() has always produced on the way out, so
     * re-entering a page now agrees with coming back from it.
     *
     * A page with no rows (Display) keeps index 0, which its own render and its own
     * "SELECT fails" checks already assumed.
     */
    count = bounce_app_flow_settings_list_count(flow);
    if (flow->settings.selected_index >= count)
        flow->settings.selected_index = 0u;
    return 0;
}

int bounce_app_flow_settings_back(BounceAppFlow *flow)
{
    if (flow == NULL || flow->state != BOUNCE_APP_STATE_SETTINGS)
        return -1;
    if (flow->settings.page != BOUNCE_SETTINGS_PAGE_ROOT) {
        flow->settings.page = BOUNCE_SETTINGS_PAGE_ROOT;
        flow->settings.selected_index = 0u;
        return 0;
    }
    return bounce_app_flow_enter_menu(flow);
}

unsigned int bounce_app_flow_available_level_count(
    const BounceAppFlow *flow
)
{
    if (flow == NULL)
        return 0u;
    return flow->level_selection.available_level_count;
}

unsigned int bounce_app_flow_level_selection_selected_index(
    const BounceAppFlow *flow
)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_LEVEL_SELECTION)
        return 0u;
    return flow->level_selection.selected_index;
}

int bounce_app_flow_pending_level_id(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0;
    return flow->pending_level_id;
}

int bounce_app_flow_set_available_level_count(
    BounceAppFlow *flow,
    unsigned int count
)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_MENU
        || count > BOUNCE_APP_LAST_LEVEL_ID)
        return -1;
    flow->level_selection.available_level_count = count;
    flow->level_selection.selected_index = 0u;
    flow->pending_level_id = 0;
    flow->menu.pending_action = BOUNCE_MENU_ACTION_NONE;
    return 0;
}

/*
 * Java e.java:313-319, the progression part of the level-completion block.
 *
 * The original performs two separate mutations, and both are reproduced here in
 * the original order:
 *
 *   e.java:317            this.level++;
 *   BounceGame.java:418   if (this.v.level > this.MaxLevels) {
 *   BounceGame.java:419       this.MaxLevels = Math.min(this.v.level, 11);
 *   BounceGame.java:420       WriteToStore(1);
 *
 * Only the RMS write is omitted; there is no persistence layer in this tree.
 * The count therefore still advances in memory exactly as it does on a handset,
 * which is what makes the Java New Game gate at BounceGame.java:238 --
 *
 *   if (this.MaxLevels > 1) { LevelSelect(); } else { a(true, 1); }
 *
 * take its source-correct branch. The already-completed level is the one whose
 * exit was reached, so the count becomes that next level's number, not the
 * completed one.
 *
 * WHY NOT THE EXISTING SETTER. bounce_app_flow_set_available_level_count() at
 * :610 correctly refuses any state other than MENU, and this block runs while
 * the flow is in BOUNCE_APP_STATE_LEVEL_COMPLETE. That guard is deliberate and
 * is not weakened here: it stops a staged or in-flight level from being
 * re-declared behind the caller's back. This function instead derives the count
 * from flow->level_id and takes no count argument at all, so it cannot be used
 * as a general setter -- the only value it can ever publish is
 * min(flow->level_id, BOUNCE_APP_LAST_LEVEL_ID).
 *
 * THE MONOTONIC GUARD IS KEPT. BounceGame.java:418 tests
 * this.v.level > this.MaxLevels, so a level replayed from Level Select does not
 * lower the count. flow->level_id is left unguarded for the same reason
 * e.java:317 is: the source performs no clamp there, which is what lets
 * `if (this.level > 11)` at e.java:320 detect the final level. The clamp lives
 * only in the count, so completing Level 11 leaves level_id at 12 and the count
 * at BOUNCE_APP_LAST_LEVEL_ID.
 */
int bounce_app_flow_complete_level(BounceAppFlow *flow)
{
    if (flow == NULL)
        return -1;
    ++flow->level_id;
    if (flow->level_id > (int)flow->level_selection.available_level_count) {
        if (flow->level_id < BOUNCE_APP_LAST_LEVEL_ID)
            flow->level_selection.available_level_count =
                (unsigned int)flow->level_id;
        else
            flow->level_selection.available_level_count =
                (unsigned int)BOUNCE_APP_LAST_LEVEL_ID;
    }
    return 0;
}

/*
 * STEP 12X-G-P4 -- the CONTINUE transition.
 *
 * Java's CONTINUE is BounceGame.java:263-266:
 *
 *   } else if (cmd == this.commandContinue) {
 *       this.K = 1;
 *       this.display.setCurrent(this.v);
 *   }
 *
 * Two statements, no arithmetic: it does not increment the level, does not
 * start the game timer, does not call InitializeGame() and does not call
 * LoadLevelId(). It only re-establishes the active-gameplay session marker (K)
 * and puts the canvas back on screen. The level it reveals was already loaded
 * and initialised before the command could be pressed, because
 * ShowLevelComplete() restarts the game timer at BounceGame.java:153 through
 * a(false, 0) at :201, BEFORE it displays the form at :211, and
 * BounceTimer.schedule(this, 0L, 40L) at BounceTimer.java:17 has a zero
 * initial delay, so e.Tick() -- whose first statement consumes the b.d latch at
 * e.java:222 -- runs InitializeGame() behind the form.
 *
 * THE NATIVE MECHANISM DIFFERS IN ONE PLACE ONLY, AND IT IS NOT ARBITRARY.
 * Native has no "already initialised behind the form" state. The frame loop
 * dispatches a gameplay tick only when app_canonical_gameplay_active() is true
 * (vertical_slice.c:4957-4961), which requires
 * flow.state == BOUNCE_APP_STATE_GAMEPLAY, and the branch it is written to take
 * otherwise is the Step 12X-G-R1/R2/R4 "CANONUAL GAMEPLAY HAS ENDED -- return
 * without routing anywhere" invariant. Making a tick run in LEVEL_COMPLETE would
 * mean rewriting that invariant, so native instead performs the same
 * initialisation when CONTINUE is actually given.
 *
 * WHAT IS REUSED, AND WHY IT IS THE SAME INITIALISATION. The destination here is
 * the ordinary gameplay-entry staging for flow->level_id, which is the native
 * counterpart of Java's `this.v.a(paramInt, 0, 3)` at BounceGame.java:151 --
 * that call, through e.java:81-90, ends in InitializeGame() as well. The staged
 * chain reproduces it exactly: bounce_app_flow_begin_gameplay_entry() records
 * BOUNCE_APP_ENTRY_INITIAL_SCORE (0) and BOUNCE_APP_ENTRY_INITIAL_LIVES (3),
 * and bounce_game_load_level_entry() is the LoadLevelId counterpart that also
 * clears the b.d latch (b.java:213). So the same load, the same new player, the
 * same camera and the same timer are performed; only WHEN differs.
 *
 * NO NEW STATE. The destination ACTION_BOUNDARY with pending_action
 * BOUNCE_MENU_ACTION_START_LEVEL is the state the ordinary New Game and Level
 * Select routes already produce through bounce_app_flow_record_start_level(),
 * and bounce_app_flow_begin_gameplay_entry() already consumes it. No enum value,
 * no state, no event and no phase is added by this function.
 *
 * THE FINAL LEVEL IS REFUSED HERE, not routed anywhere. e.java:320 sends
 * level > 11 to TODO_ShowInstructions(true) instead of ShowLevelComplete(), and
 * that final-level form is outside this step's scope, so a flow whose level_id
 * has passed BOUNCE_APP_LAST_LEVEL_ID is refused rather than silently wrapped or
 * clamped. It still carries its own deferred latch; this step does not consume
 * it.
 */
bool bounce_app_flow_level_is_final(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return false;
    /*
     * flow->level_id is the level that was JUST completed, advanced by
     * bounce_app_flow_complete_level() immediately before this is called, and
     * available_level_count is the MaxLevels equivalent that the same function
     * has just clamped. See app_flow.h for why this equals the source's
     * `this.level > 11`.
     */
    return flow->level_id > (int)flow->level_selection.available_level_count;
}

int bounce_app_flow_continue_level(BounceAppFlow *flow)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_LEVEL_COMPLETE
        || flow->level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || flow->level_id > BOUNCE_APP_LAST_LEVEL_ID)
        return -1;
    /*
     * The level is NOT incremented here: e.java:317 already did that inside the
     * completion block, which is the same value the new-game routes then load.
     * Continuing therefore re-enters the existing staged start for that level
     * rather than computing a new target.
     */
    bounce_app_flow_record_start_level(flow, flow->level_id);
    flow->state = BOUNCE_APP_STATE_ACTION_BOUNDARY;
    flow->next_tick_ms = 0;
    return flow->pending_level_id;
}

int bounce_app_flow_level_selection_move(
    BounceAppFlow *flow,
    BounceMenuDirection direction
){
    unsigned int count;

    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_LEVEL_SELECTION
        || (direction != BOUNCE_MENU_DIRECTION_UP
            && direction != BOUNCE_MENU_DIRECTION_DOWN))
        return -1;
    count = flow->level_selection.available_level_count;
    if (count == 0u)
        return -1;
    if (direction == BOUNCE_MENU_DIRECTION_UP) {
        if (flow->level_selection.selected_index > 0u)
            --flow->level_selection.selected_index;
    } else if (flow->level_selection.selected_index + 1u < count) {
        ++flow->level_selection.selected_index;
    }
    return 0;
}

bool bounce_app_flow_consume_new_game_score_reset(BounceAppFlow *flow)
{
    bool pending;

    if (flow == NULL)
        return false;
    pending = flow->menu.new_game_score_reset_pending;
    /*
     * Cleared unconditionally, not only when it happened to be set, so consuming on a
     * route that never requested a reset still leaves the flag cleared. That makes the
     * operation idempotent and keeps the reset strictly one-shot.
     */
    flow->menu.new_game_score_reset_pending = false;
    /*
     * D-07 -- `this.q = false;` at BounceGame.java:150, which is the first
     * statement of the `if (paramBoolean)` branch and therefore runs on exactly
     * the routes that request a new-game score reset: New Game
     * (BounceGame.java:242 -> :151) and the level-list SELECT (:218 -> :151).
     *
     * Consuming this flag is the native form of entering that branch, because
     * new_game_score_reset_pending is set by precisely those two origins and by
     * no other (app_flow.h:218-227). Clearing `q` here therefore covers exactly
     * the two routes Java clears it on.
     *
     * IT IS NOT CLEARED ON ANY OTHER ROUTE, matching the source. Continue, the
     * level-complete CONTINUE and the J != 0 resume all leave `q` alone in
     * Java, and so do they here; bounce_app_flow_enter_menu() does not touch
     * it either, because ShowMainMenu() does not touch `q`.
     *
     * The clear is inside the `if (pending)` sense, not unconditional: the flag
     * is one-shot, so a second consumption cannot re-clear anything, but the
     * order is stated so the correspondence with BounceGame.java:150 -- which is
     * BEFORE the level is loaded -- is not confused with a clear that happens
     * after.
     */
    if (pending)
        flow->new_high_score = false;
    return pending;
}

int bounce_app_flow_level_selection_select(BounceAppFlow *flow)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_LEVEL_SELECTION
        || flow->level_selection.available_level_count == 0u
        || flow->level_selection.selected_index
            >= flow->level_selection.available_level_count)
        return -1;
    /* BounceGame.java:218 calls a(true, selectedIndex + 1). */
    bounce_app_flow_record_start_level(
        flow,
        (int)flow->level_selection.selected_index + 1
    );
    /*
     * STEP 12X-P4-C-D-B-R2: this is the other half of Java's a(true, ...).
     * BounceGame.java:238-239 routes the single New Game menu row through
     * LevelSelect() whenever MaxLevels > 1, and the level list's own SELECT handler
     * at BounceGame.java:218 calls a(true, selectedIndex + 1), which zeroes the
     * score. Level Select is therefore NOT an independent route: it is the same
     * Java operation with a picker in front of it, and it resets the score exactly
     * as the direct branch in bounce_app_flow_menu_select() does.
     *
     * Recorded after record_start_level() for the same ordering reason, and ONLY
     * here: bounce_app_flow_continue_level() is Java's a(false, level) and must
     * never set it.
     */
    flow->menu.new_game_score_reset_pending = true;
    flow->state = BOUNCE_APP_STATE_ACTION_BOUNDARY;
    flow->next_tick_ms = 0;
    return flow->pending_level_id;
}

int bounce_app_flow_begin_gameplay_entry(BounceAppFlow *flow){
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_ACTION_BOUNDARY
        || flow->menu.pending_action != BOUNCE_MENU_ACTION_START_LEVEL
        || flow->pending_level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || flow->pending_level_id > BOUNCE_APP_LAST_LEVEL_ID)
        return -1;

    /*
     * Source-verified metadata only (BounceGame.java:151, e.java:119).
     * BLOCKED: level-load integration intentionally deferred, so
     * LoadLevelId, the f(...) player construction, and StartGameTimer are
     * recorded as not executed and no timer or tick is started here.
     */
    flow->gameplay_entry.level_id = flow->pending_level_id;
    flow->gameplay_entry.initial_score = BOUNCE_APP_ENTRY_INITIAL_SCORE;
    flow->gameplay_entry.initial_lives = BOUNCE_APP_ENTRY_INITIAL_LIVES;
    flow->gameplay_entry.countdown = BOUNCE_APP_ENTRY_COUNTDOWN;
    flow->gameplay_entry.phase =
        BOUNCE_GAMEPLAY_ENTRY_PHASE_INIT_METADATA_RECORDED;
    flow->gameplay_entry.level_loaded = false;
    flow->gameplay_entry.timer_started = false;
    flow->gameplay_entry.player_created = false;
    flow->gameplay_entry.runtime_tiles_created = false;
    flow->gameplay_entry.camera_initialized = false;
    flow->gameplay_entry.gameplay_activated = false;
    flow->state = BOUNCE_APP_STATE_GAMEPLAY_ENTRY;
    flow->next_tick_ms = 0;
    return 0;
}

/*
 * D-03/D-04 -- the J == 2 entry: e.a(level, score, lives), e.java:81-90.
 *
 * JAVA EVIDENCE, the whole method:
 *
 *     public void a(int level, int paramInt2, int paramInt3) {
 *         this.level = level;
 *         this.HoopsScored = 0;
 *         this.lives = paramInt3;
 *         this.score = paramInt2;
 *         this.e = false;
 *         this.TODO_ExitUnlocked = false;
 *         InitializeGame();
 *         this.T = true;
 *     }
 *
 * called from BounceGame.java:231-232 as
 *
 *     this.v.a(this.RecordLevel, this.RecordScore, this.RecordLives);
 *
 * WHICH IS NOT bounce_app_flow_begin_resume_entry(). That function is the J == 1
 * branch, whose target e.a(int,int) at e.java:92-114 restores a POSITION: it
 * calls k() and AddScore(), re-places the ball on its recorded tile and restores
 * the power-ups. This one starts the level from the beginning, so the two differ
 * in what they must carry -- J == 1 needs a position, J == 2 needs a SCORE.
 *
 * Everything else is deliberately identical, and that is the point:
 *
 *   - it records START_LEVEL(level) and reaches stage 1 through the same guard
 *     chain as every other entry, so the staged chain is reused unchanged. This
 *     is the R1-B property: all four producers look the same to stage 1.
 *   - it does NOT set new_game_score_reset_pending. Stage 5 would consume it and
 *     call bounce_game_reset_score() (game.c:785), which is exactly the value
 *     this route exists to deliver. bounce_app_flow_begin_resume_entry() declines
 *     the flag for the same reason, and the comment there applies unchanged.
 *   - initial_lives is re-applied after begin_gameplay_entry(), because that call
 *     writes BOUNCE_APP_ENTRY_INITIAL_LIVES, and stage 5 reads the carrier at
 *     game.c:761.
 *
 * THE SCORE IS CARRIED, NOT APPLIED. initial_score is the flow's record of what
 * the entry was told, exactly as initial_lives is; stage 5 installs lives from
 * its carrier, and the score is installed by the Continue window in
 * app_begin_canonical_gameplay, because that is the point in the staged chain
 * where nothing later can overwrite it.
 */
int bounce_app_flow_begin_record_continue_entry(
    BounceAppFlow *flow,
    int level_id,
    int score,
    int lives
){
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_MENU
        || level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || level_id > BOUNCE_APP_LAST_LEVEL_ID
        || lives < 0
        || score < 0)
        return -1;
    /* The comment block on begin_resume_entry() applies verbatim. */
    bounce_app_flow_record_start_level(flow, level_id);
    flow->state = BOUNCE_APP_STATE_ACTION_BOUNDARY;
    flow->next_tick_ms = 0;
    if (bounce_app_flow_begin_gameplay_entry(flow) != 0)
        return -1;
    /* Re-applied after begin_gameplay_entry(), for the reason given above. */
    flow->gameplay_entry.initial_lives = lives;
    flow->gameplay_entry.initial_score = score;
    return 0;
}

int bounce_app_flow_begin_resume_entry(
    BounceAppFlow *flow,
    int level_id,
    int lives
){
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_MENU
        || level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || level_id > BOUNCE_APP_LAST_LEVEL_ID
        || lives < 0)
        return -1;
    /*
     * The same three writes bounce_app_flow_record_start_level() performs, so a
     * resume reaches stage 1 through the identical guard chain: pending_action
     * must be START_LEVEL and pending_level_id must be in range, or stage 1
     * refuses. Nothing about the entry itself differs from any other producer.
     */
    bounce_app_flow_record_start_level(flow, level_id);
    flow->state = BOUNCE_APP_STATE_ACTION_BOUNDARY;
    flow->next_tick_ms = 0;
    /*
     * new_game_score_reset_pending is deliberately NOT set. It is the New Game
     * (app_flow.c:376) and Level Selection (:909) signal, consumed once at stage 5
     * (game.c:785) where it triggers bounce_game_reset_score(). Java's resume
     * restores score at e.java:96 and nothing later in a(int,int) touches it, so
     * consuming that reset would destroy the restored value. Leaving the flag
     * clear is what makes the S-C placement safe.
     */
    if (bounce_app_flow_begin_gameplay_entry(flow) != 0)
        return -1;
    /*
     * Re-applied AFTER begin_gameplay_entry(), which is the call that writes
     * BOUNCE_APP_ENTRY_INITIAL_LIVES (app_flow.c:931). R1-E decision L-A: the
     * carrier is the single source of lives, and stage 5 reads it at game.c:761,
     * so writing it here keeps the carrier and the live field consistent by
     * construction rather than patching game->lives afterwards.
     */
    flow->gameplay_entry.initial_lives = lives;
    return 0;
}

int bounce_app_flow_record_level_loaded(BounceAppFlow *flow)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_GAMEPLAY_ENTRY
        || flow->menu.pending_action != BOUNCE_MENU_ACTION_START_LEVEL
        || flow->pending_level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || flow->pending_level_id > BOUNCE_APP_LAST_LEVEL_ID
        || flow->gameplay_entry.phase
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_INIT_METADATA_RECORDED)
        return -1;

    /*
     * Native integration seam only. The BounceGame owner performs the actual
     * load through bounce_level_load_file; this records that the step ran and
     * nothing else. No player, timer, tick, camera, or persistence is touched.
     */
    flow->gameplay_entry.phase = BOUNCE_GAMEPLAY_ENTRY_PHASE_LEVEL_LOADED;
    flow->gameplay_entry.level_loaded = true;
    flow->gameplay_entry.timer_started = false;
    flow->gameplay_entry.player_created = false;
    flow->gameplay_entry.runtime_tiles_created = false;
    flow->gameplay_entry.camera_initialized = false;
    flow->gameplay_entry.gameplay_activated = false;
    flow->state = BOUNCE_APP_STATE_LEVEL_LOADED;
    flow->next_tick_ms = 0;
    return 0;
}

int bounce_app_flow_record_player_initialized(BounceAppFlow *flow)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_LEVEL_LOADED
        || !flow->gameplay_entry.level_loaded
        || flow->gameplay_entry.timer_started
        || flow->gameplay_entry.player_created
        || flow->gameplay_entry.phase
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_LEVEL_LOADED)
        return -1;

    /*
     * Native integration seam only. The BounceGame owner performs the actual
     * derivation and installation; this records that it completed. The coarse
     * application state is unchanged because no screen, input route, camera,
     * timer, or gameplay work belongs to this step.
     */
    flow->gameplay_entry.phase =
        BOUNCE_GAMEPLAY_ENTRY_PHASE_PLAYER_INITIALIZED;
    flow->gameplay_entry.level_loaded = true;
    flow->gameplay_entry.player_created = true;
    flow->gameplay_entry.timer_started = false;
    flow->gameplay_entry.runtime_tiles_created = false;
    flow->next_tick_ms = 0;
    return 0;
}

int bounce_app_flow_record_runtime_initialized(BounceAppFlow *flow)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_LEVEL_LOADED
        || !flow->gameplay_entry.level_loaded
        || flow->gameplay_entry.timer_started
        || !flow->gameplay_entry.player_created
        || flow->gameplay_entry.runtime_tiles_created
        || flow->gameplay_entry.phase
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_PLAYER_INITIALIZED)
        return -1;

    /*
     * Native implementation seam only; no recovered Java step corresponds to a
     * separate runtime object. The BounceGame owner performs the actual copy and
     * dynamic-thorn initialization; this records that it completed. The coarse
     * application state stays LEVEL_LOADED because no screen, input route,
     * camera, timer, tick, or gameplay transition belongs to this step.
     */
    flow->gameplay_entry.phase =
        BOUNCE_GAMEPLAY_ENTRY_PHASE_RUNTIME_INITIALIZED;
    flow->gameplay_entry.level_loaded = true;
    flow->gameplay_entry.player_created = true;
    flow->gameplay_entry.runtime_tiles_created = true;
    flow->gameplay_entry.camera_initialized = false;
    flow->gameplay_entry.timer_started = false;
    flow->next_tick_ms = 0;
    return 0;
}

int bounce_app_flow_record_camera_initialized(BounceAppFlow *flow)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_LEVEL_LOADED
        || !flow->gameplay_entry.level_loaded
        || flow->gameplay_entry.timer_started
        || !flow->gameplay_entry.player_created
        || !flow->gameplay_entry.runtime_tiles_created
        || flow->gameplay_entry.camera_initialized
        || flow->gameplay_entry.phase
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_RUNTIME_INITIALIZED)
        return -1;

    /*
     * Records that the already-verified camera initialization ran; it performs
     * no camera work itself. The coarse application state stays LEVEL_LOADED
     * because no screen, input route, timer, tick, repaint, or gameplay
     * transition belongs to this step.
     */
    flow->gameplay_entry.phase = BOUNCE_GAMEPLAY_ENTRY_PHASE_CAMERA_INITIALIZED;
    flow->gameplay_entry.level_loaded = true;
    flow->gameplay_entry.player_created = true;
    flow->gameplay_entry.runtime_tiles_created = true;
    flow->gameplay_entry.camera_initialized = true;
    flow->gameplay_entry.gameplay_activated = false;
    flow->gameplay_entry.timer_started = false;
    flow->next_tick_ms = 0;
    return 0;
}

int bounce_app_flow_record_gameplay_activated(BounceAppFlow *flow)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_LEVEL_LOADED
        || !flow->gameplay_entry.level_loaded
        || flow->gameplay_entry.timer_started
        || !flow->gameplay_entry.player_created
        || !flow->gameplay_entry.runtime_tiles_created
        || !flow->gameplay_entry.camera_initialized
        || flow->gameplay_entry.gameplay_activated
        || flow->gameplay_entry.phase
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_CAMERA_INITIALIZED)
        return -1;

    /*
     * Native stand-in for BounceGame.java:155 `this.display.setCurrent(this.v)`:
     * the gameplay surface becomes current. The original has no activation
     * state, so this coarse transition is a portability boundary, not a
     * recovered Java symbol.
     *
     * No timer is started. StartGameTimer() (BounceGame.java:153) is already
     * running in the original from the e constructor (e.java:78), but native
     * timer integration is a separate, later milestone, so next_tick_ms stays 0
     * and timer_started stays false. No tick, movement, collision, dyn-thorn
     * update, renderer, repaint, audio, persistence, or input work is performed.
     */
    flow->gameplay_entry.phase = BOUNCE_GAMEPLAY_ENTRY_PHASE_ACTIVATED;
    flow->gameplay_entry.level_loaded = true;
    flow->gameplay_entry.player_created = true;
    flow->gameplay_entry.runtime_tiles_created = true;
    flow->gameplay_entry.camera_initialized = true;
    flow->gameplay_entry.gameplay_activated = true;
    flow->gameplay_entry.timer_started = false;
    flow->next_tick_ms = 0;
    flow->state = BOUNCE_APP_STATE_GAMEPLAY;
    return 0;
}

int bounce_app_flow_record_timer_initialized(BounceAppFlow *flow)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_GAMEPLAY
        || !flow->gameplay_entry.level_loaded
        || flow->gameplay_entry.timer_started
        || !flow->gameplay_entry.player_created
        || !flow->gameplay_entry.runtime_tiles_created
        || !flow->gameplay_entry.camera_initialized
        || !flow->gameplay_entry.gameplay_activated
        || flow->gameplay_entry.phase
            != BOUNCE_GAMEPLAY_ENTRY_PHASE_ACTIVATED)
        return -1;

    /*
     * Records the StartGameTimer() boundary only. The nominal 40 ms period is
     * owned by the platform-independent BounceGameTimer; this record keeps no
     * deadline, so there is no accumulator and no catch-up model. The coarse
     * state stays GAMEPLAY and no Tick body or callback runs here.
     */
    flow->gameplay_entry.phase =
        BOUNCE_GAMEPLAY_ENTRY_PHASE_TIMER_INITIALIZED;
    flow->gameplay_entry.level_loaded = true;
    flow->gameplay_entry.player_created = true;
    flow->gameplay_entry.runtime_tiles_created = true;
    flow->gameplay_entry.camera_initialized = true;
    flow->gameplay_entry.gameplay_activated = true;
    flow->gameplay_entry.timer_started = true;
    flow->next_tick_ms = 0;
    return 0;
}

int bounce_app_flow_tick_pre_countdown(BounceAppFlow *flow)
{
    if (flow == NULL)
        return -1;

    /*
     * e.java:258-259, transcribed exactly, including the saturation at 0. This
     * runs only after the reset (e.java:222) and splash (e.java:227) gates have
     * both returned early, matching the source ordering.
     */
    if (flow->gameplay_entry.countdown != 0)
        --flow->gameplay_entry.countdown;
    return flow->gameplay_entry.countdown;
}

int bounce_app_flow_entry_level_id(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0;
    return flow->gameplay_entry.level_id;
}

int bounce_app_flow_entry_initial_score(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0;
    return flow->gameplay_entry.initial_score;
}

int bounce_app_flow_entry_initial_lives(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0;
    return flow->gameplay_entry.initial_lives;
}

/*
 * STEP P6 -- the write side of the carrier read above. See app_flow.h for the full
 * justification; the short form is that Java's post-completion CONTINUE performs no
 * `lives` write anywhere, so the remaining lives must reach the next level through
 * this existing field rather than through the constant that
 * bounce_app_flow_begin_gameplay_entry() records.
 *
 * The GAMEPLAY_ENTRY guard is the state both existing writers are already in:
 * bounce_app_flow_begin_resume_entry() writes immediately after
 * bounce_app_flow_begin_gameplay_entry() (app_flow.c:993-1002), and
 * bounce_game_continue_level() calls this immediately after the same call
 * (game.c:1473). Requiring it means the carrier can only be corrected for an entry
 * that is genuinely staged and waiting to be read at game.c:761, never scribbled on
 * from an arbitrary state.
 */
int bounce_app_flow_set_entry_initial_lives(BounceAppFlow *flow, int lives)
{
    if (flow == NULL
        || flow->state != BOUNCE_APP_STATE_GAMEPLAY_ENTRY)
        return -1;
    flow->gameplay_entry.initial_lives = lives;
    return 0;
}

/*
 * D-13 -- ARM THE LEVEL-START COUNTDOWN ON A LEVEL LOAD.
 *
 * JAVA EVIDENCE. `p` is armed in exactly one place, InitializeGame()
 * (e.java:115-124):
 *
 *     RunGarbageCollector();
 *     LoadLevelId(this.level);
 *     this.HoopsScored = 0;
 *     this.p = 120;                                    <-- e.java:119
 *     this.y = true;
 *     a(initX*12+6, initY*12+6, this.BallSize, 0, 0);
 *     this.aq.a(initX, initY);
 *     this.T = true;
 *
 * and InitializeGame() is reached from the top of Tick() (e.java:222-226), so
 * EVERY level start re-arms it: the first tick of a level, and the tick after
 * an af-gated level skip (e.java:340/:345 set `d`, which routes here).
 *
 * bounce_app_flow_begin_gameplay_entry() above already records the constant for
 * the staged New Game / Level Select / Continue entry, which is the only route
 * that goes through it. This function exists for the two routes that load a
 * level WITHOUT passing through the gameplay-entry metadata step:
 *
 *   - the af level skip (vertical_slice.c's RESET_GATE arm), which calls
 *     bounce_game_load_level() directly;
 *   - the legacy bounce_game_start_level() diagnostic route, used by --check
 *     and by verify_render_continuation().
 *
 * It is deliberately NOT gated on any display state, unlike
 * bounce_app_flow_set_entry_initial_lives() above. A level load is the trigger
 * in the source, not an entry state: the af skip runs while the flow is
 * GAMEPLAY, not GAMEPLAY_ENTRY, so a state gate would refuse exactly the case
 * that needs it. It cannot fail for a non-NULL flow, which is why its callers
 * may discard the result.
 */
int bounce_app_flow_arm_entry_countdown(BounceAppFlow *flow)
{
    if (flow == NULL)
        return -1;
    flow->gameplay_entry.countdown = BOUNCE_APP_ENTRY_COUNTDOWN;
    return 0;
}

int bounce_app_flow_entry_countdown(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0;
    return flow->gameplay_entry.countdown;
}

BounceGameplayEntryPhase bounce_app_flow_entry_phase(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return BOUNCE_GAMEPLAY_ENTRY_PHASE_NONE;
    return flow->gameplay_entry.phase;
}

bool bounce_app_flow_entry_step_executed(
    const BounceAppFlow *flow,
    bool level_loaded,
    bool timer_started,
    bool player_created
)
{
    if (flow == NULL)
        return false;
    return flow->gameplay_entry.level_loaded == level_loaded
        && flow->gameplay_entry.timer_started == timer_started
        && flow->gameplay_entry.player_created == player_created;
}

const char *bounce_app_flow_entry_phase_name(
    BounceGameplayEntryPhase phase
)
{
    switch (phase) {
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_NONE:
            return "NONE";
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_START_LEVEL_RECORDED:
            return "START_LEVEL_RECORDED";
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_INIT_METADATA_RECORDED:
            return "INIT_METADATA_RECORDED";
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_LEVEL_LOADED:
            return "LEVEL_LOADED";
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_PLAYER_INITIALIZED:
            return "PLAYER_INITIALIZED";
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_RUNTIME_INITIALIZED:
            return "RUNTIME_INITIALIZED";
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_CAMERA_INITIALIZED:
            return "CAMERA_INITIALIZED";
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_ACTIVATED:
            return "ACTIVATED";
        case BOUNCE_GAMEPLAY_ENTRY_PHASE_TIMER_INITIALIZED:
            return "TIMER_INITIALIZED";
    }
    return "UNKNOWN";
}

bool bounce_app_flow_entry_runtime_tiles_created(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return false;
    return flow->gameplay_entry.runtime_tiles_created;
}

bool bounce_app_flow_entry_camera_initialized(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return false;
    return flow->gameplay_entry.camera_initialized;
}

bool bounce_app_flow_entry_gameplay_activated(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return false;
    return flow->gameplay_entry.gameplay_activated;
}

void bounce_app_flow_menu_clear_action(BounceAppFlow *flow)
{
    if (flow == NULL || flow->state != BOUNCE_APP_STATE_MENU)
        return;
    flow->menu.pending_action = BOUNCE_MENU_ACTION_NONE;
}

int bounce_app_flow_menu_set_continue_available(
    BounceAppFlow *flow,
    bool available
)
{
    unsigned int count;

    if (flow == NULL || flow->state != BOUNCE_APP_STATE_MENU)
        return -1;
    flow->menu.continue_available = available;
    count = available ? BOUNCE_MENU_ROW_COUNT : BOUNCE_MENU_ROW_COUNT - 1u;
    flow->menu.enabled_item_count = count;
    /*
     * D-05 -- the :127 half of the rule, for the arrivals where availability is
     * established AFTER enter_menu() has already run.
     *
     * BounceGame.java:116 and :126 use one expression, so establishing the row
     * also fixes the highlight to 0. That is exactly what happens on the two
     * production paths that re-derive availability later: the first menu arrival
     * (bounce_app_flow_tick() reporting the splash->menu transition, after which
     * app_refresh_continue_availability() runs) and the GAME_OVER arrival (after
     * the resume capture). In Java both of those are ShowMainMenu() calls that
     * read K/J live, so both land on the :127 branch. Doing it here keeps the two
     * native paths in step with the one Java rule.
     *
     * The clamp below is pre-existing and is left in place; `q`-free sessions
     * that drop the Continue row shrink the enabled list, and a remembered N can
     * now point past its end.
     */
    if (available)
        flow->menu.selected_index = 0u;
    if (flow->menu.selected_index >= count)
        flow->menu.selected_index = count - 1u;
    flow->menu.pending_action = BOUNCE_MENU_ACTION_NONE;
    return 0;
}

/*
 * STEP 37 PERSIST -- Java BounceGame.java:422-426:
 *
 *   if (this.v.score > this.HighScore) {
 *       this.HighScore = this.v.score;
 *       this.q = true;
 *       WriteToStore(2);
 *   }
 *
 * The comparison is strict, so a tie leaves the record alone, and the record
 * never decreases. Java writes the record inside the branch; returning 1 here
 * is what lets the caller do the same, so nothing is written when it did not
 * change.
 *
 * `q` is the HUD dirty flag Java sets on the same line. It has no native
 * counterpart (see app_kill_ball's note at vertical_slice.c:1947) and nothing
 * native reads it, so it is not reproduced -- exactly as for KillBall's
 * `n.y = true`.
 */
int bounce_app_flow_note_score(BounceAppFlow *flow, int score)
{
    if (flow == NULL)
        return -1;
    if (score > flow->high_score) {
        flow->high_score = score;
        /*
         * D-07 -- `this.q = true;` at BounceGame.java:424, the assignment that
         * sits between the high-score update and WriteToStore(2) in the same
         * branch. The whole rule is transcribed at app_flow.h:377-400 and the
         * three call sites -- game over (e.java:269), level complete (e.java:319)
         * and the W1 completion consumer -- are exactly Java's three
         * WriteToStore() no-argument sites.
         */
        flow->new_high_score = true;
        return 1;
    }
    return 0;
}

/*
 * D-07 -- the reader for BounceGame.q, which BounceGame.java:188-190 uses to
 * decide whether BOTH terminal forms append Translation.NEW_HIGH_SCORE.
 * Returns false for a NULL flow, so a caller with no shell attached behaves as
 * "no new record", which is the fresh-launch value anyway (app_flow.h:293-305).
 */
bool bounce_app_flow_new_high_score(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return false;
    return flow->new_high_score;
}

int bounce_app_flow_high_score(const BounceAppFlow *flow)
{
    if (flow == NULL)
        return 0;
    return flow->high_score;
}

/*
 * STEP 37 PERSIST -- adopt the values persistence.c read at startup.
 *
 * WHY THIS DOES NOT CALL bounce_app_flow_set_available_level_count()
 *   That setter requires BOUNCE_APP_STATE_MENU (:692-694), because it is a menu
 *   navigation entry point. A store load is not navigation: the live path calls
 *   it from main() while the shell is still on its splash screen, exactly where
 *   Java calls LoadRecords() (BounceGame.java:95, in the constructor). Going
 *   through the setter there would silently drop the level count.
 *
 *   So this assigns the field directly, which is what
 *   bounce_app_flow_complete_level() already does for the same reason (:744-748).
 *   The two bounds are kept identical:
 *     - a count above BOUNCE_APP_LAST_LEVEL_ID is refused, so a hand-edited or
 *       corrupt store cannot unlock a level that does not exist. That is the
 *       setter's rule; complete_level() clamps instead, because it is writing
 *       a value it just computed rather than trusting an external one.
 *     - selected_index is reset to 0, which is what the setter does, so the
 *       level list opens on the first entry rather than on a stale highlight.
 *
 * high_score only ever moves up, for the same reason
 * bounce_app_flow_note_score() does: this runs after init, where the field is
 * already 0, so "up" simply means "take the stored value".
 */
int bounce_app_flow_restore_persisted_theme(
    BounceAppFlow *flow,
    int theme_index
)
{
    /*
     * TUGAS 2b. See app_flow.h for why this is not a parameter of
     * bounce_app_flow_restore_persisted().
     *
     * The bound is BOUNCE_UI_THEME_COUNT, the same constant
     * bounce_app_flow_settings_list_count() returns for the Theme page, so a
     * value that the Settings screen could never have produced is also a value
     * this refuses. The value is IGNORED rather than rejected: a saved setting
     * must never be able to leave the application with no usable palette, so an
     * out-of-range file falls back to the default profile exactly as an absent
     * one does.
     */
    if (flow == NULL)
        return -1;
    if (theme_index < 0
        || (unsigned int)theme_index >= BOUNCE_UI_THEME_COUNT)
        return 0;
    flow->settings.applied_theme_index = (unsigned int)theme_index;
    flow->settings.theme_index = (unsigned int)theme_index;
    return 0;
}

int bounce_app_flow_restore_persisted(
    BounceAppFlow *flow,
    int max_levels,
    int high_score
)
{
    if (flow == NULL)
        return -1;
    if (max_levels > 0) {
        if (max_levels > BOUNCE_APP_LAST_LEVEL_ID)
            return -1;
        flow->level_selection.available_level_count = (unsigned int)max_levels;
        flow->level_selection.selected_index = 0u;
    }
    if (high_score > flow->high_score)
        flow->high_score = high_score;
    return 0;
}
