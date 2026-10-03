#ifndef BOUNCE_NATIVE_LOCALE_H
#define BOUNCE_NATIVE_LOCALE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Native UI language set.
 *
 * ORIGINAL SOURCE-BACKED: the four recovered translation resources
 * src/main/resources/lang.xx, lang.zh-CN, lang.zh-TW and lang.th-TH.
 *
 *   BOUNCE_LANGUAGE_EN is /lang.xx. The recovered source never names a
 *   language "EN" or "xx" in any string: "xx" is only a filename token. What
 *   IS established is that lang.xx carries the plain English text (extract_locales.py
 *   names it TRANSLATION_FILE_PATH and it decodes to the English sentences), so
 *   it is the default language. The screen therefore shows "EN" for it and never
 *   shows "XX". This is a presentation mapping only; the resource file is
 *   untouched.
 *
 * NATIVE EXTENSION: BOUNCE_LANGUAGE_ID is Indonesian. No Indonesian resource
 * ships with the recovered Nokia Bounce tree and no source line refers to it.
 * Its strings are a native translation of the established English/default text.
 */
typedef enum BounceLanguage {
    BOUNCE_LANGUAGE_EN = 0,
    BOUNCE_LANGUAGE_ZH_CN = 1,
    BOUNCE_LANGUAGE_ZH_TW = 2,
    BOUNCE_LANGUAGE_TH_TH = 3,
    /* NATIVE EXTENSION: Indonesian. Not an original Nokia Bounce language. */
    BOUNCE_LANGUAGE_ID = 4,
    BOUNCE_LANGUAGE_COUNT = 5
} BounceLanguage;

/*
 * Semantic keys for every user-facing string the native UI draws. The UI asks
 * for a key; the active language chooses the text. English is the default and
 * the source of every other translation, so there is exactly one string per
 * key per language and no duplicated UI logic.
 */
typedef enum BounceLocaleKey {
    /* Title and the four source-verified original menu entries. */
    BOUNCE_LOCALE_TITLE = 0,
    BOUNCE_LOCALE_CONTINUE,
    BOUNCE_LOCALE_NEW_GAME,
    BOUNCE_LOCALE_HIGH_SCORE,
    BOUNCE_LOCALE_INSTRUCTIONS,
    BOUNCE_LOCALE_EXIT,
    /* NATIVE EXTENSION menu rows. */
    BOUNCE_LOCALE_SETTINGS,
    BOUNCE_LOCALE_ABOUT,
    /* Explicit action-boundary labels. */
    BOUNCE_LOCALE_START_LEVEL,
    BOUNCE_LOCALE_CONTINUE_UNAVAILABLE,
    BOUNCE_LOCALE_NO_ACTION,
    /* Settings destination and its pages (all NATIVE EXTENSION). */
    BOUNCE_LOCALE_SETTINGS_DISPLAY,
    BOUNCE_LOCALE_SETTINGS_INPUT,
    BOUNCE_LOCALE_SETTINGS_AUDIO,
    BOUNCE_LOCALE_SETTINGS_LANGUAGE,
    BOUNCE_LOCALE_DISPLAY,
    BOUNCE_LOCALE_INPUT,
    BOUNCE_LOCALE_AUDIO,
    BOUNCE_LOCALE_LANGUAGE,
    /* Settings > Theme: the temporary runtime UI color profiles. */
    BOUNCE_LOCALE_THEME,
    BOUNCE_LOCALE_THEME_DEFAULT,
    BOUNCE_LOCALE_THEME_GREEN_LCD,
    BOUNCE_LOCALE_THEME_SEPIA_TONE,
    /*
     * RETIRED, NEVER RENDERED. The intermediate "Green Light" label from the
     * previous milestone no longer exists as a user-facing name: profile 1 is
     * drawn with BOUNCE_LOCALE_THEME_GREEN_LCD, which is the pre-existing key
     * whose text was already exactly "GREEN LCD". This key is kept rather than
     * deleted, consistent with this file's no-deletion policy, and its five
     * strings now carry the Green LCD label so the retired wording survives
     * nowhere in the tree. No code path draws it.
     *
     * The key identifiers below are the canonical internal profile identity and
     * are deliberately NOT renamed: only the display strings are user-facing.
     */
    BOUNCE_LOCALE_THEME_GREEN_LIGHT,
    BOUNCE_LOCALE_THEME_INVERTED_GREEN_LCD,
    BOUNCE_LOCALE_THEME_AMBER_ORANGE,
    BOUNCE_LOCALE_THEME_DEEP_NAVY_CREAM,
    BOUNCE_LOCALE_THEME_FOREST_GREEN_PALE_MINT,
    BOUNCE_LOCALE_THEME_BURGUNDY_SOFT_BLUSH,
    BOUNCE_LOCALE_THEME_CHARCOAL_ELECTRIC_BLUE,
    BOUNCE_LOCALE_THEME_CLASSIC_NOKIA_BLUE,
    BOUNCE_LOCALE_THEME_PURPLE_HAZE,
    BOUNCE_LOCALE_THEME_ARCTIC_WHITE,
    BOUNCE_LOCALE_THEME_DARK_TEAL_IVORY,
    /* Settings > Display facts. */
    BOUNCE_LOCALE_LOGICAL,
    BOUNCE_LOCALE_LOGICAL_RESOLUTION,
    BOUNCE_LOCALE_GAMEPLAY_VIEWPORT,
    BOUNCE_LOCALE_UI_HUD_REGION,
    BOUNCE_LOCALE_READ_ONLY_NATIVE,
    /* Settings > Input fact page. */
    BOUNCE_LOCALE_LEFT,
    BOUNCE_LOCALE_RIGHT,
    BOUNCE_LOCALE_UP,
    BOUNCE_LOCALE_DOWN,
    BOUNCE_LOCALE_ARROW_A,
    BOUNCE_LOCALE_ARROW_D,
    BOUNCE_LOCALE_ARROW_W,
    BOUNCE_LOCALE_ARROW_S,
    BOUNCE_LOCALE_ESCAPE,
    BOUNCE_LOCALE_NO_REMAPPING_YET,
    /* Settings > Audio. */
    BOUNCE_LOCALE_STATUS,
    /*
     * STEP 34-F-K. The two values of the Audio ON/OFF setting. Row 0 is ON,
     * row 1 is OFF. Added after STATUS so the group stays contiguous; the two
     * placeholder keys below are retained (see locale.c) but are no longer
     * rendered by the Audio page.
     */
    BOUNCE_LOCALE_ON,
    BOUNCE_LOCALE_OFF,
    BOUNCE_LOCALE_NOT_YET_IMPLEMENTED,
    BOUNCE_LOCALE_NO_AUDIO_PATH_YET,
    /* Settings > Language. */
    BOUNCE_LOCALE_SELECT_TO_SWITCH,
    BOUNCE_LOCALE_RESOURCE_ONLY,
    BOUNCE_LOCALE_NATIVE_STRINGS,
    BOUNCE_LOCALE_DEFAULT_LANGUAGE,
    /* Main-menu status lines. */
    BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA,
    BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE,
    /* About destination, all NATIVE EXTENSION. */
    BOUNCE_LOCALE_NOKIA_BOUNCE,
    BOUNCE_LOCALE_NATIVE_LINUX_BUILD,
    BOUNCE_LOCALE_ORIGINAL_INSPIRED,
    BOUNCE_LOCALE_BEHAVIOR_BASED,
    BOUNCE_LOCALE_PLATFORM_LINUX,
    BOUNCE_LOCALE_NATIVE_SCREEN,
    /* Level selection. */
    BOUNCE_LOCALE_LEVEL_SELECT,
    BOUNCE_LOCALE_LEVEL,
    BOUNCE_LOCALE_BACK,
    BOUNCE_LOCALE_ESC_BACK,
    /* High Score destination. */
    BOUNCE_LOCALE_NO_RMS_VALUE,
    /* Gameplay-entry boundary. */
    BOUNCE_LOCALE_GAMEPLAY,
    BOUNCE_LOCALE_DESTINATION,
    BOUNCE_LOCALE_BACK_TO_MENU,
    BOUNCE_LOCALE_SCORE,
    BOUNCE_LOCALE_LIVES,
    BOUNCE_LOCALE_COUNTDOWN,
    BOUNCE_LOCALE_NOT_STARTED,
    /* The long wrapped Instructions paragraph, from the recovered lang.xx. */
    BOUNCE_LOCALE_INSTRUCTIONS_BODY,
    /* The bounded excerpt of that paragraph the page can actually draw. */
    BOUNCE_LOCALE_INSTRUCTIONS_PREFIX,
    /*
     * STEP 12X-P4-C-B -- the Game End destination, from BounceGame.java.
     *
     *   BOUNCE_LOCALE_GAME_END_TITLE     BounceGame.java:181, the Form
     *                                   title, Translation.GAME_OVER (id 6)
     *   BOUNCE_LOCALE_GAME_END_CONGRATS  BounceGame.java:183, the body when
     *                                   won, Translation.CONGRATS (id 3)
     *   BOUNCE_LOCALE_GAME_END_OK        BounceGame.java:180, the label of
     *                                   the single Command.OK, id 13
     *
     * There is deliberately NO key for the score line and NO key for the
     * new-high-score line. Both are owned by the score system, which is not
     * yet reconciled; adding a key now would invite rendering a value this
     * build cannot honestly produce. See reconciliation 13.23.6.
     */
    BOUNCE_LOCALE_GAME_END_TITLE,
    BOUNCE_LOCALE_GAME_END_CONGRATS,
    BOUNCE_LOCALE_GAME_END_OK,
    /*
     * D-07 -- Translation.NEW_HIGH_SCORE, id 12.
     *
     * BounceGame.java:188-190 appends it to BOTH terminal forms, the loss form
     * at :186 and the win form at :183:
     *
     *     if (this.q) {
     *         this.form.append(Translation.sprintf_translated(
     *             Translation.NEW_HIGH_SCORE));
     *         this.form.append("\n\n");
     *     }
     *
     * The four values below are decoded from the shipped resources, not
     * authored: lang.xx "New high score!", lang.th-TH (which contains a U+200B
     * ZERO WIDTH SPACE inside the first word), lang.zh-CN and lang.zh-TW. The
     * Indonesian row is the NATIVE EXTENSION column and is authored from the
     * English in the same way every other Indonesian row in locale.c is; no
     * lang.id resource exists in src/main/resources.
     */
    BOUNCE_LOCALE_NEW_HIGH_SCORE,
    /*
     * STEP 13G-A -- the Level Complete body, translation id 10.
     *
     * Java builds this string in b.LoadLevelId at b.java:218:
     *
     *     levelIdFormat[0] = Integer.valueOf(this.level).toString();
     *     this.LevelCompletedString =
     *         Translation.sprintf_translated(Translation.LEVEL_COMPLETED, levelIdFormat);
     *
     * and Translation.java:73-77 replaces the single "%U" with that one argument. The
     * value substituted is this.level AS OF THE LOAD, and the completion block
     * increments the level afterwards at e.java:317, so the string always names the
     * level that was just COMPLETED.
     *
     * The four source-backed strings are transcribed verbatim from the shipped
     * resources (parsed by reconciliation 13.51.2):
     *
     *     lang.xx     Level %U completed!
     *     lang.th-TH  ผ่านระดับ %U!
     *     lang.zh-CN  级别%U\n已完成
     *     lang.zh-TW  等級%U\n已完成！
     *
     * The two Chinese strings carry an EMBEDDED NEWLINE, so the body is two lines in
     * those locales. That is preserved, not normalised away.
     *
     * Indonesian has NO original lang.id and is a NATIVE EXTENSION, in the same sense
     * as the other Indonesian rows.
     *
     * There is deliberately no substitution machinery in the locale layer: the
     * "%0U"/"%1U" placeholders in BOUNCE_LOCALE_INSTRUCTIONS_BODY are present and have
     * never been substituted either. The one "%U" is expanded at the draw site by
     * ui_compose_level_completed(), which is scoped to this single string.
     */
    BOUNCE_LOCALE_LEVEL_COMPLETED,
    /*
     * NATIVE ADDITION -- the Input page's numeric-keypad setting. Appended last
     * so no existing enumerator value moves; the table below is positional and
     * every row must gain the entry in the same place.
     *
     * NOT A RECOVERY. The recovered game has no Settings screen, no Input page
     * and no keypad, so there is no Translation id for this. Like every other
     * native-extension row in this table it is translated into all five
     * languages rather than left English, because a label that renders as an
     * unknown-glyph box in Chinese or Thai is worse than an English one.
     */
    BOUNCE_LOCALE_T9_INPUT,
    BOUNCE_LOCALE_KEY_COUNT
} BounceLocaleKey;

/* The default at every fresh launch. Never persisted; there is no RMS store. */
BounceLanguage bounce_locale_default_language(void);

/*
 * Resolve one key in one language. English is returned for any language whose
 * strings are not available, so a language without decoded text degrades to the
 * source-backed default instead of showing an empty screen. Never returns NULL
 * for a valid key.
 */
const char *bounce_locale_text(BounceLanguage language, BounceLocaleKey key);

/*
 * The wrapped Instructions paragraph. It is a separate accessor only because it
 * is drawn by the wrapping helper rather than as a single row; it resolves
 * through exactly the same table as every other key.
 */
const char *bounce_locale_instructions_text(BounceLanguage language);

/*
 * Short screen label for the language list. Deliberately a language code, not a
 * native name: the native UI font is a 5x7 A-Z placeholder set with no CJK or
 * Thai glyphs, so names such as the Chinese or Thai endonyms would render as
 * unknown-glyph boxes. "EN" is used for the default rather than "XX".
 */
const char *bounce_locale_language_label(BounceLanguage language);

/*
 * Recovered resource path for a language, or NULL when the language has no
 * resource file. Only the four ORIGINAL SOURCE-BACKED languages have one;
 * Indonesian is a NATIVE EXTENSION and deliberately has no file.
 */
const char *bounce_locale_resource_name(BounceLanguage language);

/* True for the four recovered resources, false for the native extension. */
bool bounce_locale_is_recovered_resource(BounceLanguage language);

/*
 * Runtime active language. This is the smallest possible state: a single value
 * mirroring BounceAppFlow's recorded selection so the label and render helpers,
 * which take no flow, can resolve strings. The flow remains the owner; the app
 * mirrors it once per frame.
 */
void bounce_locale_set_active_language(BounceLanguage language);
BounceLanguage bounce_locale_active_language(void);

#ifdef __cplusplus
}
#endif

#endif /* BOUNCE_NATIVE_LOCALE_H */
