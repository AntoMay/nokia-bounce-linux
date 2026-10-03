#!/usr/bin/env python3
"""Generate native/app/script_font.c from real font outlines.

Run from native/app:   python3 tools/gen_script_font.py

PROVENANCE AND STATUS
---------------------
Every bitmap in script_font.c is rendered here by Pillow from the Noto Sans CJK
SC/TC and Noto Sans Thai source fonts named below, then thresholded to 1 bit.
Nothing is hand-drawn and nothing is invented, so the glyphs are genuine
simplified-Chinese and traditional-Chinese shapes rather than a plausible-looking
fabrication.

These glyphs exist so the SHIPPED Chinese and Thai resources can be seen on
screen. They are not a font implementation, and they are not a claim about the
original Nokia Bounce presentation. A production build would link a real font
instead.

LICENCE POSITION
----------------
The source-font provenance, and the licence those font files declare for
themselves, are recorded in reverse/EXTERNAL-SOURCES-REGISTRY.md under
ASSET-001 and ASSET-002. This project does NOT independently assert a separate
licence grant for the generated bitmap data, and it makes no legal claim about
that data in either direction. The generated banner repeats this so the claim
travels with the generated file.

Thai IS INCLUDED as of V-2, at 9 px with the Chinese sets' advance. The note
that said it was not, and that "the whole table needs 13 rows even at 8 px", was
wrong: re-measured over every cluster lang.th-TH uses, Thai needs ELEVEN rows at
8 px and fits the 12-row cell with nothing clipped. An earlier revision of THIS
DOCSTRING also claimed the glyphs were "simplified-Chinese, traditional-Chinese
and Thai" while main() excluded Thai, which contradicted the generated file; both
claims have now been corrected against a measurement rather than left to mislead
the next reader. The Thai CLUSTER machinery below is what makes that measurement
possible: Thai marks stack above and below their consonant, so a cluster has to be
shaped as a unit, and that is also why the sizes above 9 px lose mark pixels.
"""
import os
import re
import sys

from PIL import Image, ImageDraw, ImageFont

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import test_translations as tt

CELL_W = 12
# Mirrors BOUNCE_SCRIPT_KEY_BYTES in script_font.h; the header comment there
# records why it is 16 and not the 12 this file used to assume.
KEY_BYTES = 16
CELL_H = 12

CJK_TTC = "/usr/share/fonts/opentype/noto/NotoSansCJK-Regular.ttc"
FACE_SC = 2   # Noto Sans CJK SC
FACE_TC = 3   # Noto Sans CJK TC
THAI_TTF = "/usr/share/fonts/truetype/noto/NotoSansThai-Regular.ttf"

THAI_LEAD_VOWELS = set(range(0x0E40, 0x0E45))

# V-2 -- THE CLUSTER RULES ARE THE RUNTIME'S RULES, NOT UNICODE'S.
#
# This walker used to test `unicodedata.combining(c)`, on the reasonable but
# wrong assumption that a combining mark is a character with a non-zero combining
# class. For Thai that is false for most of the marks the game actually uses:
#
#     U+0E31 ccc=0   MAI HAN AKAT          U+0E47 ccc=0   MAI TAI KHU
#     U+0E34 ccc=0   SARA I                U+0E4C ccc=0   THANTHAKHAT
#     U+0E35 ccc=0   SARA II               U+0E4D ccc=0   NIKHAHIT
#     U+0E36 ccc=0   SARA UE               U+0E4E ccc=0   YAMAKKAN
#     U+0E37 ccc=0   SARA UEE
#
# Only U+0E38/U+0E39 (below-vowel, ccc=103), U+0E3A (phinthu, ccc=9) and the four
# tone marks U+0E48..U+0E4B (ccc=107) report a non-zero class. So the old walker
# split nine of the marks this game uses off into clusters of their own, stored
# them as single-codepoint keys, and the runtime -- which groups base + marks --
# then asked for a byte sequence the table did not contain. The result was a box
# in the middle of Thai sentences whose text was perfectly correct.
#
# It also means the measurement that justified excluding Thai was taken over the
# wrong clusters. main() re-runs it against these.
#
# (`import unicodedata` is deliberately absent: it was only ever used for the
# wrong test, and leaving it would invite the mistake back.)
#
# THE RULES BELOW ARE A VERBATIM MIRROR of ui_is_thai_leading_vowel() and
# ui_is_thai_combining_mark() in ui_shell.c. They must not be "improved" here
# independently of there: the table's keys are the byte sequences the runtime
# asks for, so any disagreement between the two is a silent missing glyph rather
# than an error.
def is_thai_leading_vowel(cp):
    return 0x0E40 <= cp <= 0x0E44


def is_thai_combining_mark(cp):
    return (cp == 0x0E31
            or 0x0E34 <= cp <= 0x0E3A
            or 0x0E47 <= cp <= 0x0E4E)


def clusters(text):
    """Split into shaping clusters, mirroring ui_char_step() in ui_shell.c.

    A leading vowel pulls in the consonant that follows it and then that
    consonant's marks; any other base pulls in its own marks. The walk order and
    the two predicates are the runtime's, so a key stored here is the byte
    sequence ui_char_step() will ask for.
    """
    out = []
    i = 0
    n = len(text)
    while i < n:
        if is_thai_leading_vowel(ord(text[i])):
            j = i
            while j < n and is_thai_leading_vowel(ord(text[j])):
                j += 1
            if j < n:
                j += 1
            while j < n and is_thai_combining_mark(ord(text[j])):
                j += 1
            out.append(text[i:j])
            i = j
        else:
            j = i + 1
            while j < n and is_thai_combining_mark(ord(text[j])):
                j += 1
            out.append(text[i:j])
            i = j
    return out


# ---------------------------------------------------------------------------
# V-2 -- THE SOURCE OF TRUTH FOR WHAT THE SHELL CAN DRAW IS locale.c ITSELF.
# ---------------------------------------------------------------------------
#
# The generator originally took its characters from test_translations.py, which
# transcribes the shipped translation ids from src/main/resources. That is the
# right input for those ids and the wrong input for a FONT, because locale.c
# holds eighty-five strings per language and sixty-nine of them are native
# extensions with no resource behind them -- the Settings page labels, the eleven
# theme names, the key names, "RESOURCE ONLY", and so on, all written out in
# Chinese and Thai.
#
# The result was a font that covered the resource text and nothing else, so every
# one of those sixty-nine strings drew unknown-glyph boxes. It was invisible while
# Chinese and Thai were documented as "text correct, glyphs absent", and shipping
# a Thai set turned a known limitation into a visible one.
#
# Parsing locale.c removes the possibility of the font falling behind the shell:
# the list below is exactly what bounce_locale_text() can return, so a new native
# string in a CJK or Thai column is covered by the next run of this generator
# rather than silently rendering as boxes. self_check() then compares the parsed
# resource ids against test_translations.py's independent reading of the binary
# resources, so this parse cannot quietly become the authority on the ids it is
# only borrowing.
#
# NOTHING IS HAND-TRANSCRIBED HERE. Every literal is read out of the C source, so
# a typo is not possible and re-running after editing locale.c is the whole update
# procedure.
LOCALE_C = os.path.join(
    os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "locale.c")


def _c_unescape(text):
    r"""Decode the C escapes that can appear in a locale_text literal.

    Only the four this table uses are handled; anything else is passed through
    unchanged rather than silently dropped, so a stray escape in a label shows up
    in the generated font instead of being papered over.

    This is not cosmetic. "LEVEL%U\nCOMPLETED" and a literal backslash-n are the
    same string to the C compiler and different strings to a Python comparison,
    which is exactly how a self-check starts reporting phantom mismatches on data
    that is actually identical.
    """
    out = []
    i = 0
    while i < len(text):
        if text[i] != "\\":
            out.append(text[i])
            i += 1
            continue
        i += 1
        if i >= len(text):
            out.append("\\")
            break
        ch = text[i]
        if ch == "n":
            out.append("\n")
        elif ch == "t":
            out.append("\t")
        elif ch == "\\":
            out.append("\\")
        elif ch == '"':
            out.append('"')
        else:
            out.append("\\" + ch)
        i += 1
    return "".join(out)


def _define_bodies(src):
    r"""Every `#define NAME ...` body in locale.c, continuations joined."""
    out = {}
    for match in re.finditer(r"^#define\s+([A-Za-z_][A-Za-z_0-9]*)", src, re.M):
        name = match.group(1)
        i = match.end()
        stop = i
        while i < len(src):
            end = src.find("\n", i)
            if end < 0:
                end = len(src)
            stop = end
            if not src[i:end].rstrip().endswith("\\"):
                break
            i = end + 1
        # `stop` is the end of the LAST line read, not the start of it: a define
        # whose value is one continued literal ends with a line carrying no
        # trailing backslash, and stopping before that line yields an empty body.
        out[name] = src[match.end():stop]
    return out


def _c_string_entries(row, defines):
    r"""The (key, text) pairs of one locale_text row.

    Three things about locale.c force this shape.

    COMMENTS ARE DATA HERE, not decoration: every entry is annotated
    `{ /* BOUNCE_LOCALE_LEVEL */ "LEVEL", ... }`, and that annotation is the only
    reliable way to know which key a literal belongs to. Positional indexing is
    NOT usable -- the table interleaves the eleven theme-name keys, which
    test_translations.py's KEYS list does not carry, so a shipped id is not at the
    index its ordinal suggests. An earlier revision indexed positionally and
    reported over a hundred mismatches, every one of them a real string compared
    against its neighbour.

    ADJACENT LITERALS JOIN, because C concatenates "ab" "cd" into "abcd" and a
    label written across two literals is one string as far as the font is
    concerned.

    TWO ENTRIES ARE MACRO REFERENCES, not literals:
    `/* BOUNCE_LOCALE_INSTRUCTIONS_BODY */ BOUNCE_INSTRUCTIONS_EN,` and its PREFIX
    sibling. Those two are the longest strings in every language, so taking only
    the literals would drop exactly the text the Thai font most needs to cover.
    The macro body is resolved here instead.

    An entry that resolves to neither a literal nor a known macro is rejected
    rather than guessed at: the generator's guarantee is that it covers every
    string the shell can draw, and an entry it cannot read is one it cannot
    account for.
    """
    literal = re.compile(r'"((?:[^"\\]|\\.)*)"')
    entries = re.findall(
        r'/\*\s*(BOUNCE_LOCALE_[A-Z0-9_]+)\s*\*/\s*'
        r'((?:"(?:[^"\\]|\\.)*"\s*)+|[A-Za-z_][A-Za-z_0-9]*)',
        row)
    if not entries:
        raise SystemExit("no key-annotated entries found in a locale_text row")
    out = []
    for key, value in entries:
        value = value.strip()
        if literal.fullmatch(value):
            text = literal.match(value).group(1)
        elif value in defines:
            text = "".join(m.group(1) for m in literal.finditer(defines[value]))
            if not text:
                raise SystemExit("%s references macro %s, which has no literal"
                                 % (key, value))
        else:
            raise SystemExit("%s is neither a literal nor a known macro: %s"
                             % (key, value))
        out.append((key, _c_unescape(text)))
    return out


def locale_strings():
    r"""{language name: [(key, text), ...]}, in table order.

    The rows are designated initialisers -- `[BOUNCE_LANGUAGE_ZH_CN] = { ... }` --
    so each is located by its designator and then brace-matched, rather than by
    counting rows: brace counting alone would count the outer array as a row.

    Raises rather than guessing if the five rows cannot be found or do not each
    hold the same set of keys, so a structural change to locale.c fails the
    generator loudly instead of quietly producing a smaller font than the shell
    needs.
    """
    src = open(LOCALE_C, encoding="utf-8").read()
    defines = _define_bodies(src)
    out = {}
    for name in ("EN", "ZH_CN", "ZH_TW", "TH_TH", "ID"):
        marker = "[BOUNCE_LANGUAGE_%s] = {" % name
        at = src.find(marker)
        if at < 0:
            raise SystemExit("locale_text row %s not found in %s"
                             % (name, LOCALE_C))
        i = src.index("{", at + len(marker) - 1)
        depth = 0
        for j in range(i, len(src)):
            if src[j] == "{":
                depth += 1
            elif src[j] == "}":
                depth -= 1
                if depth == 0:
                    break
        else:
            raise SystemExit("unterminated locale_text row %s" % name)
        out[name] = _c_string_entries(src[i:j + 1], defines)
    shapes = set(tuple(k for k, _ in v) for v in out.values())
    if len(shapes) != 1:
        # Rows with different key sets would mean one language silently ships a
        # font for a different set of strings than another.
        raise SystemExit("locale_text rows do not share one key set")
    return out


def self_check():
    """The parsed table must agree with the resource transcription.

    test_translations.py decodes the shipped ids straight out of the binary
    resources; locale.c stores the same text. The two are independent readings of
    the same strings, so comparing them is a real cross-check: if either were
    mis-transcribed, or if locale.c had drifted from the resource, this fails
    before a font is written rather than after.

    THE LOOKUP IS BY KEY NAME, for the reason given in _c_string_entries().

    This check found a real defect. locale.c's Thai Instructions macros are built
    from several string literals joined by a backslash, and the transcription had
    lost the space the resource carries at each of those joins -- fourteen of
    them across the body and the prefix. Thai does not space its words, so the
    missing spaces were invisible in the text as a whole, but the resource is
    authoritative and the stored text now matches it exactly.
    """
    from_locale = {name: dict(entries)
                   for name, entries in locale_strings().items()}
    mismatches = []
    checked = 0
    for name in ("ZH_CN", "ZH_TW", "TH_TH"):
        table = dict(tt.TABLES[name])
        table["BOUNCE_LOCALE_INSTRUCTIONS_BODY"] = tt.INSTRUCTIONS[name]
        table["BOUNCE_LOCALE_INSTRUCTIONS_PREFIX"] = tt.PREFIX[name]
        for key, expected in table.items():
            if key not in from_locale[name]:
                mismatches.append("%s %s: absent from locale.c" % (name, key))
                continue
            checked += 1
            if from_locale[name][key] != expected:
                mismatches.append("%s %s: locale.c %r != resource %r"
                                  % (name, key, from_locale[name][key],
                                     expected))
    if mismatches:
        for line in mismatches:
            print("MISMATCH", line)
        raise SystemExit("locale.c and the shipped resources disagree")
    print("self-check: locale.c agrees with all %d shipped strings" % checked)


def render(cluster, font, baseline):
    """Rasterise one cluster into the fixed cell, returning 14 row bitmasks."""
    im = Image.new("L", (CELL_W * 4, CELL_H * 4), 0)
    d = ImageDraw.Draw(im)
    # anchor "ls" = left-baseline; the cell origin is placed at the baseline row.
    d.text((CELL_W * 2, CELL_H * 2 + baseline), cluster, fill=255, font=font,
           anchor="ls")
    box = im.getbbox()
    rows = [0] * CELL_H
    if box is None:
        return None, rows
    x0, y0, x1, y1 = box
    px = im.load()
    # The cell occupies image rows [CELL_H*2, CELL_H*2 + CELL_H); ink above the
    # baseline lands in the upper rows, so the cell top, not the baseline, is the
    # row origin.
    for y in range(y0, y1):
        ry = y - (CELL_H * 2)
        if not (0 <= ry < CELL_H):
            continue
        for x in range(x0, x1):
            rx = x - (CELL_W * 2)
            if not (0 <= rx < CELL_W):
                continue
            # Threshold 64, not 128: at 10 px an ideographic full stop peaks at
            # only 82/255, so a mid-grey cut would erase it entirely while a
            # heavier cut would thin the ordinary glyph strokes. 64 keeps thin
            # punctuation and leaves the body glyphs only slightly bolder.
            if px[x, y] >= 64:
                rows[ry] |= 1 << (CELL_W - 1 - rx)
    return True, rows


def build(name, sources, path, index, size, advance, baseline):
    """Rasterise every cluster in `sources` into one language's glyph table.

    `sources` is every string the shell can draw in that language, and it comes
    from locale_strings() -- i.e. from locale.c itself. It is passed in rather
    than reconstructed here so this function has no opinion about where the text
    came from, and so the one place that decides that (main) is the one place a
    reader has to check.

    The Instructions body and prefix used to be appended here from
    test_translations.py by hand, with the comment "it lives in its own macros
    and is drawn by the Instructions page, so it must be walked too". They are
    now part of `sources` like everything else, which is both simpler and the
    reason they cannot be forgotten again.
    """
    font = ImageFont.truetype(path, size, index=index)
    glyphs = {}
    dropped = []
    for value in sources:
        for cl in clusters(value):
            # Anything ASCII (letters, digits, spaces, punctuation) stays on the
            # existing 5x7 path. Mixing a 12px script glyph with a 5x7 glyph in
            # one string would give the line two different baselines, and the
            # 5x7 font is the native one for Latin.
            if all(ord(c) < 0x80 for c in cl):
                continue
            if cl in glyphs:
                continue
            ok, rows = render(cl, font, baseline)
            # V-2 -- A CLUSTER WITH NO INK IS A BLANK CELL, NOT A DROP.
            #
            # lang.th-TH's NEW_HIGH_SCORE (Translation id 12) contains U+200B
            # ZERO WIDTH SPACE, which by definition has no glyph. Treating it as
            # "dropped" meant it was absent from the table, and the runtime's
            # fallback for an absent script cluster is the UNKNOWN-GLYPH BOX --
            # so the one Thai string that is otherwise perfectly rendered would
            # have contained a box in the middle of it.
            #
            # A blank cell is the honest representation: the advance is kept, so
            # the surrounding text keeps its spacing, and the 12x12 bitmap is all
            # zeroes so nothing is drawn. `ok` is falsy in exactly two cases --
            # render() found no ink box at all, or every pixel fell below the
            # threshold -- and both are a blank cell, not a failure.
            #
            # THIS IS ADDITIVE. Neither Chinese table contains a no-ink cluster,
            # which was checked before the change rather than assumed, so their
            # generated bytes do not move.
            glyphs[cl] = rows
    return {
        "name": name,
        "advance": advance,
        "baseline": baseline,
        "glyphs": glyphs,
        "dropped": dropped,
    }


def c_style(s):
    """Escape a UTF-8 cluster as a fixed-width C byte array."""
    b = s.encode("utf-8")
    # BOUNCE_SCRIPT_KEY_BYTES is 16, so 15 bytes plus the NUL fit. The longest
    # cluster the corrected walker produces is twelve bytes; the margin is for a
    # five-codepoint Thai cluster, which the resources do not currently use.
    assert len(b) <= KEY_BYTES - 1, (s, len(b))
    # Explicit (char) casts: a bare 0xe4 is an out-of-range conversion to signed
    # char, and the project builds with -Werror -pedantic.
    body = ", ".join(f"(char)0x{x:02x}" for x in b) + ", 0"
    return "{" + body + "}"


def emit(langs, path_out):
    total = sum(len(l["glyphs"]) for l in langs)
    out = []
    w = out.append
    w('/*')
    w(' * GENERATED FILE - DO NOT EDIT BY HAND.')
    w(' *')
    w(' * Regenerate with:  python3 tools/gen_script_font.py')
    w(' * (run from native/app; writes native/app/script_font.c)')
    w(' *')
    w(' * Provenance: every bitmap below is a thresholded 1-bit rasterisation')
    w(' * generated by Pillow from Noto Sans CJK SC/TC and Noto Sans Thai')
    w(' * source fonts. Nothing here is hand-drawn.')
    w(' *')
    w(' * The source-font provenance, and the licence those font files declare')
    w(' * for themselves, are recorded in reverse/EXTERNAL-SOURCES-REGISTRY.md')
    w(' * under ASSET-001 and ASSET-002.')
    w(' *')
    w(' * This project does NOT independently assert a separate licence grant')
    w(' * for the generated bitmap data, and makes no legal claim about it.')
    w(' *')
    w(' * These glyphs exist ONLY so the temporary CN / TW / TH test strings are')
    w(' * visible on the 128x128 UI. They are not a font implementation, and')
    w(' * they are not a claim about the original Nokia Bounce presentation.')
    w(' * A production build would link a real font.')
    w(' *')
    w(' * Thai clusters are pre-shaped here (Pillow had Raqm/HarfBuzz available at')
    w(' * generation time), so the runtime does no shaping and only looks a')
    w(' * cluster up by its UTF-8 bytes.')
    w(' */')
    w('')
    w('#include "script_font.h"')
    w('')
    w('#include <string.h>')
    w('')
    w('/* Cluster order is irrelevant: the lookup is a linear scan. */')
    for l in langs:
        w(f'static const BounceScriptGlyph {l["name"]}_glyphs[] = {{')
        for cl in sorted(l["glyphs"], key=lambda s: s.encode("utf-8")):
            rows = l["glyphs"][cl]
            w(f'    {{ {c_style(cl)},')
            w('      { ' + ", ".join(f"0x{r:04x}" for r in rows) + ' } },')
        w('};')
        w('')
    w('static const BounceScriptSet script_sets[] = {')
    for l in langs:
        w(f'    {{ "{l["name"]}", {l["name"]}_glyphs,')
        w(f'      (unsigned int)(sizeof {l["name"]}_glyphs'
          f' / sizeof {l["name"]}_glyphs[0]),')
        w(f'      {l["advance"]}, {l["baseline"]}, '
          f'BOUNCE_LANGUAGE_{l["name"]} }},')
    w('};')
    w('')
    w('#define BOUNCE_SCRIPT_SET_COUNT '
      f'(sizeof script_sets / sizeof script_sets[0])')
    w('')
    w('static BounceLanguage script_active_language = BOUNCE_LANGUAGE_EN;')
    w('')
    w('void bounce_script_font_set_active_language(BounceLanguage language)')
    w('{')
    w('    if ((int)language < 0 || (int)language >= (int)BOUNCE_LANGUAGE_COUNT)')
    w('        return;')
    w('    script_active_language = language;')
    w('}')
    w('')
    w('BounceLanguage bounce_script_font_active_language(void)')
    w('{')
    w('    return script_active_language;')
    w('}')
    w('')
    w('static const BounceScriptSet *active_set(void)')
    w('{')
    w('    unsigned int i;')
    w('')
    w('    for (i = 0u; i < BOUNCE_SCRIPT_SET_COUNT; ++i) {')
    w('        if (script_sets[i].language == script_active_language)')
    w('            return &script_sets[i];')
    w('    }')
    w('    /* EN and ID are pure Latin and use the 5x7 font, so there is no set. */')
    w('    return NULL;')
    w('}')
    w('')
    w('const BounceScriptMetrics *bounce_script_font_metrics(void)')
    w('{')
    w('    /* One object, refilled on each call: the sets are const, and a caller')
    w('     * must never be able to write through the returned pointer. */')
    w('    static BounceScriptMetrics current;')
    w('    const BounceScriptSet *set = active_set();')
    w('')
    w('    current.cell_width = BOUNCE_SCRIPT_CELL_WIDTH;')
    w('    current.cell_height = BOUNCE_SCRIPT_CELL_HEIGHT;')
    w('    if (set == NULL) {')
    w('        current.advance = 0;')
    w('        current.baseline = 0;')
    w('    } else {')
    w('        current.advance = set->advance;')
    w('        current.baseline = set->baseline;')
    w('    }')
    w('    return &current;')
    w('}')
    w('')
    w('const BounceScriptGlyph *bounce_script_font_lookup(const char *utf8,')
    w('    int length)')
    w('{')
    w('    const BounceScriptSet *set = active_set();')
    w('    char key[BOUNCE_SCRIPT_KEY_BYTES];')
    w('    unsigned int i;')
    w('    int j;')
    w('')
    w('    if (set == NULL || utf8 == NULL)')
    w('        return NULL;')
    w('    /* length is the caller\'s cluster length, not a scan to the NUL. */')
    w('    if (length <= 0 || length >= (int)BOUNCE_SCRIPT_KEY_BYTES)')
    w('        return NULL;')
    w('    for (j = 0; j < length; ++j)')
    w('        key[j] = utf8[j];')
    w('    key[length] = \'\\0\';')
    w('')
    w('    /*')
    w('     * A LINEAR scan on purpose.')
    w('     *')
    w('     * A binary search would need the table sorted in exactly the order')
    w('     * strcmp() implies, and strcmp() compares char, which is signed on this')
    w('     * target: every UTF-8 byte above 0x7F compares as negative. A table')
    w('     * generated in unsigned byte order is therefore NOT sorted in strcmp')
    w('     * order, and the search silently misses most clusters. That is exactly')
    w('     * what happened here. A linear scan cannot depend on an ordering')
    w('     * assumption at all, and at a few hundred entries the cost is')
    w('     * irrelevant for a 128x128 UI.')
    w('     */')
    w('    for (i = 0u; i < set->count; ++i) {')
    w('        if (strcmp(set->glyphs[i].utf8, key) == 0)')
    w('            return &set->glyphs[i];')
    w('    }')
    w('    return NULL;')
    w('}')
    w('')
    w('unsigned int bounce_script_font_glyph_count(void)')
    w('{')
    w('    const BounceScriptSet *set = active_set();')
    w('')
    w('    return set == NULL ? 0u : set->count;')
    w('}')
    w('')
    with open(path_out, "w", encoding="utf-8") as f:
        f.write("\n".join(out))
    return total


def main():
    # V-2 -- THAI IS NOW INCLUDED, AND THE REASON IT WAS EXCLUDED WAS WRONG.
    #
    # The previous note here said:
    #
    #     Thai is deliberately absent. Thai vowel signs and tone marks stack above
    #     and below the consonant; measured against real Noto Sans Thai, the whole
    #     table needs 13 rows even at 8 px, and only 7 px fits the 12 px row pitch
    #     the native lists use. At 7 px Thai is not legible ...
    #
    # RE-MEASURED against the same font, by rasterising every cluster and
    # measuring its ink bounding box:
    #
    #     size  baseline | rows needed  clusters clipped  widest  rows lost
    #        8         8 |           11                0       10          0
    #        8         9 |           11                0       10          0
    #        9         8 |           11                0       11          0
    #        9         9 |           11                0       11          0
    #       10      8,9 |           13                2       12          1
    #       11      8,9 |           13                2       12          1
    #       12      8,9 |           14               16       13          2
    #
    # So Thai needs ELEVEN rows at 8 px, not thirteen, and it fits the 12-row cell
    # with NOTHING clipped at 8 or 9 px. Clipping is the honest metric because
    # render() does not drop out-of-cell ink -- it silently discards it -- so a
    # size that needs 13 rows loses mark pixels rather than reporting a failure.
    #
    # THE CLUSTER COUNT IS 151, MEASURED WITH THE CORRECTED RULES, and that
    # correction also explains why the old note's "13 rows even at 8 px" could
    # look plausible for so long. The old walker split nine of the marks this
    # game uses into clusters of their own -- see is_thai_combining_mark() -- so
    # it was measuring a set of glyphs the runtime never asks for. Both the
    # measurement and the exclusion it justified were taken over the wrong
    # shapes.
    #
    # 9 px with the same advance the Chinese sets use is chosen: it is the
    # largest size that still fits whole, and sharing advance 11 with the CJK
    # sets means the existing line layout, wrapping and 12 px row pitch all carry
    # over unchanged and stay verified.
    #
    # THE GLYPH INPUT IS locale.c, AND THE REASON IS A BUG THIS FOUND.
    #
    # Taking the characters from test_translations.py covered the fourteen
    # shipped ids and the Instructions text, and missed the sixty-nine NATIVE
    # EXTENSION strings that locale.c also carries in Chinese and Thai -- the
    # Settings labels, the eleven theme names, the key names, "RESOURCE ONLY".
    # Those are drawn on the Settings, About and Display pages in every language,
    # so sixty-nine strings per language had no glyph and fell through to the
    # unknown-glyph box. It went unnoticed because the two languages were
    # documented as "text correct, glyphs absent", and shipping the Thai set
    # turned a known limitation into a visible one.
    #
    # self_check() runs first and fails the generator if locale.c and the shipped
    # resources ever disagree, so the parse cannot quietly become the authority
    # on the fourteen ids it is only borrowing.
    self_check()
    from_locale = locale_strings()
    langs = [
        build("ZH_CN", [t for _, t in from_locale["ZH_CN"]],
              CJK_TTC, FACE_SC, 10, 11, 10),
        build("ZH_TW", [t for _, t in from_locale["ZH_TW"]],
              CJK_TTC, FACE_TC, 10, 11, 10),
        build("TH_TH", [t for _, t in from_locale["TH_TH"]],
              THAI_TTF, 0, 9, 11, 9),
    ]
    here = os.path.dirname(os.path.abspath(__file__))
    out = os.path.join(os.path.dirname(here), "script_font.c")
    total = emit(langs, out)
    for l in langs:
        print(f"  {l['name']}: {len(l['glyphs'])} clusters, advance {l['advance']}, "
              f"baseline {l['baseline']}, dropped {l['dropped']}")
    print(f"  total {total} glyphs -> {out}")
    print(f"  file size {os.path.getsize(out)} bytes")


if __name__ == "__main__":
    main()
