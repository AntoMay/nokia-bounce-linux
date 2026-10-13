#ifndef BOUNCE_NATIVE_APP_INPUT_STATE_H
#define BOUNCE_NATIVE_APP_INPUT_STATE_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * App-local canonical input boundary. The source does not establish the
 * original Nokia physical-key mapping for the development aliases below.
 */
typedef enum BounceInputSource {
    BOUNCE_INPUT_SOURCE_LEFT_ARROW = 0,
    BOUNCE_INPUT_SOURCE_LEFT_A = 1,
    BOUNCE_INPUT_SOURCE_RIGHT_ARROW = 2,
    BOUNCE_INPUT_SOURCE_RIGHT_D = 3,
    BOUNCE_INPUT_SOURCE_UP_ARROW = 4,
    BOUNCE_INPUT_SOURCE_UP_W = 5,
    BOUNCE_INPUT_SOURCE_DOWN_ARROW = 6,
    BOUNCE_INPUT_SOURCE_DOWN_S = 7,
    BOUNCE_INPUT_SOURCE_NUM0 = 8,
    BOUNCE_INPUT_SOURCE_NUM1 = 9,
    BOUNCE_INPUT_SOURCE_NUM2 = 10,
    BOUNCE_INPUT_SOURCE_NUM3 = 11,
    BOUNCE_INPUT_SOURCE_NUM4 = 12,
    BOUNCE_INPUT_SOURCE_NUM5 = 13,
    BOUNCE_INPUT_SOURCE_NUM6 = 14,
    BOUNCE_INPUT_SOURCE_NUM7 = 15,
    BOUNCE_INPUT_SOURCE_NUM8 = 16,
    BOUNCE_INPUT_SOURCE_NUM9 = 17,
    BOUNCE_INPUT_SOURCE_PAUSE = 18,
    BOUNCE_INPUT_SOURCE_CONFIRM_RETURN = 19,
    BOUNCE_INPUT_SOURCE_CONFIRM_KP_ENTER = 20,
    BOUNCE_INPUT_SOURCE_CONFIRM_SPACE = 21,
    BOUNCE_INPUT_SOURCE_ESCAPE = 22,
    BOUNCE_INPUT_SOURCE_QUIT = 23,
    /*
     * STEP 64-AF -- MIDP Canvas.KEY_POUND, raw key code 35 ('#').
     *
     * e.java:383 is `case 35:` on the RAW key code in keyPressed, and its only
     * body is `if (this.af) this.aq.powerUpGravity = 300`. The recovered tree
     * carries no KEY_POUND symbol, so the binding is established by the literal
     * 35 in the recovered switch arm itself.
     *
     * Appended last so no existing enumerator value moves.
     */
    BOUNCE_INPUT_SOURCE_HASH = 24,
    /*
     * ===================================================================
     * NATIVE ADDITION -- THE NUMERIC KEYPAD AS A GAME CONTROLLER.
     * ===================================================================
     *
     * THIS IS NOT A RECOVERY. The recovered game has no keypad input at all.
     * Bounce controls itself entirely through getGameAction() --
     * e.java:392-408 switches on UP(1), LEFT(2), RIGHT(5), DOWN(6) and
     * GAME_A(8), and the raw key codes it also reads are 35 ('#'), 49 ('1'),
     * 51 ('3') and the raw negatives -6 and -7. Not one of those is a keypad
     * digit, and no keypad keysym appears anywhere in the eleven Java files.
     *
     * The digits this build DOES read -- '1', '3', '7', '8', '9' -- are CHEATS:
     * 7-8-7-8-9-8 is the GodMode sequence (e.java:349-381, the a-g detector at
     * e.java:57) and 1/3 skip a level only while `af` is set (e.java:338-346).
     * They are not gameplay controls and are not treated as such here.
     *
     * So these four sources are a NATIVE FEATURE a player asked for: the
     * numeric keypad driving the ball, in the standard T9 arrangement --
     *
     *     8 = up / jump      2 = down
     *     4 = left           6 = right
     *
     * WHY NEW SOURCES AND NOT A REUSE OF THE ARROW ONES. Each of these is an
     * ALIAS, added to the same source_is_one_of() set as its arrow, so numpad 8
     * sets input.up through exactly the code path XK_Up already uses and there
     * is no second jump implementation. A new source rather than mapping the
     * keysym onto BOUNCE_INPUT_SOURCE_UP_ARROW directly is what makes two held
     * keys independent: held_sources keeps one bit per source, so holding numpad
     * 8 and arrow-up together and releasing only one leaves input.up true. Point
     * both keysyms at one source and releasing either would clear the action
     * while the other key was still down.
     *
     * ONE SOURCE PER DIRECTION, NOT PER KEYSYM. NumLock makes the same physical
     * key deliver two different keysyms -- XK_KP_8 with NumLock on, XK_KP_Up with
     * it off (keysymdef.h: XK_KP_8 0xffb8, XK_KP_Up 0xff97) -- and both are
     * mapped to the same source, so the keypad works either way.
     *
     * APPENDED AFTER HASH so no existing enumerator value moves, which the
     * GodMode detector's ordering assertion depends on.
     */
    BOUNCE_INPUT_SOURCE_KP_UP = 25,
    BOUNCE_INPUT_SOURCE_KP_DOWN = 26,
    BOUNCE_INPUT_SOURCE_KP_LEFT = 27,
    BOUNCE_INPUT_SOURCE_KP_RIGHT = 28,
    /*
     * ===================================================================
     * NATIVE ADDITION -- THE REST OF THE KEYPAD, 0/1/3/5/7/9.
     * ===================================================================
     *
     * A player asked for the full T9 arrangement instead of 2/4/6/8 alone. These
     * six are the SAME alias pattern as the four above: each is added to an
     * existing action's source set, so numpad 7 jumps and steers through exactly
     * the code path XK_Up and XK_Left already use and there is no second jump,
     * steer or pause implementation anywhere.
     *
     *     8 = up / jump      2 = down        7 = up-left     9 = up-right
     *     4 = left           6 = right       1 = down-left   3 = down-right
     *     0 = pause          5 = confirm
     *
     * WHY THE DIAGONALS ARE PAIRS AND NOT NEW AXES. There is no vertical
     * movement axis in this game to be diagonal with. Reading the recovered
     * direction mask f.w (f.java:140 c(int) sets bits 1/2/4/8, f.java:145 a(int)
     * clears them) every one of its reads is accounted for:
     *
     *     f.java:754   this.v && (this.w & 0x8)   the wall-contact gate
     *     f.java:806   (this.w & 0x2)             right acceleration
     *     f.java:808   (this.w & 0x1)             left acceleration
     *     f.java:821   this.m && (this.w & 0x8)   the JUMP
     *
     * Bit 4 -- the DOWN case at e.java:397 -- is set by c(4) and then never read,
     * because f.java:151 `this.w &= 0xFFFFFFF0` clears 0xE outright. So "up" in
     * this game means JUMP, not upward travel, and "down" means nothing at all.
     * A diagonal therefore cannot be a new action; it is a COMBINATION of the
     * existing ones, which is why these sources appear in two action sets each
     * (KP_UP_LEFT is in both `up` and `left`). The down half of KP_1/KP_3 is
     * carried faithfully and still has no effect, because that is what the
     * recovered source does with bit 4. It is not a no-op invented here.
     *
     * KP_PAUSE ALIASES AN EXISTING BUT CURRENTLY UNREACHABLE ACTION. `pause` is
     * already a field (below) and BOUNCE_INPUT_SOURCE_PAUSE already exists; what
     * is missing is a producer, because BOUNCE_APP_EVENT_PAUSE is fired only from
     * verifiers today. Aliasing 0 here gives the field a producer and changes
     * nothing else: with no consumer the flag is still inert, and this commit
     * deliberately does NOT wire a new pause producer, which would be a separate
     * feature.
     *
     * KP_CONFIRM IS THE ONE MAPPING THAT IS A CHOICE, NOT A DERIVATION, and the
     * player chose it explicitly. The recovered game has no FIRE: e.java:405
     * `case 8:` is `if (this.af) this.e = true`, the af-gated level-skip cheat
     * which is already bound to XK_c as a labelled native binding, and binding a
     * keypad key to it would duplicate that cheat. So numpad 5 aliases
     * `confirm_start`, the action the source already has for Return, KP_Enter
     * and Space. THE KNOWN CONSEQUENCE, recorded here rather than discovered later:
     * confirm_start is a MENU SELECT (app_ui_input_from_source maps it to
     * BOUNCE_UI_INPUT_SELECT), so numpad 5 confirms menu items and not a
     * gameplay action. There is no FIRE in the source to alias instead.
     *
     * APPENDED AFTER KP_RIGHT so no existing enumerator value moves, which both
     * the GodMode detector's ordering assertion and the keypad's own regression
     * cases depend on. bounce_input_source_is_keypad() is a single contiguous
     * range, so appending in order is also what keeps the T9 setting gating every
     * one of these: both that range's upper bound and bounce_input_apply_source()'s
     * bound were extended to KP_DOWN_RIGHT, and a source left outside either would
     * bypass the gate and drive the game with the setting off.
     */
    BOUNCE_INPUT_SOURCE_KP_PAUSE = 29,
    BOUNCE_INPUT_SOURCE_KP_CONFIRM = 30,
    BOUNCE_INPUT_SOURCE_KP_UP_LEFT = 31,
    BOUNCE_INPUT_SOURCE_KP_UP_RIGHT = 32,
    BOUNCE_INPUT_SOURCE_KP_DOWN_LEFT = 33,
    BOUNCE_INPUT_SOURCE_KP_DOWN_RIGHT = 34
} BounceInputSource;

typedef struct BounceInputState {
    bool left;
    bool right;
    bool up;
    bool down;
    bool numeric[10];
    bool pause;
    bool confirm_start;
    bool escape_back;
    bool quit;

    /* Internal source mask keeps simultaneous aliases independently held. */
    uint64_t held_sources;
} BounceInputState;

/*
 * NATIVE ADDITION -- is this one of the numeric-keypad sources?
 *
 * The keypad is the one group of sources that is conditional: it drives the
 * game only while the Settings toggle is on, because the keypad also carries
 * NumLock-off navigation keys and a player who never asked for a keypad
 * controller must not have their game played by reaching for it.
 *
 * Kept here rather than in the event loop so "which sources are keypad" has one
 * answer: the same predicate guards the mapping, the option's own rendering and
 * the verifier, instead of three lists that can disagree.
 */
bool bounce_input_source_is_keypad(BounceInputSource source);

void bounce_input_reset(BounceInputState *input);

/*
 * Applies a press/release for one physical development source. Releasing one
 * alias does not clear another alias that is still held.
 */
int bounce_input_apply_source(
    BounceInputState *input,
    BounceInputSource source,
    bool pressed
);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_APP_INPUT_STATE_H */
