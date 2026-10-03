# EXTERNAL AND BASE SOURCES REGISTRY

**Status:**
- Base repository: primary evidence with decompiler caveat.
- External references: secondary evidence only.
- Knowledge-only sources: recorded for context, never cited as evidence.

**Last updated:** 2026-10-03

## Purpose

This registry documents (a) the local base repository from which this
project was derived, (b) external sources used during
reverse-engineering and native porting, and (c) knowledge-only sources
recorded for context.

"External" means: not the original recovered Java source, not the
original recovered resources, and not the in-tree reverse documentation.

## Categories

| Code | Meaning |
|---|---|
| BASE | Base repository — the project is derived from this |
| REF  | Implementation reference — used to inform code, not to prove behavior |
| DOC  | Documentation |
| ASSET| Asset source |
| TOOL | Tool used |
| AUDIT| External audit or analysis |
| KNOW | Knowledge only — recorded for context, never cited as evidence |

## Evidence hierarchy

This hierarchy is authoritative and must be preserved:

```text
BASE-001
    ↓
Primary evidence

EXT-001
    ↓
Secondary / cross-source evidence

EXT-002
    ↓
Behavioral / visual reference

EXT-003
    ↓
Implementation reference only

EXT-004
    ↓
Knowledge only
```

External sources must NEVER silently become primary evidence. If an external
source conflicts with BASE-001, the conflict must be recorded as `CONFLICT`
and must NOT be resolved automatically.

## Registry

| ID | Name | Category | URL | Used for | Evidence class |
|---|---|---|---|---|---|
| BASE-001 | rndtrash/nokia-bounce-decomp | BASE | https://github.com/rndtrash/nokia-bounce-decomp | Base repository | Primary (with decompiler caveat) |
| EXT-001 | Bounce Zero (fan remake) | REF | https://github.com/amdray/bounce_zero | Secondary / cross-source evidence | Secondary / Cross-source evidence |
| EXT-002 | Bounce (2001) Remake by Mycron | REF / BEHAVIORAL / VISUAL | https://mycron.itch.io/bounce-remake | Behavioral and visual reference | Behavioral / Visual reference |
| EXT-003 | Bounce Nokia Game Clone (anish-g) | REF | https://github.com/anish-g/Bounce-Nokia-Game-Clone | Implementation reference only | Implementation reference only |
| EXT-004 | BouncEdit Documentation | DOC / REF / KNOW | https://chrismoyles.net/bounce/documentation.shtml | Contextual knowledge only | Secondary (knowledge only) |
| ASSET-001 | Noto Sans CJK SC / TC | ASSET | — (installed font, no URL recorded) | Glyph bitmaps in native/app/script_font.c | Asset source (licence declared by the font itself) |
| ASSET-002 | Noto Sans Thai | ASSET | — (installed font, no URL recorded) | Glyph bitmaps in native/app/script_font.c | Asset source (licence declared by the font itself) |

---

## Part A — Base Repository

### BASE-001 — rndtrash/nokia-bounce-decomp

- **Category:** BASE
- **URL:** https://github.com/rndtrash/nokia-bounce-decomp
- **Classification:** BASE / PRIMARY SOURCE
- **Role:** The base repository and primary source for recovered/decompiled
  Java code and original recovered resources.
- **Used for:** Base repository
- **Evidence class:** Primary (with decompiler caveat)
- **Important qualifier:** The repository is a decompilation, not original
  Nokia source. The decompiler caveat is preserved here and in Rule 7; no
  claim may present it as original Nokia source.
- **Recovery toolchain:** the recovered Java classes carry JD-Core 1.1.3
  footers and were compiled by Java compiler version 1 (45.3). Each footer
  names the originating artifact `nokiabounc_jdifc8jb.jar` and the class
  path within it, e.g.
  `nokiabounc_jdifc8jb.jar!\com\nokia\mid\appl\boun\f.class`. The artifact
  name and class path are the provenance record; the local filesystem path
  of the artifact on the machine where recovery happened was removed and is
  deliberately not recorded here.
- **Notes:** See also Rule 1, Rule 2, Rule 6, Rule 7 and Rule 8.

---

## Part B — External Reference Sources

### EXT-001 — Bounce Zero (fan remake)

- **Category:** REF
- **URL:** https://github.com/amdray/bounce_zero
- **Classification:** REF / SECONDARY / CROSS-SOURCE EVIDENCE
- **Role:** Secondary cross-source evidence. It may be used to cross-check
  observations against another implementation of Bounce using original game
  data.
- **Used for:** Secondary / cross-source evidence
- **Evidence class:** Secondary / Cross-source evidence
- **Notes:** It does NOT override BASE-001. A disagreement with BASE-001 is
  recorded as `CONFLICT` and is not resolved automatically.

### EXT-002 — Bounce (2001) Remake by Mycron

- **Category:** REF / BEHAVIORAL / VISUAL
- **URL:** https://mycron.itch.io/bounce-remake
- **Classification:** REF / BEHAVIORAL / VISUAL REFERENCE
- **Role:** Behavioral and visual reference. It may be used as a reference for
  observable gameplay/visual behavior.
- **Used for:** Behavioral and visual reference
- **Evidence class:** Behavioral / Visual reference
- **Notes:** It is NOT primary evidence for internal implementation details,
  and it does NOT override BASE-001. A disagreement with BASE-001 is
  recorded as `CONFLICT` and is not resolved automatically.

### EXT-003 — Bounce Nokia Game Clone (anish-g)

- **Category:** REF
- **URL:** https://github.com/anish-g/Bounce-Nokia-Game-Clone
- **Classification:** REF / IMPLEMENTATION REFERENCE ONLY
- **Role:** Implementation reference only. It may provide implementation ideas
  or comparative context.
- **Used for:** Implementation reference only
- **Evidence class:** Implementation reference only
- **Notes:** It must NOT be treated as proof of original Nokia Bounce
  behavior, and it does NOT override BASE-001. A disagreement with BASE-001 is
  recorded as `CONFLICT` and is not resolved automatically.

---

## Part C — Knowledge-Only Sources

### EXT-004 — BouncEdit Documentation (chrismoyles.net)

- **Category:** DOC / REF / KNOW
- **URL:** https://chrismoyles.net/bounce/documentation.shtml
- **Author / maintainer:** Chris Moyles
- **License:** Not stated in source
- **Accessed:** 2026-09-26
- **Version documented:** BouncEdit 0.1.31 (page explicitly states "older version")
- **Used for:** Contextual knowledge only. Not used to inform any implementation decision, not used to resolve any UNKNOWN, not used to change any native code.
- **Evidence class:** Secondary only. Third-party tool documentation. Never promoted to original evidence.
- **What was taken from it:**
  - Confirmation (independent, secondary) that:
    - Bounce ships with 11 levels
    - Tile 9 is the exit (four tiles in a square)
    - Tile 10 is the moving object (dynamic thorn) tile
    - Large balls cannot pass through small rings
    - Moving objects have a top-left origin, a direction, an extent, and a starting offset
    - Level design expects a "screen scroll" model (one full screen up or down at a time)
- **What was NOT taken from it:**
  - No technical specification: the page is a user guide for a third-party editor, not a format or behavior specification.
  - No level file format, no thorn record layout, no collision formula, no camera formula.
  - No static visual for tile 9 or tile 10.
  - No behavior claims that could override the recovered Java source.
- **Conflicts recorded:**
  - **"Rule of eight" vs observed level heights.** The documentation describes an eight-high design guideline. The shipped level binaries have heights 8, 22, 36, 29, 43, 36, 36, 36, 36, 35, 57. This is recorded only as context; the source Java and level binaries remain authoritative.
- **Notes:**
  - This is a knowledge-only source.
  - It is recorded so future readers know that this public documentation was checked and found insufficient for behavior claims.
  - It must never be used to resolve an UNKNOWN or justify an implementation decision.

---

## Part D — Asset Sources

These are the font sources for the generated glyph tables. They are recorded
here because `native/app/script_font.c` ships in the project tree, so the
registry would otherwise be incomplete for a file that is published. They did
not come from BASE-001; they were already installed on the machine where the
bitmaps were generated.

### ASSET-001 — Noto Sans CJK SC / TC

- **Category:** ASSET
- **Classification:** ASSET / GENERATOR INPUT
- **Source/version:** Noto Sans CJK 2.004, `NotoSansCJK-Regular.ttc`,
  faces 2 and 3. Face 2 is the SC family and face 3 is the TC family, as
  selected by the generator.
- **Used for:** Thresholded glyph bitmaps generated into
  `native/app/script_font.c`. These two faces supply the `ZH_CN` and `ZH_TW`
  glyph sets.
- **Licence/provenance:** The local source font files declare SIL Open Font
  License, Version 1.1. OS/2 `fsType` is 0.
- **Notes:** See the note below the two asset entries.

### ASSET-002 — Noto Sans Thai

- **Category:** ASSET
- **Classification:** ASSET / GENERATOR INPUT
- **Source/version:** Noto Sans Thai 2.000, `NotoSansThai-Regular.ttf`,
  face 0.
- **Used for:** Thresholded glyph bitmaps generated into
  `native/app/script_font.c`. This face supplies the `TH_TH` glyph set.
- **Licence/provenance:** The local source font file declares SIL Open Font
  License, Version 1.1. OS/2 `fsType` is 0.
- **Notes:** See the note below the two asset entries.

### Note on the generated glyph bitmaps

- `native/app/script_font.c` contains 1-bit rasterisations derived from the
  font outlines of ASSET-001 and ASSET-002. It is a generated file and it
  carries no hand-written glyph data.
- The rasterisation is performed by `native/app/tools/gen_script_font.py`
  using Pillow, which loads these fonts, draws each cluster, and thresholds
  the result to one bit per pixel.
- This project records the fonts' own licence declaration as provenance
  evidence. That declaration is a statement made by the font files
  themselves.
- This project does NOT independently assert a separate licence grant for the
  generated bitmap data.
- No legal conclusion is drawn, and none should be read into these entries,
  about whether the generated bitmap form is itself a "Font Software"
  derivative under the SIL Open Font License. Resolving that question is
  outside the scope of this registry.
- These entries record provenance only. They do not extend, restrict, or
  reinterpret Rules 1 to 9.

---

## Rules

1. The base repository is the source of the audited Java and resources.
   It is primary evidence for this project, with a decompiler caveat.
2. External sources never override the base repository.
3. When an external source conflicts with the base repository, the base
   wins and the conflict is recorded.
4. If a native implementation was informed by an external source, the
   in-tree code comment must say so.
5. This registry is append-only for existing entries.
6. Any external source claiming to reproduce original behavior must be
   verified against the base repository before being cited as evidence.
7. The base repository itself is a decompilation, not original Nokia
   source. No claim should present it as such.
8. Attribution to rndtrash and the base repository must appear in the
   project's README or a NOTICE file if the base license requires it.
9. A source classified as "knowledge only" must not be cited as
   evidence for any behavior, value, or implementation decision.
   It exists in this registry so that future readers know it was
   checked and found insufficient. If a knowledge-only source is
   ever needed to support a claim, that is a signal that a proper
   audit is required instead.
