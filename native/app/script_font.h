/*
 * NATIVE BOUNDARY: a tiny pre-rendered fallback font for the temporary CN / TW /
 * TH test strings.
 *
 * WHY THIS EXISTS
 * ---------------
 * The native UI font is a 5x7 A-Z plus space placeholder set. It has no CJK
 * glyphs, so without this module the temporary test strings could not be shown at
 * all, and the honest options would be to fall back to English, which would hide
 * the very thing being tested, or to draw a placeholder box for every character,
 * which would not show the text either.
 *
 * WHAT IT IS NOT
 * --------------
 * It is not a font implementation, it is not a text layout engine, and it is not
 * a claim about the original Nokia Bounce presentation. It is a lookup table of
 * fixed-size one-bit bitmaps covering only the clusters the three temporary tables
 * actually use. Every bitmap was rasterised from a real installed font by
 * tools/gen_script_font.py, so the shapes are genuine rather than invented.
 *
 * A production build would link a real font and a real shaper instead. This
 * module exists so runtime language switching can be tested on screen at 128x128
 * without pretending to be more than it is.
 *
 * CELL SIZE
 * ---------
 * 12x12 is not arbitrary. The native lists are drawn on a 12 px row pitch for the
 * main menu, 14 px for the language list, 16 px for the Settings root, and only
 * 8 px for the level list. A 14 px cell overflowed the 12 px pitch and the menu
 * labels visibly collided, so 12 is the largest cell that fits the pitch the menu
 * and Settings rows actually use. CJK is therefore rendered inside the existing
 * layout rather than by changing that layout.
 *
 * THAI: SUPPORTED, AND THE REASON IT WAS NOT IS CORRECTED HERE
 * -----------------------------------------------------------
 * This section used to read:
 *
 *     THAI: NOT SUPPORTED, AND DELIBERATELY SO
 *     Thai vowel signs and tone marks stack above and below the consonant, so a
 *     Thai glyph is far taller than a Latin or CJK one. Measured against real
 *     Noto Sans Thai, the whole test table needs 13 rows even at 8 px, and only
 *     7 px fits in 12 rows, which is not legible. ... so no Thai glyph set is
 *     shipped: TH falls through to the 5x7 path ...
 *
 * THE MEASUREMENT IN THAT TEXT WAS WRONG, TWICE OVER, and both errors pointed the
 * same way. Thai does need a taller cell than Latin, because its marks stack above
 * and below the consonant -- that part is right. But:
 *
 *   - Re-measuring every cluster the Thai column actually contains (151 of them,
 *     across all eighty-five strings in locale.c, not just the resource ids)
 *     against the same font gives ELEVEN rows at 8 px, not thirteen. Eleven fits
 *     the 12-row cell with nothing clipped. The set therefore ships at 9 px, the
 *     largest size that still fits whole, with the SAME advance as the Chinese
 *     sets so no existing layout moves.
 *   - The measurement itself had been taken over the wrong shapes. The generator
 *     used `unicodedata.combining()` to decide what joins a Thai cluster, which
 *     is 0 for nine of the marks this game uses -- including U+0E31, U+0E34-37
 *     and U+0E47. Those were measured as standalone glyphs the runtime never
 *     looks up. gen_script_font.py now mirrors ui_char_step()'s predicates
 *     exactly, and the generator asserts the resulting cluster-key length.
 *
 * Clipping is the metric that matters and the reason the old figure was
 * misleading: render() does not fail on out-of-cell ink, it silently discards it.
 * At 10 px Thai needs 13 rows and loses one row of mark pixels from two clusters;
 * at 12 px it needs 14 and loses two rows from eleven. Neither would have been
 * reported as a problem, which is why "13 rows even at 8 px" went unchecked.
 *
 * ONE CLUSTER HAS NO INK BY DESIGN. lang.th-TH's NEW_HIGH_SCORE (id 12) contains
 * U+200B ZERO WIDTH SPACE. It is emitted as an all-zero cell at the normal
 * advance, so it draws nothing and keeps the surrounding spacing -- rather than
 * being absent from the table, which would have made the runtime's
 * unknown-glyph fallback draw a box in the middle of an otherwise correct
 * string.
 *
 * CLUSTERING
 * ----------
 * The table is keyed by whole clusters. A cluster boundary is a shaping decision,
 * so the caller owns it: it decodes the UTF-8 and passes the cluster length in.
 * See bounce_script_font_lookup().
 *
 * SCOPE
 * -----
 * Used only by the UI shell. Gameplay, level and camera rendering never call into
 * this module.
 */
#ifndef BOUNCE_SCRIPT_FONT_H
#define BOUNCE_SCRIPT_FONT_H

#include <stdint.h>

#include "locale.h"

/* Every glyph is one fixed-size cell so the table needs no per-entry size. */
#define BOUNCE_SCRIPT_CELL_WIDTH 12
#define BOUNCE_SCRIPT_CELL_HEIGHT 12
/*
 * V-2 -- THIS IS 16, AND WAS 12, AND THE OLD VALUE WAS DERIVED WRONG.
 *
 * It used to read:
 *
 *     The longest cluster the generator emits is three Thai codepoints; 12 bytes
 *     bounds that with room for the NUL.
 *
 * Three Thai codepoints is nine bytes, so 12 did bound that -- but the generator
 * was splitting Thai clusters wrongly (see is_thai_combining_mark() in
 * tools/gen_script_font.py for why `unicodedata.combining()` is the wrong test),
 * so the clusters it actually measured were not the clusters the runtime asks
 * for. With the rules corrected the longest cluster across all three script
 * languages is FOUR codepoints: BOUNCE_LOCALE_START_LEVEL's "เริ่" is
 * U+0E40 U+0E23 U+0E34 U+0E48, twelve bytes, thirteen with the NUL.
 *
 * 16 is that thirteen rounded up with room for a five-codepoint cluster. The
 * generator asserts the bound, so a longer cluster would fail loudly at
 * generation time rather than silently truncating a key and turning that
 * cluster into a box.
 */
#define BOUNCE_SCRIPT_KEY_BYTES 16

typedef struct BounceScriptGlyph {
    /* NUL-padded UTF-8 bytes of the cluster this bitmap is shaped for. */
    char utf8[BOUNCE_SCRIPT_KEY_BYTES];
    /* One bitmask per cell row; bit (WIDTH-1-column) is the leftmost pixel. */
    uint16_t rows[BOUNCE_SCRIPT_CELL_HEIGHT];
} BounceScriptGlyph;

typedef struct BounceScriptMetrics {
    int cell_width;
    int cell_height;
    int advance;   /* horizontal pixels per cluster, including the gap */
    int baseline;  /* row inside the cell that sits on the text baseline */
} BounceScriptMetrics;

typedef struct BounceScriptSet {
    const char *name;
    const BounceScriptGlyph *glyphs;
    unsigned int count;
    int advance;
    int baseline;
    BounceLanguage language;
} BounceScriptSet;

/*
 * Mirror the flow's applied language, the same way the locale module does. A
 * fresh flow starts at EN, which has no script set and keeps the 5x7 font.
 */
void bounce_script_font_set_active_language(BounceLanguage language);
BounceLanguage bounce_script_font_active_language(void);

/*
 * Metrics for the active language, or all zeroes for a language with no script
 * set (EN and ID), which use the 5x7 font. The returned object is owned by
 * this module and must not be written through.
 */
const BounceScriptMetrics *bounce_script_font_metrics(void);

/*
 * Look up ONE cluster of exactly `length` UTF-8 bytes at the start of utf8.
 *
 * The length is supplied by the caller on purpose. The table is keyed by whole
 * clusters, and a cluster boundary is a shaping decision, not something this
 * module can rediscover from a bare pointer: copying bytes until the string ends
 * compares the entire remaining string against three-byte keys and misses
 * everything except the final character. The caller owns the UTF-8 decode and
 * therefore owns the cluster length.
 *
 * Returns NULL when the active language has no script set or the cluster is not
 * in the table.
 */
const BounceScriptGlyph *bounce_script_font_lookup(
    const char *utf8,
    int length
);

/* Glyph count in the active language's set; 0 when it has no set. */
unsigned int bounce_script_font_glyph_count(void);

#endif
