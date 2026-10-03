#include "input_state.h"

#include <stddef.h>

static uint64_t source_bit(BounceInputSource source)
{
    return UINT64_C(1) << (unsigned int)source;
}

/*
 * ONE ACTION TEST: a contiguous source range, plus any number of distant bits.
 *
 * NATIVE ADDITION -- the "plus any number" part. The keypad started with exactly
 * one distant alias per action, which source_is_one_of_two() expressed. The four
 * diagonal keypad keys need TWO distant aliases each, because a diagonal belongs
 * to two actions at once: KP_UP_LEFT is in both `up` and `left`. Expressing that
 * by widening source_is_one_of_two() to a third and fourth positional argument
 * would have produced a helper whose parameter list encodes a count that is
 * already known to be "all the extra bits this action has", so the mask is passed
 * instead and the count lives with the action.
 *
 * The range loop is unchanged from the original source_is_one_of(), and the two
 * older helpers below are now one-line wrappers over this, so the alias semantics
 * the shipped 2/4/6/8 mapping relies on are the same code, not a reimplementation.
 */
static bool source_is_range_plus(
    uint64_t held_sources,
    BounceInputSource first,
    BounceInputSource last,
    uint64_t extra_mask
)
{
    uint64_t mask = extra_mask;
    unsigned int source;

    for (source = (unsigned int)first;
         source <= (unsigned int)last;
         ++source)
        mask |= UINT64_C(1) << source;
    return (held_sources & mask) != UINT64_C(0);
}

/*
 * NATIVE ADDITION -- an alias can be a DISJOINT pair of ranges.
 *
 * The plain range case needs no helper of its own any more: source_is_range_plus()
 * with an empty extra mask is that expression, and every caller that used to want
 * a bare range now wants at least one distant bit, so a separate wrapper would
 * have been dead code.
 *
 * source_is_one_of() covered one contiguous range, which was enough for the
 * shipped aliases because each is adjacent: LEFT_ARROW..LEFT_A is 0..1. The numeric
 * keypad cannot join them, because it was appended after HASH precisely so that
 * no existing enumerator value would move, and the GodMode detector's ordering
 * assertion depends on that. So each alias is "its own range, or one distant
 * bit", and this is that expression.
 */
static bool source_is_one_of_two(
    uint64_t held_sources,
    BounceInputSource first,
    BounceInputSource last,
    BounceInputSource extra
)
{
    return source_is_range_plus(
        held_sources,
        first,
        last,
        UINT64_C(1) << (unsigned int)extra
    );
}

/* Every distant alias an action has, as one mask. */
static uint64_t alias_mask(
    BounceInputSource first,
    BounceInputSource second,
    BounceInputSource third
)
{
    return (UINT64_C(1) << (unsigned int)first)
        | (UINT64_C(1) << (unsigned int)second)
        | (UINT64_C(1) << (unsigned int)third);
}

void bounce_input_reset(BounceInputState *input)
{
    if (input == NULL)
        return;
    *input = (BounceInputState){0};
}

bool bounce_input_source_is_keypad(BounceInputSource source)
{
    /*
     * The upper bound is KP_DOWN_RIGHT, the last keypad source. It must be: this
     * predicate is the T9 setting's gate, and a source left outside the range
     * would not be recognised as keypad, so the gate would skip it and that key
     * would drive the game with the setting OFF. Appending the sources in
     * ascending order is what lets one contiguous range cover all of them.
     */
    return source >= BOUNCE_INPUT_SOURCE_KP_UP
        && source <= BOUNCE_INPUT_SOURCE_KP_DOWN_RIGHT;
}

int bounce_input_apply_source(
    BounceInputState *input,
    BounceInputSource source,
    bool pressed
)
{
    uint64_t bit;

    if (input == NULL
        || source < BOUNCE_INPUT_SOURCE_LEFT_ARROW
        || source > BOUNCE_INPUT_SOURCE_KP_DOWN_RIGHT)
        return -1;
    /*
     * NATIVE ADDITION -- HASH IS DELIBERATELY NOT HELD, and this is now stated
     * rather than left to fall out of a range test.
     *
     * BOUNCE_INPUT_SOURCE_HASH sits inside the accepted range above, and until
     * this change it was excluded only because the old bound stopped at
     * BOUNCE_INPUT_SOURCE_QUIT, one below it. Widening the bound for the keypad
     * sources would have swept HASH in as a side effect. It is a one-shot cheat
     * (`if (this.af) this.aq.powerUpGravity = 300`, e.java:383) with no held
     * state to track, so keeping it out preserves exactly what shipped.
     *
     * The keypad sources are held normally; they are real controls.
     */
    if (source == BOUNCE_INPUT_SOURCE_HASH)
        return -1;

    bit = source_bit(source);
    if (pressed)
        input->held_sources |= bit;
    else
        input->held_sources &= ~bit;

    input->left = source_is_range_plus(
        input->held_sources,
        BOUNCE_INPUT_SOURCE_LEFT_ARROW,
        BOUNCE_INPUT_SOURCE_LEFT_A,
        /*
         * NATIVE ADDITION: the keypad's 4 key, plus both of its diagonals. An
         * alias, not a second path -- input.left is the transcription of the
         * source's `(this.w & 0x1)` at f.java:808.
         */
        alias_mask(
            BOUNCE_INPUT_SOURCE_KP_LEFT,
            BOUNCE_INPUT_SOURCE_KP_UP_LEFT,
            BOUNCE_INPUT_SOURCE_KP_DOWN_LEFT
        )
    );
    input->right = source_is_range_plus(
        input->held_sources,
        BOUNCE_INPUT_SOURCE_RIGHT_ARROW,
        BOUNCE_INPUT_SOURCE_RIGHT_D,
        /* NATIVE ADDITION: the keypad's 6 key, plus both of its diagonals. */
        alias_mask(
            BOUNCE_INPUT_SOURCE_KP_RIGHT,
            BOUNCE_INPUT_SOURCE_KP_UP_RIGHT,
            BOUNCE_INPUT_SOURCE_KP_DOWN_RIGHT
        )
    );
    input->up = source_is_range_plus(
        input->held_sources,
        BOUNCE_INPUT_SOURCE_UP_ARROW,
        BOUNCE_INPUT_SOURCE_UP_W,
        /*
         * NATIVE ADDITION: the keypad's 8 key, which is the JUMP, plus both
         * up-diagonals. input.up is the transcription of the source's `w & 0x8`
         * (f.java:754 and f.java:821), so this alias is what makes numpad 8 jump
         * -- there is no second jump rule anywhere. Holding 7 or 9 therefore
         * jumps AND steers, which is what a diagonal has to mean here: the only
         * vertical action in this game is the jump.
         */
        alias_mask(
            BOUNCE_INPUT_SOURCE_KP_UP,
            BOUNCE_INPUT_SOURCE_KP_UP_LEFT,
            BOUNCE_INPUT_SOURCE_KP_UP_RIGHT
        )
    );
    input->down = source_is_range_plus(
        input->held_sources,
        BOUNCE_INPUT_SOURCE_DOWN_ARROW,
        BOUNCE_INPUT_SOURCE_DOWN_S,
        /*
         * NATIVE ADDITION: the keypad's 2 key, plus both down-diagonals.
         *
         * REPRODUCED AS THE SOURCE HAS IT, INERT AND ALL. e.java:397 sets bit 4
         * through c(4) and nothing ever reads it, because f.java:151 clears 0xE.
         * So this flag has no consumer, and that is faithful rather than a stub:
         * the diagonal keys carry their down half so the held state matches what
         * was asked for, and no second "move down" rule is invented to make it
         * matter.
         */
        alias_mask(
            BOUNCE_INPUT_SOURCE_KP_DOWN,
            BOUNCE_INPUT_SOURCE_KP_DOWN_LEFT,
            BOUNCE_INPUT_SOURCE_KP_DOWN_RIGHT
        )
    );
    input->numeric[0] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM0)) != UINT64_C(0);
    input->numeric[1] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM1)) != UINT64_C(0);
    input->numeric[2] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM2)) != UINT64_C(0);
    input->numeric[3] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM3)) != UINT64_C(0);
    input->numeric[4] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM4)) != UINT64_C(0);
    input->numeric[5] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM5)) != UINT64_C(0);
    input->numeric[6] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM6)) != UINT64_C(0);
    input->numeric[7] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM7)) != UINT64_C(0);
    input->numeric[8] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM8)) != UINT64_C(0);
    input->numeric[9] = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_NUM9)) != UINT64_C(0);
    /*
     * NATIVE ADDITION -- numpad 0. `pause` was already a field and
     * BOUNCE_INPUT_SOURCE_PAUSE already existed; this only adds the keypad's 0 as
     * an alias of it, through the same one-distant-bit helper every other alias
     * uses. It changes no gameplay, because nothing consumes `pause` yet:
     * BOUNCE_APP_EVENT_PAUSE is fired only from verifiers, so the PAUSE screen
     * stays unreachable. Giving the flag a producer is the whole of this change
     * and wiring the pause itself is deliberately left as a separate feature.
     */
    input->pause = source_is_one_of_two(
        input->held_sources,
        BOUNCE_INPUT_SOURCE_PAUSE,
        BOUNCE_INPUT_SOURCE_PAUSE,
        BOUNCE_INPUT_SOURCE_KP_PAUSE
    );
    /*
     * NATIVE ADDITION -- numpad 5 joins the existing CONFIRM aliases, so all four
     * of Return, KP_Enter, Space and numpad 5 are ONE action and releasing any
     * one of them leaves the others held.
     *
     * THIS IS THE MAPPING THAT IS A CHOICE. The recovered game has no FIRE:
     * e.java:405 `case 8:` is `if (this.af) this.e = true`, the af cheat already
     * bound to XK_c, so there is no FIRE action to alias. The player chose
     * confirm_start for 5. THE CONSEQUENCE IS RECORDED, NOT HIDDEN:
     * confirm_start is a MENU SELECT (app_ui_input_from_source maps it to
     * BOUNCE_UI_INPUT_SELECT), so numpad 5 will confirm menu items.
     */
    input->confirm_start = source_is_one_of_two(
        input->held_sources,
        BOUNCE_INPUT_SOURCE_CONFIRM_RETURN,
        BOUNCE_INPUT_SOURCE_CONFIRM_SPACE,
        BOUNCE_INPUT_SOURCE_KP_CONFIRM
    );
    input->escape_back = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_ESCAPE)) != UINT64_C(0);
    input->quit = (input->held_sources
        & source_bit(BOUNCE_INPUT_SOURCE_QUIT)) != UINT64_C(0);
    return 0;
}
