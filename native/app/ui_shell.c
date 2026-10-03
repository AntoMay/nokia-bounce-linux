#include "ui_shell.h"

#include "locale.h"
#include "script_font.h"
#include "visual_assets.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/*
 * Resolve one user-facing string for the currently active language. English is
 * the default at every launch; Settings > Language changes the active value at
 * runtime, so no call site needs to know which language is selected.
 */
static const char *tr(BounceLocaleKey key)
{
    return bounce_locale_text(bounce_locale_active_language(), key);
}

/*
 * NATIVE UI TEXT PLACEHOLDER — NOT ORIGINAL FONT VERIFIED
 *
 * This tiny 5x7 uppercase seam exists only to make the provisional menu and
 * bounded Instructions, High Score, and Level Selection states visible in the
 * native shell. It is not a Nokia font, does not claim MIDP metrics, and is
 * intentionally isolated from the logical renderer.
 */
/*
 * Row 0..3 are the source-verified original entries in the order LoadMenuStrings()
 * creates them; rows 4 and 5 are NATIVE EXTENSIONS; row 6 is the source's
 * Command.EXIT made selectable. Each row names a semantic key, so the same seven
 * rows are drawn in whatever language is active.
 */
/*
 * The runtime UI color profiles. NATIVE EXTENSION: the recovered Java has no
 * theme system at all, so nothing here is claimed to be original.
 *
 * FOUR AUTHORITATIVE ROLES per profile, given by the milestone specification:
 *
 *     background            -> the `background` field
 *     text                  -> the `text` field
 *     selected_background   -> the `highlight` field
 *     selected_text         -> the `selected_text` field
 *
 * A selected Theme row is drawn as `highlight` fill plus `selected_text`
 * glyphs, so an inverted profile (bright fill, dark glyphs) reads correctly.
 *
 * THE OTHER SIX ROLES ARE DERIVED, NOT SPECIFIED. The brief fixes four colors
 * per profile, but the UI consumes nine roles. The six unspecified roles come
 * from one deterministic, documented formula applied per 8-bit channel:
 *
 *     mix(a, b, t) = round(a + (b - a) * t)
 *
 *     panel    = mix(background, text, 0.08)
 *     border   = mix(background, text, 0.30)
 *     body     = mix(background, text, 0.82)
 *     muted    = mix(background, text, 0.55)
 *     disabled = mix(background, text, 0.38)
 *     accent   = selected_background
 *
 * Mixing toward `text` raises contrast against `background` in BOTH
 * directions, so the same formula serves the dark profiles and the light ones
 * (Sepia, Arctic) without a special case. `accent` reuses
 * selected_background because that color is already required to contrast with
 * the background, so the Back hint stays legible in every profile.
 *
 * PROFILE 0 (Default) IS NOT REDESIGNED. Its nine values are the exact ones
 * this build shipped before profiles existed, and its selected_text equals its
 * text, so its selected row is drawn bit-for-bit as before.
 *
 * "GREEN LCD" KEPT ITS SLOT AND ITS NAME. Profile 1 is the same entry it always
 * was. It was briefly relabelled "Green Light" in an intermediate milestone; it
 * is "Green LCD" again, drawn through the pre-existing GREEN_LCD locale key, so
 * there is exactly one such profile and no duplicate palette. The much earlier
 * (#06180C / #BDF7C2 / #43C95A) values are superseded and appear nowhere here.
 *
 * COUNT IS 13 OF A 15 TARGET. The last two target profiles have no established
 * name or palette in this repository, in reverse/, in the reflog or on any
 * branch. They were deliberately NOT invented, so the target is not met.
 *
 * Nothing here is persisted. A fresh launch is always profile 0. There is no
 * config file, RMS, registry, environment variable or external theme file.
 */
static const BounceUiPalette ui_palettes[BOUNCE_UI_COLOR_PROFILE_COUNT] = {
    /*  0  Default                     */
    {
        UINT32_C(0xff121a30), /* background */
        UINT32_C(0xff202c50), /* panel */
        UINT32_C(0xff5069a8), /* border */
        UINT32_C(0xff3c65b5), /* highlight = selected_background */
        UINT32_C(0xfff5f5f5), /* selected_text: same as text, so the selected row is drawn exactly as before */
        UINT32_C(0xfff5f5f5), /* text */
        UINT32_C(0xffd7e0ff), /* body */
        UINT32_C(0xff9aa6c4), /* muted */
        UINT32_C(0xffffc857), /* accent */
        UINT32_C(0xff777f91)  /* disabled */
    },
    /*  1  Green LCD                   */
    {
        UINT32_C(0xff8eb038), /* background = #8EB038 */
        UINT32_C(0xff85a534), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff6b862a), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xff0b1003), /* highlight = selected_background = #0B1003 */
        UINT32_C(0xff8eb038), /* selected_text = #8EB038 */
        UINT32_C(0xff1a2308), /* text = #1A2308 */
        UINT32_C(0xff2f3c11), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff4e621e), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xff0b1003), /* accent = selected_background */
        UINT32_C(0xff627a26)  /* disabled = mix(bg,text,0.38) */
    },
    /*  2  Sepia                       */
    {
        UINT32_C(0xfff2ebd9), /* background = #F2EBD9 */
        UINT32_C(0xffe5dccb), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xffc0b4a3), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xff4a3525), /* highlight = selected_background = #4A3525 */
        UINT32_C(0xfff2ebd9), /* selected_text = #F2EBD9 */
        UINT32_C(0xff4a3525), /* text = #4A3525 */
        UINT32_C(0xff685645), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff968776), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xff4a3525), /* accent = selected_background */
        UINT32_C(0xffb2a695)  /* disabled = mix(bg,text,0.38) */
    },
    /*  3  Green Inverted              */
    {
        UINT32_C(0xff0a140a), /* background = #0A140A */
        UINT32_C(0xff0d230d), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff164b16), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xff33cc33), /* highlight = selected_background = #33CC33 */
        UINT32_C(0xff0a140a), /* selected_text = #0A140A */
        UINT32_C(0xff33cc33), /* text = #33CC33 */
        UINT32_C(0xff2cab2c), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff217921), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xff33cc33), /* accent = selected_background */
        UINT32_C(0xff1a5a1a)  /* disabled = mix(bg,text,0.38) */
    },
    /*  4  Amber                       */
    {
        UINT32_C(0xff140c02), /* background = #140C02 */
        UINT32_C(0xff271802), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff5a3a01), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xffffa500), /* highlight = selected_background = #FFA500 */
        UINT32_C(0xff140c02), /* selected_text = #140C02 */
        UINT32_C(0xffffa500), /* text = #FFA500 */
        UINT32_C(0xffd58900), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff956001), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xffffa500), /* accent = selected_background */
        UINT32_C(0xff6d4601)  /* disabled = mix(bg,text,0.38) */
    },
    /*  5  Navy Cream                  */
    {
        UINT32_C(0xff001133), /* background = #001133 */
        UINT32_C(0xff142341), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff4c5666), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xff00bfff), /* highlight = selected_background = #00BFFF */
        UINT32_C(0xff001133), /* selected_text = #001133 */
        UINT32_C(0xfffff8dc), /* text = #FFF8DC */
        UINT32_C(0xffd1cebe), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff8c9090), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xff00bfff), /* accent = selected_background */
        UINT32_C(0xff616973)  /* disabled = mix(bg,text,0.38) */
    },
    /*  6  Forest Mint                 */
    {
        UINT32_C(0xff112a1c), /* background = #112A1C */
        UINT32_C(0xff1f392a), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff466152), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xffffd700), /* highlight = selected_background = #FFD700 */
        UINT32_C(0xff112a1c), /* selected_text = #112A1C */
        UINT32_C(0xffc2e0d1), /* text = #C2E0D1 */
        UINT32_C(0xffa2bfb0), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff728e80), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xffffd700), /* accent = selected_background */
        UINT32_C(0xff546f61)  /* disabled = mix(bg,text,0.38) */
    },
    /*  7  Burgundy                    */
    {
        UINT32_C(0xff5a121e), /* background = #5A121E */
        UINT32_C(0xff67222d), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff8a4f56), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xffe5c158), /* highlight = selected_background = #E5C158 */
        UINT32_C(0xff5a121e), /* selected_text = #5A121E */
        UINT32_C(0xfffadcd8), /* text = #FADCD8 */
        UINT32_C(0xffddb8b7), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xffb28184), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xffe5c158), /* accent = selected_background */
        UINT32_C(0xff975f65)  /* disabled = mix(bg,text,0.38) */
    },
    /*  8  Charcoal Blue               */
    {
        UINT32_C(0xff1e1e1e), /* background = #1E1E1E */
        UINT32_C(0xff303030), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff626262), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xff007acc), /* highlight = selected_background = #007ACC */
        UINT32_C(0xffffffff), /* selected_text = #FFFFFF */
        UINT32_C(0xffffffff), /* text = #FFFFFF */
        UINT32_C(0xffd6d6d6), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff9a9a9a), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xff007acc), /* accent = selected_background */
        UINT32_C(0xff747474)  /* disabled = mix(bg,text,0.38) */
    },
    /*  9  Nokia Blue                  */
    {
        UINT32_C(0xff3b629b), /* background = #3B629B */
        UINT32_C(0xff486da2), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff6c8bb7), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xffff8c00), /* highlight = selected_background = #FF8C00 */
        UINT32_C(0xff3b629b), /* selected_text = #3B629B */
        UINT32_C(0xffe0ecf8), /* text = #E0ECF8 */
        UINT32_C(0xffc2d3e7), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff96aece), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xffff8c00), /* accent = selected_background */
        UINT32_C(0xff7a96be)  /* disabled = mix(bg,text,0.38) */
    },
    /* 10  Purple                      */
    {
        UINT32_C(0xff2e1a47), /* background = #2E1A47 */
        UINT32_C(0xff3c2956), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff62517e), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xffffdab9), /* highlight = selected_background = #FFDAB9 */
        UINT32_C(0xff2e1a47), /* selected_text = #2E1A47 */
        UINT32_C(0xffdcd0ff), /* text = #DCD0FF */
        UINT32_C(0xffbdafde), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff8e7eac), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xffffdab9), /* accent = selected_background */
        UINT32_C(0xff705f8d)  /* disabled = mix(bg,text,0.38) */
    },
    /* 11  Arctic                      */
    {
        UINT32_C(0xffffffff), /* background = #FFFFFF */
        UINT32_C(0xffebedef), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xffb2bbc5), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xff00bfff), /* highlight = selected_background = #00BFFF */
        UINT32_C(0xffffffff), /* selected_text = #FFFFFF */
        UINT32_C(0xff001c3d), /* text = #001C3D */
        UINT32_C(0xff2e4560), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff738294), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xff00bfff), /* accent = selected_background */
        UINT32_C(0xff9ea9b5)  /* disabled = mix(bg,text,0.38) */
    },
    /* 12  Teal Ivory                  */
    {
        UINT32_C(0xff0c3b3f), /* background = #0C3B3F */
        UINT32_C(0xff1f4b4d), /* panel   = mix(bg,text,0.08) */
        UINT32_C(0xff557674), /* border  = mix(bg,text,0.30) */
        UINT32_C(0xffff8c00), /* highlight = selected_background = #FF8C00 */
        UINT32_C(0xff0c3b3f), /* selected_text = #0C3B3F */
        UINT32_C(0xfffffff0), /* text = #FFFFF0 */
        UINT32_C(0xffd3dcd0), /* body  = mix(bg,text,0.82) */
        UINT32_C(0xff92a7a0), /* muted = mix(bg,text,0.55) */
        UINT32_C(0xffff8c00), /* accent = selected_background */
        UINT32_C(0xff688582)  /* disabled = mix(bg,text,0.38) */
    }
};

static unsigned int active_color_profile;

void bounce_ui_shell_set_color_profile(unsigned int index)
{
    if (index >= BOUNCE_UI_COLOR_PROFILE_COUNT)
        return;
    active_color_profile = index;
}

unsigned int bounce_ui_shell_active_color_profile(void)
{
    return active_color_profile;
}

const BounceUiPalette *bounce_ui_shell_palette(void)
{
    return &ui_palettes[active_color_profile];
}

/*
 * Theme list windowing.
 *
 * 13 profiles cannot fit the panel at the existing 20 px row pitch: 13 * 20 is
 * 260 px against roughly 90 px of usable height between the separator and
 * ESC BACK. The global row pitch is deliberately NOT changed, and neither is
 * any other page's geometry. Instead the Theme list keeps its own existing
 * 20 px rows and shows a window of BOUNCE_UI_THEME_VISIBLE_ROWS of them.
 *
 * The offset is DERIVED, NEVER STORED: theme_first_row() is a pure function of
 * the selected index, so there is no scroll field in BounceSettings, nothing to
 * persist, and nothing to restore on a fresh launch. The selected row is inside
 * the window by construction, which is what keeps all 13 reachable.
 */
#define BOUNCE_UI_THEME_VISIBLE_ROWS 4u
#define BOUNCE_UI_THEME_ROW_Y0 30
#define BOUNCE_UI_THEME_ROW_PITCH 20
#define BOUNCE_UI_THEME_ROW_HIT_H 16

static unsigned int theme_first_row(unsigned int count, unsigned int selected)
{
    int first;
    int last;

    if (count <= BOUNCE_UI_THEME_VISIBLE_ROWS)
        return 0u;
    last = (int)count - (int)BOUNCE_UI_THEME_VISIBLE_ROWS;
    first = (int)selected - 2;
    if (first < 0)
        first = 0;
    if (first > last)
        first = last;
    return (unsigned int)first;
}

/*
 * THE SCROLLBAR, AS ONE FUNCTION BOTH PAGES CALL.
 *
 * This is the Theme page's scroll indicator with its geometry taken as
 * parameters instead of hardcoded. The arithmetic below is the Theme page's,
 * character for character; only the five numbers it used to close over are now
 * arguments. theme_draw_scrollbar() immediately below passes the Theme page's
 * own values, so the Theme page's pixels are unchanged.
 *
 * WHY IT IS SHARED RATHER THAN DUPLICATED. The Instructions page needs the same
 * indicator for the same reason -- its text is longer than the screen, and a
 * reader cannot see what they are missing. Two copies of this arithmetic would
 * be two things to keep in step, and the second copy would have no test pinning
 * it. One copy means the Instructions scrollbar is correct because the Theme
 * scrollbar is.
 *
 * WHAT IT DRAWS. A two-pixel-wide track in the palette's border role, and a
 * thumb of the same width in the accent role, its height proportional to the
 * visible fraction of the content and never below six pixels, its position
 * proportional to how far down the content is. It draws NOTHING when the whole
 * content fits, which is what makes the indicator honest: a scrollbar that is
 * always on screen teaches the reader to ignore it.
 */
static int ui_draw_scrollbar(
    BounceRenderer *renderer,
    int x,
    int track_y,
    int track_h,
    unsigned int count,
    unsigned int visible,
    unsigned int first
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
    int thumb_h;
    int thumb_y;
    int last;

    if (renderer == NULL || track_h <= 0)
        return -1;
    if (count <= visible || visible == 0u)
        return 0;
    last = (int)count - (int)visible;
    if (bounce_renderer_fill_rect(renderer, x, track_y, 2, track_h, ui->border)
        != 0)
        return -1;
    thumb_h = track_h * (int)visible / (int)count;
    if (thumb_h < 6)
        thumb_h = 6;
    thumb_y = track_y
        + (last > 0 ? (track_h - thumb_h) * (int)first / last : 0);
    return bounce_renderer_fill_rect(renderer, x, thumb_y, 2, thumb_h, ui->accent);
}

/*
 * The Theme page's scrollbar: THE SAME FUNCTION, with the Theme page's own
 * geometry. The five values are exactly the ones this function used to close
 * over, which is why the Theme page renders byte for byte as it did before the
 * Instructions page joined it.
 */
static int theme_draw_scrollbar(
    BounceRenderer *renderer,
    unsigned int count,
    unsigned int first
)
{
    return ui_draw_scrollbar(
        renderer,
        118,
        BOUNCE_UI_THEME_ROW_Y0,
        77,
        count,
        BOUNCE_UI_THEME_VISIBLE_ROWS,
        first
    );
}

static const BounceLocaleKey menu_label_keys[BOUNCE_MENU_ROW_COUNT] = {
    BOUNCE_LOCALE_CONTINUE,
    BOUNCE_LOCALE_NEW_GAME,
    BOUNCE_LOCALE_HIGH_SCORE,
    BOUNCE_LOCALE_INSTRUCTIONS,
    /* NATIVE EXTENSION: no original counterpart. */
    BOUNCE_LOCALE_SETTINGS,
    /* NATIVE EXTENSION: no original counterpart. */
    BOUNCE_LOCALE_ABOUT,
    /* The original attaches Exit as Command.EXIT, not a list element. */
    BOUNCE_LOCALE_EXIT
};

/*
 * SOURCE-VERIFIED lang.xx ID 1 (MORE_INSTRUCTIONS).
 * The complete English fallback is retained as data. The renderer uses only
 * the bounded prefix below because no source-verified native scroll/layout
 * policy exists; the remainder is not silently paged or scrolled.
 */

/*
 * SOURCE-VERIFIED HighScore default is Java int 0 when no RMS record exists.
 * NATIVE LIMITATION NOTICE — NOT ORIGINAL BOUNCE TEXT: no RMS value is
 * connected to this native shell, so 0 is a default placeholder, not a
 * fabricated loaded historical score.
 */
/* The default is the Java int 0; a number, so it is never translated. */
static const char high_score_value[] = "0";

/* SOURCE-VERIFIED lang.xx fallback labels for the MIDP level List. */
/*
 * D-06 -- the previous note here read:
 *
 *     NATIVE EXTENSION title. The original reaches this list from "New game"
 *     (BounceGame.java:236-243) and the MIDP List supplied its own title, so no
 *     recovered string exists for it. The canonical UI specification calls this
 *     screen Level Select, and that is the clearer native label. It is localized
 *     through BOUNCE_LOCALE_LEVEL_SELECT.
 *
 * THAT WAS INCORRECT, and it is replaced rather than amended so the wrong claim
 * cannot be read again as current. A MIDP List takes its title as the first
 * constructor argument, and BounceGame.java:142 passes one:
 *
 *     new List(Translation.sprintf_translated(Translation.NEW_GAME),
 *              List.IMPLICIT, levelStrings, null)
 *
 * so the title IS a recovered string -- Translation.NEW_GAME, id 11 -- and the
 * level-selection renderer now draws tr(BOUNCE_LOCALE_NEW_GAME), the same row
 * the main menu's New Game entry uses. This was recorded as a
 * comment-vs-source CONFLICT; the source wins. See the renderer for the full
 * argument and for why BOUNCE_LOCALE_LEVEL_SELECT is left in the table.
 */
static const char level_selection_back[] = "Back";

static const uint8_t placeholder_glyph_rows[40][7] = {
    {0x0eu, 0x11u, 0x11u, 0x1fu, 0x11u, 0x11u, 0x11u}, /* A */
    {0x1eu, 0x11u, 0x11u, 0x1eu, 0x11u, 0x11u, 0x1eu}, /* B */
    {0x0fu, 0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x0fu}, /* C */
    {0x1eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x1eu}, /* D */
    {0x1fu, 0x10u, 0x10u, 0x1eu, 0x10u, 0x10u, 0x1fu}, /* E */
    {0x1fu, 0x10u, 0x10u, 0x1eu, 0x10u, 0x10u, 0x10u}, /* F */
    {0x0fu, 0x10u, 0x10u, 0x17u, 0x11u, 0x11u, 0x0fu}, /* G */
    {0x11u, 0x11u, 0x11u, 0x1fu, 0x11u, 0x11u, 0x11u}, /* H */
    {0x0eu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x0eu}, /* I */
    {0x07u, 0x02u, 0x02u, 0x02u, 0x02u, 0x12u, 0x0cu}, /* J */
    {0x11u, 0x12u, 0x14u, 0x18u, 0x14u, 0x12u, 0x11u}, /* K */
    {0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x10u, 0x1fu}, /* L */
    {0x11u, 0x1bu, 0x15u, 0x15u, 0x11u, 0x11u, 0x11u}, /* M */
    {0x11u, 0x19u, 0x1du, 0x17u, 0x13u, 0x11u, 0x11u}, /* N */
    {0x0eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0eu}, /* O */
    {0x1eu, 0x11u, 0x11u, 0x1eu, 0x10u, 0x10u, 0x10u}, /* P */
    {0x0eu, 0x11u, 0x11u, 0x11u, 0x15u, 0x12u, 0x0du}, /* Q */
    {0x1eu, 0x11u, 0x11u, 0x1eu, 0x14u, 0x12u, 0x11u}, /* R */
    {0x0fu, 0x10u, 0x10u, 0x0eu, 0x01u, 0x01u, 0x1eu}, /* S */
    {0x1fu, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x04u}, /* T */
    {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0eu}, /* U */
    {0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0au, 0x04u}, /* V */
    {0x11u, 0x11u, 0x11u, 0x15u, 0x15u, 0x15u, 0x0au}, /* W */
    {0x11u, 0x11u, 0x0au, 0x04u, 0x0au, 0x11u, 0x11u}, /* X */
    {0x11u, 0x11u, 0x0au, 0x04u, 0x04u, 0x04u, 0x04u}, /* Y */
    {0x1fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x10u, 0x1fu}, /* Z */
    /*
     * V-2 -- rows 26..39, the characters that the native shell actually paints.
     * See the note under the table for how this set was chosen and what each
     * row's provenance is.
     */
    {0x04u, 0x04u, 0x04u, 0x04u, 0x04u, 0x00u, 0x04u}, /* 26 '!' */
    /*
     * Rows 27..36, the digits. COPIED VERBATIM from visual_hud_digit_rows in
     * visual_assets.c -- this game's own 5x7 digits, already transcribed from the
     * HUD score rendering. Copying rather than redrawing is the point: these are
     * the tree's existing digit shapes, so nothing here is a new design.
     */
    {0x0eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0eu}, /* 27 '0' */
    {0x04u, 0x0cu, 0x04u, 0x04u, 0x04u, 0x04u, 0x0eu}, /* 28 '1' */
    {0x0eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1fu}, /* 29 '2' */
    {0x1fu, 0x02u, 0x04u, 0x02u, 0x01u, 0x11u, 0x0eu}, /* 30 '3' */
    {0x02u, 0x06u, 0x0au, 0x12u, 0x1fu, 0x02u, 0x02u}, /* 31 '4' */
    {0x1fu, 0x10u, 0x1eu, 0x01u, 0x01u, 0x11u, 0x0eu}, /* 32 '5' */
    {0x06u, 0x08u, 0x10u, 0x1eu, 0x11u, 0x11u, 0x0eu}, /* 33 '6' */
    {0x1fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x08u, 0x08u}, /* 34 '7' */
    {0x0eu, 0x11u, 0x11u, 0x0eu, 0x11u, 0x11u, 0x0eu}, /* 35 '8' */
    {0x0eu, 0x11u, 0x11u, 0x0fu, 0x01u, 0x02u, 0x0cu}, /* 36 '9' */
    /* Rows 37..39, punctuation, native font work like the '!' above. */
    {0x00u, 0x00u, 0x00u, 0x00u, 0x00u, 0x0cu, 0x04u}, /* 37 '.' */
    {0x00u, 0x00u, 0x00u, 0x00u, 0x0cu, 0x04u, 0x08u}, /* 38 ',' */
    {0x11u, 0x0au, 0x04u, 0x1fu, 0x04u, 0x0au, 0x11u}  /* 39 '*' */
};

/*
 * V-2 -- WHY THE TABLE IS NOW 27 ROWS AND NOT 26.
 *
 * The 5x7 font had letters and a space. Every other ASCII character fell through
 * placeholder_text_index()'s -1 and drew placeholder_unknown_rows -- the box.
 * That was never visible, because every English and Indonesian label is letters
 * and spaces, and the %0U/%1U/%2U and %U substitutions consume the placeholders
 * before anything is drawn.
 *
 * IT BECAME VISIBLE WITH THAI. lang.th-TH's GAME_END_CONGRATS, LEVEL_COMPLETED
 * and NEW_HIGH_SCORE each END IN a plain ASCII '!', and nothing substitutes it.
 * So with the Thai set shipped (V-2, script_font.c) three Thai screens showed
 * real, correctly shaped Thai text with a box sitting in the middle of the
 * congratulation -- which is precisely the "renders but cannot be read" outcome
 * this font's absence used to produce, moved rather than removed.
 *
 * WHAT THIS ROW IS, PRECISELY. The original game's text font is NOT in this
 * repository: it drew its labels through the handset's system font, so there is
 * no Nokia '!' bitmap anywhere to copy and none is claimed here. This row is
 * NATIVE FONT WORK, in the same style as the twenty-six letters above, which
 * carry no provenance claim either. It is an honest '!' at 5x7, not a recovered
 * glyph.
 *
 * HOW THE SET WAS CHOSEN: BY MEASUREMENT, NOT BY GUESS. Every renderer the
 * shell exposes was run in all five languages with the box counter armed, and
 * every un-drawable codepoint that was actually PAINTED was recorded. That came
 * to exactly thirteen, and no others:
 *
 *     '.' ',' '*'                 and the digits '0'..'7'.
 *
 * All thirteen come from the shell's OWN native text, not from any shipped
 * resource string: the hardcoded "Level 1 completed!" and "Level 11 completed!"
 * on the level-complete screen, the four ".h" module names on the About screen,
 * and the "<w> X <h>" dimension hint. None of them is Thai-specific -- they drew
 * boxes in English before V-2 as well -- but they appear on the Thai
 * game-end, game-over and level-complete screens too, so leaving them would have
 * left Thai output still containing unexplained boxes and V-2 unfinished.
 *
 * Digits '8' and '9' are included although only '0'..'7' were measured: a digit
 * table that stops at seven is a table with a hole in it, and the two rows are
 * already written above in visual_assets.c's own form.
 *
 * NOTHING ELSE WAS ADDED. Letters, space and '!' complete the ASCII the screens
 * need; every other character a future resource might use still draws the box,
 * which is the honest fallback rather than a guessed shape.
 */

static const uint8_t placeholder_space_rows[7] = {
    0u, 0u, 0u, 0u, 0u, 0u, 0u
};

static const uint8_t placeholder_unknown_rows[7] = {
    0x0eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1fu
};

/*
 * One step of a UTF-8 walk.
 *
 * Returns the number of bytes consumed, or 0 at the end of the string or on a
 * malformed sequence. A malformed byte is reported as one byte so the caller
 * always makes progress and can never loop forever.
 */
static int ui_utf8_step(const char *text, uint32_t *codepoint)
{
    const unsigned char *bytes = (const unsigned char *)text;
    uint32_t value;
    int length;
    int i;

    if (text == NULL || codepoint == NULL || bytes[0] == 0x00u)
        return 0;
    if (bytes[0] < 0x80u) {
        *codepoint = bytes[0];
        return 1;
    }
    if ((bytes[0] & 0xe0u) == 0xc0u) {
        value = bytes[0] & 0x1fu;
        length = 2;
    } else if ((bytes[0] & 0xf0u) == 0xe0u) {
        value = bytes[0] & 0x0fu;
        length = 3;
    } else if ((bytes[0] & 0xf8u) == 0xf0u) {
        value = bytes[0] & 0x07u;
        length = 4;
    } else {
        *codepoint = 0xfffdu;
        return 1;
    }
    for (i = 1; i < length; ++i) {
        if ((bytes[i] & 0xc0u) != 0x80u) {
            *codepoint = 0xfffdu;
            return 1;
        }
        value = (value << 6) | (bytes[i] & 0x3fu);
    }
    *codepoint = value;
    return length;
}

/*
 * Thai CLUSTER boundaries.
 *
 * Thai vowel signs and tone marks stack around their consonant and reorder, so
 * a cluster is not one codepoint. The generator already shaped each cluster and
 * stored it under its full UTF-8 bytes, which means the runtime has to agree
 * with the generator about where a cluster ends. These two predicates encode the
 * same rule the generator uses:
 *
 *   - a Thai LEADING vowel (U+0E40..U+0E44) is written before its consonant and
 *     belongs to the cluster that FOLLOWS it;
 *   - a Thai COMBINING mark attaches to the codepoint before it.
 *
 * This is a targeted rule, not a Unicode property table. It covers Thai and CJK,
 * which is exactly what the three temporary tables contain. A production
 * implementation would need real Unicode general-category data; saying so here
 * is why the rule is written as two small explicit predicates.
 */
static int ui_is_thai_leading_vowel(uint32_t codepoint)
{
    return codepoint >= 0x0e40u && codepoint <= 0x0e44u;
}

static int ui_is_thai_combining_mark(uint32_t codepoint)
{
    return codepoint == 0x0e31u
        || (codepoint >= 0x0e34u && codepoint <= 0x0e3au)
        || (codepoint >= 0x0e47u && codepoint <= 0x0e4eu);
}

/*
 * One laid-out character.
 *
 * A character takes one of two paths:
 *
 *  - ASCII keeps the existing 5x7 font at a 6 px advance, exactly as before.
 *  - Anything else is looked up in the script font as a whole CLUSTER, so a Thai
 *    base plus its marks is measured and drawn as the single pre-shaped unit the
 *    generator stored. A cluster the table does not have keeps the 6 px advance
 *    and draws the unknown-glyph box, which is the same thing the 5x7 font
 *    always did for a character it could not draw.
 */
typedef struct UiCharStep {
    uint32_t codepoint;
    int bytes;    /* UTF-8 bytes consumed */
    int advance;  /* horizontal pixels consumed, including the trailing gap */
    int script;   /* 1 when a script glyph was found and will be drawn */
} UiCharStep;

static void ui_char_step(const char *text, UiCharStep *step)
{
    const BounceScriptMetrics *metrics = bounce_script_font_metrics();
    const char *cursor = text;
    uint32_t codepoint = 0u;
    int total = 0;
    int leading_vowel;

    step->codepoint = 0u;
    step->bytes = 0;
    step->advance = 6;
    step->script = 0;
    if (text == NULL)
        return;

    total = ui_utf8_step(cursor, &codepoint);
    if (total == 0)
        return;

    /*
     * Grow the cluster. A leading vowel pulls in the consonant that follows it;
     * otherwise the base codepoint pulls in any combining marks after it.
     */
    leading_vowel = ui_is_thai_leading_vowel(codepoint);
    step->codepoint = codepoint;
    if (leading_vowel) {
        int extra;

        while (ui_is_thai_leading_vowel(codepoint)) {
            extra = ui_utf8_step(cursor + total, &codepoint);
            if (extra == 0)
                break;
            total += extra;
        }
        if (codepoint != 0u && !ui_is_thai_leading_vowel(codepoint)) {
            /* the consonant is the visual base; keep any marks after it */
            for (;;) {
                uint32_t mark = 0u;
                int extra = ui_utf8_step(cursor + total, &mark);

                if (extra == 0 || !ui_is_thai_combining_mark(mark))
                    break;
                total += extra;
            }
        }
    } else {
        for (;;) {
            uint32_t mark = 0u;
            int extra = ui_utf8_step(cursor + total, &mark);

            if (extra == 0 || !ui_is_thai_combining_mark(mark))
                break;
            total += extra;
        }
    }
    step->bytes = total;

    if (codepoint < 0x80u || metrics->advance <= 0)
        return;
    {
        const BounceScriptGlyph *glyph =
            bounce_script_font_lookup(cursor, total);

        if (glyph != NULL) {
            step->script = 1;
            step->advance = metrics->advance;
        }
    }
    (void)leading_vowel;
}

static int placeholder_text_width(const char *text)
{
    const char *cursor = text;
    int width = 0;

    if (text == NULL)
        return 0;
    /*
     * Pure Latin still returns exactly strlen(text) * 6 - 1, so every existing
     * English and Indonesian screen is laid out bit for bit as it was before the
     * script font was added.
     */
    while (*cursor != '\0') {
        UiCharStep step;

        ui_char_step(cursor, &step);
        if (step.bytes == 0)
            break;
        width += step.advance;
        cursor += step.bytes;
    }
    if (width == 0)
        return 0;
    return width - 1;
}

static int placeholder_text_index(char character)
{
    if (character >= 'a' && character <= 'z')
        character = (char)(character - 'a' + 'A');
    if (character >= 'A' && character <= 'Z')
        return (int)(character - 'A');
    /*
     * V-2 -- the measured remainder of the ASCII the screens paint. '!' is the
     * only one a SHIPPED RESOURCE reaches: lang.th-TH's GAME_END_CONGRATS,
     * LEVEL_COMPLETED and NEW_HIGH_SCORE each end in it. The digits and the
     * three punctuation marks come from this shell's own native strings.
     */
    switch (character) {
        case '!': return 26;
        case '0': return 27;
        case '1': return 28;
        case '2': return 29;
        case '3': return 30;
        case '4': return 31;
        case '5': return 32;
        case '6': return 33;
        case '7': return 34;
        case '8': return 35;
        case '9': return 36;
        case '.': return 37;
        case ',': return 38;
        case '*': return 39;
        default: break;
    }
    return -1;
}

/*
 * V-2 -- A COUNT OF THE BOXES THIS SHELL ACTUALLY PAINTED.
 *
 * Incremented at the one place placeholder_unknown_rows is selected, and nowhere
 * else, so it counts boxes that were really drawn -- not strings that merely
 * contain a character the font lacks. It is an observer: the draw path is
 * untouched, the box is still painted, and nothing consults this number to
 * decide what to render. It exists because "Thai renders now" is a claim that
 * is only worth anything if it is measured over every screen and every shipped
 * string rather than asserted.
 */
static unsigned long ui_shell_unknown_glyph_draws;

int bounce_ui_shell_unknown_glyph_draws(void)
{
    return ui_shell_unknown_glyph_draws == 0ul ? 0 : 1;
}

void bounce_ui_shell_reset_unknown_glyph_draws(void)
{
    ui_shell_unknown_glyph_draws = 0ul;
}

int bounce_ui_shell_can_draw_ascii(int codepoint)
{
    if (codepoint == ' ')
        return 1;
    if (codepoint < 0 || codepoint > 0x7f)
        return 0;  /* the box: a script cluster uses the table, not this font */
    /* placeholder_text_index() returns the row index, and -1 for no glyph. */
    return placeholder_text_index((char)codepoint) >= 0 ? 1 : 0;
}

static int draw_placeholder_text(
    BounceRenderer *renderer,
    int x,
    int y,
    const char *text,
    uint32_t color
)
{
    const char *cursor;
    int pen;

    if (renderer == NULL || text == NULL)
        return -1;
    cursor = text;
    pen = x;
    while (*cursor != '\0') {
        const uint8_t *rows;
        UiCharStep step;
        int glyph_index;
        int row;
        int column;

        ui_char_step(cursor, &step);
        if (step.bytes == 0)
            break;

        if (step.script) {
            const BounceScriptGlyph *glyph =
                bounce_script_font_lookup(cursor, step.bytes);

            if (glyph == NULL)
                return -1;
            for (row = 0; row < BOUNCE_SCRIPT_CELL_HEIGHT; ++row) {
                for (column = 0; column < BOUNCE_SCRIPT_CELL_WIDTH; ++column) {
                    if ((glyph->rows[row]
                         & (uint16_t)(1u << (BOUNCE_SCRIPT_CELL_WIDTH - 1
                                             - column))) != 0u
                        && bounce_renderer_fill_rect(
                            renderer,
                            pen + column,
                            y + row,
                            1,
                            1,
                            color
                        ) != 0)
                        return -1;
                }
            }
            cursor += step.bytes;
            pen += step.advance;
            continue;
        }

        /* The 5x7 path, unchanged apart from tracking the pen explicitly. */
        if (step.codepoint == (uint32_t)' ') {
            rows = placeholder_space_rows;
            glyph_index = -1;
        } else if (step.codepoint > 0x7fu) {
            /*
             * V-2 -- A SCRIPT CLUSTER THAT MISSED ITS TABLE MUST BOX, NOT DRAW A
             * LETTER.
             *
             * The lookup below casts to char. For a Thai codepoint that cast is
             * not a no-op: U+0E41..U+0E5A truncate to 'A'..'Z'. A Thai cluster
             * absent from script_font.c would therefore have been painted as an
             * unrelated Latin letter rather than flagged, which would turn the
             * coverage check into a check that cannot fail.
             *
             * This is currently unreachable -- the Thai table is complete against
             * lang.th-TH, which is what V-2's verifier asserts -- but it is a
             * silent wrong-glyph path in a font fallback, and the whole point of
             * the fallback is to be visibly wrong instead of quietly plausible.
             * ASCII is routed to the font and everything above it boxes.
             *
             * `rows` is set here rather than left to the common tail, because
             * this branch cannot fall through to it.
             */
            glyph_index = -1;
            rows = placeholder_unknown_rows;
        } else {
            glyph_index = placeholder_text_index((char)step.codepoint);
            rows = glyph_index >= 0
                ? placeholder_glyph_rows[glyph_index]
                : placeholder_unknown_rows;
        }
        /*
         * V-2: counted here, AFTER all three branches, so it covers a missing
         * script cluster as well as a missing ASCII glyph. It was briefly placed
         * inside the ASCII branch, where it missed every non-ASCII fallback --
         * and a counter that cannot see half the cases it exists to catch is
         * worse than no counter, because the check reading it would report a
         * pass.
         *
         * THE SPACE IS EXCLUDED, and it has to be: the space branch above sets
         * glyph_index to -1 as a "use placeholder_space_rows" marker rather than
         * as a failure. Counting it would report a box on every screen that draws
         * a single word. A space draws a real, blank cell and is not a fallback.
         */
        if (glyph_index < 0 && step.codepoint != (uint32_t)' ')
            ++ui_shell_unknown_glyph_draws;
        for (row = 0; row < 7; ++row) {
            for (column = 0; column < 5; ++column) {
                if ((rows[row] & (uint8_t)(1u << (4 - column))) != 0u
                    && bounce_renderer_fill_rect(
                        renderer,
                        pen + column,
                        y + row,
                        1,
                        1,
                        color
                    ) != 0)
                    return -1;
            }
        }
        cursor += step.bytes;
        pen += step.advance;
    }
    return 0;
}

static int draw_centered_placeholder_text(
    BounceRenderer *renderer,
    int y,
    const char *text,
    uint32_t color
)
{
    int width;

    if (renderer == NULL || text == NULL)
        return -1;
    width = placeholder_text_width(text);
    return draw_placeholder_text(
        renderer,
        ((int)BOUNCE_SURFACE_WIDTH - width) / 2,
        y,
        text,
        color
    );
}

static int draw_native_digit(
    BounceRenderer *renderer,
    int x,
    int y,
    int digit,
    uint32_t color
);
static int draw_native_number(
    BounceRenderer *renderer,
    int x,
    int y,
    int value,
    uint32_t color
);

/*
 * D-15 / J-04 -- SUBSTITUTE THE THREE CONTROL PLACEHOLDERS.
 *
 * JAVA EVIDENCE, BounceGame.java:167-175 in full:
 *
 *     public void ShowInstructions() {
 *         this.form = new Form(
 *             Translation.sprintf_translated(Translation.INSTRUCTIONS));
 *         String[] arrayOfString = {
 *             this.v.getKeyName(this.v.getKeyCode(2)),
 *             this.v.getKeyName(this.v.getKeyCode(5)),
 *             this.v.getKeyName(this.v.getKeyCode(1))};
 *         this.form.append(Translation.sprintf_translated(
 *             Translation.MORE_INSTRUCTIONS, arrayOfString));
 *         ...
 *
 * and Translation.java:73-79 decides what the three arguments become:
 *
 *     if (format.length == 1) { str = sprintf_translated(str, "%U", format[0]); }
 *     else {
 *         for (byte b = 0; b < format.length; b++)
 *             str = sprintf_translated(str, "%" + b + "U", format[b]);
 *     }
 *
 * format.length is 3, so the mapping is positional and unambiguous:
 *
 *     %0U  <-  getKeyName(getKeyCode(2))   MIDP LEFT
 *     %1U  <-  getKeyName(getKeyCode(5))   MIDP FIRE
 *     %2U  <-  getKeyName(getKeyCode(1))   MIDP UP
 *
 * WHICH PHYSICAL KEY EACH GAME ACTION MEANT IS NOT RECOVERED, and this does not
 * guess it. Canvas.getGameAction delegates to KeyMap and then to ITUKeyMap,
 * whose tables are built by ACC_NATIVE methods whose JNI library is absent from
 * the repository -- the same gap game.h:426-432 records for the completion
 * producer and input_state.h:11-14 records for the development aliases. What
 * IS known is which BINDING each game action drives, because that is in the
 * recovered Java:
 *
 *   e.java:399-401  case 2 (LEFT): this.aq.c(1) -> w |= 1
 *   f.java:808-809  (w & 1) != 0 && this.l > -b1  ->  this.l -= 6   LEFTWARD
 *   e.java:403-404  case 5 (FIRE): this.aq.c(2) -> w |= 2
 *   f.java:806-807  (w & 2) != 0 && this.l < b1   ->  this.l += 6   RIGHTWARD
 *   e.java:393-394  case 1 (UP): this.aq.c(8) -> w |= 8
 *   f.java:821-827  (this.m && (w & 8))        ->  the jump
 *
 * so the three actions are the native bindings app_step_horizontal() reads as
 * input.left, input.right and input.up (vertical_slice.c:4625-4692). Those are
 * bound in x11_keysym_to_source() to XK_Left / XK_a, XK_Right / XK_d and
 * XK_Up / XK_w. The labels substituted here are therefore the localized
 * direction names the Settings > Input page ALREADY uses for exactly those
 * three rows -- BOUNCE_LOCALE_LEFT, BOUNCE_LOCALE_RIGHT and BOUNCE_LOCALE_UP --
 * so the Instructions page and the Input page cannot disagree about which key
 * does what, and every language gets a translated name without adding a single
 * new locale key.
 *
 * WHY THIS RUNS AT RENDER TIME AND NOT IN LOCALE.C. The shipped resource must
 * stay verbatim: verify_ui_shell asserts that the raw text still contains "%0U",
 * "%1U" and "%2U" (vertical_slice.c:8432-8434), which is the correct statement
 * about the RESOURCE, and bounce_ui_shell_instructions_text() therefore still
 * returns the untranslated string. Substitution is a property of drawing, and
 * the replacement must happen BEFORE wrapping and measuring, because "%0U" is
 * three characters and "LEFT" is four: laying the page out on the raw string
 * and substituting afterwards would wrap at the wrong offsets.
 *
 * UNKNOWN, EXPLICITLY: the original device's own key names. On a Nokia those
 * come from getKeyName() and are whatever the platform's KeyMap reports, which
 * this repository cannot supply. The text below is therefore the NATIVE
 * binding's name, not the original's.
 */
static int ui_compose_instructions(
    const char *text,
    char *out,
    size_t out_size
)
{
    const char *names[3];
    size_t written = 0u;
    int slot;

    if (text == NULL || out == NULL || out_size == 0u)
        return -1;

    names[0] = tr(BOUNCE_LOCALE_LEFT);   /* %0U, game action 2, LEFT  */
    names[1] = tr(BOUNCE_LOCALE_RIGHT);  /* %1U, game action 5, FIRE  */
    names[2] = tr(BOUNCE_LOCALE_UP);     /* %2U, game action 1, UP    */
    for (slot = 0; slot < 3; ++slot) {
        if (names[slot] == NULL)
            return -1;
    }

    out[0] = '\0';
    while (*text != '\0') {
        if (text[0] == '%' && text[1] >= '0' && text[1] <= '2'
            && text[2] == 'U') {
            const char *source = names[text[1] - '0'];
            while (*source != '\0') {
                if (written + 1u >= out_size)
                    return -1;
                out[written] = *source;
                ++written;
                ++source;
            }
            text += 3;
            continue;
        }
        if (written + 1u >= out_size)
            return -1;
        out[written] = *text;
        ++written;
        ++text;
    }
    out[written] = '\0';
    return 0;
}

/*
 * PROVISIONAL NATIVE INSTRUCTIONS WRAP — NOT ORIGINAL MIDP LAYOUT.
 *
 * Space-delimited wrapping, with two changes needed by the temporary CN / TW /
 * TH test strings:
 *
 *  - The line is measured with real per-cluster advances, so a CJK or Thai line
 *    is the width it actually occupies rather than a byte count.
 *  - CJK and Thai have no spaces, so the wrap also breaks at cluster boundaries.
 *    Previously a space-free string was treated as one giant word and the render
 *    returned -1, which would have taken the whole Instructions page down.
 *
 * The page still draws a BOUNDED excerpt. No source-verified native scroll or
 * paging exists, and the taller CJK and Thai cells mean fewer lines fit, which
 * is a layout consequence rather than a new policy.
 */
/*
 * INSTRUCTIONS PAGE GEOMETRY.
 *
 * BODY_Y0 AND THE EIGHT VISIBLE LINES. The body starts at y=21, under the title
 * at y=7, and ESC BACK is drawn at y=118. The line pitch is
 * bounce_script_font_metrics()->cell_height, which is BOUNCE_SCRIPT_CELL_HEIGHT
 * (12) for EVERY language -- for English and Indonesian, which have no script
 * set, the metric that goes to zero is `advance`, not `cell_height`. So a line
 * occupies twelve rows, and the last line that clears ESC BACK is the one
 * starting at y=105: 21, 33, 45, 57, 69, 81, 93, 105. That is eight.
 *
 * MEASURED, BEFORE THE SCROLLBAR EXISTED. With the body drawn from y=21 and no
 * window, every language lost four lines off the bottom of the 128-row surface,
 * and the twelve-line bound silently discarded more on top of that:
 *
 *     language   lines needed   drawn   fully visible   off-surface   discarded
 *     EN              12          12          8              4             0
 *     zh-CN           16          12          8              4             4
 *     zh-TW           16          12          8              4             4
 *     th-TH           13          12          8              4             1
 *     ID              14          12          8              4             2
 *
 * MAX_WIDTH IS 124, NOT THE 126 THIS PAGE USED, AND THAT OVERRIDES A
 * DELIBERATE EARLIER DECISION. The removed note said:
 *
 *     The old wrap budgeted 21 CHARACTER columns at a 6 px advance, which is
 *     126 px. Budgeting 126 px here reproduces that limit exactly for Latin, so
 *     every English and Indonesian line breaks in precisely the same place it
 *     did before, while a script line gets the same pixel budget measured in its
 *     own wider advances.
 *
 * That reasoning is sound and it is why 126 was chosen: 126 px was 21 Latin
 * columns, and keeping it kept the Latin line breaks byte-identical to the
 * wrapper they replaced. This page now needs a two-pixel strip the text must not
 * enter, because the scrollbar occupies columns 126 and 127 and the body starts
 * at x=2, so 2 + 124 = 126 is the widest measure that leaves it clear.
 *
 * SO LATIN LINES NOW BREAK SLIGHTLY EARLIER. That is a real consequence and it is
 * recorded rather than absorbed: the text is unchanged, the measure is two pixels
 * narrower, and English wraps one character sooner on some lines. It is the same
 * direction the source goes -- MIDP's Form RESERVES a strip for its scrollbar,
 * so Java's text area was narrower than its screen too, and there is no way to
 * have both a scrollbar and the full measure on 128 pixels.
 *
 * The removed note also recorded why this page bounded its excerpt rather than
 * failing: "the Indonesian Instructions text needs 14 lines at this width and
 * the budget is 12, so the page returned -1 and took the application down. Every
 * existing check rendered this page in English only, and English happens to fit
 * in exactly 12." That fault is gone for a better reason than the bound was --
 * the text is no longer excerpted at all -- but the rule it taught stands, so
 * instructions_walk() refuses rather than lies in the same way: it walks the whole
 * text and reports the count, and it returns -1 rather than drawing a partial
 * window if anything about the layout is inconsistent.
 */
#define BOUNCE_UI_INSTRUCTIONS_BODY_X 2
#define BOUNCE_UI_INSTRUCTIONS_BODY_Y0 21
#define BOUNCE_UI_INSTRUCTIONS_MAX_WIDTH 124

/*
 * The Instructions wrap, as ONE function that can either measure or draw.
 *
 * `renderer` is NULL to measure only. `visible` lines starting at `first` are
 * drawn; the rest of the text is walked but not painted. `*total_out` always
 * receives the number of lines the whole text needs, whether or not any of them
 * are shown, which is what lets the input path clamp a scroll and the scrollbar
 * size a thumb from the same number the drawing used.
 *
 * ONE WRAP, TWO USES. The wrap rules -- pending spaces, whole-word breaks for
 * Latin, per-cluster breaks for CJK and Thai, hard breaks at '\n', the
 * single-character fallback for a word wider than the measure -- are unchanged
 * from the version this replaces, and they live here and nowhere else. A second
 * copy for the counting path would be a second thing to keep correct, and would
 * be exactly the kind of drift that makes a scrollbar jump past a line.
 */
static int instructions_walk(
    BounceRenderer *renderer,
    const char *text,
    uint32_t color,
    unsigned int first,
    unsigned int visible,
    unsigned int *total_out
)
{
    const BounceScriptMetrics *metrics = bounce_script_font_metrics();
    const int max_width = BOUNCE_UI_INSTRUCTIONS_MAX_WIDTH;
    const int line_height = metrics->cell_height > 0 ? metrics->cell_height : 7;
    char line[160];
    const char *cursor = text;
    int line_length = 0;
    int line_width = 0;
    unsigned int line_count = 0u;
    int pending_space = 0;
    int paint;

    if (text == NULL || total_out == NULL)
        return -1;
    *total_out = 0u;
    if (renderer == NULL)
        visible = 0u;   /* measuring: nothing is painted, whatever was asked for */

    while (*cursor != '\0') {
        UiCharStep step;
        int word_width;
        int need_space;

        ui_char_step(cursor, &step);
        if (step.bytes == 0)
            break;

        if (step.codepoint == (uint32_t)'\n') {
            paint = renderer != NULL
                && line_count >= first
                && line_count - first < visible;
            if (paint && line_length > 0
                && draw_placeholder_text(
                        renderer,
                        BOUNCE_UI_INSTRUCTIONS_BODY_X,
                        BOUNCE_UI_INSTRUCTIONS_BODY_Y0
                            + (int)line_count * line_height,
                        line,
                        color
                    ) != 0)
                return -1;
            ++line_count;
            line_length = 0;
            line_width = 0;
            pending_space = 0;
            cursor += step.bytes;
            continue;
        }

        if (step.codepoint == (uint32_t)' ') {
            /*
             * A space is only PENDING here. Committing it immediately would
             * spend 6 px that the following word may not need, which shortens
             * every line by one character and pushes the Indonesian Instructions
             * text past the 12-line bound. This mirrors the original behaviour,
             * where the separator was written only when the word also fitted.
             */
            if (line_length > 0)
                pending_space = 1;
            cursor += step.bytes;
            continue;
        }

        /*
         * How wide is this character together with the following run of
         * non-space characters? For a script string each cluster is measured on
         * its own, which is already correct; for Latin the existing word
         * wrapping has to keep whole words together.
         */
        word_width = step.advance;
        if (step.script == 0) {
            const char *scan = cursor + step.bytes;
            int word_end = step.advance;

            while (*scan != '\0') {
                UiCharStep inner;

                ui_char_step(scan, &inner);
                if (inner.bytes == 0 || inner.codepoint == (uint32_t)' ')
                    break;
                word_end += inner.advance;
                scan += inner.bytes;
            }
            word_width = word_end;
            /*
             * A single "word" wider than a whole line cannot be kept together:
             * CJK and Thai have no spaces, so without this the wrapper would try
             * to fit the whole remaining paragraph on one line and end up
             * emitting one character per line. Break per character instead.
             */
            if (word_width > max_width)
                word_width = step.advance;
        }

        need_space = pending_space && line_length > 0 ? 6 : 0;
        if (line_length > 0 && line_width + need_space + word_width > max_width) {
            paint = renderer != NULL
                && line_count >= first
                && line_count - first < visible;
            if (paint
                && draw_placeholder_text(
                        renderer,
                        BOUNCE_UI_INSTRUCTIONS_BODY_X,
                        BOUNCE_UI_INSTRUCTIONS_BODY_Y0
                            + (int)line_count * line_height,
                        line,
                        color
                    ) != 0)
                return -1;
            ++line_count;
            line_length = 0;
            line_width = 0;
            need_space = 0;
        }
        if (step.bytes <= 0
            || (size_t)(line_length + need_space + step.bytes) >= sizeof line)
            return -1;
        if (need_space != 0) {
            line[line_length++] = ' ';
            line[line_length] = '\0';
            line_width += need_space;
        }
        pending_space = 0;
        memcpy(&line[line_length], cursor, (size_t)step.bytes);
        line_length += step.bytes;
        line[line_length] = '\0';
        line_width += step.advance;
        cursor += step.bytes;
    }

    if (line_length > 0) {
        paint = renderer != NULL
            && line_count >= first
            && line_count - first < visible;
        if (paint
            && draw_placeholder_text(
                    renderer,
                    BOUNCE_UI_INSTRUCTIONS_BODY_X,
                    BOUNCE_UI_INSTRUCTIONS_BODY_Y0
                        + (int)line_count * line_height,
                    line,
                    color
                ) != 0)
            return -1;
        ++line_count;
    }
    *total_out = line_count;
    return 0;
}

/*
 * The composed Instructions text, or NULL if it will not compose.
 *
 * Both the renderer and the scroll input need the same string, and composing it
 * twice is cheaper than disagreeing about it: the line count the input clamps
 * against must be the count the renderer actually draws.
 */
static const char *instructions_composed(char *buffer, size_t size)
{
    if (ui_compose_instructions(
            bounce_ui_shell_instructions_prefix(),
            buffer,
            size
        ) != 0)
        return NULL;
    return buffer;
}

/* The first line index that puts the END of the text on screen. */
unsigned int bounce_ui_shell_instructions_max_scroll(void)
{
    char buffer[1024];
    unsigned int total = 0u;

    if (instructions_composed(buffer, sizeof buffer) == NULL)
        return 0u;
    if (instructions_walk(NULL, buffer, 0u, 0u, 0u, &total) != 0)
        return 0u;
    if (total <= BOUNCE_UI_INSTRUCTIONS_VISIBLE_LINES)
        return 0u;
    return total - BOUNCE_UI_INSTRUCTIONS_VISIBLE_LINES;
}

/*
 * How many lines the active language's Instructions text needs, wrapped by the
 * real wrapper at the real measure. Exposed because the scroll bound is only
 * meaningful next to the line count it is derived from, and a verifier that had
 * to recompute the wrap would be testing a second implementation of it.
 */
unsigned int bounce_ui_shell_instructions_total_lines(void)
{
    char buffer[1024];
    unsigned int total = 0u;

    if (instructions_composed(buffer, sizeof buffer) == NULL)
        return 0u;
    if (instructions_walk(NULL, buffer, 0u, 0u, 0u, &total) != 0)
        return 0u;
    return total;
}

/*
 * The shared scrollbar, reachable from a verifier.
 *
 * A TEST SEAM, and labelled as one. ui_draw_scrollbar() is file-static because
 * nothing in the shell needs to call it with arbitrary numbers, but its
 * "draws nothing when the content fits" rule and its thumb arithmetic are
 * exactly the two things the Instructions scrollbar must get right, and neither
 * can be reached through a page: every shipped language has text longer than the
 * window, so no page ever takes the "fits" branch, and the Theme page's count is
 * fixed at thirteen.
 *
 * This changes no rendering decision. It is the same V-2 pattern as
 * bounce_ui_shell_can_draw_ascii(): an observer's window onto production code so
 * a check can exercise a branch the pages do not reach.
 */
int bounce_ui_shell_draw_scrollbar_for_test(
    BounceRenderer *renderer,
    int x,
    int track_y,
    int track_h,
    unsigned int count,
    unsigned int visible,
    unsigned int first
)
{
    return ui_draw_scrollbar(
        renderer, x, track_y, track_h, count, visible, first);
}

/*
 * Draw one centred line, or two when the text is wider than the 125 px the
 * placeholder font can occupy inside the 128 px surface. Splitting happens at the
 * space nearest the middle, so a translation is never truncated or clipped.
 * This is native layout only; the wording itself is never altered.
 */
static int draw_centered_placeholder_wrapped(
    BounceRenderer *renderer,
    int y,
    const char *text,
    uint32_t color
)
{
    const int max_width = 125;
    char buffer[160];

    if (renderer == NULL || text == NULL)
        return -1;
    if (placeholder_text_width(text) <= max_width)
        return draw_centered_placeholder_text(renderer, y, text, color);

    /*
     * Find the split point. A space is preferred and keeps the previous
     * behaviour, but CJK and Thai text has no spaces at all, so the scan also
     * considers every cluster boundary and keeps the last one that leaves both
     * halves inside the 125 px budget. Without that fallback a Thai string such
     * as the remapping notice has no space to break on and used to fail the
     * whole render.
     */
    {
        const char *cursor;
        const char *best_space = NULL;
        const char *best_cluster = NULL;
        const char *best;
        int total = placeholder_text_width(text);
        int length;

        for (cursor = text; *cursor != '\0'; ) {
            UiCharStep step;
            int tail;

            ui_char_step(cursor, &step);
            if (step.bytes == 0)
                break;
            if (cursor != text
                && cursor[-1] == ' '
                && (tail = placeholder_text_width(cursor)) <= max_width
                && total - tail <= max_width)
                best_space = cursor;
            if (step.codepoint != (uint32_t)' '
                && (tail = placeholder_text_width(cursor + step.bytes))
                    <= max_width
                && total - tail <= max_width)
                best_cluster = cursor;
            cursor += step.bytes;
        }
        best = best_space != NULL ? best_space : best_cluster;
        if (best == NULL || best == text)
            return -1;
        length = (int)(best - text);
        if (length <= 0 || (size_t)length >= sizeof buffer)
            return -1;
        memcpy(buffer, text, (size_t)length);
        buffer[length] = '\0';
        if (draw_centered_placeholder_text(renderer, y, buffer, color) != 0)
            return -1;
        if (*best == ' ')
            ++best;
        return draw_centered_placeholder_text(renderer, y + 10, best, color);
    }
}

const char *bounce_ui_shell_menu_label(unsigned int row)
{
    if (row >= BOUNCE_MENU_ROW_COUNT)
        return NULL;
    return tr(menu_label_keys[row]);
}

const char *bounce_ui_shell_action_label(BounceMenuAction action)
{
    switch (action) {
        case BOUNCE_MENU_ACTION_CONTINUE:
            return tr(BOUNCE_LOCALE_CONTINUE);
        case BOUNCE_MENU_ACTION_NEW_GAME:
            return tr(BOUNCE_LOCALE_NEW_GAME);
        case BOUNCE_MENU_ACTION_HIGH_SCORE:
            return tr(BOUNCE_LOCALE_HIGH_SCORE);
        case BOUNCE_MENU_ACTION_INSTRUCTIONS:
            return tr(BOUNCE_LOCALE_INSTRUCTIONS);
        case BOUNCE_MENU_ACTION_SETTINGS:
            /* NATIVE EXTENSION. */
            return tr(BOUNCE_LOCALE_SETTINGS);
        case BOUNCE_MENU_ACTION_ABOUT:
            /* NATIVE EXTENSION. */
            return tr(BOUNCE_LOCALE_ABOUT);
        case BOUNCE_MENU_ACTION_EXIT:
            return tr(BOUNCE_LOCALE_EXIT);
        case BOUNCE_MENU_ACTION_CONTINUE_UNAVAILABLE:
            return tr(BOUNCE_LOCALE_CONTINUE_UNAVAILABLE);
        case BOUNCE_MENU_ACTION_START_LEVEL:
            return tr(BOUNCE_LOCALE_START_LEVEL);
        case BOUNCE_MENU_ACTION_NONE:
        default:
            return tr(BOUNCE_LOCALE_NO_ACTION);
    }
}

const char *bounce_ui_shell_instructions_text(void)
{
    /*
     * The wrapped Instructions paragraph. In English this is the recovered
     * lang.xx text unchanged; Indonesian is the native translation of it.
     */
    return bounce_locale_instructions_text(bounce_locale_active_language());
}

int bounce_ui_shell_instructions_composed(
    char *out,
    size_t out_size
)
{
    return ui_compose_instructions(
        bounce_ui_shell_instructions_prefix(),
        out,
        out_size
    );
}

const char *bounce_ui_shell_instructions_prefix(void)
{
    /*
     * The bounded portion the screen actually draws. The full paragraph is
     * longer than the 12-line page, and no source-verified native scroll or
     * layout policy exists, so the same bounded excerpt is kept per language
     * rather than inventing paging.
     */
    return bounce_locale_text(
        bounce_locale_active_language(),
        BOUNCE_LOCALE_INSTRUCTIONS_PREFIX);
}

/*
 * STEP 37 PERSIST -- SUPERSEDED as a description of the screen.
 *
 * This returns the fixed fresh-install sentinel, and the High Score screen no
 * longer draws it: render_high_score() now renders
 * bounce_app_flow_high_score(flow), which is the real record.
 *
 * The declaration is kept because it is public API in ui_shell.h, and the value
 * it returns is still the correct answer to "what does a never-played game
 * report". The verification that used to compare this string against "0" now
 * asserts bounce_app_flow_high_score() == 0 at fresh init, which is the
 * invariant that actually matters.
 */
const char *bounce_ui_shell_high_score_value(void)
{
    return high_score_value;
}

/*
 * D-23 -- there is NO High Score notice any more, and that is the source's
 * shape.
 *
 * BounceGame.java:159-165 is the whole screen:
 *
 *     public void ShowHighScore() {
 *         this.form = new Form(
 *             Translation.sprintf_translated(Translation.HIGH_SCORE));
 *         this.form.append(String.valueOf(this.HighScore));
 *         this.form.addCommand(this.commandBack);
 *         this.form.setCommandListener(this);
 *         this.display.setCurrent(this.form);
 *     }
 *
 * Two Items -- the title and the number -- plus the Back command. Native used
 * to draw a third line reading BOUNCE_LOCALE_NO_RMS_VALUE, and this accessor
 * existed only to expose it. It is removed rather than left returning an unused
 * row, because dead exported API is worse than a removed one; the locale row
 * BOUNCE_LOCALE_NO_RMS_VALUE stays in the table because locale.c records that
 * the table is POSITIONAL and removing a key would mean renumbering it.
 *
 * The notice was also FACTUALLY WRONG by the time it was written: it claimed the
 * value was not from an RMS record, while the value drawn directly above it had
 * been the persisted, RMS-derived high score since STEP 37.
 *
 * Returns NULL, which is how a caller can tell there is no notice to draw.
 */
const char *bounce_ui_shell_high_score_notice(void)
{
    return NULL;
}

/*
 * NATIVE EXTENSION: mouse hit testing.
 *
 * Every geometry below is copied from the renderer that draws it, so a click and
 * a press always address the same row. The tables are declared next to the hit
 * test rather than shared with the renderers to keep the renderers untouched.
 */
static bool ui_point_in_row(int y, int row_y, int row_h)
{
    return y >= row_y && y < row_y + row_h;
}

/*
 * Apply a click by synthesizing the UI edge a keyboard press would have
 * produced and handing it to the ordinary handler. Returns 1 when an edge was
 * produced, 0 when the click was outside every interactive element.
 */
int bounce_ui_shell_handle_click(
    BounceAppFlow *flow,
    int x,
    int y
)
{
    static const int menu_row_y[BOUNCE_MENU_ROW_COUNT] = {
        30, 42, 54, 66, 78, 90, 102
    };
    static const int settings_row_y[BOUNCE_SETTINGS_ROOT_ITEM_COUNT] = {
        28, 44, 60, 76, 92
    };
    static const int level_row_y[BOUNCE_APP_LAST_LEVEL_ID] = {
        24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104
    };
    unsigned int index;
    BounceUiInput input;

    if (flow == NULL)
        return 0;

    /*
     * The Back band. Every destination that already has a Back action draws its
     * hint on the same row, so one band covers all of them. The main menu has no
     * Back action in the recovered source (only Exit), so it is excluded.
     */
    if (y >= 112 && y < 128 && x >= 4 && x < 124
        && (flow->state == BOUNCE_APP_STATE_INSTRUCTIONS
            || flow->state == BOUNCE_APP_STATE_HIGH_SCORE
            || flow->state == BOUNCE_APP_STATE_LEVEL_SELECTION
            || flow->state == BOUNCE_APP_STATE_SETTINGS
            || flow->state == BOUNCE_APP_STATE_ABOUT
            || flow->state == BOUNCE_APP_STATE_ACTION_BOUNDARY
            || flow->state == BOUNCE_APP_STATE_GAMEPLAY_ENTRY)) {
        return bounce_ui_shell_handle_press(flow, BOUNCE_UI_INPUT_BACK) == 0
            ? 1
            : 0;
    }

    if (flow->state == BOUNCE_APP_STATE_MENU) {
        if (x < 12 || x >= 116)
            return 0;
        for (index = 0u; index < BOUNCE_MENU_ROW_COUNT; ++index) {
            unsigned int selection;

            if (!ui_point_in_row(y, menu_row_y[index], 12))
                continue;
            /* A disabled row, such as Continue, is not clickable. */
            if (!bounce_app_flow_menu_item_enabled(flow, index))
                return 0;
            /* Find the selection index that maps to this row. */
            for (selection = 0u;
                 selection < bounce_app_flow_menu_enabled_item_count(flow);
                 ++selection) {
                if (bounce_app_flow_menu_row_for_selection(flow, selection)
                    == index) {
                    flow->menu.selected_index = selection;
                    input = BOUNCE_UI_INPUT_SELECT;
                    return bounce_ui_shell_handle_press(flow, input) == 0
                        ? 1
                        : 0;
                }
            }
            return 0;
        }
        return 0;
    }

    if (flow->state == BOUNCE_APP_STATE_LEVEL_SELECTION) {
        unsigned int count = flow->level_selection.available_level_count;

        if (x < 4 || x >= 124 || count == 0u || count > BOUNCE_APP_LAST_LEVEL_ID)
            return 0;
        for (index = 0u; index < count; ++index) {
            if (!ui_point_in_row(y, level_row_y[index], 8))
                continue;
            /*
             * Select first, activate second. Clicking the row that is already
             * selected starts the level; any other click only moves the
             * highlight, so no level can be launched by a stray click.
             */
            if (flow->level_selection.selected_index == index) {
                input = BOUNCE_UI_INPUT_SELECT;
            } else {
                flow->level_selection.selected_index = index;
                return 1;
            }
            return bounce_ui_shell_handle_press(flow, input) == 0 ? 1 : 0;
        }
        return 0;
    }

    if (flow->state == BOUNCE_APP_STATE_SETTINGS) {
        if (x < 12 || x >= 116)
            return 0;
        if (bounce_app_flow_settings_page(flow) == BOUNCE_SETTINGS_PAGE_ROOT) {
            for (index = 0u; index < BOUNCE_SETTINGS_ROOT_ITEM_COUNT; ++index) {
                if (!ui_point_in_row(y, settings_row_y[index], 16))
                    continue;
                if (bounce_app_flow_settings_selected_index(flow) == index) {
                    input = BOUNCE_UI_INPUT_SELECT;
                } else {
                    flow->settings.selected_index = index;
                    return 1;
                }
                return bounce_ui_shell_handle_press(flow, input) == 0 ? 1 : 0;
            }
            return 0;
        }
        if (bounce_app_flow_settings_page(flow)
            == BOUNCE_SETTINGS_PAGE_LANGUAGE) {
            unsigned int count = bounce_app_flow_settings_list_count(flow);

            for (index = 0u; index < count; ++index) {
                if (!ui_point_in_row(y, 24 + (int)index * 14, 12))
                    continue;
                if (bounce_app_flow_settings_language_index(flow) == index) {
                    input = BOUNCE_UI_INPUT_SELECT;
                } else {
                    flow->settings.language_index = index;
                    return 1;
                }
                return bounce_ui_shell_handle_press(flow, input) == 0 ? 1 : 0;
            }
            return 0;
        }
        if (bounce_app_flow_settings_page(flow) == BOUNCE_SETTINGS_PAGE_THEME) {
            unsigned int count = bounce_app_flow_settings_list_count(flow);

            const unsigned int limit
                = count < BOUNCE_UI_THEME_COUNT ? count : BOUNCE_UI_THEME_COUNT;
            const unsigned int first
                = theme_first_row(limit, bounce_app_flow_settings_theme_index(flow));
            unsigned int slot;

            for (slot = 0u; slot < BOUNCE_UI_THEME_VISIBLE_ROWS; ++slot) {
                index = first + slot;
                if (index >= limit)
                    break;
                if (!ui_point_in_row(
                        y,
                        BOUNCE_UI_THEME_ROW_Y0
                            + (int)slot * BOUNCE_UI_THEME_ROW_PITCH,
                        BOUNCE_UI_THEME_ROW_HIT_H))
                    continue;
                if (bounce_app_flow_settings_theme_index(flow) == index) {
                    input = BOUNCE_UI_INPUT_SELECT;
                } else {
                    flow->settings.theme_index = index;
                    return 1;
                }
                return bounce_ui_shell_handle_press(flow, input) == 0 ? 1 : 0;
            }
            return 0;
        }
        return 0;
    }

    /*
     * Deliberately no gameplay mouse control. The recovered source defines no
     * pointer input at all, and the canonical gameplay path is driven by the
     * 40 ms timer and the key mapping, so adding pointer control here would be
     * an invented mechanic. Clicks during gameplay are ignored.
     */
    return 0;
}

int bounce_ui_shell_handle_number(
    BounceAppFlow *flow,
    unsigned int digit
)
{
    unsigned int index;
    unsigned int count;

    if (flow == NULL || digit > 9u)
        return 0;
    /* Digits mean nothing outside Level Select; ignore them everywhere else. */
    if (flow->state != BOUNCE_APP_STATE_LEVEL_SELECTION)
        return 0;

    count = flow->level_selection.available_level_count;
    if (count == 0u || count > BOUNCE_APP_LAST_LEVEL_ID)
        return 0;

    /* 1..9 select levels 1..9; 0 selects level 10, the conventional position. */
    index = (digit == 0u) ? 9u : digit - 1u;
    /* Never select a level that does not exist. */
    if (index >= count)
        return 0;
    flow->level_selection.selected_index = index;
    return 1;
}

int bounce_ui_shell_handle_press(
    BounceAppFlow *flow,
    BounceUiInput input
)
{
    if (flow == NULL)
        return -1;

    if (flow->state == BOUNCE_APP_STATE_SPLASH) {
        /* The original key path sets a threshold; transition occurs on Tick. */
        return bounce_app_flow_request_splash_skip(flow);
    }
    if (flow->state == BOUNCE_APP_STATE_INSTRUCTIONS) {
        if (input == BOUNCE_UI_INPUT_UP || input == BOUNCE_UI_INPUT_DOWN) {
            /*
             * SCROLL, ON THE SAME TWO KEYS THE THEME PAGE SCROLLS WITH.
             *
             * UP and DOWN were no-ops here: this branch handled only BACK and
             * EXIT and returned 0 for everything else. They are the keys the
             * Theme page already uses to move its window, so using them keeps one
             * scroll gesture across the shell rather than inventing a second one,
             * and it takes nothing away -- nothing was bound here before.
             *
             * ONE LINE PER PRESS, which is what the Theme page does per press as
             * well. Deliberately not a page-sized jump and deliberately not
             * key-repeat: the Theme page has neither, and Instructions is
             * reusing that mechanism rather than extending it.
             *
             * STEP DOWN IS SATURATED, NOT WRAPPED. Wrapping would silently jump
             * the reader from the end of the text back to the top, which is the
             * one behaviour that makes a scrollbar feel broken; saturating leaves
             * the thumb where it is and simply does nothing further. UP from the
             * top is likewise inert.
             */
            unsigned int max_scroll = bounce_ui_shell_instructions_max_scroll();
            unsigned int first = flow->instructions_scroll;

            if (input == BOUNCE_UI_INPUT_DOWN) {
                if (first < max_scroll)
                    flow->instructions_scroll = first + 1u;
            } else if (first > 0u) {
                flow->instructions_scroll = first - 1u;
            }
            return 0;
        }
        if (input == BOUNCE_UI_INPUT_BACK)
            return bounce_app_flow_return_to_menu(flow);
        if (input == BOUNCE_UI_INPUT_EXIT)
            flow->menu.pending_action = BOUNCE_MENU_ACTION_EXIT;
        return 0;
    }
    if (flow->state == BOUNCE_APP_STATE_GAME_END) {
        /*
         * STEP 12X-P4-B -- the game-complete terminal screen.
         *
         * BounceGame.ShowGameEnd (BounceGame.java:177-197) attaches exactly one
         * command, `this.i` = Command.OK (:180, :193), and commandAction handles
         * it at :256-262 by calling ShowMainMenu() when the main menu is not the
         * current displayable -- which it never is, because ShowGameEnd is the
         * thing that was displayed at :195. So the terminal screen's ONLY exit is
         * to the main menu.
         *
         * The three accepted inputs are the same ones the GAME_OVER branch below
         * accepts, and for the same reasons: SELECT is the existing confirm/OK
         * representation produced from Return, KP_Enter and Space by
         * app_ui_input_from_source, and BACK and EXIT are the native desktop
         * affordance for leaving a form destination. BounceGame.java:256-262
         * handles all three in one branch, so none substitutes for another.
         *
         * NO CONTINUE AFFORDANCE. commandContinue is attached only inside
         * ShowLevelComplete() (BounceGame.java:209), which e.java:320 skips when
         * the completed level was the last one, so the original screen has no
         * Continue command either.
         *
         * The transition is bounce_app_flow_apply(BOUNCE_APP_EVENT_MENU), which
         * accepts BOUNCE_APP_STATE_GAME_END at app_flow.c and calls
         * bounce_app_flow_enter_menu, where the selection, pending-action,
         * gameplay-entry and menu resets all live.
         * bounce_app_flow_return_to_menu is deliberately not used, for the reason
         * the GAME_OVER branch documents: its guard is a whitelist of form-like
         * destinations and widening it would change the contract for every other
         * caller.
         *
         * No timer work belongs here. The Step 12X-G-R2 tail already stopped the
         * timer on the tick that completed the final level, and
         * bounce_app_flow_enter_menu neither starts nor stops it, so the menu is
         * entered with the timer stopped -- which is what
         * BounceGame.ShowMainMenu's own v.StopGameTimer() (BounceGame.java:131)
         * achieves in the source.
         */
        if (input == BOUNCE_UI_INPUT_SELECT
            || input == BOUNCE_UI_INPUT_BACK
            || input == BOUNCE_UI_INPUT_EXIT)
            return bounce_app_flow_apply(flow, BOUNCE_APP_EVENT_MENU);
        return 0;
    }
    if (flow->state == BOUNCE_APP_STATE_GAME_OVER) {
        /*
         * BounceGame.java:177-197 ShowGameEnd(false) builds a MIDP Form whose
         * ONLY command is Command.OK (`this.i`, created at :180 and attached at
         * :193). BounceGame.java:256-262 handles `cmd == this.i` by calling
         * ShowMainMenu() -- and it handles commandBack and commandExit in that
         * same branch, so all three leave by the same edge. No other gameplay
         * state is reachable from here.
         *
         * BOUNCE_UI_INPUT_SELECT is the existing confirm/OK representation and
         * is already produced from Return, KP_Enter and Space by
         * app_ui_input_from_source (vertical_slice.c:8804-8807), so no new key
         * mapping is introduced. BACK and EXIT are accepted for the same reason
         * the INSTRUCTIONS and HIGH_SCORE branches accept them: they are the
         * native desktop affordance for leaving a form destination, and the
         * X11 layer maps Escape to BACK (vertical_slice.c:8811-8812). All three
         * take the identical transition, so none substitutes for another.
         *
         * The transition is the already-verified one, not an assignment:
         * bounce_app_flow_apply(BOUNCE_APP_EVENT_MENU) is accepted from
         * BOUNCE_APP_STATE_GAME_OVER at app_flow.c:165 and calls
         * bounce_app_flow_enter_menu (app_flow.c:174), which is where the
         * selection reset, pending-action reset, gameplay-entry reset and menu
         * reset all live (app_flow.c:92-101).
         *
         * bounce_app_flow_return_to_menu is deliberately NOT used here. Its
         * guard (app_flow.c:367-377) is an explicit whitelist of the form-like
         * destinations and does not list BOUNCE_APP_STATE_GAME_OVER, so it
         * would reject this state with -1. Widening that shared whitelist would
         * change the contract for every other caller, whereas the event path
         * above reaches the same bounce_app_flow_enter_menu without touching it.
         *
         * No timer work belongs here. The terminal arm in
         * app_respawn_after_death already stopped the timer, and
         * bounce_app_flow_enter_menu neither starts nor stops it, so the menu is
         * entered with the timer still stopped -- which is what
         * BounceGame.ShowMainMenu's own v.StopGameTimer() (BounceGame.java:131)
         * achieves in the source, and what the boundary audit measured.
         */
        if (input == BOUNCE_UI_INPUT_SELECT
            || input == BOUNCE_UI_INPUT_BACK
            || input == BOUNCE_UI_INPUT_EXIT)
            return bounce_app_flow_apply(flow, BOUNCE_APP_EVENT_MENU);
        return 0;
    }
    if (flow->state == BOUNCE_APP_STATE_LEVEL_SELECTION) {
        if (input == BOUNCE_UI_INPUT_UP)
            return bounce_app_flow_level_selection_move(
                flow,
                BOUNCE_MENU_DIRECTION_UP
            );
        if (input == BOUNCE_UI_INPUT_DOWN)
            return bounce_app_flow_level_selection_move(
                flow,
                BOUNCE_MENU_DIRECTION_DOWN
            );
        if (input == BOUNCE_UI_INPUT_SELECT) {
            /*
             * One edge performs the source-shaped chain up to the verified
             * boundary: START_LEVEL(N) then the gameplay-entry record. No
             * level, player, timer, or tick is started.
             */
            if (bounce_app_flow_level_selection_select(flow) < 0)
                return -1;
            return bounce_app_flow_begin_gameplay_entry(flow);
        }
        if (input == BOUNCE_UI_INPUT_BACK)
            return bounce_app_flow_return_to_menu(flow);
        if (input == BOUNCE_UI_INPUT_EXIT)
            flow->menu.pending_action = BOUNCE_MENU_ACTION_EXIT;
        return 0;
    }
    if (flow->state == BOUNCE_APP_STATE_HIGH_SCORE) {
        if (input == BOUNCE_UI_INPUT_BACK)
            return bounce_app_flow_return_to_menu(flow);
        if (input == BOUNCE_UI_INPUT_EXIT)
            flow->menu.pending_action = BOUNCE_MENU_ACTION_EXIT;
        return 0;
    }
    if (flow->state == BOUNCE_APP_STATE_SETTINGS) {
        /* NATIVE EXTENSION destination. */
        if (input == BOUNCE_UI_INPUT_UP
            || input == BOUNCE_UI_INPUT_DOWN
            || input == BOUNCE_UI_INPUT_SELECT) {
            /*
             * Display, Input, and Audio are deliberately read-only fact pages and
             * report no navigable list. A press there is a legal no-op, not a
             * failure, so it must not propagate as an error. The flow functions
             * still refuse (-1) for a genuine request it cannot satisfy.
             */
            if (bounce_app_flow_settings_list_count(flow) == 0u)
                return 0;
            if (input == BOUNCE_UI_INPUT_SELECT)
                return bounce_app_flow_settings_select(flow);
            return bounce_app_flow_settings_move(
                flow,
                input == BOUNCE_UI_INPUT_UP
                    ? BOUNCE_MENU_DIRECTION_UP
                    : BOUNCE_MENU_DIRECTION_DOWN
            );
        }
        if (input == BOUNCE_UI_INPUT_BACK)
            return bounce_app_flow_settings_back(flow);
        if (input == BOUNCE_UI_INPUT_EXIT)
            return bounce_app_flow_return_to_menu(flow);
        return 0;
    }
    if (flow->state == BOUNCE_APP_STATE_ABOUT) {
        /* NATIVE EXTENSION destination. */
        if (input == BOUNCE_UI_INPUT_BACK || input == BOUNCE_UI_INPUT_EXIT)
            return bounce_app_flow_return_to_menu(flow);
        return 0;
    }
    if (flow->state == BOUNCE_APP_STATE_GAMEPLAY_ENTRY) {
        /*
         * Native development escape only: the original has no Back command
         * once the canvas is current. No other input is accepted here.
         */
        if (input == BOUNCE_UI_INPUT_BACK)
            return bounce_app_flow_return_to_menu(flow);
        if (input == BOUNCE_UI_INPUT_EXIT)
            flow->menu.pending_action = BOUNCE_MENU_ACTION_EXIT;
        return 0;
    }
    if (flow->state == BOUNCE_APP_STATE_ACTION_BOUNDARY) {
        if (input == BOUNCE_UI_INPUT_SELECT
            && flow->menu.pending_action == BOUNCE_MENU_ACTION_START_LEVEL)
            return bounce_app_flow_begin_gameplay_entry(flow);
        if (input == BOUNCE_UI_INPUT_BACK)
            return bounce_app_flow_return_to_menu(flow);
        if (input == BOUNCE_UI_INPUT_EXIT)
            flow->menu.pending_action = BOUNCE_MENU_ACTION_EXIT;
        return 0;
    }
    if (flow->state != BOUNCE_APP_STATE_MENU)
        return 0;

    switch (input) {
        case BOUNCE_UI_INPUT_UP:
            return bounce_app_flow_menu_move(
                flow,
                BOUNCE_MENU_DIRECTION_UP
            );
        case BOUNCE_UI_INPUT_DOWN:
            return bounce_app_flow_menu_move(
                flow,
                BOUNCE_MENU_DIRECTION_DOWN
            );
        case BOUNCE_UI_INPUT_SELECT:
            (void)bounce_app_flow_menu_select(flow);
            /*
             * Direct New Game records START_LEVEL(level 1) and stops at the
             * UI 5 action boundary; one edge continues into the UI 6
             * gameplay-entry record without executing any source step.
             */
            if (flow->state == BOUNCE_APP_STATE_ACTION_BOUNDARY
                && flow->menu.pending_action
                    == BOUNCE_MENU_ACTION_START_LEVEL)
                return bounce_app_flow_begin_gameplay_entry(flow);
            return 0;
        case BOUNCE_UI_INPUT_EXIT:
            flow->menu.pending_action = BOUNCE_MENU_ACTION_EXIT;
            return 0;
        case BOUNCE_UI_INPUT_OTHER:
        default:
            return 0;
    }
}

int bounce_ui_shell_render_splash(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceVisualAssets *assets
)
{
    if (surface == NULL || renderer == NULL || assets == NULL)
        return -1;
    if (bounce_surface_fill(surface, UINT32_C(0xff000000)) != 0
        || bounce_renderer_reset_clip(renderer) != 0)
        return -1;
    /* bouncesplash.png is already 128x128; this is an unscaled 1:1 blit. */
    return bounce_visual_assets_draw_splash(renderer, assets);
}

int bounce_ui_shell_render_menu(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
    /* PROVISIONAL NATIVE GEOMETRY — NOT ORIGINAL COORDINATES. */
    /*
     * Seven rows now fit the source-verified original order plus the two native
     * extensions and the selectable Exit row. Row order and vertical rhythm are
     * the existing native skin; only the row count changed.
     */
    static const int row_y[BOUNCE_MENU_ROW_COUNT] = {
        30, 42, 54, 66, 78, 90, 102
    };
const uint32_t background = ui->background;
const uint32_t panel = ui->panel;
const uint32_t border = ui->border;
const uint32_t highlight = ui->highlight;
const uint32_t selected_text = ui->selected_text;
const uint32_t text = ui->text;
const uint32_t disabled = ui->disabled;
    unsigned int row;
    unsigned int selected_row;

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_MENU)
        return -1;
    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
        || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0)
        return -1;

    if (draw_centered_placeholder_text(renderer, 12, tr(BOUNCE_LOCALE_TITLE), text) != 0)
        return -1;

    selected_row = bounce_app_flow_menu_row_for_selection(
        flow,
        flow->menu.selected_index
    );
    for (row = 0u; row < BOUNCE_MENU_ROW_COUNT; ++row) {
        bool enabled = bounce_app_flow_menu_item_enabled(flow, row);
        uint32_t row_color;

/*
 * SELECTED-ROW TEXT ROLE.
 *
 * A row drawn over the `highlight` (selected_background) fill must be labelled in
 * `selected_text`, not `text`. The two are independent roles precisely so an
 * inverted profile can read. Drawing the label in `text` made the selected row
 * unreadable wherever the two were close: for Green LCD the label sat on the
 * fill at 1.18:1 contrast, and for Sepia, Green Inverted, Amber, Forest Mint,
 * Burgundy, Purple and Teal Ivory the two colors were equal or near-equal.
 *
 * This is a role correction only. No palette value is changed and no rendering
 * primitive is touched; unselected rows keep exactly the role they had.
 */
        if (row == selected_row
            && bounce_renderer_fill_rect(
                renderer,
                12,
                row_y[row] - 2,
                104,
                11,
                highlight
            ) != 0)
            return -1;
        /* Unselected keeps `text`; the selected row is labelled in selected_text. */
        if (!enabled)
            row_color = disabled;
        else if (row == selected_row)
            row_color = selected_text;
        else
            row_color = text;
        if (draw_placeholder_text(
                renderer,
                20,
                row_y[row],
                tr(menu_label_keys[row]),
                row_color
            ) != 0)
            return -1;
    }

    /*
     * HIDDEN: the two development status lines that used to sit here
     * (BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA and
     * BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE).
     *
     * They were honest notes about inactive functionality, not menu content, and
     * a polished menu should not carry them. Only the presentation is removed:
     *
     *  - Continue is still drawn as a greyed, non-selectable row, because the
     *    original omits the item entirely when there is no live or persisted
     *    session (BounceGame.java:116-119). That behavior is unchanged.
     *  - Settings and About are still present and still reachable.
     *  - Both strings still exist in the locale table for both languages, so no
     *    translation work is lost and the keys keep their meaning.
     *
     * The freed rows are simply left empty, which keeps the seven menu rows at
     * their existing positions and changes no geometry.
     */
    return 0;
}

/*
 * NATIVE ACTION BOUNDARY — DESTINATION CONTENT IS NOT IMPLEMENTED.
 * This notice is deliberately not a High Score, Continue, or New Game
 * screen; Instructions and High Score have their own bounded renderers.
 */
int bounce_ui_shell_render_action_boundary(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
const uint32_t background = ui->background;
const uint32_t panel = ui->panel;
const uint32_t border = ui->border;
const uint32_t text = ui->text;
const uint32_t warning = ui->accent;
    const char *action_label;
    int pending_level;

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_ACTION_BOUNDARY)
        return -1;
    action_label = bounce_ui_shell_action_label(flow->menu.pending_action);
    if (action_label == NULL)
        return -1;
    pending_level = bounce_app_flow_pending_level_id(flow);
    if (flow->menu.pending_action == BOUNCE_MENU_ACTION_START_LEVEL
        && (pending_level < BOUNCE_APP_FIRST_LEVEL_ID
            || pending_level > BOUNCE_APP_LAST_LEVEL_ID))
        return -1;
    if (flow->menu.pending_action == BOUNCE_MENU_ACTION_START_LEVEL) {
        if (bounce_surface_fill(surface, background) != 0
            || bounce_renderer_reset_clip(renderer) != 0
            || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
            || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
            || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
            || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
            || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0
            || draw_centered_placeholder_text(
                renderer, 14, tr(BOUNCE_LOCALE_TITLE), text) != 0
            || draw_centered_placeholder_text(renderer, 34, action_label, text) != 0
            || draw_centered_placeholder_text(renderer, 55, tr(BOUNCE_LOCALE_LEVEL), text) != 0
            || draw_native_number(
                renderer,
                61,
                68,
                pending_level,
                text
            ) != 0
            || draw_centered_placeholder_text(
                renderer,
                84,
                tr(BOUNCE_LOCALE_GAMEPLAY),
                warning
            ) != 0
            || draw_centered_placeholder_text(
                renderer,
                94,
                tr(BOUNCE_LOCALE_NOT_STARTED),
                warning
            ) != 0
            || draw_centered_placeholder_text(
                renderer,
                108,
                tr(BOUNCE_LOCALE_BACK_TO_MENU),
                text
            ) != 0)
            return -1;
        return 0;
    }
    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
        || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0
        || draw_centered_placeholder_text(
                renderer, 14, tr(BOUNCE_LOCALE_TITLE), text) != 0
        || draw_centered_placeholder_text(renderer, 38, action_label, text) != 0
        || draw_centered_placeholder_text(
            renderer,
            62,
            tr(BOUNCE_LOCALE_DESTINATION),
            warning
        ) != 0
        || draw_centered_placeholder_text(
            renderer,
            73,
            tr(BOUNCE_LOCALE_NOT_YET_IMPLEMENTED),
            warning
        ) != 0
        || draw_centered_placeholder_text(
            renderer,
            94,
            tr(BOUNCE_LOCALE_BACK_TO_MENU),
            text
        ) != 0)
        return -1;
    return 0;
}

/* PROVISIONAL NATIVE INSTRUCTIONS GEOMETRY — NOT ORIGINAL MIDP FORM. */
/*
 * Draw the Instructions body as a WINDOW onto all of it, plus the scrollbar.
 *
 * `scroll` is the reader's position, and it is clamped here rather than trusted:
 * the text's length depends on the active language and on the script font's
 * metrics, so a position stored while one language was active can be past the end
 * under another. Clamping at the point of use means the renderer can never draw a
 * window that starts beyond the text, whatever the stored value is.
 */
static int draw_instructions_window(
    BounceRenderer *renderer,
    const unsigned int *scroll,
    char *buffer,
    size_t size,
    uint32_t color
)
{
    const char *text;
    unsigned int total = 0u;
    unsigned int first;
    unsigned int max_scroll;

    if (renderer == NULL || scroll == NULL || buffer == NULL)
        return -1;
    text = instructions_composed(buffer, size);
    if (text == NULL)
        return -1;
    if (instructions_walk(NULL, text, 0u, 0u, 0u, &total) != 0)
        return -1;
    max_scroll = total > BOUNCE_UI_INSTRUCTIONS_VISIBLE_LINES
        ? total - BOUNCE_UI_INSTRUCTIONS_VISIBLE_LINES
        : 0u;
    first = *scroll;
    if (first > max_scroll)
        first = max_scroll;
    if (instructions_walk(
            renderer,
            text,
            color,
            first,
            BOUNCE_UI_INSTRUCTIONS_VISIBLE_LINES,
            &total
        ) != 0)
        return -1;
    /*
     * The SAME shared scrollbar the Theme page draws, with this page's geometry.
     * It draws nothing at all when the text fits, which is the case for no
     * shipped language today -- and that is asserted by the verifier rather than
     * assumed, because an indicator that is always visible is worse than none.
     */
    return ui_draw_scrollbar(
        renderer,
        BOUNCE_UI_INSTRUCTIONS_SCROLL_X,
        BOUNCE_UI_INSTRUCTIONS_SCROLL_Y0,
        BOUNCE_UI_INSTRUCTIONS_SCROLL_H,
        total,
        BOUNCE_UI_INSTRUCTIONS_VISIBLE_LINES,
        first
    );
}

int bounce_ui_shell_render_instructions(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
const uint32_t background = ui->background;
const uint32_t title = ui->text;
const uint32_t body = ui->body;
const uint32_t back = ui->accent;
    /*
     * Bounded, and deliberately generous: the substituted text is only ever
     * longer than its source, and the longest shipped prefix is the Thai one.
     * ui_compose_instructions() returns -1 rather than truncating if this is
     * ever too small, which is the same "refuse rather than lie" rule the rest
     * of this file follows.
     *
     * WHAT CHANGED. This page used to call draw_instructions_body(), which drew
     * at most twelve lines from the top and DISCARDED the rest -- four lines fell
     * off the bottom of the surface in every language, and Chinese discarded four
     * more. It now calls draw_instructions_window(), which draws the same eight
     * lines but starting wherever the reader has scrolled to, and discards
     * nothing: every line is reachable. The wrap rules are the same ones, in the
     * same function, reached through instructions_walk().
     */
    char instructions_text[1024];

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_INSTRUCTIONS)
        return -1;
    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || draw_centered_placeholder_text(
            renderer,
            7,
            tr(BOUNCE_LOCALE_INSTRUCTIONS),
            title
        ) != 0
        || draw_instructions_window(
            renderer,
            &flow->instructions_scroll,
            instructions_text,
            sizeof instructions_text,
            body
        ) != 0
        || draw_centered_placeholder_text(
            renderer,
            118,
            tr(BOUNCE_LOCALE_BACK),
            back
        ) != 0)
        return -1;
    return 0;
}

/* PROVISIONAL NATIVE NUMERIC PLACEHOLDER SHAPES — NOT ORIGINAL FONT VERIFIED. */
static int draw_native_digit(
    BounceRenderer *renderer,
    int x,
    int y,
    int digit,
    uint32_t color
)
{
    static const uint8_t digit_rows[10][7] = {
        {0x0eu, 0x11u, 0x11u, 0x11u, 0x11u, 0x11u, 0x0eu},
        {0x04u, 0x0cu, 0x04u, 0x04u, 0x04u, 0x04u, 0x0eu},
        {0x0eu, 0x11u, 0x01u, 0x02u, 0x04u, 0x08u, 0x1fu},
        {0x1fu, 0x02u, 0x04u, 0x02u, 0x01u, 0x11u, 0x0eu},
        {0x02u, 0x06u, 0x0au, 0x12u, 0x1fu, 0x02u, 0x02u},
        {0x1fu, 0x10u, 0x1eu, 0x01u, 0x01u, 0x11u, 0x0eu},
        {0x06u, 0x08u, 0x10u, 0x1eu, 0x11u, 0x11u, 0x0eu},
        {0x1fu, 0x01u, 0x02u, 0x04u, 0x08u, 0x08u, 0x08u},
        {0x0eu, 0x11u, 0x11u, 0x0eu, 0x11u, 0x11u, 0x0eu},
        {0x0eu, 0x11u, 0x11u, 0x0fu, 0x01u, 0x02u, 0x0cu}
    };
    int row;
    int column;

    if (renderer == NULL || digit < 0 || digit > 9)
        return -1;
    for (row = 0; row < 7; ++row) {
        for (column = 0; column < 5; ++column) {
            if ((digit_rows[digit][row]
                    & (uint8_t)(1u << (4 - column))) != 0u
                && bounce_renderer_fill_rect(
                    renderer,
                    x + column,
                    y + row,
                    1,
                    1,
                    color
                ) != 0)
                return -1;
        }
    }
    return 0;
}

static int draw_native_number(
    BounceRenderer *renderer,
    int x,
    int y,
    int value,
    uint32_t color
)
{
    if (renderer == NULL || value < 0 || value > 999)
        return -1;
    if (value >= 100) {
        if (draw_native_digit(
                renderer,
                x,
                y,
                (value / 100) % 10,
                color
            ) != 0)
            return -1;
        x += 6;
    }
    if (value >= 10) {
        if (draw_native_digit(
                renderer,
                x,
                y,
                (value / 10) % 10,
                color
            ) != 0)
            return -1;
        x += 6;
    }
    return draw_native_digit(renderer, x, y, value % 10, color);
}

/*
 * STEP 37 PERSIST -- the High Score value is now real.
 *
 * This used to be draw_native_score_zero(), a single hard-coded "0" glyph, and
 * bounce_ui_shell_high_score_value() returned the literal string "0". Java shows
 * `String.valueOf(this.HighScore)` (BounceGame.java:161) on a Form, i.e. as many
 * digits as the number has, so a fixed glyph cannot stand in for it.
 *
 * draw_native_number() above covers 0..999 and returns -1 outside that range.
 * A score above 999 is ordinary in this game -- one hoop is 500
 * (f.java:229) and one completed level is 5000 (e.java:318) -- so a failure
 * return here would reach the frame loop, set x11_context.failed and exit the
 * process. This routine therefore draws the digits directly with the same 6px
 * advance, for any non-negative value, and cannot fail on magnitude.
 *
 * The digit range and the 6px pitch are the existing HUD convention, not a new
 * layout. Only the High Score screen is affected: no other caller changed.
 */
static int draw_native_high_score(
    BounceRenderer *renderer,
    int x,
    int y,
    int value,
    uint32_t color
)
{
    int digits[10];
    int count = 0;
    int i;
    int remaining = value;

    if (renderer == NULL || value < 0)
        return -1;

    if (value == 0)
        return draw_native_digit(renderer, x, y, 0, color);

    while (remaining > 0 && count < 10) {
        digits[count] = remaining % 10;
        remaining /= 10;
        count++;
    }
    for (i = count - 1; i >= 0; i--) {
        if (draw_native_digit(renderer, x, y, digits[i], color) != 0)
            return -1;
        x += 6;
    }
    return 0;
}

/* PROVISIONAL NATIVE HIGH SCORE GEOMETRY — NOT ORIGINAL MIDP FORM. */
int bounce_ui_shell_render_high_score(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
const uint32_t background = ui->background;
const uint32_t title = ui->text;
const uint32_t value = ui->body;
const uint32_t back = ui->accent;

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_HIGH_SCORE)
        return -1;
    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || draw_centered_placeholder_text(
            renderer,
            20,
            tr(BOUNCE_LOCALE_HIGH_SCORE),
            title
        ) != 0
        || draw_native_high_score(
               renderer,
               61,
               50,
               bounce_app_flow_high_score(flow),
               value
           ) != 0
        || draw_centered_placeholder_text(
            renderer,
            112,
            tr(BOUNCE_LOCALE_BACK),
            back
        ) != 0)
        return -1;
    return 0;
}

/* PROVISIONAL NATIVE LEVEL LIST GEOMETRY — NOT ORIGINAL MIDP LIST. */
/* ------------------------------------------------------------------ */
/* NATIVE EXTENSION: Settings and About.                               */
/*                                                                    */
/* The recovered Nokia Bounce source has no Options, Settings,        */
/* Language, About, or Help item (reverse/                            */
/* ui-menu-persistence-localization-audit.md section 4, complete       */
/* source search). These screens are a Linux-reimplementation           */
/* addition and are labelled as native wherever they are drawn.        */
/* ------------------------------------------------------------------ */

/*
 * The native font is a 5x7 placeholder glyph set with a 6 px advance and covers
 * A-Z, 0-9, and space only. Every string below is deliberately restricted to
 * those characters so no label degrades into the unknown-glyph box.
 */
/*
 * Draw "<w> X <h>" using the real digit font. The placeholder letter font has no
 * digit glyphs, so a size written as a plain string would render the
 * unknown-glyph box for every digit instead of the number.
 */
static int draw_native_size(
    BounceRenderer *renderer,
    int x,
    int y,
    int width_value,
    int height_value,
    uint32_t color
)
{
    if (renderer == NULL)
        return -1;
    if (draw_native_number(renderer, x, y, width_value, color) != 0
        || draw_placeholder_text(renderer, x + 24, y, "X", color) != 0
        || draw_native_number(renderer, x + 36, y, height_value, color) != 0)
        return -1;
    return 0;
}

int bounce_ui_shell_render_settings(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
    /* Keys, not strings: the active language is resolved at draw time. */
    static const BounceLocaleKey root_label_keys[BOUNCE_SETTINGS_ROOT_ITEM_COUNT]
        = {
        BOUNCE_LOCALE_DISPLAY,
        BOUNCE_LOCALE_INPUT,
        BOUNCE_LOCALE_AUDIO,
        BOUNCE_LOCALE_LANGUAGE,
        /* NATIVE EXTENSION: temporary runtime UI color profiles. */
        BOUNCE_LOCALE_THEME
    };
    /*
     * Five rows now: the four original native Settings destinations plus the
     * temporary Theme page. Spacing is 16 px so the last row still clears the
     * ESC BACK hint at y = 115 and the highlight band never overlaps it.
     */
    static const int root_y[BOUNCE_SETTINGS_ROOT_ITEM_COUNT] = {
        28, 44, 60, 76, 92
    };
const uint32_t background = ui->background;
const uint32_t panel = ui->panel;
const uint32_t border = ui->border;
const uint32_t highlight = ui->highlight;
const uint32_t selected_text = ui->selected_text;
const uint32_t text = ui->text;
const uint32_t muted = ui->muted;
const uint32_t back = ui->accent;
    const char *title;
    unsigned int index;

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_SETTINGS)
        return -1;

    switch (bounce_app_flow_settings_page(flow)) {
        case BOUNCE_SETTINGS_PAGE_ROOT:
            title = tr(BOUNCE_LOCALE_SETTINGS);
            break;
        case BOUNCE_SETTINGS_PAGE_DISPLAY:
            title = tr(BOUNCE_LOCALE_SETTINGS_DISPLAY);
            break;
        case BOUNCE_SETTINGS_PAGE_INPUT:
            title = tr(BOUNCE_LOCALE_SETTINGS_INPUT);
            break;
        case BOUNCE_SETTINGS_PAGE_AUDIO:
            title = tr(BOUNCE_LOCALE_SETTINGS_AUDIO);
            break;
        case BOUNCE_SETTINGS_PAGE_LANGUAGE:
            title = tr(BOUNCE_LOCALE_SETTINGS_LANGUAGE);
            break;
        case BOUNCE_SETTINGS_PAGE_THEME:
            title = tr(BOUNCE_LOCALE_THEME);
            break;
        default:
            return -1;
    }

    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
        || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0
        || draw_centered_placeholder_text(renderer, 8, title, text) != 0
        || bounce_renderer_fill_rect(renderer, 12, 18, 104, 1, border) != 0)
        return -1;

    /*
     * HIDDEN: BOUNCE_LOCALE_READ_ONLY_NATIVE and
     * BOUNCE_LOCALE_NO_REMAPPING_YET are no longer drawn on these pages.
     *
     * They were development notes about functionality that is not active, not
     * settings content, so a polished Settings screen should not carry them.
     * Only the presentation is removed. The pages remain read-only in exactly
     * the same way: no remapping control, no display mode, and no volume
     * control were added to stand in for them, so nothing is fabricated. Both
     * strings still exist in the locale table for both languages.
     */
    if (bounce_app_flow_settings_page(flow) == BOUNCE_SETTINGS_PAGE_ROOT) {
        for (index = 0u; index < BOUNCE_SETTINGS_ROOT_ITEM_COUNT; ++index) {
            if (index == bounce_app_flow_settings_selected_index(flow)
                && bounce_renderer_fill_rect(
                       renderer, 12, root_y[index] - 2, 104, 11, highlight) != 0)
                return -1;
            if (draw_placeholder_text(
                    renderer,
                    24,
                    root_y[index],
                    tr(root_label_keys[index]),
                    index == bounce_app_flow_settings_selected_index(flow)
                        ? selected_text
                        : text) != 0)
                return -1;
        }
    } else if (bounce_app_flow_settings_page(flow)
               == BOUNCE_SETTINGS_PAGE_DISPLAY) {
        /*
         * Only the presentation concepts this build actually supports. The
         * numbers are drawn with the real digit font because the placeholder
         * letter font has no digit glyphs; embedding them in a string would
         * render the unknown-glyph box instead.
         */
        if (draw_placeholder_text(
                renderer, 14, 24, tr(BOUNCE_LOCALE_LOGICAL_RESOLUTION), muted) != 0
            || draw_native_size(renderer, 22, 34, 128, 128, text) != 0
            || draw_placeholder_text(
                renderer, 14, 50, tr(BOUNCE_LOCALE_GAMEPLAY_VIEWPORT), muted) != 0
            || draw_native_size(renderer, 22, 60, 128, 96, text) != 0
            || draw_placeholder_text(
                renderer, 14, 76, tr(BOUNCE_LOCALE_UI_HUD_REGION), muted) != 0
            || draw_native_size(renderer, 22, 86, 128, 32, text) != 0)
            return -1;
    } else if (bounce_app_flow_settings_page(flow)
               == BOUNCE_SETTINGS_PAGE_INPUT) {
        /*
         * The live native mapping, and -- NATIVE ADDITION -- the one control on
         * this page.
         *
         * The fact rows are unchanged in content and in order: LEFT/ARROW-A,
         * RIGHT/ARROW-D, UP/ARROW-W, DOWN/ARROW-S, BACK/ESCAPE. They are one row
         * shorter in pitch, 10 px instead of 12, because the keypad setting needs
         * room below them and ESC BACK sits at y = 115 on every Settings page.
         * Nothing was dropped to make space.
         *
         * THE CONTROL IS THE AUDIO PAGE'S SHAPE, not a new one: two rows, the
         * highlight showing the value the user is ABOUT to choose, and a '*' on
         * the row that is actually in force, dimmed. SELECT applies; moving does
         * not. The applied value is drawn as its own row precisely so the
         * highlight and the setting can never be mistaken for each other.
         */
        static const BounceLocaleKey value_keys[BOUNCE_SETTINGS_T9_VALUE_COUNT]
            = {
                BOUNCE_LOCALE_ON,
                BOUNCE_LOCALE_OFF
            };
        const unsigned int count = bounce_app_flow_settings_list_count(flow);
        const unsigned int limit
            = count < BOUNCE_SETTINGS_T9_VALUE_COUNT
                  ? count
                  : BOUNCE_SETTINGS_T9_VALUE_COUNT;
        const unsigned int selected
            = bounce_app_flow_settings_selected_index(flow);
        const unsigned int applied
            = bounce_app_flow_settings_t9_enabled(flow)
                  ? BOUNCE_SETTINGS_T9_ROW_ON
                  : BOUNCE_SETTINGS_T9_ROW_OFF;
        unsigned int row;
        int y;

        if (draw_placeholder_text(renderer, 20, 22, tr(BOUNCE_LOCALE_LEFT), text) != 0
            || draw_placeholder_text(renderer, 66, 22, tr(BOUNCE_LOCALE_ARROW_A), muted) != 0
            || draw_placeholder_text(renderer, 20, 32, tr(BOUNCE_LOCALE_RIGHT), text) != 0
            || draw_placeholder_text(renderer, 66, 32, tr(BOUNCE_LOCALE_ARROW_D), muted) != 0
            || draw_placeholder_text(renderer, 20, 42, tr(BOUNCE_LOCALE_UP), text) != 0
            || draw_placeholder_text(renderer, 66, 42, tr(BOUNCE_LOCALE_ARROW_W), muted) != 0
            || draw_placeholder_text(renderer, 20, 52, tr(BOUNCE_LOCALE_DOWN), text) != 0
            || draw_placeholder_text(renderer, 66, 52, tr(BOUNCE_LOCALE_ARROW_S), muted) != 0
            || draw_placeholder_text(renderer, 20, 62, tr(BOUNCE_LOCALE_BACK), text) != 0
            || draw_placeholder_text(renderer, 66, 62, tr(BOUNCE_LOCALE_ESCAPE), muted) != 0
            || draw_centered_placeholder_text(
                   renderer, 78, tr(BOUNCE_LOCALE_T9_INPUT), muted) != 0)
            return -1;

        for (row = 0u; row < limit; ++row) {
            const uint32_t row_color
                = (row == selected) ? selected_text : text;
            const uint32_t mark_color
                = (row == applied) ? back : muted;
            const char marker = (row == applied) ? '*' : ' ';

            y = 90 + (int)row * 12;
            if (row == selected
                && bounce_renderer_fill_rect(
                        renderer, 12, y - 2, 104, 13, highlight) != 0)
                return -1;
            if (draw_placeholder_text(
                       renderer, 24, y, tr(value_keys[row]), row_color) != 0
                || draw_placeholder_text(
                       renderer, 104, y, &marker, mark_color) != 0)
                return -1;
        }
    } else if (bounce_app_flow_settings_page(flow)
               == BOUNCE_SETTINGS_PAGE_AUDIO) {
        /*
         * STEP 34-F-K -- NATIVE ADDITION. The Audio page now offers exactly one
         * setting, Audio, with the two values ON and OFF. It is the same
         * highlight-and-SELECT shape the Language and Theme pages use, drawn
         * with the same primitives; nothing new was introduced.
         *
         * It is deliberately minimal. There is no device row, no volume, no
         * mute, no test button, no backend report and no status line: the
         * decision to keep this page at one setting is the point of the step.
         *
         * The row shows the value the user is ABOUT to choose, which is what
         * SELECT means on every other settings page. The value actually in
         * force is the same row plus one, drawn dimmed, so the highlight and
         * the applied value are never confused. That dimmed marker is what
         * distinguishes this from the old page, which asserted a status it had
         * no way to know.
         */
        static const BounceLocaleKey value_keys[BOUNCE_SETTINGS_AUDIO_VALUE_COUNT]
            = {
                BOUNCE_LOCALE_ON,
                BOUNCE_LOCALE_OFF
            };
        const unsigned int count = bounce_app_flow_settings_list_count(flow);
        const unsigned int limit
            = count < BOUNCE_SETTINGS_AUDIO_VALUE_COUNT
                  ? count
                  : BOUNCE_SETTINGS_AUDIO_VALUE_COUNT;
        const unsigned int selected = bounce_app_flow_settings_selected_index(flow);
        const unsigned int applied
            = bounce_app_flow_settings_audio_enabled(flow)
                  ? BOUNCE_SETTINGS_AUDIO_ROW_ON
                  : BOUNCE_SETTINGS_AUDIO_ROW_OFF;
        unsigned int row;
        int y;

        if (draw_centered_placeholder_text(
                renderer,
                32,
                tr(BOUNCE_LOCALE_AUDIO),
                text) != 0
            || draw_centered_placeholder_text(
                   renderer,
                   48,
                   tr(BOUNCE_LOCALE_STATUS),
                   muted) != 0)
            return -1;

        for (row = 0u; row < limit; ++row) {
            const uint32_t row_color
                = (row == selected) ? selected_text : text;
            const uint32_t mark_color
                = (row == applied) ? back : muted;
            const char marker = (row == applied) ? '*' : ' ';

            y = 66 + (int)row * 14;
            if (row == selected
                && bounce_renderer_fill_rect(
                        renderer, 12, y - 2, 104, 13, highlight) != 0)
                return -1;
            if (draw_placeholder_text(renderer, 24, y, &marker, mark_color) != 0
                || draw_placeholder_text(
                       renderer, 36, y, tr(value_keys[row]), row_color) != 0)
                return -1;
        }
    } else if (bounce_app_flow_settings_page(flow) == BOUNCE_SETTINGS_PAGE_THEME) {
        /*
         * NATIVE EXTENSION: the temporary runtime UI color profiles. Selecting a
         * row applies it immediately. Nothing is saved and nothing is persisted.
         */
        static const BounceLocaleKey theme_keys[BOUNCE_UI_THEME_COUNT] = {
            BOUNCE_LOCALE_THEME_DEFAULT,
            /*
             * Same profile slot throughout: the original "GREEN LCD" entry,
             * briefly called "Green Light", is Green LCD again. The pre-existing
             * GREEN_LCD key is reused, so there is exactly one such profile and
             * no duplicate palette. Key identifiers stay stable; only the
             * display strings are user-facing.
             */
            BOUNCE_LOCALE_THEME_GREEN_LCD,
            BOUNCE_LOCALE_THEME_SEPIA_TONE,
            BOUNCE_LOCALE_THEME_INVERTED_GREEN_LCD,
            BOUNCE_LOCALE_THEME_AMBER_ORANGE,
            BOUNCE_LOCALE_THEME_DEEP_NAVY_CREAM,
            BOUNCE_LOCALE_THEME_FOREST_GREEN_PALE_MINT,
            BOUNCE_LOCALE_THEME_BURGUNDY_SOFT_BLUSH,
            BOUNCE_LOCALE_THEME_CHARCOAL_ELECTRIC_BLUE,
            BOUNCE_LOCALE_THEME_CLASSIC_NOKIA_BLUE,
            BOUNCE_LOCALE_THEME_PURPLE_HAZE,
            BOUNCE_LOCALE_THEME_ARCTIC_WHITE,
            BOUNCE_LOCALE_THEME_DARK_TEAL_IVORY
        };
        const unsigned int count = bounce_app_flow_settings_list_count(flow);
        const unsigned int limit
            = count < BOUNCE_UI_THEME_COUNT ? count : BOUNCE_UI_THEME_COUNT;
        const unsigned int selected = bounce_app_flow_settings_theme_index(flow);
        const unsigned int first = theme_first_row(limit, selected);
        unsigned int slot;

        /*
         * Rows keep this list's own existing geometry: y = 30 + slot * 20 with a
         * 104x13 highlight band at y - 2. Slots 0..2 land on exactly the y values
         * the three original profiles used, so the first three rows are drawn
         * where they always were.
         */
        for (slot = 0u; slot < BOUNCE_UI_THEME_VISIBLE_ROWS; ++slot) {
            const unsigned int row = first + slot;
            int y = BOUNCE_UI_THEME_ROW_Y0 + (int)slot * BOUNCE_UI_THEME_ROW_PITCH;
            /*
             * The selected row is the one place selected_background and
             * selected_text are used together, independently of `text`. For the
             * Default profile selected_text equals text, so its selected row is
             * unchanged; for an inverted profile it is what keeps the label
             * readable on a bright fill.
             */
            const uint32_t row_text
                = (row == selected && row < limit) ? selected_text : text;

            if (row >= limit)
                break;
            if (row == selected
                && bounce_renderer_fill_rect(
                       renderer, 12, y - 2, 104, 13, highlight) != 0)
                return -1;
            if (draw_placeholder_text(
                    renderer, 24, y, tr(theme_keys[row]), row_text) != 0)
                return -1;
        }
        if (theme_draw_scrollbar(renderer, limit, first) != 0)
            return -1;
            /*
         * HIDDEN: BOUNCE_LOCALE_SELECT_TO_SWITCH is no longer drawn here.
           *
           * It was a development hint, not a control. Only the presentation is
           * removed: the locale key and all five of its translations stay in
           * locale.c, theme_index and applied_theme_index are untouched, the
           * highlight band and the three profile labels are drawn exactly as
           * before, and the palette values are unchanged.
           *
           * Row geometry is untouched. The list already ends at y = 77 and
         * ESC BACK is drawn at y = 115, so the screen simply has clear space
         * between them and no reserved area is left behind.
         */
    } else {
        /*
         * The Language list: the four ORIGINAL SOURCE-BACKED recovered
         * resources plus Indonesian, a NATIVE EXTENSION. The default resource
         * is shown as "EN", never as its "xx" filename token, because the
         * recovered source never calls any language "xx" in a user-facing
         * string. Codes are used rather than native endonyms because the UI
         * font has no CJK or Thai glyphs.
         */
        const unsigned int count = bounce_app_flow_settings_list_count(flow);
        const unsigned int applied
            = bounce_app_flow_settings_applied_language_index(flow);

        for (index = 0u; index < count; ++index) {
            const char *label = bounce_locale_language_label(
                (BounceLanguage)index);
            int y = 24 + (int)index * 14;

            if (label == NULL)
                return -1;
            if (index == bounce_app_flow_settings_language_index(flow)
                && bounce_renderer_fill_rect(
                       renderer, 12, y - 2, 104, 11, highlight) != 0)
                return -1;
            if (draw_placeholder_text(
                    renderer,
                    20,
                    y,
                    label,
                    index == bounce_app_flow_settings_language_index(flow)
                        ? selected_text
                        : text) != 0)
                return -1;
        }

        /*
         * HIDDEN: the two helper lines this page used to draw under the list,
         * BOUNCE_LOCALE_SELECT_TO_SWITCH and the per-language status built from
         * BOUNCE_LOCALE_DEFAULT_LANGUAGE or BOUNCE_LOCALE_NATIVE_STRINGS. They
         * were development hints, not controls.
         *
         * Only the presentation is removed. All three locale keys and all five
         * of their translations remain in locale.c, so nothing is lost and
         * nothing had to be deleted to achieve this. language_index and
         * applied_language_index are untouched, SELECT still applies a language,
         * the five rows and their highlight band are drawn exactly as before,
         * and the CJK and Thai rows are unaffected.
         *
         * The bound check below is NOT rendering, so it is deliberately kept: it
         * is a real precondition on the applied index, not a caption.
         *
         * Row geometry is untouched. The last row already ends at y = 89 and
         * ESC BACK is drawn at y = 115, so the screen simply has clear space
         * between them and no reserved area is left behind.
         */
        if (applied >= count
            || bounce_locale_language_label((BounceLanguage)applied) == NULL)
            return -1;
    }

    if (draw_centered_placeholder_text(renderer, 115, tr(BOUNCE_LOCALE_ESC_BACK), back) != 0)
        return -1;
    return 0;
}

/* --------------------------------------------------------------------------
 * STEP 12X-P4-C-B -- the Game End static UI shell.
 *
 * SOURCE. BounceGame.ShowGameEnd(boolean won) is BounceGame.java:177-197. For
 * the win path with q == false it builds a MIDP Form in exactly this order:
 *
 *   :181  new Form(sprintf_translated(Translation.GAME_OVER))   <-- TITLE
 *   :183  form.append(sprintf_translated(Translation.CONGRATS))  <-- BODY ITEM
 *   :187  form.append("\n\n")                                    <-- SEPARATE ITEM
 *   :188  if (this.q) { ... }                                    <-- absent, q==false
 *   :192  form.append(String.valueOf(this.E))                   <-- SCORE ITEM
 *   :193  form.addCommand(this.i)  // Command(sprintf(OK), OK, 1) <-- OK
 *
 * Each append() is its own Item, so the Java body is three or five separate
 * StringItems and never one concatenated string. The two "\n\n" spacers are
 * therefore SEPARATE items, and this rendering keeps them separate: the spacer
 * and the score each occupy their own reserved band instead of being folded
 * into the body line. The screen is deliberately not concatenated.
 *
 * SCORE DISPLAY DEFERRED -- SCORE SYSTEM NOT YET RECONCILED.
 *
 * The :192 score item and the :188 new-high-score item are NOT drawn, and the
 * reason is not cosmetic: this build has no score field at all. There is no E,
 * no q, no HighScore, no AddScore, no WriteToStore and no completion bonus
 * anywhere in native/. Rendering a number here would mean inventing one, and
 * rendering "0" would be a fabricated value a reader could mistake for a real
 * score. The bands below are therefore RESERVED AND LEFT EMPTY: they draw no
 * pixels, hold no string, and carry no numeric or textual placeholder. When the
 * score milestone lands, these bands are where its StringItem belongs.
 *
 * PIXEL GEOMETRY IS NOT SOURCE-DERIVED and is not claimed to be. The Java Form's
 * item heights, fonts, margins, title-bar presence and screen size are not
 * establishable from the recovered source (reconciliation 13.22.8). What is
 * reproduced is the ITEM ORDER and the SEPARATENESS of the spacer and score
 * bands. The panel geometry, the 5x7 font, the 6 px advance and the 11..14 px
 * row pitch are the existing native placeholders, copied from the About
 * destination; every band was checked to fit the 128 px logical width. No Form
 * dimension is invented and no pixel parity is claimed.
 *
 * COLOURS come from the active theme palette, so this destination participates
 * in the native theme system like every other form.
 *
 * STEP 13G-A CORRECTION. This comment previously ended by contrasting itself with
 * render_level_complete_state(), "which still fills with the hard-coded
 * COLOR_LEVEL_COMPLETE_BASE green and draws no text at all". That was accurate when
 * written and is now FALSE: the Level Complete placeholder has been replaced by
 * bounce_ui_shell_render_level_complete() below, which draws real text, uses this
 * same theme palette, and lives in this same file. The two destination renderers
 * are no longer distinguishable in that respect, and the hard-coded
 * COLOR_LEVEL_COMPLETE_BASE / _ACCENT tokens have been removed with the placeholder
 * they belonged to.
 *
 * This function is PURE RENDERING. It reads only flow->state and touches
 * neither final-level detection, level_id, the available level count, the
 * GAME_END or GAME_OVER identities, the timer, nor gameplay cleanup. All of
 * that is Step 12X-P4-B behaviour and is unchanged.
 * ------------------------------------------------------------------------ */
/*
 * The two shared numeric helpers below are defined further down this file,
 * beside the Level Complete composer that introduced them. Both are needed by
 * bounce_ui_shell_render_game_end() and bounce_ui_shell_render_game_over(), which
 * sit earlier, so the declaration is lifted rather than duplicated and the
 * definition is left exactly where it was.
 */
static int ui_decimal(int value, char *out, size_t out_size);

enum {
    /*
     * D-13 / J-01 -- the level-start banner origin, e.java:204:
     *
     *     this.X.drawString(this.LevelString, 44, 84, 20);
     *
     * 20 is Graphics.TOP | Graphics.LEFT, so this is a top-left anchor at
     * (44, 84) and is NOT centred: the pen starts there and runs right, exactly
     * as bounce_visual_assets_draw_hud_score documents for the score string at
     * e.java:183. Both numbers are the source's own literals.
     */
    UI_LEVEL_BANNER_X = 44,
    UI_LEVEL_BANNER_Y = 84,
    /* The longest composed line is "Level 11 completed!"; 64 is ample. */
    UI_LEVEL_COMPLETED_LINE = 64,
    /* The two Chinese strings are the only two-line forms in the shipped set. */
    UI_LEVEL_COMPLETED_LINES = 2,
    /* 7 px glyphs on a 12 px row, inside a panel that ends at y=124. */
    UI_LEVEL_COMPLETED_BODY_Y = 32,
    UI_LEVEL_COMPLETED_ROW = 12
};

int bounce_ui_shell_render_game_end(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow,
    int32_t score,
    bool new_high_score
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
  const uint32_t background = ui->background;
  const uint32_t panel = ui->panel;
  const uint32_t border = ui->border;
  const uint32_t title = ui->text;
  const uint32_t body = ui->body;
  const uint32_t value = ui->text;
  const uint32_t ok = ui->accent;
    char score_text[UI_LEVEL_COMPLETED_LINE];

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_GAME_END)
        return -1;

    /*
     * D-11 -- :192 `this.form.append(String.valueOf(this.E));`, which the win
     * form reaches exactly as the loss form does. ShowGameEnd has ONE body and
     * only the CONGRATS/GAME_OVER line at :182-186 differs between the two, so
     * the score and the NEW_HIGH_SCORE notice are common to both.
     *
     * `E` is this.v.score as captured by the no-argument WriteToStore()
     * (BounceGame.java:427, `this.E = this.v.score;`), which the completion
     * block calls at e.java:319 BEFORE branching on level > 11 at :320. So on
     * the win screen E already includes the +5000 awarded at e.java:318, and the
     * value handed to this renderer is the live total for the same reason
     * bounce_ui_shell_render_level_complete already receives it unadjusted
     * (ui_shell.h:239-242). Nothing is added or subtracted here.
     *
     * Same ui_decimal() as the loss form and the Level Complete score, so all
     * three print an integer identically: no zero padding, no separator,
     * negatives as given.
     */
    if (ui_decimal((int)score, score_text, sizeof score_text) != 0)
        return -1;
    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
        || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0
        /* :181 Form title, Translation.GAME_OVER. */
        || draw_centered_placeholder_text(
               renderer,
               9,
               tr(BOUNCE_LOCALE_GAME_END_TITLE),
               title
           ) != 0
        || bounce_renderer_fill_rect(renderer, 12, 19, 104, 1, border) != 0
        /* :183 the win body, Translation.CONGRATS, as its own Item. */
        || draw_centered_placeholder_text(
               renderer,
               32,
               tr(BOUNCE_LOCALE_GAME_END_CONGRATS),
               body
           ) != 0
        /*
         * D-07 -- :188-190 the NEW_HIGH_SCORE Item, on the same reserved row the
         * loss form uses. Same reasoning, same flag, same reasoning about the
         * reserved row; see bounce_ui_shell_render_game_over above.
         */
        || draw_centered_placeholder_text(
               renderer,
               58,
               new_high_score
                   ? tr(BOUNCE_LOCALE_NEW_HIGH_SCORE) : "",
               new_high_score ? value : body
           ) != 0
        /*
         * D-11 -- :192 the score. This row was previously left empty; it is now
         * filled, because the value has a reconciled native owner.
         */
        || draw_centered_placeholder_text(
               renderer,
               69,
               score_text,
               value
           ) != 0
        /* :180/:193 the single Command.OK affordance, on the same bottom row */
        /* every other destination uses. */
        || draw_centered_placeholder_text(
               renderer,
               115,
               tr(BOUNCE_LOCALE_GAME_END_OK),
               ok
           ) != 0)
        return -1;
    return 0;
}

/*
 * STEP 13G-A -- Level Complete. See ui_shell.h for the contract and
 * reconciliation 13.51 for the audit this implements.
 *
 * WHAT IS DRAWN, and why each item exists:
 *
 *   panel + border      the Game End skeleton (ui_shell.c:2291-2297), reused as the
 *                       structural reference the brief names. The previous
 *                       placeholder's own panel was (18,26,92,76) with two 56 px
 *                       accent bars at (36,50,56,8) and (36,66,56,8). Those bars
 *                       cannot hold the Java body text -- "Level 1 completed!" is
 *                       19 characters, about 114 px at the 6 px advance, against a
 *                       56 px band -- so the panel is widened to the shared geometry
 *                       rather than shrinking the content.
 *   NO title, NO rule   Java's Form title is the empty string (BounceGame.java:205),
 *                       so there is nothing to draw where render_game_end draws its
 *                       title at y=9 and its rule at y=19.
 *   body line(s)        BounceGame.java:206, `form.append(v.LevelCompletedString)`.
 *                       One line in most locales; the two Chinese strings carry an
 *                       embedded newline and are drawn as two.
 *   ONE blank row       BounceGame.java:207, `form.append("\n\n")`. Drawn as exactly
 *                       one empty row, i.e. the score always sits one full row below
 *                       the LAST body line, in every locale.
 *   score               BounceGame.java:208, `form.append("" + this.E + "\n")`. The
 *                       live total, which already includes the completion bonus. It
 *                       is a raw integer: no zero padding, no separators, and NOT
 *                       draw_native_score_zero, whose leading-zero HUD treatment is a
 *                       different, gameplay-only path (e.java:183).
 *   CONTINUE            BounceGame.java:204/:209, the single Command.OK. On the
 *                       shared bottom row at y=115.
 *
 * NOT DRAWN, because Java does not draw them: the high score (the `q` flag is read
 * only inside ShowGameEnd at BounceGame.java:188), lives, an image, an icon, and any
 * second command.
 *
 * PIXEL GEOMETRY IS NOT SOURCE-DERIVED and no parity is claimed. `new Form` carries
 * no coordinates anywhere in the recovered source, so item positions, fonts, margins
 * and title-bar presence are not establishable; the same discipline
 * render_game_end documents at :2254-2261 is applied here. What is reproduced is the
 * ITEM ORDER, the SEPARATENESS of the blank and score bands, and the content.
 *
 * The geometry anchors below are the existing native values, not measurements of
 * the original.
 */

/*
 * Expand a signed decimal into `out`. Hand-rolled rather than snprintf() so the
 * buffer arithmetic is local and auditable, and so no locale-dependent digit
 * grouping can be introduced by the C library.
 */
static int ui_decimal(int value, char *out, size_t out_size)
{
    char reverse[16];
    size_t length = 0u;
    size_t index = 0u;
    unsigned int magnitude;
    int negative = value < 0;

    if (out == NULL || out_size < 2u)
        return -1;
    magnitude = negative ? (unsigned int)(-(value + 1)) + 1u : (unsigned int)value;
    do {
        reverse[length] = (char)('0' + (int)(magnitude % 10u));
        ++length;
        magnitude /= 10u;
    } while (magnitude != 0u && length < sizeof reverse);
    if (negative && length + 1u < out_size)
        out[index++] = '-';
    while (length > 0u && index + 1u < out_size) {
        --length;
        out[index++] = reverse[length];
    }
    out[index] = '\0';
    return 0;
}

/*
 * Expand the single "%U" of BOUNCE_LOCALE_LEVEL_COMPLETED into `level_id` and split
 * the embedded newline into at most UI_LEVEL_COMPLETED_LINES lines.
 *
 * DELIBERATELY NOT A FORMATTING ENGINE. It handles one placeholder and one
 * separator, for one string, into a caller-supplied fixed buffer. The locale layer
 * is untouched, exactly as it is for the "%0U"/"%1U" placeholders in
 * BOUNCE_LOCALE_INSTRUCTIONS_BODY, which are likewise never substituted.
 *
 * Bytes are copied, not decoded, so the Thai and CJK strings pass through intact
 * and reach the existing script-font step unchanged.
 */
static int ui_compose_level_completed(
    const char *text,
    int level_id,
    char out[UI_LEVEL_COMPLETED_LINES][UI_LEVEL_COMPLETED_LINE],
    int *count_out
)
{
    char digits[16];
    size_t written = 0u;
    int line = 0;

    if (text == NULL || out == NULL || count_out == NULL)
        return -1;
    if (ui_decimal(level_id, digits, sizeof digits) != 0)
        return -1;
    out[0][0] = '\0';
    *count_out = 1;
    while (*text != '\0') {
        char byte = *text;

        if (byte == '\n') {
            ++line;
            if (line >= UI_LEVEL_COMPLETED_LINES) {
                /* A third line has no band; stop rather than overflow a buffer. */
                *count_out = UI_LEVEL_COMPLETED_LINES;
                return 0;
            }
            out[line][0] = '\0';
            written = 0u;
            ++text;
            continue;
        }
        if (byte == '%' && text[1] == 'U') {
            const char *source = digits;
            while (*source != '\0') {
                if (written + 1u >= UI_LEVEL_COMPLETED_LINE)
                    return -1;
                out[line][written] = *source;
                ++written;
                ++source;
            }
            text += 2;
            continue;
        }
        if (written + 1u >= UI_LEVEL_COMPLETED_LINE)
            return -1;
        out[line][written] = byte;
        ++written;
        text += 1;
    }
    out[line][written] = '\0';
    *count_out = line + 1;
    return 0;
}

int bounce_ui_shell_render_level_complete(
    BounceSurface *surface,
    BounceRenderer *renderer,
    int completed_level_id,
    int32_t score
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
    const uint32_t background = ui->background;
    const uint32_t panel = ui->panel;
    const uint32_t border = ui->border;
    const uint32_t body = ui->body;
    const uint32_t value = ui->text;
    const uint32_t ok = ui->accent;
    char lines[UI_LEVEL_COMPLETED_LINES][UI_LEVEL_COMPLETED_LINE];
    char score_text[UI_LEVEL_COMPLETED_LINE];
    int line_count = 0;
    int line;
    int score_y;

    if (surface == NULL || renderer == NULL)
        return -1;
    if (ui_compose_level_completed(
            tr(BOUNCE_LOCALE_LEVEL_COMPLETED),
            completed_level_id,
            lines,
            &line_count
        ) != 0)
        return -1;
    if (ui_decimal((int)score, score_text, sizeof score_text) != 0)
        return -1;
    if (line_count < 1)
        return -1;

    /* One blank row after the LAST body line is the "\n\n" at :207. */
    score_y = UI_LEVEL_COMPLETED_BODY_Y
        + (line_count * UI_LEVEL_COMPLETED_ROW)
        + UI_LEVEL_COMPLETED_ROW;

    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
        || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0)
        return -1;

    /* :206 the LevelCompletedString, one row per embedded-newline segment. */
    for (line = 0; line < line_count; ++line) {
        if (draw_centered_placeholder_wrapped(
                renderer,
                UI_LEVEL_COMPLETED_BODY_Y + (line * UI_LEVEL_COMPLETED_ROW),
                lines[line],
                body
            ) != 0)
            return -1;
    }

    /* :208 the score. No pad, no separator, no second score variable. */
    if (draw_centered_placeholder_text(
            renderer,
            score_y,
            score_text,
            value
        ) != 0)
        return -1;

    /* :204/:209 the single Command.OK, on the shared bottom row. */
    if (draw_centered_placeholder_text(
            renderer,
            115,
            tr(BOUNCE_LOCALE_CONTINUE),
            ok
        ) != 0)
        return -1;
    return 0;
}


/*
 * STEP 13G-B -- Game Over. See ui_shell.h for the contract.
 *
 * WHAT REPLACED WHAT. The Game Over destination was rendered by
 * render_game_over_state() in vertical_slice.c, a twelve-line function marked
 * "TEMPORARY NATIVE UI REPRESENTATION -- NOT ORIGINAL GAME-OVER PIXEL PARITY"
 * which drew five rectangles and NO TEXT. It drew no title, no body, no score and
 * no command. This function draws all four of the items
 * BounceGame.java:177-195 puts in the Form. The placeholder is removed at its call
 * site rather than left as dead code.
 *
 * STRUCTURAL TEMPLATE. bounce_ui_shell_render_game_end() is the reference, because
 * it is the same MIDP Form shape for the sibling destination and already uses the
 * established native Form conventions: background fill, a (4,4,120,120) panel, four
 * 1 px borders, a title at y=9 with the separator rule at y=19, the body row, then
 * the single OK on the shared bottom row at y=115. Reusing that geometry is what
 * keeps this addition small and consistent; it is NOT a claim of original-device
 * pixel parity, which is device-owned (U-05, U-07).
 *
 * WHY THE TITLE KEY IS PASSED TWICE. Java's not-won arm at :186 appends
 * Translation.GAME_OVER, and the Form title at :181 is also Translation.GAME_OVER.
 * BOUNCE_LOCALE_GAME_END_TITLE is that one translation (locale.h:164, id 6), so
 * title and body legitimately read the same string. No second key is invented.
 *
 * SCORE DISPLAY, PARTIALLY DEFERRED. The score line IS drawn, from the live
 * `score` argument, because BounceGame.java:427 establishes this.E = this.v.score
 * and that is exactly what :192 prints as String.valueOf(this.E). Nothing is added
 * to it: the +5000 completion bonus (game.c:1350) and any pickup score are already
 * in that total, and this renderer is display-only.
 *
 * THE NEW-HIGH-SCORE LINE IS NOT DRAWN, deliberately. BounceGame.java:188-190
 * appends Translation.NEW_HIGH_SCORE when this.q is set. This build has no
 * reconciled native owner for that value, and locale.h:170-173 states there is
 * deliberately no key for it precisely because adding one "would invite rendering
 * a value this build cannot honestly produce". That is still true, so the band is
 * left empty rather than filled with a fabricated string. This mirrors
 * bounce_ui_shell_render_game_end(), which likewise reserves its score and
 * spacer bands without drawing them.
 *
 * INPUT-RELATED CONTROLS ARE UNTOUCHED. draw_input_text(), draw_velocity_bar()
 * and draw_input_slot() remain in vertical_slice.c under the existing debug-only
 * gate; nothing in this file reads, hides or alters them.
 */
int bounce_ui_shell_render_game_over(
    BounceSurface *surface,
    BounceRenderer *renderer,
    int32_t score,
    bool new_high_score
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
    const uint32_t background = ui->background;
    const uint32_t panel = ui->panel;
    const uint32_t border = ui->border;
    const uint32_t title = ui->text;
    const uint32_t body = ui->body;
    const uint32_t value = ui->text;
    const uint32_t ok = ui->accent;
    char score_text[UI_LEVEL_COMPLETED_LINE];

    if (surface == NULL || renderer == NULL)
        return -1;

    /* :192 String.valueOf(this.E). Same ui_decimal() the Level Complete score
     * uses, so both screens print an integer the same way: no zero padding, no
     * separator, negatives as given. */
    if (ui_decimal((int)score, score_text, sizeof score_text) != 0)
        return -1;

    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
        || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0
        /* :181 the Form title, Translation.GAME_OVER. */
        || draw_centered_placeholder_text(
               renderer,
               9,
               tr(BOUNCE_LOCALE_GAME_END_TITLE),
               title
           ) != 0
        || bounce_renderer_fill_rect(renderer, 12, 19, 104, 1, border) != 0
        /* :186 the not-won body, which is the same translation as the title. */
        || draw_centered_placeholder_text(
               renderer,
               32,
               tr(BOUNCE_LOCALE_GAME_END_TITLE),
               body
           ) != 0
        /*
         * D-07 -- :188-190, the NEW_HIGH_SCORE Item.
         *
         *     if (this.q) {
         *         this.form.append(Translation.sprintf_translated(
         *             Translation.NEW_HIGH_SCORE));
         *         this.form.append("\n\n");
         *     }
         *
         * The row was previously left empty on the grounds that no reconciled
         * native owner existed for `q`. One now does:
         * bounce_app_flow_new_high_score() is BounceGame.q, set by
         * bounce_app_flow_note_score() (BounceGame.java:424) and cleared only on
         * the new-game routes (:150), so the flag this renderer is handed IS the
         * source flag rather than a reconstruction of it.
         *
         * The band keeps its row either way, so the score Item at :192 stays at
         * y=58 whether or not the notice is present. That is the source's
         * arrangement: the notice and its "\n\n" are appended BETWEEN the body
         * and the score, so the score moves down by one row when `q` is set and
         * stays put when it is not. This renderer reserves the row unconditionally
         * and therefore prints the score in the same place on both forms, which
         * is the layout choice already made for the Game End form below and the
         * one that keeps the two screens consistent. It is recorded here rather
         * than hidden, because it IS a layout difference from a MIDP Form, whose
         * items flow top to bottom.
         */
        || draw_centered_placeholder_text(
               renderer,
               58,
               new_high_score
                   ? tr(BOUNCE_LOCALE_NEW_HIGH_SCORE) : "",
               new_high_score ? value : body
           ) != 0
        /* :192 the score, on the row below the notice band. */
        || draw_centered_placeholder_text(
               renderer,
               69,
               score_text,
               value
           ) != 0
        /* :180/:193 the single Command.OK, on the shared bottom row. */
        || draw_centered_placeholder_text(
               renderer,
               115,
               tr(BOUNCE_LOCALE_GAME_END_OK),
               ok
           ) != 0)
        return -1;
    return 0;
}


int bounce_ui_shell_render_about(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
const uint32_t background = ui->background;
const uint32_t panel = ui->panel;
const uint32_t border = ui->border;
const uint32_t text = ui->text;
const uint32_t muted = ui->muted;
const uint32_t back = ui->accent;

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_ABOUT)
        return -1;

    /*
     * Only facts established inside this repository. There is deliberately no
     * project URL, author, contact, or social line: none is established anywhere
     * in the source tree, and inventing one would be a fabrication. Every line is
     * kept within the 21-character placeholder text width, and the logical size is
     * drawn with the real digit font.
     */
    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
        || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0
        || draw_centered_placeholder_text(renderer, 9, tr(BOUNCE_LOCALE_ABOUT), text) != 0
        || bounce_renderer_fill_rect(renderer, 12, 19, 104, 1, border) != 0
        || draw_centered_placeholder_text(renderer, 26, tr(BOUNCE_LOCALE_NOKIA_BOUNCE), text) != 0
        || draw_centered_placeholder_text(
               renderer, 38, tr(BOUNCE_LOCALE_NATIVE_LINUX_BUILD), text) != 0
        || draw_centered_placeholder_text(
               renderer, 52, tr(BOUNCE_LOCALE_ORIGINAL_INSPIRED), muted) != 0
        || draw_centered_placeholder_text(
               renderer, 63, tr(BOUNCE_LOCALE_BEHAVIOR_BASED), muted) != 0
        || draw_centered_placeholder_text(
               renderer, 76, tr(BOUNCE_LOCALE_PLATFORM_LINUX), muted) != 0
        || draw_placeholder_text(
               renderer, 12, 89, tr(BOUNCE_LOCALE_LOGICAL), muted) != 0
        || draw_native_size(renderer, 60, 89, 128, 128, text) != 0
        || draw_centered_placeholder_text(
               renderer, 100, tr(BOUNCE_LOCALE_NATIVE_SCREEN), muted) != 0
        || draw_centered_placeholder_text(renderer, 115, tr(BOUNCE_LOCALE_ESC_BACK), back) != 0)
        return -1;
    return 0;
}

/*
 * D-13 / J-01 -- the level-start banner. See ui_shell.h for the contract.
 *
 * The text is composed from the SAME two calls the level List already uses:
 * draw_placeholder_text for the localized label and draw_native_number for the
 * integer, because placeholder_text_index() has no glyph for a digit.
 *
 * The digit's x is derived, not guessed. The 5x7 cell advances 6 px per glyph
 * and placeholder_text_width() returns advance*n - 1, so for a label of width W
 * the pen sits at x + W + 1 after it. lang.xx Translation.LEVEL (id 9) is
 * "Level %U", so the number follows a single space, and one more 6 px cell is
 * consumed by that space:
 *
 *     digit_x = x + placeholder_text_width(label) + 1 + 6
 *
 * For the English "LEVEL" that is 44 + 29 + 1 + 6 = 80, i.e. the digit begins in
 * the sixth cell after the origin, which is where "Level 1" puts it once the
 * lower-case fold (ui_shell.h documents that the 5x7 font folds a-z to A-Z) has
 * made the label "LEVEL".
 */
int bounce_ui_shell_render_level_banner(
    BounceRenderer *renderer,
    int level_id,
    uint32_t color
)
{
    const char *label = tr(BOUNCE_LOCALE_LEVEL);
    int digit_x;

    if (renderer == NULL || label == NULL)
        return -1;
    /* The banner names the level being played, so only a real level id is
     * printable. b.java:221-225 builds the resource name from the same value
     * and would produce an unloadable path outside 1..11. */
    if (level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || level_id > BOUNCE_APP_LAST_LEVEL_ID)
        return -1;

    digit_x = UI_LEVEL_BANNER_X + placeholder_text_width(label) + 1 + 6;
    if (draw_placeholder_text(
            renderer,
            UI_LEVEL_BANNER_X,
            UI_LEVEL_BANNER_Y,
            label,
            color
        ) != 0)
        return -1;
    return draw_native_number(renderer, digit_x, UI_LEVEL_BANNER_Y, level_id, color);
}

/*
 * D-06 -- the Level List title, exposed so the verifier can assert WHICH locale
 * row the renderer uses rather than only that the screen draws.
 *
 * The value is Translation.NEW_GAME, id 11, because BounceGame.java:142 passes
 * it as the first constructor argument of the MIDP List:
 *
 *     new List(Translation.sprintf_translated(Translation.NEW_GAME),
 *              List.IMPLICIT, levelStrings, null)
 *
 * In Java that row is ALSO the main menu's second entry (BounceGame.java:106,
 * appended at :118), so the two must resolve to the same string in every
 * language. The verifier checks exactly that, by comparing this accessor against
 * bounce_ui_shell_menu_label(1u) -- an identity check, which a text comparison
 * could not make, because two different rows could read alike.
 */
const char *bounce_ui_shell_level_selection_title(void)
{
    return tr(BOUNCE_LOCALE_NEW_GAME);
}

int bounce_ui_shell_render_level_selection(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
    static const int row_y[BOUNCE_APP_LAST_LEVEL_ID] = {
        24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104
    };
const uint32_t background = ui->background;
const uint32_t title = ui->text;
const uint32_t row = ui->body;
const uint32_t highlight = ui->highlight;
const uint32_t selected_text = ui->selected_text;
const uint32_t back = ui->accent;
    unsigned int index;
    unsigned int count;
    unsigned int selected_index;

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_LEVEL_SELECTION)
        return -1;
    count = flow->level_selection.available_level_count;
    selected_index = flow->level_selection.selected_index;
    if (count == 0u
        || count > BOUNCE_APP_LAST_LEVEL_ID
        || selected_index >= count)
        return -1;
    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        /*
         * D-06 -- the title is a RECOVERED STRING, not a native label.
         *
         * BounceGame.java:135-146 builds this exact list:
         *
         *     public void LevelSelect() {
         *         String[] levelStrings = new String[this.MaxLevels];
         *         ...
         *         this.currentList = new List(
         *             Translation.sprintf_translated(Translation.NEW_GAME),
         *             List.IMPLICIT, levelStrings, null);
         *         this.currentList.addCommand(this.commandBack);
         *         ...
         *
         * A MIDP List's title is the FIRST CONSTRUCTOR ARGUMENT. Here it is
         * passed literally, so it is Translation.NEW_GAME -- id 11, "New game" in
         * lang.xx -- and not anything the invoking item supplies.
         *
         * THE PREVIOUS COMMENT WAS WRONG and is corrected below. It stated that
         * "the MIDP List supplied its own title, so no recovered string exists
         * for it", and named BOUNCE_LOCALE_LEVEL_SELECT a NATIVE EXTENSION.
         * LevelSelect() contradicts that on the face of it. The source wins, and
         * this is recorded as a comment-vs-source CONFLICT rather than a silent
         * edit.
         *
         * The title is therefore the SAME locale row the main menu's second entry
         * uses (BOUNCE_LOCALE_NEW_GAME), which is also what Java does: the menu
         * row and this list title are both Translation.NEW_GAME, so they are
         * identical strings in every language.
         *
         * BOUNCE_LOCALE_LEVEL_SELECT is left in the locale table. It is a
         * resource-shaped table mirroring the 14 shipped translation ids plus
         * the native rows, and one unused key is cheaper than renumbering a
         * positional table -- which is exactly the hazard locale.c records
         * ("the table is POSITIONAL").
         */
        || draw_centered_placeholder_text(
            renderer,
            7,
            bounce_ui_shell_level_selection_title(),
            title
        ) != 0)
        return -1;
    for (index = 0u; index < count; ++index) {
        if (index == selected_index
            && bounce_renderer_fill_rect(
                renderer,
                12,
                row_y[index] - 2,
                104,
                9,
                highlight
            ) != 0)
            return -1;
        if (draw_placeholder_text(
                renderer,
                22,
                row_y[index],
                tr(BOUNCE_LOCALE_LEVEL),
                index == selected_index ? selected_text : row
            ) != 0
            || draw_native_number(
                renderer,
                62,
                row_y[index],
                (int)index + 1,
                index == selected_index ? selected_text : row
            ) != 0)
            return -1;
    }
    if (draw_centered_placeholder_text(
            renderer,
            116,
            level_selection_back,
            back
        ) != 0)
        return -1;
    return 0;
}

/* Draw one "LABEL value" row with a fixed provisional column layout. */
static int draw_entry_metadata_row(
    BounceRenderer *renderer,
    int y,
    const char *label,
    int label_x,
    int value_x,
    int value,
    uint32_t label_color,
    uint32_t value_color
)
{
    if (draw_placeholder_text(
            renderer,
            label_x,
            y,
            label,
            label_color
        ) != 0)
        return -1;
    return draw_native_number(renderer, value_x, y, value, value_color);
}

/* PROVISIONAL NATIVE GAMEPLAY-ENTRY GEOMETRY — NOT ORIGINAL CANVAS. */
int bounce_ui_shell_render_gameplay_entry(
    BounceSurface *surface,
    BounceRenderer *renderer,
    const BounceAppFlow *flow
)
{
    const BounceUiPalette *ui = bounce_ui_shell_palette();
const uint32_t background = ui->background;
const uint32_t panel = ui->panel;
const uint32_t border = ui->border;
const uint32_t text = ui->text;
const uint32_t value = ui->body;
const uint32_t warning = ui->accent;
    const char *action_label;
    const BounceGameplayEntry *entry;

    if (surface == NULL || renderer == NULL || flow == NULL)
        return -1;
    if (flow->state != BOUNCE_APP_STATE_GAMEPLAY_ENTRY)
        return -1;
    if (flow->gameplay_entry.phase
        != BOUNCE_GAMEPLAY_ENTRY_PHASE_INIT_METADATA_RECORDED)
        return -1;
    if (flow->gameplay_entry.level_id < BOUNCE_APP_FIRST_LEVEL_ID
        || flow->gameplay_entry.level_id > BOUNCE_APP_LAST_LEVEL_ID)
        return -1;
    if (flow->gameplay_entry.level_loaded
        || flow->gameplay_entry.timer_started
        || flow->gameplay_entry.player_created)
        return -1;
    action_label = bounce_ui_shell_action_label(flow->menu.pending_action);
    if (action_label == NULL)
        return -1;
    entry = &flow->gameplay_entry;

    if (bounce_surface_fill(surface, background) != 0
        || bounce_renderer_reset_clip(renderer) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 120, panel) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 123, 120, 1, border) != 0
        || bounce_renderer_fill_rect(renderer, 4, 4, 1, 120, border) != 0
        || bounce_renderer_fill_rect(renderer, 123, 4, 1, 120, border) != 0
        || draw_centered_placeholder_text(
            renderer, 10, tr(BOUNCE_LOCALE_TITLE), text) != 0
        || draw_centered_placeholder_text(renderer, 24, action_label, text) != 0
        || draw_entry_metadata_row(
            renderer,
            38,
            tr(BOUNCE_LOCALE_LEVEL),
            24,
            68,
            entry->level_id,
            text,
            value
        ) != 0
        || draw_entry_metadata_row(
            renderer,
            50,
            tr(BOUNCE_LOCALE_SCORE),
            24,
            68,
            entry->initial_score,
            text,
            value
        ) != 0
        || draw_entry_metadata_row(
            renderer,
            62,
            tr(BOUNCE_LOCALE_LIVES),
            24,
            68,
            entry->initial_lives,
            text,
            value
        ) != 0
        || draw_entry_metadata_row(
            renderer,
            74,
            tr(BOUNCE_LOCALE_COUNTDOWN),
            8,
            74,
            entry->countdown,
            text,
            value
        ) != 0
        || draw_centered_placeholder_text(renderer, 90, tr(BOUNCE_LOCALE_GAMEPLAY), warning) != 0
        || draw_centered_placeholder_text(
            renderer,
            100,
            tr(BOUNCE_LOCALE_NOT_STARTED),
            warning
        ) != 0
        || draw_centered_placeholder_text(renderer, 113, tr(BOUNCE_LOCALE_BACK_TO_MENU), text)
            != 0)
        return -1;
    return 0;
}
