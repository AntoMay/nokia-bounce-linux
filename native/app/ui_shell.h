#ifndef BOUNCE_NATIVE_UI_SHELL_H
#define BOUNCE_NATIVE_UI_SHELL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "app_flow.h"
#include "../renderer/renderer.h"
#include "../renderer/surface.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct BounceVisualAssets BounceVisualAssets;

/*
 * NATIVE BOUNDARY: the UI color palette.
 *
 * AUDIT RESULT: no theme, palette, or style structure existed before this
 * point. Every UI color was a function-local `const uint32_t` literal inside
 * ui_shell.c, and the renderer takes the color as a per-call argument
 * (bounce_renderer_fill_rect(renderer, x, y, w, h, color)). That makes this file
 * the single, safe seam: a palette can be introduced here and switched at
 * runtime without touching the renderer primitives, the gameplay scene, the
 * level renderer, the camera, or any core system.
 *
 * Gameplay and level colors live in vertical_slice.c, outside this struct, so a
 * profile cannot leak into the game view.
 */
typedef struct BounceUiPalette {
    uint32_t background; /* page fill */
    uint32_t panel;      /* inner card */
    uint32_t border;     /* card outline */
    uint32_t highlight;  /* selected row fill; authoritative selected_background */
    /*
     * NATIVE BOUNDARY: a dedicated selected-row text role.
     *
     * The four AUTHORITATIVE Theme roles are background, text,
     * selected_background and selected_text. selected_background is the
     * `highlight` field above; selected_text is this field. A selected row is
     * drawn as highlight fill + selected_text glyphs, independently of `text`,
     * which is what lets an inverted profile (bright fill, dark glyphs) read
     * correctly. The Default profile sets selected_text equal to text, so its
     * selected row is drawn exactly as it was before this role existed.
     */
    uint32_t selected_text;
    uint32_t text;       /* primary text and titles */
    uint32_t body;       /* secondary content rows */
    uint32_t muted;      /* de-emphasized labels */
    uint32_t accent;     /* back hints and notices */
    uint32_t disabled;   /* rows that exist but are not selectable */
} BounceUiPalette;

/*
 * Runtime-only UI color profiles. No persistence: a fresh launch is always
 * index 0. There is deliberately no save, load, RMS, registry, environment or
 * external theme file path.
 *
 * Count is 13, not the 15 the milestone targets. Thirteen profiles are fully
 * specified and implemented: Default, Green LCD, Sepia, and the ten
 * additional themes. The final two target profiles have no established name or
 * palette anywhere in this repository, in reverse/, in the reflog or on any
 * branch, so they were NOT invented. See the milestone report.
 */
#define BOUNCE_UI_COLOR_PROFILE_COUNT 13u

/*
 * Select the active UI palette. Out-of-range values are ignored, so a bad index
 * can never blank the UI. There is deliberately no save or load path.
 */
void bounce_ui_shell_set_color_profile(unsigned int index);
unsigned int bounce_ui_shell_active_color_profile(void);
const BounceUiPalette *bounce_ui_shell_palette(void);

/*
 * V-2 -- Can the 5x7 UI font draw this ASCII character?
 *
 * Returns non-zero for a character the renderer paints with a real glyph, and
 * zero for one it would paint with placeholder_unknown_rows, the box. A space is
 * drawable and returns non-zero: it draws placeholder_space_rows, which is a
 * real (blank) cell rather than a box. a-z answer as their A-Z form because
 * placeholder_text_index() folds case.
 *
 * This exists so the coverage check can be exhaustive over every character of
 * every shipped resource string. It is a read-only query over a static table and
 * has no effect on rendering; it does not bypass, replace or weaken any draw
 * path.
 */
int bounce_ui_shell_can_draw_ascii(int codepoint);

/*
 * V-2 -- The number of unknown-glyph boxes this shell has painted, as a flag.
 *
 * Non-zero means at least one box has been drawn since the last reset. It is
 * bumped at the single site that chooses placeholder_unknown_rows, so it reports
 * what was PAINTED rather than what a string contains. Reading and resetting it
 * change no rendering decision; nothing in the draw path consults it.
 */
int bounce_ui_shell_unknown_glyph_draws(void);
void bounce_ui_shell_reset_unknown_glyph_draws(void);

/*
 * INSTRUCTIONS SCROLL GEOMETRY.
 *
 * The body starts at y=21 and ESC BACK is drawn at y=118, and the line pitch is
 * always BOUNCE_SCRIPT_CELL_HEIGHT, so eight lines is what clears the label. The
 * scrollbar lane is the two rightmost pixel columns, which is why the body
 * measure is 124 and not 126. Exposed so a verifier can find the lane without
 * repeating these numbers and drifting from them.
 */
#define BOUNCE_UI_INSTRUCTIONS_VISIBLE_LINES 8u
#define BOUNCE_UI_INSTRUCTIONS_SCROLL_X 126
#define BOUNCE_UI_INSTRUCTIONS_SCROLL_Y0 21
#define BOUNCE_UI_INSTRUCTIONS_SCROLL_H 77

/* Lines the active language's Instructions text needs, and the largest legal
 * scroll offset for it. Both come from the real wrapper at the real measure. */
unsigned int bounce_ui_shell_instructions_total_lines(void);
unsigned int bounce_ui_shell_instructions_max_scroll(void);

/* The scrollbar the Theme page and the Instructions page share. A test seam --
 * see the definition for why nothing in the shell needs arbitrary numbers. */
int bounce_ui_shell_draw_scrollbar_for_test(
    BounceRenderer *renderer,
    int x,
    int track_y,
    int track_h,
    unsigned int count,
    unsigned int visible,
    unsigned int first);

typedef enum BounceUiInput {
    BOUNCE_UI_INPUT_OTHER = 0,
    BOUNCE_UI_INPUT_UP = 1,
    BOUNCE_UI_INPUT_DOWN = 2,
    BOUNCE_UI_INPUT_SELECT = 3,
    BOUNCE_UI_INPUT_EXIT = 4,
    BOUNCE_UI_INPUT_BACK = 5
} BounceUiInput;

/*
 * NATIVE EXTENSION: turn a mouse click at a logical 128x128 surface coordinate
 * into the same BounceUiInput the keyboard produces, then apply it through the
 * existing bounce_ui_shell_handle_press(). There is deliberately no second UI
 * path: a click is translated into a UI edge and the ordinary handler runs, so
 * mouse and keyboard cannot diverge and cannot both fire for one gesture.
 *
 * The row geometry mirrors the renderers exactly. Rows use the "select, then
 * select again to activate" rule so a single click can never launch a level by
 * accident. Clicks outside any row are ignored.
 *
 * Returns 1 when the click produced a UI edge, 0 when it was ignored.
 */
int bounce_ui_shell_handle_click(
    BounceAppFlow *flow,
    int x,
    int y
);

/*
 * NATIVE EXTENSION: apply a numeric key to the current UI.
 *
 * The digit keys are already produced by x11_keysym_to_source() as
 * BOUNCE_INPUT_SOURCE_NUM0..NUM9, so this only decides what they mean.
 *
 * Supported: in LEVEL SELECT, digits 1..9 select levels 1..9 and digit 0 selects
 * level 10, which is the conventional numeric position and is documented here
 * rather than being an undiscoverable convention. Level 11 has NO single-key
 * mapping: inventing one would need a hidden second keystroke, so it is reached
 * with the arrow keys or the mouse instead. This is a limitation of the design,
 * not an oversight.
 *
 * The digit only SELECTS. Starting the level still needs the ordinary SELECT
 * action, so a stray digit can never launch a level by accident.
 *
 * Returns 1 when the digit was consumed, 0 when it is not meaningful here.
 */
int bounce_ui_shell_handle_number(
    BounceAppFlow *flow,
    unsigned int digit
);

/* Apply one already-collected UI edge; no gameplay or persistence action runs. */
int bounce_ui_shell_handle_press(
    BounceAppFlow *flow,
    BounceUiInput input
);

/* Temporary native labels; this is not a localization implementation. */
const char *bounce_ui_shell_menu_label(unsigned int row);
const char *bounce_ui_shell_action_label(BounceMenuAction action);
/* Complete source-verified English fallback; renderer uses a bounded prefix. */
const char *bounce_ui_shell_instructions_text(void);
/* The bounded excerpt of the Instructions page, in the active language. */
const char *bounce_ui_shell_instructions_prefix(void);

/*
 * D-15 / J-04 -- the Instructions text WITH the three control placeholders
 * substituted, i.e. what the page actually draws.
 *
 * The raw resource keeps its "%0U"/"%1U"/"%2U" -- verify_ui_shell asserts that,
 * because it is the correct statement about the SHIPPED FILE -- and
 * bounce_ui_shell_instructions_text()/..._prefix() keep returning it verbatim.
 * This is the render-time composition, and it exists publicly so the verifier
 * can assert the substitution instead of only inferring it from pixels.
 *
 * %0U is the LEFT binding, %1U the FIRE binding, %2U the UP binding; see the
 * ui_compose_instructions() comment in ui_shell.c for the Java citations and
 * for why the original DEVICE key names are UNKNOWN and are not guessed here.
 *
 * Returns 0, or -1 for a NULL/too-small buffer. It refuses rather than
 * truncating.
 */
int bounce_ui_shell_instructions_composed(
    char *out,
    size_t out_size
);
/* Source-backed default only; no native persistent score is connected. */
const char *bounce_ui_shell_high_score_value(void);
/*
 * D-23 -- always NULL. BounceGame.java:159-165 gives the High Score form exactly
 * two Items (the title and String.valueOf(HighScore)) plus the Back command, so
 * there is no notice to draw. The accessor is retained so a caller that used to
 * draw one has a single, documented place to find out there is none.
 */
const char *bounce_ui_shell_high_score_notice(void);

/* Render the source-backed 128x128 splash through the existing asset boundary. */
int bounce_ui_shell_render_splash(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceVisualAssets *assets
);

/* Render the provisional native menu shell; geometry is not original MIDP geometry. */
int bounce_ui_shell_render_menu(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
);

/* Render only the explicit native boundary; no destination content. */
int bounce_ui_shell_render_action_boundary(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
);

/* Render the bounded source-backed Instructions destination. */
int bounce_ui_shell_render_instructions(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
);

/* Render the bounded source-backed High Score destination. */
int bounce_ui_shell_render_high_score(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
);

/* Render the bounded source-shaped level List. */
/*
 * NATIVE EXTENSION: render the Settings destination and its read-only pages.
 * The recovered source has no Settings item; this screen is a native addition.
 */
int bounce_ui_shell_render_settings(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
);

/*
 * STEP 38-RESET-UI -- THE Y OF EACH ROW ON THE TWO SETTINGS SCREENS THAT HAVE ONE.
 *
 * Exposed so that a verifier can ask where a row is DRAWN and click exactly there,
 * instead of keeping a private copy of the geometry. That private copy is the defect
 * this pair exists to prevent: bounce_ui_shell_handle_click() had its own table, five
 * entries long at a different pitch from the one the renderer used, so the sixth root
 * row -- the Reset row -- had a hit band at y = 0 and could not be clicked at all.
 *
 * Both return -1 for an out-of-range index rather than a number, so a caller that loops
 * wrong reads a failure instead of silently probing y = 0, which is a real coordinate.
 */
int bounce_ui_shell_settings_root_row_y(unsigned int index);
int bounce_ui_shell_reset_row_y(unsigned int index);

/*
 * NATIVE EXTENSION: render the About destination. It shows only facts already
 * established in this repository and deliberately contains no project URL,
 * author, contact, or social line, because none is established anywhere.
 */
int bounce_ui_shell_render_about(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
);

/*
 * STEP 12X-P4-C-B: render the Game End destination, the static structural shell
 * of BounceGame.ShowGameEnd(true) (BounceGame.java:177-197) for the q == false
 * variant.
 *
 * It draws the Form title, the win body, a SEPARATE reserved spacer band, a
 * SEPARATE reserved score band, and the OK affordance. Nothing about the score
 * or the high score is drawn, read or implied: both belong to the score system,
 * which is not yet reconciled. The reserved bands draw NO pixels, so no number
 * and no placeholder that could be mistaken for one can appear.
 *
 * The renderer lives here rather than in vertical_slice.c because tr() and
 * draw_centered_placeholder_text() are file-static in this translation unit,
 * which is where every other destination renderer already lives.
 */
int bounce_ui_shell_render_game_end(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow,
    int32_t score,
    bool new_high_score
);

/*
 * STEP 13G-A -- the UI 4 destination, the Level Complete Form.
 *
 * It is the ONLY destination whose source Form has an EMPTY title: Java builds
 * `new Form("")` at BounceGame.java:205, so no title and no title rule are drawn
 * here. The body reproduces the two appends of :206 and :208 in order -- the
 * LevelCompletedString, then one blank row, then the score -- and the single
 * CONTINUE affordance of :204/:209 goes on the shared bottom row at y=115, the
 * same row every other destination uses.
 *
 * The two values are passed explicitly rather than as a BounceGame *, for two
 * reasons. This header deliberately does not include game.h, so the dependency
 * surface is unchanged; and naming the parameters is the enforcement of the
 * completed-level proof. `completed_level_id` must be the field that still
 * holds the LOADED level, which at this point in the lifecycle is the level that
 * was just completed: bounce_app_flow_complete_level() advances only flow->level_id
 * (app_flow.c:697) and nothing in the completion block writes game->level_id.
 * `score` is the live total and ALREADY includes the +5000 awarded at game.c:1350,
 * so this renderer must not add anything to it.
 *
 * Neither value is invented, defaulted or clamped: a negative completed_level_id
 * or score is drawn as given, exactly as the source would print it.
 */
int bounce_ui_shell_render_level_complete(
    BounceSurface *surface,
    BounceRenderer *renderer,
    int completed_level_id,
    int32_t score
);

/*
 * D-06 -- the Level List title, i.e. Translation.NEW_GAME (id 11).
 * BounceGame.java:142 passes it as the first MIDP List constructor argument, and
 * the same row is the main menu's second entry, so the verifier compares this
 * against bounce_ui_shell_menu_label(1u). Returns the active language's row.
 */
const char *bounce_ui_shell_level_selection_title(void);

/* Render the bounded source-shaped level List. */
int bounce_ui_shell_render_level_selection(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
);

/*
 * D-13 / J-01 -- the level-start banner, e.java:201-205:
 *
 *     if (this.p != 0) {
 *         this.X.setColor(0xfffffe);
 *         this.X.setFont(this.font);
 *         this.X.drawString(this.LevelString, 44, 84, 20);
 *     }
 *
 * WHY THE COLOUR IS A PARAMETER AND NOT A PALETTE ROLE. This draws into the
 * GAMEPLAY scene, not into a form. ui_shell.h:26-29 records that gameplay and
 * level colours live in vertical_slice.c precisely so a theme profile cannot
 * leak into the game view, and the Gameplay HUD already works the same way:
 * draw_hud() in vertical_slice.c passes COLOR_HUD_SCORE in as an argument. This
 * renderer therefore takes the colour from its caller and only writes pixels.
 * The value passed by draw_level_banner() is COLOR_HUD_SCORE, which is
 * TOKEN(255,255,254) == the source's own 0xfffffe literal, because e.java:202
 * sets exactly that colour for the score string at e.java:183.
 *
 * WHY IT LIVES IN ui_shell.c ANYWAY. ui_shell.c owns the only letter-capable
 * text renderer in the tree (draw_placeholder_text over the 26-letter 5x7
 * table) and the only integer renderer (draw_native_number); placeholder_text_index()
 * returns -1 for a digit, which is exactly why the level List and the High
 * Score screen already draw a label and a number as two calls. Reproducing
 * that pair here rather than in vertical_slice.c avoids a second copy of the
 * font, and the localized label (tr(BOUNCE_LOCALE_LEVEL)) is reachable from
 * here only, since tr() is file-static.
 *
 * level_id is the LOADED level, which is the value Java's LevelString was built
 * from: b.java:216-217 formats Translation.LEVEL with Integer.valueOf(this.level)
 * at LoadLevelId time, and this.level is only advanced later, in the completion
 * block at e.java:317. So both name the level that is on screen.
 *
 * Returns 0 on success, -1 for a NULL renderer or an out-of-range level id.
 */
int bounce_ui_shell_render_level_banner(
    BounceRenderer *renderer,
    int level_id,
    uint32_t color
);

/*
 * Render the Game Over destination, BounceGame.java:177-195 (ShowGameOver).
 *
 * This is the LOSS destination, reached from app_flow.c:164 when the run ends with
 * no lives left. It is distinct from BOUNCE_APP_STATE_GAME_END, which is the win
 * destination and is rendered by bounce_ui_shell_render_game_end().
 *
 * FOUR ITEMS ARE DRAWN, each traced to the source:
 *
 *   title    BounceGame.java:181  Form(Translation.GAME_OVER)
 *   body     BounceGame.java:186  the not-won arm appends
 *                                 Translation.GAME_OVER -- the SAME translation as
 *                                 the title, which is why this renderer passes
 *                                 BOUNCE_LOCALE_GAME_END_TITLE twice rather than
 *                                 inventing a key for the body.
 *   score    BounceGame.java:192  String.valueOf(this.E), and BounceGame.java:427
 *                                 establishes this.E = this.v.score. `score` is
 *                                 therefore the live total AS IS: this renderer
 *                                 adds nothing, subtracts nothing and recomputes
 *                                 nothing. A raw integer, no zero padding and no
 *                                 separator, exactly as String.valueOf prints it.
 *   OK       BounceGame.java:180/:193  the single Command.OK, on the same bottom
 *                                 row every other destination uses.
 *
 * THE NEW-HIGH-SCORE LINE IS DELIBERATELY NOT DRAWN. BounceGame.java:188-190 adds
 * Translation.NEW_HIGH_SCORE when this.q is set, but this build has no reconciled
 * native owner for that value: locale.h:170-173 records that there is deliberately
 * no key for it, on the grounds that adding one "would invite rendering a value
 * this build cannot honestly produce". That reasoning is unchanged, so the line is
 * left out rather than filled with a fabricated number. See the SCORE DISPLAY
 * DEFERRED note in ui_shell.c.
 *
 * LAYOUT follows the established native Form conventions -- background, panel,
 * four borders, title at y=9 with the y=19 rule, body, then the score, then OK on
 * the shared y=115 row -- as used by bounce_ui_shell_render_game_end(). Original
 * MIDP Form metrics are NOT claimed: those are device-owned (Master Index U-05,
 * U-07). No value here is invented, defaulted or clamped.
 */
int bounce_ui_shell_render_game_over(
    BounceSurface *surface,
    BounceRenderer *renderer,
    int32_t score,
    bool new_high_score
);

/*
 * Render the UI 6 gameplay-entry boundary. The screen displays recorded
 * source-verified metadata only; it is not gameplay and not a native
 * approximation of the original canvas.
 */
int bounce_ui_shell_render_gameplay_entry(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_UI_SHELL_H */
