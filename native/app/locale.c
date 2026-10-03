#include "locale.h"

#include <stddef.h>

/*
 * The complete user-facing string set of the native UI, in two languages.
 *
 * Column 0 is ENGLISH, which is the source-backed default: its menu entries come
 * from the recovered lang.xx strings, and its remaining rows are the native
 * strings this project already draws.
 *
 * Column 1 is INDONESIAN, a NATIVE EXTENSION translated from column 0 only. It
 * was not translated from the Chinese, Thai, or any other secondary text, and it
 * is not claimed to have existed in the recovered Nokia Bounce resources.
 *
 * There is deliberately no Chinese or Thai column. Those resources ship, but no
 * localization decoder exists, so adding a column of invented translations would
 * be a fabrication. bounce_locale_text() falls back to English for them and the
 * Language screen labels them "RESOURCE ONLY".
 */

/* The recovered English Instructions body, verbatim from lang.xx. */
#define BOUNCE_INSTRUCTIONS_EN \
    "Guide the ball past many obstacles to pass through all the hoops and " \
    "open the door to the next level. Use key %0U to move the ball left, key " \
    "%1U to move the ball right and key %2U to bounce the ball. Watch out for " \
    "spikes. Use deflators to shrink the ball and inflators to return to " \
    "normal size. Large balls float in water, small balls don't. Collect " \
    "crystals for extra points and to store your game position. Collect " \
    "crystal balls to give your ball an extra life. Jump and speed boosts " \
    "temporarily boost your powers while rubber floors give you extra bounce."

/* The bounded excerpt of the English body the page can draw. */
#define BOUNCE_INSTRUCTIONS_PREFIX_EN \
    "Guide the ball past many obstacles to pass through all the hoops and " \
    "open the door to the next level. Use key %0U to move the ball left, key " \
    "%1U to move the ball right and key %2U to bounce the ball. Watch out for " \
    "spikes."

/* Indonesian excerpt of the same semantic span, translated from the English. */
#define BOUNCE_INSTRUCTIONS_PREFIX_ID \
    "Antar bola melewati banyak rintangan untuk melewati semua lingkaran dan " \
    "membuka pintu ke level berikutnya. Gunakan tombol %0U untuk menggerakkan " \
    "bola ke kiri, tombol %1U untuk menggerakkan bola ke kanan, dan tombol " \
    "%2U untuk memantulkan bola. Waspadai duri."

/* Indonesian translation of the same English text, rendered by the same page. */
#define BOUNCE_INSTRUCTIONS_ID \
    "Antar bola melewati banyak rintangan untuk melewati semua lingkaran dan " \
    "membuka pintu ke level berikutnya. Gunakan tombol %0U untuk menggerakkan " \
    "bola ke kiri, tombol %1U untuk menggerakkan bola ke kanan, dan tombol " \
    "%2U untuk memantulkan bola. Waspadai duri. Gunakan deflator untuk " \
    "mengecilkan bola dan inflator untuk mengembalikan ukuran normal. Bola " \
    "besar mengambang di air, sedangkan bola kecil tidak. Kumpulkan kristal " \
    "untuk mendapat skor tambahan dan menyimpan posisi permainan. Kumpulkan " \
    "bola kristal untuk memberikan satu nyawa tambahan pada bolamu. Lompatan " \
    "dan peningkatan kecepatan memperkuat kemampuanmu untuk sementara, " \
    "sedangkan lantai karet memberi pantulan ekstra."

/*
 * D-24 -- Translation.MORE_INSTRUCTIONS, id 1, DECODED FROM THE SHIPPED
 * RESOURCE lang.zh-CN.
 *
 * These four bodies were previously TEMPORARY TEST STRINGS authored for the
 * runtime language-switching test, and locale.c said so. That is no longer true:
 * every string below is the recovered resource text, transcribed from
 * src/main/resources/lang.zh-CN, which is the same file the game itself loaded. The
 * %U / %1U / %2U placeholders are the source's own and are substituted at draw
 * time by ui_compose_instructions() -- see ui_shell.c for why that is a render
 * step and not a locale step.
 *
 * ONE CHARACTER DELIBERATELY DIFFERS FROM THE RESOURCE: the English body spells
 * "don't" with an ASCII apostrophe where lang.xx uses U+2019 RIGHT SINGLE
 * QUOTATION MARK. The 5x7 font has no glyph for U+2019 -- placeholder_text_index()
 * covers A-Z and space only -- so the verbatim character would render as the
 * unknown-glyph box, which is strictly worse than an ASCII apostrophe. The same
 * reason keeps the English rows upper case.
 */
#define BOUNCE_INSTRUCTIONS_ZH_CN \
"使球越过重重障碍以通过所有环圈并打开大门进入下一级别。用按键%0U将球向左移动，用按键%1U将球向右移动，用按键%2U可使球弹跳。小心尖状物。用放气筒会收缩球的体积，用充气筒会使球的大小恢复正常。大球可以在水上漂浮，而小球则不行。积攒水晶可增加分数并储存游戏级别。积攒水晶球可延长球的寿命。使用跳跃和速度推进器可为您增力片刻，而用橡皮地板将使球的弹力更大。"

/*
 * D-24 -- the bounded excerpt of the same shipped ZH_CN body that the
 * page can actually draw. See the note above for why an excerpt is
 * drawn at all and for how each language's excerpt is bounded.
 */
#define BOUNCE_INSTRUCTIONS_PREFIX_ZH_CN \
"使球越过重重障碍以通过所有环圈并打开大门进入下一级别。用按键%0U将球向左移动，用按键%1U将球向右移动，用按键%2U可使球弹跳。小心尖状物。用放气筒会收缩球的体积，用充气筒会使球的大小恢复正常。大球可以在水上漂浮，而小球则不行。积攒水晶可增加分数并储存游戏级别。积攒水晶球可延长球的寿命。使用跳跃和速度推进器可为您增力片刻，而用橡皮地板将使球的弹力更大。"

/*
 * D-24 -- Translation.MORE_INSTRUCTIONS, id 1, DECODED FROM THE SHIPPED
 * RESOURCE lang.zh-TW.
 *
 * These four bodies were previously TEMPORARY TEST STRINGS authored for the
 * runtime language-switching test, and locale.c said so. That is no longer true:
 * every string below is the recovered resource text, transcribed from
 * src/main/resources/lang.zh-TW, which is the same file the game itself loaded. The
 * %U / %1U / %2U placeholders are the source's own and are substituted at draw
 * time by ui_compose_instructions() -- see ui_shell.c for why that is a render
 * step and not a locale step.
 *
 * ONE CHARACTER DELIBERATELY DIFFERS FROM THE RESOURCE: the English body spells
 * "don't" with an ASCII apostrophe where lang.xx uses U+2019 RIGHT SINGLE
 * QUOTATION MARK. The 5x7 font has no glyph for U+2019 -- placeholder_text_index()
 * covers A-Z and space only -- so the verbatim character would render as the
 * unknown-glyph box, which is strictly worse than an ASCII apostrophe. The same
 * reason keeps the English rows upper case.
 */
#define BOUNCE_INSTRUCTIONS_ZH_TW \
"設法讓球通過許多障礙物，穿越所有圓箍，最終打開通往下一個關卡的門。按鍵%0U可將球往左移，按鍵%1U可將球往右移，按鍵%2U可將球往上彈。小心避開釘子。抽氣筒可以把球縮小，充氣筒可以把球恢復成原狀。大球可以浮在水面上，小球則不行。收集水晶來獲得加分，並儲存遊戲位置。收集水晶球可增加額外生命。跳躍和加速可以暫時提高你的力量，而橡膠地板則可以增加彈跳力。"

/*
 * D-24 -- the bounded excerpt of the same shipped ZH_TW body that the
 * page can actually draw. See the note above for why an excerpt is
 * drawn at all and for how each language's excerpt is bounded.
 */
#define BOUNCE_INSTRUCTIONS_PREFIX_ZH_TW \
"設法讓球通過許多障礙物，穿越所有圓箍，最終打開通往下一個關卡的門。按鍵%0U可將球往左移，按鍵%1U可將球往右移，按鍵%2U可將球往上彈。小心避開釘子。抽氣筒可以把球縮小，充氣筒可以把球恢復成原狀。大球可以浮在水面上，小球則不行。收集水晶來獲得加分，並儲存遊戲位置。收集水晶球可增加額外生命。跳躍和加速可以暫時提高你的力量，而橡膠地板則可以增加彈跳力。"

/*
 * D-24 -- Translation.MORE_INSTRUCTIONS, id 1, DECODED FROM THE SHIPPED
 * RESOURCE lang.th-TH.
 *
 * These four bodies were previously TEMPORARY TEST STRINGS authored for the
 * runtime language-switching test, and locale.c said so. That is no longer true:
 * every string below is the recovered resource text, transcribed from
 * src/main/resources/lang.th-TH, which is the same file the game itself loaded. The
 * %U / %1U / %2U placeholders are the source's own and are substituted at draw
 * time by ui_compose_instructions() -- see ui_shell.c for why that is a render
 * step and not a locale step.
 *
 * ONE CHARACTER DELIBERATELY DIFFERS FROM THE RESOURCE: the English body spells
 * "don't" with an ASCII apostrophe where lang.xx uses U+2019 RIGHT SINGLE
 * QUOTATION MARK. The 5x7 font has no glyph for U+2019 -- placeholder_text_index()
 * covers A-Z and space only -- so the verbatim character would render as the
 * unknown-glyph box, which is strictly worse than an ASCII apostrophe. The same
 * reason keeps the English rows upper case.
 */
#define BOUNCE_INSTRUCTIONS_TH_TH \
"เลื่อนลูกบอลผ่านสิ่งกีดขวาง\nให้ลอดห่วงทั้งหมด " \
    "และเปิดประตูไปยัง\nด่านถัดไปใช้ปุ่ม %0U " \
    "บังคับบอลไปทางซ้าย ปุ่ม %1U ไปทางขวา และปุ่ม " \
    "%2U เพื่อดีดลูกบอล ระวังเกล็ดน้ำแข็ง " \
    "ใช้ตัวถ่ายลมเพื่อ\nลดขนาดลูกบอล " \
    "และตัวสูบลมเพื่อเปลี่ยน\nเป็นขนาดปกติ " \
    "บอลลูกใหญ่จะลอยบนน้ำได้ ลูกเล็กลอยไม่ได้ " \
    "เก็บคริสตัลจะได้คะแนน\nพิเศษและบันทึกจุด\nที่เล่นอยู่ " \
    "เก็บลูกบอลคริสตัลจะได้\nพลังชีวิตพิเศษ " \
    "การกระโดดและเร่งความเร็ว\nจะเพิ่มพลังชั่วคราว " \
    "ส่วนพื้นยางทำให้ดีด\nได้สูงขึ้นเป็นพิเศษ"

/*
 * D-24 -- the bounded excerpt of the same shipped TH_TH body that the
 * page can actually draw. See the note above for why an excerpt is
 * drawn at all and for how each language's excerpt is bounded.
 */
#define BOUNCE_INSTRUCTIONS_PREFIX_TH_TH \
"เลื่อนลูกบอลผ่านสิ่งกีดขวาง\nให้ลอดห่วงทั้งหมด " \
    "และเปิดประตูไปยัง\nด่านถัดไปใช้ปุ่ม %0U " \
    "บังคับบอลไปทางซ้าย ปุ่ม %1U ไปทางขวา และปุ่ม " \
    "%2U เพื่อดีดลูกบอล ระวังเกล็ดน้ำแข็ง " \
    "ใช้ตัวถ่ายลมเพื่อ"

static const char *const locale_text[BOUNCE_LANGUAGE_COUNT][BOUNCE_LOCALE_KEY_COUNT]
    = {
    /* ---------------- ENGLISH: source-backed default ---------------- */
    [BOUNCE_LANGUAGE_EN] = {
        /* BOUNCE_LOCALE_TITLE */ "BOUNCE",
        /* BOUNCE_LOCALE_CONTINUE */ "CONTINUE",
        /* BOUNCE_LOCALE_NEW_GAME */ "NEW GAME",
        /* BOUNCE_LOCALE_HIGH_SCORE */ "HIGH SCORE",
        /* BOUNCE_LOCALE_INSTRUCTIONS */ "INSTRUCTIONS",
        /* BOUNCE_LOCALE_EXIT */ "EXIT",
        /* BOUNCE_LOCALE_SETTINGS */ "SETTINGS",
        /* BOUNCE_LOCALE_ABOUT */ "ABOUT",
        /* BOUNCE_LOCALE_START_LEVEL */ "START LEVEL",
        /* BOUNCE_LOCALE_CONTINUE_UNAVAILABLE */ "CONTINUE UNAVAILABLE",
        /* BOUNCE_LOCALE_NO_ACTION */ "NO ACTION",
        /* BOUNCE_LOCALE_SETTINGS_DISPLAY */ "SETTINGS DISPLAY",
        /* BOUNCE_LOCALE_SETTINGS_INPUT */ "SETTINGS INPUT",
        /* BOUNCE_LOCALE_SETTINGS_AUDIO */ "SETTINGS AUDIO",
        /* BOUNCE_LOCALE_SETTINGS_LANGUAGE */ "SETTINGS LANGUAGE",
        /* BOUNCE_LOCALE_DISPLAY */ "DISPLAY",
        /* BOUNCE_LOCALE_INPUT */ "INPUT",
        /* BOUNCE_LOCALE_AUDIO */ "AUDIO",
        /* BOUNCE_LOCALE_LANGUAGE */ "LANGUAGE",
        /* BOUNCE_LOCALE_THEME */ "THEME",
        /* BOUNCE_LOCALE_THEME_DEFAULT */ "DEFAULT",
        /* BOUNCE_LOCALE_THEME_GREEN_LCD */ "GREEN LCD",
        /* BOUNCE_LOCALE_THEME_SEPIA_TONE */ "SEPIA",
        /* BOUNCE_LOCALE_THEME_GREEN_LIGHT */ "GREEN LCD",
        /* BOUNCE_LOCALE_THEME_INVERTED_GREEN_LCD */ "GREEN INVERTED",
        /* BOUNCE_LOCALE_THEME_AMBER_ORANGE */ "AMBER",
        /* BOUNCE_LOCALE_THEME_DEEP_NAVY_CREAM */ "NAVY CREAM",
        /* BOUNCE_LOCALE_THEME_FOREST_GREEN_PALE_MINT */ "FOREST MINT",
        /* BOUNCE_LOCALE_THEME_BURGUNDY_SOFT_BLUSH */ "BURGUNDY",
        /* BOUNCE_LOCALE_THEME_CHARCOAL_ELECTRIC_BLUE */ "CHARCOAL BLUE",
        /* BOUNCE_LOCALE_THEME_CLASSIC_NOKIA_BLUE */ "NOKIA BLUE",
        /* BOUNCE_LOCALE_THEME_PURPLE_HAZE */ "PURPLE",
        /* BOUNCE_LOCALE_THEME_ARCTIC_WHITE */ "ARCTIC",
        /* BOUNCE_LOCALE_THEME_DARK_TEAL_IVORY */ "TEAL IVORY",
        /* BOUNCE_LOCALE_LOGICAL */ "LOGICAL",
        /* BOUNCE_LOCALE_LOGICAL_RESOLUTION */ "LOGICAL RESOLUTION",
        /* BOUNCE_LOCALE_GAMEPLAY_VIEWPORT */ "GAMEPLAY VIEWPORT",
        /* BOUNCE_LOCALE_UI_HUD_REGION */ "UI HUD REGION",
        /* BOUNCE_LOCALE_READ_ONLY_NATIVE */ "READ ONLY NATIVE",
        /* BOUNCE_LOCALE_LEFT */ "LEFT",
        /* BOUNCE_LOCALE_RIGHT */ "RIGHT",
        /* BOUNCE_LOCALE_UP */ "UP",
        /* BOUNCE_LOCALE_DOWN */ "DOWN",
        /* BOUNCE_LOCALE_ARROW_A */ "ARROW A",
        /* BOUNCE_LOCALE_ARROW_D */ "ARROW D",
        /* BOUNCE_LOCALE_ARROW_W */ "ARROW W",
        /* BOUNCE_LOCALE_ARROW_S */ "ARROW S",
        /* BOUNCE_LOCALE_ESCAPE */ "ESCAPE",
        /* BOUNCE_LOCALE_NO_REMAPPING_YET */ "NO REMAPPING YET",
        /* BOUNCE_LOCALE_STATUS */ "STATUS",
        /* BOUNCE_LOCALE_ON */ "ON",
        /* BOUNCE_LOCALE_OFF */ "OFF",
        /* BOUNCE_LOCALE_NOT_YET_IMPLEMENTED */ "NOT YET IMPLEMENTED",
        /* BOUNCE_LOCALE_NO_AUDIO_PATH_YET */ "NO AUDIO PATH YET",
        /* BOUNCE_LOCALE_SELECT_TO_SWITCH */ "SELECT TO SWITCH",
        /* BOUNCE_LOCALE_RESOURCE_ONLY */ "RESOURCE ONLY",
        /* BOUNCE_LOCALE_NATIVE_STRINGS */ "NATIVE STRINGS",
        /* BOUNCE_LOCALE_DEFAULT_LANGUAGE */ "DEFAULT",
        /* BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA */ "CONTINUE NO SAVE DATA",
        /* BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE */ "SETTINGS ABOUT NATIVE",
        /* BOUNCE_LOCALE_NOKIA_BOUNCE */ "NOKIA BOUNCE",
        /* BOUNCE_LOCALE_NATIVE_LINUX_BUILD */ "NATIVE LINUX BUILD",
        /* BOUNCE_LOCALE_ORIGINAL_INSPIRED */ "ORIGINAL INSPIRED",
        /* BOUNCE_LOCALE_BEHAVIOR_BASED */ "BEHAVIOR BASED",
        /* BOUNCE_LOCALE_PLATFORM_LINUX */ "PLATFORM LINUX",
        /* BOUNCE_LOCALE_NATIVE_SCREEN */ "NATIVE SCREEN",
        /* BOUNCE_LOCALE_LEVEL_SELECT */ "LEVEL SELECT",
        /* BOUNCE_LOCALE_LEVEL */ "LEVEL",
        /* BOUNCE_LOCALE_BACK */ "BACK",
        /* BOUNCE_LOCALE_ESC_BACK */ "ESC BACK",
        /* BOUNCE_LOCALE_NO_RMS_VALUE */ "NO RMS VALUE",
        /* BOUNCE_LOCALE_GAMEPLAY */ "GAMEPLAY",
        /* BOUNCE_LOCALE_DESTINATION */ "DESTINATION",
        /* BOUNCE_LOCALE_BACK_TO_MENU */ "BACK TO MENU",
        /* BOUNCE_LOCALE_SCORE */ "SCORE",
        /* BOUNCE_LOCALE_LIVES */ "LIVES",
        /* BOUNCE_LOCALE_COUNTDOWN */ "COUNTDOWN",
        /* BOUNCE_LOCALE_NOT_STARTED */ "NOT STARTED",
        /* BOUNCE_LOCALE_INSTRUCTIONS_BODY */ BOUNCE_INSTRUCTIONS_EN,
        /* BOUNCE_LOCALE_INSTRUCTIONS_PREFIX */ BOUNCE_INSTRUCTIONS_PREFIX_EN,
        /* STEP 12X-P4-C-B Game End. lang.xx id 6 decodes to "Game over"; the
         * 5x7 placeholder font folds lower-case to upper-case, so the stored
         * text is upper-case to match every other EN row in this column. */
        /* BOUNCE_LOCALE_GAME_END_TITLE */ "GAME OVER",
        /* BOUNCE_LOCALE_GAME_END_CONGRATS */ "CONGRATS",
        /* BOUNCE_LOCALE_GAME_END_OK */ "OK",
        /* D-07, Translation id 12, lang.xx verbatim. */
        /* BOUNCE_LOCALE_NEW_HIGH_SCORE */ "New high score!",
        /* STEP 13G-A, Translation id 10, lang.xx verbatim. */
        /* BOUNCE_LOCALE_LEVEL_COMPLETED */ "Level %U completed!",
        /*
         * NATIVE ADDITION -- the Input page's numeric-keypad setting. The 5x7
         * font has no digits or symbols, so the label names the KEYPAD rather
         * than printing the T9 arrangement: "NUMPAD" is letters only and
         * renders on every row of the table with the same pixels the rest of
         * this page already uses.
         */
        /* BOUNCE_LOCALE_T9_INPUT */ "NUMPAD"
    },
    /* ------- INDONESIAN: NATIVE EXTENSION, from ENGLISH ------- */
    [BOUNCE_LANGUAGE_ID] = {
        /* BOUNCE_LOCALE_TITLE */ "BOUNCE",
        /* BOUNCE_LOCALE_CONTINUE */ "LANJUTKAN",
        /* BOUNCE_LOCALE_NEW_GAME */ "GAME BARU",
        /* BOUNCE_LOCALE_HIGH_SCORE */ "SKOR TERTINGGI",
        /* BOUNCE_LOCALE_INSTRUCTIONS */ "PETUNJUK",
        /* BOUNCE_LOCALE_EXIT */ "KELUAR",
        /* BOUNCE_LOCALE_SETTINGS */ "PENGATURAN",
        /* BOUNCE_LOCALE_ABOUT */ "TENTANG",
        /* BOUNCE_LOCALE_START_LEVEL */ "MULAI LEVEL",
        /* BOUNCE_LOCALE_CONTINUE_UNAVAILABLE */ "LANJUTKAN TIDAK ADA",
        /* BOUNCE_LOCALE_NO_ACTION */ "TIDAK ADA AKSI",
        /* BOUNCE_LOCALE_SETTINGS_DISPLAY */ "PENGATURAN TAMPILAN",
        /* BOUNCE_LOCALE_SETTINGS_INPUT */ "PENGATURAN INPUT",
        /* BOUNCE_LOCALE_SETTINGS_AUDIO */ "PENGATURAN AUDIO",
        /* BOUNCE_LOCALE_SETTINGS_LANGUAGE */ "PENGATURAN BAHASA",
        /* BOUNCE_LOCALE_DISPLAY */ "TAMPILAN",
        /* BOUNCE_LOCALE_INPUT */ "INPUT",
        /* BOUNCE_LOCALE_AUDIO */ "AUDIO",
        /* BOUNCE_LOCALE_LANGUAGE */ "BAHASA",
        /* BOUNCE_LOCALE_THEME */ "TEMA",
        /* BOUNCE_LOCALE_THEME_DEFAULT */ "BAWAAN",
        /* BOUNCE_LOCALE_THEME_GREEN_LCD */ "LCD HIJAU",
        /* BOUNCE_LOCALE_THEME_SEPIA_TONE */ "SEPIA",
        /* BOUNCE_LOCALE_THEME_GREEN_LIGHT */ "GREEN LCD",
        /* BOUNCE_LOCALE_THEME_INVERTED_GREEN_LCD */ "HIJAU TERBALIK",
        /* BOUNCE_LOCALE_THEME_AMBER_ORANGE */ "JERUH AMBER",
        /* BOUNCE_LOCALE_THEME_DEEP_NAVY_CREAM */ "BIRU KREM",
        /* BOUNCE_LOCALE_THEME_FOREST_GREEN_PALE_MINT */ "MINT HUTAN",
        /* BOUNCE_LOCALE_THEME_BURGUNDY_SOFT_BLUSH */ "BURGUNDY",
        /* BOUNCE_LOCALE_THEME_CHARCOAL_ELECTRIC_BLUE */ "BIRU ARANG",
        /* BOUNCE_LOCALE_THEME_CLASSIC_NOKIA_BLUE */ "BIRU NOKIA",
        /* BOUNCE_LOCALE_THEME_PURPLE_HAZE */ "UNGU",
        /* BOUNCE_LOCALE_THEME_ARCTIC_WHITE */ "ARKTIK",
        /* BOUNCE_LOCALE_THEME_DARK_TEAL_IVORY */ "TEAL GIGI",
        /* BOUNCE_LOCALE_LOGICAL */ "LOGIS",
        /* BOUNCE_LOCALE_LOGICAL_RESOLUTION */ "RESOLUSI LOGIS",
        /* BOUNCE_LOCALE_GAMEPLAY_VIEWPORT */ "LAYAR PERMAINAN",
        /* BOUNCE_LOCALE_UI_HUD_REGION */ "WILAYAH HUD ANTARMUKA",
        /* BOUNCE_LOCALE_READ_ONLY_NATIVE */ "HANYA BACA ASLI",
        /* BOUNCE_LOCALE_LEFT */ "KIRI",
        /* BOUNCE_LOCALE_RIGHT */ "KANAN",
        /* BOUNCE_LOCALE_UP */ "ATAS",
        /* BOUNCE_LOCALE_DOWN */ "BAWAH",
        /* BOUNCE_LOCALE_ARROW_A */ "PANAH A",
        /* BOUNCE_LOCALE_ARROW_D */ "PANAH D",
        /* BOUNCE_LOCALE_ARROW_W */ "PANAH W",
        /* BOUNCE_LOCALE_ARROW_S */ "PANAH S",
        /* BOUNCE_LOCALE_ESCAPE */ "ESCAPE",
        /* BOUNCE_LOCALE_NO_REMAPPING_YET */ "BELUM ADA PETA ULANG",
        /* BOUNCE_LOCALE_STATUS */ "STATUS",
        /* BOUNCE_LOCALE_ON */ "AKTIF",
        /* BOUNCE_LOCALE_OFF */ "NONAKTIF",
        /* BOUNCE_LOCALE_NOT_YET_IMPLEMENTED */ "BELUM DIIMPLEMENTASIKAN",
        /* BOUNCE_LOCALE_NO_AUDIO_PATH_YET */ "BELUM ADA JALUR AUDIO",
        /* BOUNCE_LOCALE_SELECT_TO_SWITCH */ "PILIH UNTUK GANTI",
        /* BOUNCE_LOCALE_RESOURCE_ONLY */ "HANYA SUMBER",
        /* BOUNCE_LOCALE_NATIVE_STRINGS */ "STRING ASLI",
        /* BOUNCE_LOCALE_DEFAULT_LANGUAGE */ "BAWAAN",
        /* BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA */ "TIDAK ADA SIMPANAN",
        /* BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE */ "PENGATURAN ASLI",
        /* BOUNCE_LOCALE_NOKIA_BOUNCE */ "NOKIA BOUNCE",
        /* BOUNCE_LOCALE_NATIVE_LINUX_BUILD */ "BANGUNAN ASLI LINUX",
        /* BOUNCE_LOCALE_ORIGINAL_INSPIRED */ "TERINSPIRASI ASLI",
        /* BOUNCE_LOCALE_BEHAVIOR_BASED */ "BERBASIS PERILAKU",
        /* BOUNCE_LOCALE_PLATFORM_LINUX */ "PLATFORM LINUX",
        /* BOUNCE_LOCALE_NATIVE_SCREEN */ "LAYAR ASLI",
        /* BOUNCE_LOCALE_LEVEL_SELECT */ "PILIH LEVEL",
        /* BOUNCE_LOCALE_LEVEL */ "LEVEL",
        /* BOUNCE_LOCALE_BACK */ "KEMBALI",
        /* BOUNCE_LOCALE_ESC_BACK */ "ESC KEMBALI",
        /* BOUNCE_LOCALE_NO_RMS_VALUE */ "TIDAK ADA NILAI RMS",
        /* BOUNCE_LOCALE_GAMEPLAY */ "PERMAINAN",
        /* BOUNCE_LOCALE_DESTINATION */ "TUJUAN",
        /* BOUNCE_LOCALE_BACK_TO_MENU */ "KEMBALI KE MENU",
        /* BOUNCE_LOCALE_SCORE */ "SKOR",
        /* BOUNCE_LOCALE_LIVES */ "NYAWA",
        /* BOUNCE_LOCALE_COUNTDOWN */ "HITUNG MUNDUR",
        /* BOUNCE_LOCALE_NOT_STARTED */ "BELUM DIMULAI",
        /* BOUNCE_LOCALE_INSTRUCTIONS_BODY */ BOUNCE_INSTRUCTIONS_ID,
        /* BOUNCE_LOCALE_INSTRUCTIONS_PREFIX */ BOUNCE_INSTRUCTIONS_PREFIX_ID,
        /*
         * STEP 13G-A. NATIVE EXTENSION: there is no lang.id, so this is not a
         * Java-original string.
         *
         * The three Game End keys are present here ONLY because the table is
         * POSITIONAL and BOUNCE_LOCALE_KEY_COUNT now reaches them. They were
         * absent before Step 13G-A and resolved to the English fallback through
         * bounce_locale_text(). The Indonesian values are marked as such rather
         * than claimed to be recovered: the game-over wording is Indonesian
         * ("Permainan Selesai" / "Selamat!") and the OK label is the same
         * "OK" the source uses.
         */
        /* BOUNCE_LOCALE_GAME_END_TITLE */ "PERMAINAN SELESAI",
        /* BOUNCE_LOCALE_GAME_END_CONGRATS */ "SELAMAT!",
        /* BOUNCE_LOCALE_GAME_END_OK */ "OK",
        /* D-07, Translation id 12. NATIVE EXTENSION: authored
         * from the English column, like every other Indonesian
         * row here. No lang.id resource exists. */
        /* BOUNCE_LOCALE_NEW_HIGH_SCORE */ "REKOR BARU!",
        /* BOUNCE_LOCALE_LEVEL_COMPLETED */ "Level %U selesai!",
        /* BOUNCE_LOCALE_T9_INPUT */ "NUMPAD"
    },
    /* ------- ZH_CN: TEMPORARY TEST TABLE, NOT A RECOVERED RESOURCE ------- */
    [BOUNCE_LANGUAGE_ZH_CN] = {
        /* BOUNCE_LOCALE_TITLE */ "BOUNCE",
        /* BOUNCE_LOCALE_CONTINUE */ "继续",
        /* BOUNCE_LOCALE_NEW_GAME */ "新游戏",
        /* BOUNCE_LOCALE_HIGH_SCORE */ "最高分",
        /* BOUNCE_LOCALE_INSTRUCTIONS */ "游戏规则说明",
        /* BOUNCE_LOCALE_EXIT */ "退出",
        /* BOUNCE_LOCALE_SETTINGS */ "设置",
        /* BOUNCE_LOCALE_ABOUT */ "关于",
        /* BOUNCE_LOCALE_START_LEVEL */ "开始关卡",
        /* BOUNCE_LOCALE_CONTINUE_UNAVAILABLE */ "继续不可用",
        /* BOUNCE_LOCALE_NO_ACTION */ "无操作",
        /* BOUNCE_LOCALE_SETTINGS_DISPLAY */ "显示设置",
        /* BOUNCE_LOCALE_SETTINGS_INPUT */ "按键设置",
        /* BOUNCE_LOCALE_SETTINGS_AUDIO */ "声音设置",
        /* BOUNCE_LOCALE_SETTINGS_LANGUAGE */ "语言设置",
        /* BOUNCE_LOCALE_DISPLAY */ "显示",
        /* BOUNCE_LOCALE_INPUT */ "按键",
        /* BOUNCE_LOCALE_AUDIO */ "声音",
        /* BOUNCE_LOCALE_LANGUAGE */ "语言",
        /* BOUNCE_LOCALE_THEME */ "主题",
        /* BOUNCE_LOCALE_THEME_DEFAULT */ "默认",
        /* BOUNCE_LOCALE_THEME_GREEN_LCD */ "绿色液晶",
        /* BOUNCE_LOCALE_THEME_SEPIA_TONE */ "古棕色调",
        /* BOUNCE_LOCALE_THEME_GREEN_LIGHT */ "GREEN LCD",
        /* BOUNCE_LOCALE_THEME_INVERTED_GREEN_LCD */ "反色绿液晶",
        /* BOUNCE_LOCALE_THEME_AMBER_ORANGE */ "琥珀单色",
        /* BOUNCE_LOCALE_THEME_DEEP_NAVY_CREAM */ "深蓝奶白",
        /* BOUNCE_LOCALE_THEME_FOREST_GREEN_PALE_MINT */ "森林浅薄荷",
        /* BOUNCE_LOCALE_THEME_BURGUNDY_SOFT_BLUSH */ "酒红淡粉",
        /* BOUNCE_LOCALE_THEME_CHARCOAL_ELECTRIC_BLUE */ "炭黑电蓝",
        /* BOUNCE_LOCALE_THEME_CLASSIC_NOKIA_BLUE */ "经典诺基亚蓝",
        /* BOUNCE_LOCALE_THEME_PURPLE_HAZE */ "紫色迷雾",
        /* BOUNCE_LOCALE_THEME_ARCTIC_WHITE */ "极地白",
        /* BOUNCE_LOCALE_THEME_DARK_TEAL_IVORY */ "深青象牙",
        /* BOUNCE_LOCALE_LOGICAL */ "逻辑",
        /* BOUNCE_LOCALE_LOGICAL_RESOLUTION */ "逻辑分辨率",
        /* BOUNCE_LOCALE_GAMEPLAY_VIEWPORT */ "游戏视口",
        /* BOUNCE_LOCALE_UI_HUD_REGION */ "界面信息区",
        /* BOUNCE_LOCALE_READ_ONLY_NATIVE */ "只读",
        /* BOUNCE_LOCALE_LEFT */ "左",
        /* BOUNCE_LOCALE_RIGHT */ "右",
        /* BOUNCE_LOCALE_UP */ "上",
        /* BOUNCE_LOCALE_DOWN */ "下",
        /* BOUNCE_LOCALE_ARROW_A */ "方向键 A",
        /* BOUNCE_LOCALE_ARROW_D */ "方向键 D",
        /* BOUNCE_LOCALE_ARROW_W */ "方向键 W",
        /* BOUNCE_LOCALE_ARROW_S */ "方向键 S",
        /* BOUNCE_LOCALE_ESCAPE */ "返回键",
        /* BOUNCE_LOCALE_NO_REMAPPING_YET */ "暂不支持改键",
        /* BOUNCE_LOCALE_STATUS */ "状态",
        /* BOUNCE_LOCALE_ON */ "开",
        /* BOUNCE_LOCALE_OFF */ "关",
        /* BOUNCE_LOCALE_NOT_YET_IMPLEMENTED */ "尚未实现",
        /* BOUNCE_LOCALE_NO_AUDIO_PATH_YET */ "尚无声音输出",
        /* BOUNCE_LOCALE_SELECT_TO_SWITCH */ "选择以切换",
        /* BOUNCE_LOCALE_RESOURCE_ONLY */ "仅资源",
        /* BOUNCE_LOCALE_NATIVE_STRINGS */ "原生文本",
        /* BOUNCE_LOCALE_DEFAULT_LANGUAGE */ "默认",
        /* BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA */ "继续 无存档",
        /* BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE */ "设置 关于 原生扩展",
        /* BOUNCE_LOCALE_NOKIA_BOUNCE */ "NOKIA BOUNCE",
        /* BOUNCE_LOCALE_NATIVE_LINUX_BUILD */ "原生 Linux 版本",
        /* BOUNCE_LOCALE_ORIGINAL_INSPIRED */ "参照原作",
        /* BOUNCE_LOCALE_BEHAVIOR_BASED */ "以行为为准",
        /* BOUNCE_LOCALE_PLATFORM_LINUX */ "PLATFORM LINUX",
        /* BOUNCE_LOCALE_NATIVE_SCREEN */ "原生画面",
        /* BOUNCE_LOCALE_LEVEL_SELECT */ "选关",
        /* BOUNCE_LOCALE_LEVEL */ "级别",
        /* BOUNCE_LOCALE_BACK */ "返回",
        /* BOUNCE_LOCALE_ESC_BACK */ "ESC 返回",
        /* BOUNCE_LOCALE_NO_RMS_VALUE */ "无存档数值",
        /* BOUNCE_LOCALE_GAMEPLAY */ "游戏中",
        /* BOUNCE_LOCALE_DESTINATION */ "目标",
        /* BOUNCE_LOCALE_BACK_TO_MENU */ "返回菜单",
        /* BOUNCE_LOCALE_SCORE */ "分数",
        /* BOUNCE_LOCALE_LIVES */ "生命",
        /* BOUNCE_LOCALE_COUNTDOWN */ "倒计时",
        /* BOUNCE_LOCALE_NOT_STARTED */ "未开始",
        /* BOUNCE_LOCALE_INSTRUCTIONS_BODY */ BOUNCE_INSTRUCTIONS_ZH_CN,
        /* BOUNCE_LOCALE_INSTRUCTIONS_PREFIX */ BOUNCE_INSTRUCTIONS_PREFIX_ZH_CN,
        /* STEP 12X-P4-C-B Game End. DECODED from the shipped lang.zh-CN,
         * lang.zh-TW and lang.th-TH resources (Translation ids 6, 3, 13). Each
         * source string ends in an exclamation mark, which is dropped: the
         * native font policy covers A-Z, 0-9 and space only. The pre-existing
         * rows above stay the provisional test strings that
         * bounce_locale_text() already documents. */
        /* BOUNCE_LOCALE_GAME_END_TITLE */ "游戏结束",
        /* BOUNCE_LOCALE_GAME_END_CONGRATS */ "祝贺你！",
        /* BOUNCE_LOCALE_GAME_END_OK */ "确认",
        /* D-07, Translation id 12, decoded from the shipped
         * lang resource. */
        /* BOUNCE_LOCALE_NEW_HIGH_SCORE */ "新最高分！",
        /* STEP 13G-A, Translation id 10, lang.zh-CN verbatim, newline INCLUDED. */
        /* BOUNCE_LOCALE_LEVEL_COMPLETED */ "级别%U\n已完成",
        /* BOUNCE_LOCALE_T9_INPUT */ "数字键盘"
    },
    /* ------- ZH_TW: TEMPORARY TEST TABLE, NOT A RECOVERED RESOURCE ------- */
    [BOUNCE_LANGUAGE_ZH_TW] = {
        /* BOUNCE_LOCALE_TITLE */ "BOUNCE",
        /* BOUNCE_LOCALE_CONTINUE */ "繼續",
        /* BOUNCE_LOCALE_NEW_GAME */ "新遊戲",
        /* BOUNCE_LOCALE_HIGH_SCORE */ "最高分",
        /* BOUNCE_LOCALE_INSTRUCTIONS */ "遊戲規則說明",
        /* BOUNCE_LOCALE_EXIT */ "退出",
        /* BOUNCE_LOCALE_SETTINGS */ "設定",
        /* BOUNCE_LOCALE_ABOUT */ "關於",
        /* BOUNCE_LOCALE_START_LEVEL */ "開始關卡",
        /* BOUNCE_LOCALE_CONTINUE_UNAVAILABLE */ "繼續不可用",
        /* BOUNCE_LOCALE_NO_ACTION */ "無操作",
        /* BOUNCE_LOCALE_SETTINGS_DISPLAY */ "顯示設定",
        /* BOUNCE_LOCALE_SETTINGS_INPUT */ "按鍵設定",
        /* BOUNCE_LOCALE_SETTINGS_AUDIO */ "聲音設定",
        /* BOUNCE_LOCALE_SETTINGS_LANGUAGE */ "語言設定",
        /* BOUNCE_LOCALE_DISPLAY */ "顯示",
        /* BOUNCE_LOCALE_INPUT */ "按鍵",
        /* BOUNCE_LOCALE_AUDIO */ "聲音",
        /* BOUNCE_LOCALE_LANGUAGE */ "語言",
        /* BOUNCE_LOCALE_THEME */ "主題",
        /* BOUNCE_LOCALE_THEME_DEFAULT */ "預設",
        /* BOUNCE_LOCALE_THEME_GREEN_LCD */ "綠色液晶",
        /* BOUNCE_LOCALE_THEME_SEPIA_TONE */ "古棕色調",
        /* BOUNCE_LOCALE_THEME_GREEN_LIGHT */ "GREEN LCD",
        /* BOUNCE_LOCALE_THEME_INVERTED_GREEN_LCD */ "反色綠液晶",
        /* BOUNCE_LOCALE_THEME_AMBER_ORANGE */ "琥珀單色",
        /* BOUNCE_LOCALE_THEME_DEEP_NAVY_CREAM */ "深藍奶白",
        /* BOUNCE_LOCALE_THEME_FOREST_GREEN_PALE_MINT */ "森林淺薄荷",
        /* BOUNCE_LOCALE_THEME_BURGUNDY_SOFT_BLUSH */ "酒紅淡粉",
        /* BOUNCE_LOCALE_THEME_CHARCOAL_ELECTRIC_BLUE */ "炭黑電藍",
        /* BOUNCE_LOCALE_THEME_CLASSIC_NOKIA_BLUE */ "經典諾基亞藍",
        /* BOUNCE_LOCALE_THEME_PURPLE_HAZE */ "紫色迷霧",
        /* BOUNCE_LOCALE_THEME_ARCTIC_WHITE */ "極地白",
        /* BOUNCE_LOCALE_THEME_DARK_TEAL_IVORY */ "深青象牙",
        /* BOUNCE_LOCALE_LOGICAL */ "邏輯",
        /* BOUNCE_LOCALE_LOGICAL_RESOLUTION */ "邏輯解析度",
        /* BOUNCE_LOCALE_GAMEPLAY_VIEWPORT */ "遊戲視窗",
        /* BOUNCE_LOCALE_UI_HUD_REGION */ "介面資訊區",
        /* BOUNCE_LOCALE_READ_ONLY_NATIVE */ "唯讀",
        /* BOUNCE_LOCALE_LEFT */ "左",
        /* BOUNCE_LOCALE_RIGHT */ "右",
        /* BOUNCE_LOCALE_UP */ "上",
        /* BOUNCE_LOCALE_DOWN */ "下",
        /* BOUNCE_LOCALE_ARROW_A */ "方向鍵 A",
        /* BOUNCE_LOCALE_ARROW_D */ "方向鍵 D",
        /* BOUNCE_LOCALE_ARROW_W */ "方向鍵 W",
        /* BOUNCE_LOCALE_ARROW_S */ "方向鍵 S",
        /* BOUNCE_LOCALE_ESCAPE */ "返回鍵",
        /* BOUNCE_LOCALE_NO_REMAPPING_YET */ "暫不支援改鍵",
        /* BOUNCE_LOCALE_STATUS */ "狀態",
        /* BOUNCE_LOCALE_ON */ "開",
        /* BOUNCE_LOCALE_OFF */ "關",
        /* BOUNCE_LOCALE_NOT_YET_IMPLEMENTED */ "尚未實作",
        /* BOUNCE_LOCALE_NO_AUDIO_PATH_YET */ "尚無聲音輸出",
        /* BOUNCE_LOCALE_SELECT_TO_SWITCH */ "選擇以切換",
        /* BOUNCE_LOCALE_RESOURCE_ONLY */ "僅資源",
        /* BOUNCE_LOCALE_NATIVE_STRINGS */ "原生文字",
        /* BOUNCE_LOCALE_DEFAULT_LANGUAGE */ "預設",
        /* BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA */ "繼續 無存檔",
        /* BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE */ "設定 關於 原生擴充",
        /* BOUNCE_LOCALE_NOKIA_BOUNCE */ "NOKIA BOUNCE",
        /* BOUNCE_LOCALE_NATIVE_LINUX_BUILD */ "原生 Linux 版本",
        /* BOUNCE_LOCALE_ORIGINAL_INSPIRED */ "參照原作",
        /* BOUNCE_LOCALE_BEHAVIOR_BASED */ "以行為為準",
        /* BOUNCE_LOCALE_PLATFORM_LINUX */ "PLATFORM LINUX",
        /* BOUNCE_LOCALE_NATIVE_SCREEN */ "原生畫面",
        /* BOUNCE_LOCALE_LEVEL_SELECT */ "選關",
        /* BOUNCE_LOCALE_LEVEL */ "等級",
        /* BOUNCE_LOCALE_BACK */ "返回",
        /* BOUNCE_LOCALE_ESC_BACK */ "ESC 返回",
        /* BOUNCE_LOCALE_NO_RMS_VALUE */ "無存檔數值",
        /* BOUNCE_LOCALE_GAMEPLAY */ "遊戲中",
        /* BOUNCE_LOCALE_DESTINATION */ "目標",
        /* BOUNCE_LOCALE_BACK_TO_MENU */ "返回選單",
        /* BOUNCE_LOCALE_SCORE */ "分數",
        /* BOUNCE_LOCALE_LIVES */ "生命",
        /* BOUNCE_LOCALE_COUNTDOWN */ "倒數計時",
        /* BOUNCE_LOCALE_NOT_STARTED */ "未開始",
        /* BOUNCE_LOCALE_INSTRUCTIONS_BODY */ BOUNCE_INSTRUCTIONS_ZH_TW,
        /* BOUNCE_LOCALE_INSTRUCTIONS_PREFIX */ BOUNCE_INSTRUCTIONS_PREFIX_ZH_TW,
        /* STEP 12X-P4-C-B Game End. DECODED from the shipped lang.zh-CN,
         * lang.zh-TW and lang.th-TH resources (Translation ids 6, 3, 13). Each
         * source string ends in an exclamation mark, which is dropped: the
         * native font policy covers A-Z, 0-9 and space only. The pre-existing
         * rows above stay the provisional test strings that
         * bounce_locale_text() already documents. */
        /* BOUNCE_LOCALE_GAME_END_TITLE */ "遊戲結束",
        /* BOUNCE_LOCALE_GAME_END_CONGRATS */ "恭喜！",
        /* BOUNCE_LOCALE_GAME_END_OK */ "確認",
        /* D-07, Translation id 12, decoded from the shipped
         * lang resource. */
        /* BOUNCE_LOCALE_NEW_HIGH_SCORE */ "刷新紀錄！",
        /* STEP 13G-A, Translation id 10, lang.zh-TW verbatim, newline INCLUDED. */
        /* BOUNCE_LOCALE_LEVEL_COMPLETED */ "等級%U\n已完成！",
        /* BOUNCE_LOCALE_T9_INPUT */ "數字鍵盤"
    },
    /* ------- TH_TH: TEMPORARY TEST TABLE, NOT A RECOVERED RESOURCE ------- */
    [BOUNCE_LANGUAGE_TH_TH] = {
        /* BOUNCE_LOCALE_TITLE */ "BOUNCE",
        /* BOUNCE_LOCALE_CONTINUE */ "เล่นต่อ",
        /* BOUNCE_LOCALE_NEW_GAME */ "เกมส์ใหม่",
        /* BOUNCE_LOCALE_HIGH_SCORE */ "คะแนนสูงสุด",
        /* BOUNCE_LOCALE_INSTRUCTIONS */ "วิธีการเล่น",
        /* BOUNCE_LOCALE_EXIT */ "ออก",
        /* BOUNCE_LOCALE_SETTINGS */ "ตั้งค่า",
        /* BOUNCE_LOCALE_ABOUT */ "ข้อมูล",
        /* BOUNCE_LOCALE_START_LEVEL */ "เริ่มด่าน",
        /* BOUNCE_LOCALE_CONTINUE_UNAVAILABLE */ "เล่นต่อไม่ได้",
        /* BOUNCE_LOCALE_NO_ACTION */ "ไม่มีคำสั่ง",
        /* BOUNCE_LOCALE_SETTINGS_DISPLAY */ "ตั้งค่าจอ",
        /* BOUNCE_LOCALE_SETTINGS_INPUT */ "ตั้งค่าปุ่ม",
        /* BOUNCE_LOCALE_SETTINGS_AUDIO */ "ตั้งค่าเสียง",
        /* BOUNCE_LOCALE_SETTINGS_LANGUAGE */ "ตั้งค่าภาษา",
        /* BOUNCE_LOCALE_DISPLAY */ "จอ",
        /* BOUNCE_LOCALE_INPUT */ "ปุ่ม",
        /* BOUNCE_LOCALE_AUDIO */ "เสียง",
        /* BOUNCE_LOCALE_LANGUAGE */ "ภาษา",
        /* BOUNCE_LOCALE_THEME */ "ธีม",
        /* BOUNCE_LOCALE_THEME_DEFAULT */ "ค่าเริ่มต้น",
        /* BOUNCE_LOCALE_THEME_GREEN_LCD */ "จอเขียว",
        /* BOUNCE_LOCALE_THEME_SEPIA_TONE */ "โทนน้ำตาล",
        /* BOUNCE_LOCALE_THEME_GREEN_LIGHT */ "GREEN LCD",
        /* BOUNCE_LOCALE_THEME_INVERTED_GREEN_LCD */ "จอเขียวกลับสี",
        /* BOUNCE_LOCALE_THEME_AMBER_ORANGE */ "สีส้มอำพันเดียว",
        /* BOUNCE_LOCALE_THEME_DEEP_NAVY_CREAM */ "น้ำเงินเข้มครีม",
        /* BOUNCE_LOCALE_THEME_FOREST_GREEN_PALE_MINT */ "เขียวป่ามิ้นต์อ่อน",
        /* BOUNCE_LOCALE_THEME_BURGUNDY_SOFT_BLUSH */ "แดงเบอร์กันดี้บลัชอ่อน",
        /* BOUNCE_LOCALE_THEME_CHARCOAL_ELECTRIC_BLUE */ "ดำถ่านน้ำเงินไฟฟ้า",
        /* BOUNCE_LOCALE_THEME_CLASSIC_NOKIA_BLUE */ "น้ำเงินโนเกียคลาสสิก",
        /* BOUNCE_LOCALE_THEME_PURPLE_HAZE */ "ม่วงหมอก",
        /* BOUNCE_LOCALE_THEME_ARCTIC_WHITE */ "ขาวอาร์กติก",
        /* BOUNCE_LOCALE_THEME_DARK_TEAL_IVORY */ "เขียวแก่น้ำทะเลงาช้าง",
        /* BOUNCE_LOCALE_LOGICAL */ "เชิงตรรกะ",
        /* BOUNCE_LOCALE_LOGICAL_RESOLUTION */ "ความละเอียด",
        /* BOUNCE_LOCALE_GAMEPLAY_VIEWPORT */ "พื้นที่เล่น",
        /* BOUNCE_LOCALE_UI_HUD_REGION */ "แถบข้อมูล",
        /* BOUNCE_LOCALE_READ_ONLY_NATIVE */ "อ่านอย่างเดียว",
        /* BOUNCE_LOCALE_LEFT */ "ซ้าย",
        /* BOUNCE_LOCALE_RIGHT */ "ขวา",
        /* BOUNCE_LOCALE_UP */ "บน",
        /* BOUNCE_LOCALE_DOWN */ "ล่าง",
        /* BOUNCE_LOCALE_ARROW_A */ "ลูกศร A",
        /* BOUNCE_LOCALE_ARROW_D */ "ลูกศร D",
        /* BOUNCE_LOCALE_ARROW_W */ "ลูกศร W",
        /* BOUNCE_LOCALE_ARROW_S */ "ลูกศร S",
        /* BOUNCE_LOCALE_ESCAPE */ "ปุ่ม Esc",
        /* BOUNCE_LOCALE_NO_REMAPPING_YET */ "ยังเปลี่ยนปุ่มไม่ได้",
        /* BOUNCE_LOCALE_STATUS */ "สถานะ",
        /* BOUNCE_LOCALE_ON */ "เปิด",
        /* BOUNCE_LOCALE_OFF */ "ปิด",
        /* BOUNCE_LOCALE_NOT_YET_IMPLEMENTED */ "ยังไม่ได้ทำ",
        /* BOUNCE_LOCALE_NO_AUDIO_PATH_YET */ "ยังไม่มีเสียง",
        /* BOUNCE_LOCALE_SELECT_TO_SWITCH */ "เลือกเพื่อเปลี่ยน",
        /* BOUNCE_LOCALE_RESOURCE_ONLY */ "มีแต่ไฟล์",
        /* BOUNCE_LOCALE_NATIVE_STRINGS */ "ข้อความต้นฉบับ",
        /* BOUNCE_LOCALE_DEFAULT_LANGUAGE */ "ค่าเริ่มต้น",
        /* BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA */ "เล่นต่อ ไม่มีข้อมูล",
        /* BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE */ "ตั้งค่า ข้อมูล ต้นฉบับ",
        /* BOUNCE_LOCALE_NOKIA_BOUNCE */ "NOKIA BOUNCE",
        /* BOUNCE_LOCALE_NATIVE_LINUX_BUILD */ "รุ่น Linux",
        /* BOUNCE_LOCALE_ORIGINAL_INSPIRED */ "ดัดแปลงจากต้นฉบับ",
        /* BOUNCE_LOCALE_BEHAVIOR_BASED */ "อิงพฤติกรรมต้นฉบับ",
        /* BOUNCE_LOCALE_PLATFORM_LINUX */ "ระบบ Linux",
        /* BOUNCE_LOCALE_NATIVE_SCREEN */ "จอต้นฉบับ",
        /* BOUNCE_LOCALE_LEVEL_SELECT */ "เลือกด่าน",
        /* BOUNCE_LOCALE_LEVEL */ "ระดับ",
        /* BOUNCE_LOCALE_BACK */ "ย้อนกลับ",
        /* BOUNCE_LOCALE_ESC_BACK */ "ESC กลับ",
        /* BOUNCE_LOCALE_NO_RMS_VALUE */ "ไม่มีค่าใน RMS",
        /* BOUNCE_LOCALE_GAMEPLAY */ "กำลังเล่น",
        /* BOUNCE_LOCALE_DESTINATION */ "ปลายทาง",
        /* BOUNCE_LOCALE_BACK_TO_MENU */ "กลับเมนู",
        /* BOUNCE_LOCALE_SCORE */ "คะแนน",
        /* BOUNCE_LOCALE_LIVES */ "ชีวิต",
        /* BOUNCE_LOCALE_COUNTDOWN */ "นับถอยหลัง",
        /* BOUNCE_LOCALE_NOT_STARTED */ "ยังไม่เริ่ม",
        /* BOUNCE_LOCALE_INSTRUCTIONS_BODY */ BOUNCE_INSTRUCTIONS_TH_TH,
        /* BOUNCE_LOCALE_INSTRUCTIONS_PREFIX */ BOUNCE_INSTRUCTIONS_PREFIX_TH_TH,
        /* STEP 12X-P4-C-B Game End. DECODED from the shipped lang.zh-CN,
         * lang.zh-TW and lang.th-TH resources (Translation ids 6, 3, 13). Each
         * source string ends in an exclamation mark, which is dropped: the
         * native font policy covers A-Z, 0-9 and space only. The pre-existing
         * rows above stay the provisional test strings that
         * bounce_locale_text() already documents. */
        /* BOUNCE_LOCALE_GAME_END_TITLE */ "จบเกมส์",
        /* BOUNCE_LOCALE_GAME_END_CONGRATS */ "ยินดีด้วย!",
        /* BOUNCE_LOCALE_GAME_END_OK */ "ตกลง",
        /* D-07, Translation id 12, decoded from the shipped
         * lang resource. */
        /* BOUNCE_LOCALE_NEW_HIGH_SCORE */ "คะแนน​สูงสุด ใหม่!",
        /* STEP 13G-A, Translation id 10, lang.th-TH verbatim. */
        /* BOUNCE_LOCALE_LEVEL_COMPLETED */ "ผ่านระดับ %U!",
        /* BOUNCE_LOCALE_T9_INPUT */ "แป้นตัวเลข"
    }
};

/*
 * Recovered resource identity. The four ORIGINAL SOURCE-BACKED languages point
 * at the files that ship in src/main/resources. Indonesian is a NATIVE
 * EXTENSION with no file, so its entry is deliberately NULL.
 */
static const char *const locale_resource_names[BOUNCE_LANGUAGE_COUNT] = {
    "/lang.xx",
    "/lang.zh-CN",
    "/lang.zh-TW",
    "/lang.th-TH",
    NULL
};

/*
 * Compact user-facing labels.
 *
 * These are DISPLAY labels only. The locale identity is the resource path in
 * locale_resource_names[] and is unchanged: /lang.xx, /lang.zh-CN, /lang.zh-TW,
 * /lang.th-TH, and the native Indonesian extension. Shortening "zh-CN" to "CN"
 * does not create a new language, rename a resource, or change any mapping; it
 * only keeps the 128x128 list visually even. Two-letter codes were chosen over
 * the endonyms because the native UI font has no CJK or Thai glyphs, and over
 * the longer "zh-CN" form because the shorter label is what the existing
 * two-character rows use, so all five entries read consistently.
 */
static const char *const locale_language_labels[BOUNCE_LANGUAGE_COUNT] = {
    /* The default English resource is shown as EN, never as its "xx" token. */
    "EN",
    "CN",
    "TW",
    "TH",
    "ID"
};

static BounceLanguage active_language = BOUNCE_LANGUAGE_EN;

BounceLanguage bounce_locale_default_language(void)
{
    return BOUNCE_LANGUAGE_EN;
}

const char *bounce_locale_text(BounceLanguage language, BounceLocaleKey key)
{
    if ((int)key < 0 || (int)key >= BOUNCE_LOCALE_KEY_COUNT)
        return NULL;
    if ((int)language < 0 || (int)language >= (int)BOUNCE_LANGUAGE_COUNT)
        return NULL;

    /*
     * Every language has its own column, so this is a straight lookup. A missing
     * entry falls back to the source-backed English default; that is a safety net
     * for an unpopulated key, not the normal path.
     *
     * D-24 -- THE CHINESE AND THAI COLUMNS ARE NOW THE SHIPPED RESOURCES. The
     * note they used to carry here said:
     *
     *     The Chinese and Thai columns are TEMPORARY TEST STRINGS authored for
     *     the runtime language switching test. They are not decoded from
     *     lang.zh-CN, lang.zh-TW or lang.th-TH, those files are untouched, and
     *     no localization decoder exists yet.
     *
     * THAT IS NO LONGER TRUE and is quoted rather than merely replaced so the
     * wrong claim cannot be read again as current. Every value for the fourteen
     * shipped translation ids in this table is now decoded from
     * src/main/resources/lang.zh-CN, lang.zh-TW and lang.th-TH, by the same
     * fourteen-id offset layout Translation.java addresses. Indonesian remains
     * the one NATIVE EXTENSION column, because no lang.id resource exists.
     *
     * THE THAI COLUMN NOW RENDERS, which it did not when this note was written.
     * It previously said:
     *
     *     THE THAI COLUMN STILL RENDERS AS THE UNKNOWN-GLYPH BOX ... The text is
     *     CORRECT and the glyphs are absent; nothing here invents either.
     *
     * The glyphs were absent because the stated reason was wrong. script_font.c
     * has carried a Thai set since V-2: measured over every cluster lang.th-TH
     * uses, Thai needs ELEVEN rows at 8 px -- not the thirteen the old note
     * claimed -- so it fits this 12-row pitch whole at 9 px, the largest size
     * that does. Every glyph is rasterised from real Noto Sans Thai outlines by
     * gen_script_font.py; nothing is hand-drawn.
     *
     * bounce_locale_is_recovered_resource() still reports RESOURCE ONLY for the
     * three languages with a shipped file, which is a statement about the FILES
     * and not about whether the language can be applied.
     */
    if (locale_text[language][key] != NULL)
        return locale_text[language][key];
    return locale_text[BOUNCE_LANGUAGE_EN][key];
}

const char *bounce_locale_instructions_text(BounceLanguage language)
{
    return bounce_locale_text(language, BOUNCE_LOCALE_INSTRUCTIONS_BODY);
}

const char *bounce_locale_language_label(BounceLanguage language)
{
    if ((int)language < 0 || (int)language >= (int)BOUNCE_LANGUAGE_COUNT)
        return NULL;
    return locale_language_labels[language];
}

const char *bounce_locale_resource_name(BounceLanguage language)
{
    if ((int)language < 0 || (int)language >= (int)BOUNCE_LANGUAGE_COUNT)
        return NULL;
    return locale_resource_names[language];
}

bool bounce_locale_is_recovered_resource(BounceLanguage language)
{
    return (int)language >= 0
        && (int)language < (int)BOUNCE_LANGUAGE_COUNT
        && locale_resource_names[language] != NULL;
}

void bounce_locale_set_active_language(BounceLanguage language)
{
    if ((int)language < 0 || (int)language >= (int)BOUNCE_LANGUAGE_COUNT)
        return;
    active_language = language;
}

BounceLanguage bounce_locale_active_language(void)
{
    return active_language;
}
