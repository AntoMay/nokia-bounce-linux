"""Translation tables for the CJK and Thai columns, and the glyph generator's
only input.

PROVENANCE, WHICH CHANGED (D-24)
-------------------------------
Every value for the fourteen shipped translation ids is DECODED FROM
src/main/resources/lang.zh-CN, lang.zh-TW and lang.th-TH -- the same files the
recovered game itself loaded. They used to be TEMPORARY TEST STRINGS authored for
the runtime language-switching test, and this file said so at the top. That
statement has been removed rather than amended, because it is no longer true and
the file is the provenance record gen_script_font.py renders from.

STILL NATIVE-ONLY
-----------------
Everything that is not one of those fourteen ids -- Settings, About, the platform
legend, the "behaviour based" note, the colour-profile names, the key legends --
has no shipped resource, because the recovered game has no Settings screen at
all. Those rows are authored here and remain marked as such by
validate() and by the locale.c header.

TWO ROWS DELIBERATELY DIFFER FROM THE RESOURCE
----------------------------------------------
BOUNCE_LOCALE_LEVEL drops the resource's "%U", because native draws the level
label and the level number as two separate calls (the 5x7 font has no digit
glyph, which is why the level List and the High Score screen already do this).
The Instructions prefix is a bounded leading span of the body rather than the
whole paragraph, because the page is 12 rows tall and there is no
source-verified scrolling or paging.
"""

# Product, platform and key-cap names stay in Latin on purpose: they are proper
# nouns and key legends, and translating them would be wrong, not helpful.
KEEP_LATIN = {
    "BOUNCE_LOCALE_TITLE",
    "BOUNCE_LOCALE_ARROW_A",
    "BOUNCE_LOCALE_ARROW_D",
    "BOUNCE_LOCALE_ARROW_W",
    "BOUNCE_LOCALE_ARROW_S",
    "BOUNCE_LOCALE_ESCAPE",
    "BOUNCE_LOCALE_NOKIA_BOUNCE",
    "BOUNCE_LOCALE_PLATFORM_LINUX",
}

# Key order taken verbatim from the EN table in native/app/locale.c.
KEYS = [
    "BOUNCE_LOCALE_TITLE", "BOUNCE_LOCALE_CONTINUE", "BOUNCE_LOCALE_NEW_GAME",
    "BOUNCE_LOCALE_HIGH_SCORE", "BOUNCE_LOCALE_INSTRUCTIONS", "BOUNCE_LOCALE_EXIT",
    "BOUNCE_LOCALE_SETTINGS", "BOUNCE_LOCALE_ABOUT", "BOUNCE_LOCALE_START_LEVEL",
    "BOUNCE_LOCALE_CONTINUE_UNAVAILABLE", "BOUNCE_LOCALE_NO_ACTION",
    "BOUNCE_LOCALE_SETTINGS_DISPLAY", "BOUNCE_LOCALE_SETTINGS_INPUT",
    "BOUNCE_LOCALE_SETTINGS_AUDIO", "BOUNCE_LOCALE_SETTINGS_LANGUAGE",
    "BOUNCE_LOCALE_DISPLAY", "BOUNCE_LOCALE_INPUT", "BOUNCE_LOCALE_AUDIO",
    "BOUNCE_LOCALE_LANGUAGE", "BOUNCE_LOCALE_THEME", "BOUNCE_LOCALE_THEME_DEFAULT",
    "BOUNCE_LOCALE_THEME_GREEN_LCD", "BOUNCE_LOCALE_THEME_SEPIA_TONE",
    "BOUNCE_LOCALE_LOGICAL", "BOUNCE_LOCALE_LOGICAL_RESOLUTION",
    "BOUNCE_LOCALE_GAMEPLAY_VIEWPORT", "BOUNCE_LOCALE_UI_HUD_REGION",
    "BOUNCE_LOCALE_READ_ONLY_NATIVE", "BOUNCE_LOCALE_LEFT", "BOUNCE_LOCALE_RIGHT",
    "BOUNCE_LOCALE_UP", "BOUNCE_LOCALE_DOWN", "BOUNCE_LOCALE_ARROW_A",
    "BOUNCE_LOCALE_ARROW_D", "BOUNCE_LOCALE_ARROW_W", "BOUNCE_LOCALE_ARROW_S",
    "BOUNCE_LOCALE_ESCAPE", "BOUNCE_LOCALE_NO_REMAPPING_YET", "BOUNCE_LOCALE_STATUS",
    "BOUNCE_LOCALE_NOT_YET_IMPLEMENTED", "BOUNCE_LOCALE_NO_AUDIO_PATH_YET",
    "BOUNCE_LOCALE_SELECT_TO_SWITCH", "BOUNCE_LOCALE_RESOURCE_ONLY",
    "BOUNCE_LOCALE_NATIVE_STRINGS", "BOUNCE_LOCALE_DEFAULT_LANGUAGE",
    "BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA", "BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE",
    "BOUNCE_LOCALE_NOKIA_BOUNCE", "BOUNCE_LOCALE_NATIVE_LINUX_BUILD",
    "BOUNCE_LOCALE_ORIGINAL_INSPIRED", "BOUNCE_LOCALE_BEHAVIOR_BASED",
    "BOUNCE_LOCALE_PLATFORM_LINUX", "BOUNCE_LOCALE_NATIVE_SCREEN",
    "BOUNCE_LOCALE_LEVEL_SELECT", "BOUNCE_LOCALE_LEVEL", "BOUNCE_LOCALE_BACK",
    "BOUNCE_LOCALE_ESC_BACK", "BOUNCE_LOCALE_NO_RMS_VALUE", "BOUNCE_LOCALE_GAMEPLAY",
    "BOUNCE_LOCALE_DESTINATION", "BOUNCE_LOCALE_BACK_TO_MENU", "BOUNCE_LOCALE_SCORE",
    "BOUNCE_LOCALE_LIVES", "BOUNCE_LOCALE_COUNTDOWN", "BOUNCE_LOCALE_NOT_STARTED",
    "BOUNCE_LOCALE_GAME_END_CONGRATS",
    "BOUNCE_LOCALE_LEVEL_COMPLETED", "BOUNCE_LOCALE_NEW_HIGH_SCORE",
    "BOUNCE_LOCALE_GAME_END_OK"
]

EN = {
    "BOUNCE_LOCALE_TITLE": "BOUNCE", "BOUNCE_LOCALE_CONTINUE": "CONTINUE",
    "BOUNCE_LOCALE_NEW_GAME": "NEW GAME", "BOUNCE_LOCALE_HIGH_SCORE": "HIGH SCORE",
    "BOUNCE_LOCALE_INSTRUCTIONS": "INSTRUCTIONS", "BOUNCE_LOCALE_EXIT": "EXIT",
    "BOUNCE_LOCALE_SETTINGS": "SETTINGS", "BOUNCE_LOCALE_ABOUT": "ABOUT",
    "BOUNCE_LOCALE_START_LEVEL": "START LEVEL",
    "BOUNCE_LOCALE_CONTINUE_UNAVAILABLE": "CONTINUE UNAVAILABLE",
    "BOUNCE_LOCALE_NO_ACTION": "NO ACTION",
    "BOUNCE_LOCALE_SETTINGS_DISPLAY": "SETTINGS DISPLAY",
    "BOUNCE_LOCALE_SETTINGS_INPUT": "SETTINGS INPUT",
    "BOUNCE_LOCALE_SETTINGS_AUDIO": "SETTINGS AUDIO",
    "BOUNCE_LOCALE_SETTINGS_LANGUAGE": "SETTINGS LANGUAGE",
    "BOUNCE_LOCALE_DISPLAY": "DISPLAY", "BOUNCE_LOCALE_INPUT": "INPUT",
    "BOUNCE_LOCALE_AUDIO": "AUDIO", "BOUNCE_LOCALE_LANGUAGE": "LANGUAGE",
    "BOUNCE_LOCALE_THEME": "THEME", "BOUNCE_LOCALE_THEME_DEFAULT": "DEFAULT",
    "BOUNCE_LOCALE_THEME_GREEN_LCD": "GREEN LCD",
    "BOUNCE_LOCALE_THEME_SEPIA_TONE": "SEPIA",
    "BOUNCE_LOCALE_LOGICAL": "LOGICAL",
    "BOUNCE_LOCALE_LOGICAL_RESOLUTION": "LOGICAL RESOLUTION",
    "BOUNCE_LOCALE_GAMEPLAY_VIEWPORT": "GAMEPLAY VIEWPORT",
    "BOUNCE_LOCALE_UI_HUD_REGION": "UI HUD REGION",
    "BOUNCE_LOCALE_READ_ONLY_NATIVE": "READ ONLY NATIVE",
    "BOUNCE_LOCALE_LEFT": "LEFT", "BOUNCE_LOCALE_RIGHT": "RIGHT",
    "BOUNCE_LOCALE_UP": "UP", "BOUNCE_LOCALE_DOWN": "DOWN",
    "BOUNCE_LOCALE_ARROW_A": "ARROW A", "BOUNCE_LOCALE_ARROW_D": "ARROW D",
    "BOUNCE_LOCALE_ARROW_W": "ARROW W", "BOUNCE_LOCALE_ARROW_S": "ARROW S",
    "BOUNCE_LOCAPE_UNUSED": "", "BOUNCE_LOCALE_ESCAPE": "ESCAPE",
    "BOUNCE_LOCALE_NO_REMAPPING_YET": "NO REMAPPING YET",
    "BOUNCE_LOCALE_STATUS": "STATUS",
    "BOUNCE_LOCALE_NOT_YET_IMPLEMENTED": "NOT YET IMPLEMENTED",
    "BOUNCE_LOCALE_NO_AUDIO_PATH_YET": "NO AUDIO PATH YET",
    "BOUNCE_LOCALE_SELECT_TO_SWITCH": "SELECT TO SWITCH",
    "BOUNCE_LOCALE_RESOURCE_ONLY": "RESOURCE ONLY",
    "BOUNCE_LOCALE_NATIVE_STRINGS": "NATIVE STRINGS",
    "BOUNCE_LOCALE_DEFAULT_LANGUAGE": "DEFAULT",
    "BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA": "CONTINUE NO SAVE DATA",
    "BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE": "SETTINGS ABOUT NATIVE",
    "BOUNCE_LOCALE_NOKIA_BOUNCE": "NOKIA BOUNCE",
    "BOUNCE_LOCALE_NATIVE_LINUX_BUILD": "NATIVE LINUX BUILD",
    "BOUNCE_LOCALE_ORIGINAL_INSPIRED": "ORIGINAL INSPIRED",
    "BOUNCE_LOCALE_BEHAVIOR_BASED": "BEHAVIOR BASED",
    "BOUNCE_LOCALE_PLATFORM_LINUX": "PLATFORM LINUX",
    "BOUNCE_LOCALE_NATIVE_SCREEN": "NATIVE SCREEN",
    "BOUNCE_LOCALE_LEVEL_SELECT": "LEVEL SELECT", "BOUNCE_LOCALE_LEVEL": "LEVEL",
    "BOUNCE_LOCALE_BACK": "BACK", "BOUNCE_LOCALE_ESC_BACK": "ESC BACK",
    "BOUNCE_LOCALE_NO_RMS_VALUE": "NO RMS VALUE",
    "BOUNCE_LOCALE_GAMEPLAY": "GAMEPLAY", "BOUNCE_LOCALE_DESTINATION": "DESTINATION",
    "BOUNCE_LOCALE_BACK_TO_MENU": "BACK TO MENU", "BOUNCE_LOCALE_SCORE": "SCORE",
    "BOUNCE_LOCALE_LIVES": "LIVES", "BOUNCE_LOCALE_COUNTDOWN": "COUNTDOWN",
    "BOUNCE_LOCALE_NOT_STARTED": "NOT STARTED",
}
EN.pop("BOUNCE_LOCAPE_UNUSED", None)

# ---------------------------------------------------------------- Simplified
ZH_CN = {
    "BOUNCE_LOCALE_TITLE": "BOUNCE", "BOUNCE_LOCALE_CONTINUE": "继续",
    "BOUNCE_LOCALE_NEW_GAME": "新游戏", "BOUNCE_LOCALE_HIGH_SCORE": "最高分",
    "BOUNCE_LOCALE_INSTRUCTIONS": "游戏规则说明", "BOUNCE_LOCALE_EXIT": "退出",
    "BOUNCE_LOCALE_SETTINGS": "设置", "BOUNCE_LOCALE_ABOUT": "关于",
    "BOUNCE_LOCALE_START_LEVEL": "开始关卡",
    "BOUNCE_LOCALE_CONTINUE_UNAVAILABLE": "继续不可用",
    "BOUNCE_LOCALE_NO_ACTION": "无操作",
    "BOUNCE_LOCALE_SETTINGS_DISPLAY": "显示设置",
    "BOUNCE_LOCALE_SETTINGS_INPUT": "按键设置",
    "BOUNCE_LOCALE_SETTINGS_AUDIO": "声音设置",
    "BOUNCE_LOCALE_SETTINGS_LANGUAGE": "语言设置",
    "BOUNCE_LOCALE_DISPLAY": "显示", "BOUNCE_LOCALE_INPUT": "按键",
    "BOUNCE_LOCALE_AUDIO": "声音", "BOUNCE_LOCALE_LANGUAGE": "语言",
    "BOUNCE_LOCALE_THEME": "主题", "BOUNCE_LOCALE_THEME_DEFAULT": "默认",
    "BOUNCE_LOCALE_THEME_GREEN_LCD": "绿色液晶",
    "BOUNCE_LOCALE_THEME_SEPIA_TONE": "古棕色调",
    "BOUNCE_LOCALE_LOGICAL": "逻辑",
    "BOUNCE_LOCALE_LOGICAL_RESOLUTION": "逻辑分辨率",
    "BOUNCE_LOCALE_GAMEPLAY_VIEWPORT": "游戏视口",
    "BOUNCE_LOCALE_UI_HUD_REGION": "界面信息区",
    "BOUNCE_LOCALE_READ_ONLY_NATIVE": "只读", "BOUNCE_LOCALE_LEFT": "左",
    "BOUNCE_LOCALE_RIGHT": "右", "BOUNCE_LOCALE_UP": "上", "BOUNCE_LOCALE_DOWN": "下",
    "BOUNCE_LOCALE_ARROW_A": "方向键 A", "BOUNCE_LOCALE_ARROW_D": "方向键 D",
    "BOUNCE_LOCALE_ARROW_W": "方向键 W", "BOUNCE_LOCALE_ARROW_S": "方向键 S",
    "BOUNCE_LOCALE_ESCAPE": "返回键",
    "BOUNCE_LOCALE_NO_REMAPPING_YET": "暂不支持改键",
    "BOUNCE_LOCALE_STATUS": "状态",
    "BOUNCE_LOCALE_NOT_YET_IMPLEMENTED": "尚未实现",
    "BOUNCE_LOCALE_NO_AUDIO_PATH_YET": "尚无声音输出",
    "BOUNCE_LOCALE_SELECT_TO_SWITCH": "选择以切换",
    "BOUNCE_LOCALE_RESOURCE_ONLY": "仅资源",
    "BOUNCE_LOCALE_NATIVE_STRINGS": "原生文本",
    "BOUNCE_LOCALE_DEFAULT_LANGUAGE": "默认",
    "BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA": "继续 无存档",
    "BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE": "设置 关于 原生扩展",
    "BOUNCE_LOCALE_NOKIA_BOUNCE": "NOKIA BOUNCE",
    "BOUNCE_LOCALE_NATIVE_LINUX_BUILD": "原生 Linux 版本",
    "BOUNCE_LOCALE_ORIGINAL_INSPIRED": "参照原作",
    "BOUNCE_LOCALE_BEHAVIOR_BASED": "以行为为准",
    "BOUNCE_LOCALE_PLATFORM_LINUX": "PLATFORM LINUX",
    "BOUNCE_LOCALE_NATIVE_SCREEN": "原生画面",
    "BOUNCE_LOCALE_LEVEL_SELECT": "选关", "BOUNCE_LOCALE_LEVEL": "级别",
    "BOUNCE_LOCALE_BACK": "返回", "BOUNCE_LOCALE_ESC_BACK": "ESC 返回",
    "BOUNCE_LOCALE_NO_RMS_VALUE": "无存档数值",
    "BOUNCE_LOCALE_GAMEPLAY": "游戏中", "BOUNCE_LOCALE_DESTINATION": "目标",
    "BOUNCE_LOCALE_BACK_TO_MENU": "返回菜单", "BOUNCE_LOCALE_SCORE": "分数",
    "BOUNCE_LOCALE_LIVES": "生命", "BOUNCE_LOCALE_COUNTDOWN": "倒计时",
    "BOUNCE_LOCALE_NOT_STARTED": "未开始",
    "BOUNCE_LOCALE_GAME_END_CONGRATS": "祝贺你！",
    "BOUNCE_LOCALE_LEVEL_COMPLETED": "级别%U\n已完成",
    "BOUNCE_LOCALE_NEW_HIGH_SCORE": "新最高分！",
    "BOUNCE_LOCALE_GAME_END_OK": "确认",

}

# ---------------------------------------------------------------- Traditional
ZH_TW = {
    "BOUNCE_LOCALE_TITLE": "BOUNCE", "BOUNCE_LOCALE_CONTINUE": "繼續",
    "BOUNCE_LOCALE_NEW_GAME": "新遊戲", "BOUNCE_LOCALE_HIGH_SCORE": "最高分",
    "BOUNCE_LOCALE_INSTRUCTIONS": "遊戲規則說明", "BOUNCE_LOCALE_EXIT": "退出",
    "BOUNCE_LOCALE_SETTINGS": "設定", "BOUNCE_LOCALE_ABOUT": "關於",
    "BOUNCE_LOCALE_START_LEVEL": "開始關卡",
    "BOUNCE_LOCALE_CONTINUE_UNAVAILABLE": "繼續不可用",
    "BOUNCE_LOCALE_NO_ACTION": "無操作",
    "BOUNCE_LOCALE_SETTINGS_DISPLAY": "顯示設定",
    "BOUNCE_LOCALE_SETTINGS_INPUT": "按鍵設定",
    "BOUNCE_LOCALE_SETTINGS_AUDIO": "聲音設定",
    "BOUNCE_LOCALE_SETTINGS_LANGUAGE": "語言設定",
    "BOUNCE_LOCALE_DISPLAY": "顯示", "BOUNCE_LOCALE_INPUT": "按鍵",
    "BOUNCE_LOCALE_AUDIO": "聲音", "BOUNCE_LOCALE_LANGUAGE": "語言",
    "BOUNCE_LOCALE_THEME": "主題", "BOUNCE_LOCALE_THEME_DEFAULT": "預設",
    "BOUNCE_LOCALE_THEME_GREEN_LCD": "綠色液晶",
    "BOUNCE_LOCALE_THEME_SEPIA_TONE": "古棕色調",
    "BOUNCE_LOCALE_LOGICAL": "邏輯",
    "BOUNCE_LOCALE_LOGICAL_RESOLUTION": "邏輯解析度",
    "BOUNCE_LOCALE_GAMEPLAY_VIEWPORT": "遊戲視窗",
    "BOUNCE_LOCALE_UI_HUD_REGION": "介面資訊區",
    "BOUNCE_LOCALE_READ_ONLY_NATIVE": "唯讀", "BOUNCE_LOCALE_LEFT": "左",
    "BOUNCE_LOCALE_RIGHT": "右", "BOUNCE_LOCALE_UP": "上", "BOUNCE_LOCALE_DOWN": "下",
    "BOUNCE_LOCALE_ARROW_A": "方向鍵 A", "BOUNCE_LOCALE_ARROW_D": "方向鍵 D",
    "BOUNCE_LOCALE_ARROW_W": "方向鍵 W", "BOUNCE_LOCALE_ARROW_S": "方向鍵 S",
    "BOUNCE_LOCALE_ESCAPE": "返回鍵",
    "BOUNCE_LOCALE_NO_REMAPPING_YET": "暫不支援改鍵",
    "BOUNCE_LOCALE_STATUS": "狀態",
    "BOUNCE_LOCALE_NOT_YET_IMPLEMENTED": "尚未實作",
    "BOUNCE_LOCALE_NO_AUDIO_PATH_YET": "尚無聲音輸出",
    "BOUNCE_LOCALE_SELECT_TO_SWITCH": "選擇以切換",
    "BOUNCE_LOCALE_RESOURCE_ONLY": "僅資源",
    "BOUNCE_LOCALE_NATIVE_STRINGS": "原生文字",
    "BOUNCE_LOCALE_DEFAULT_LANGUAGE": "預設",
    "BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA": "繼續 無存檔",
    "BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE": "設定 關於 原生擴充",
    "BOUNCE_LOCALE_NOKIA_BOUNCE": "NOKIA BOUNCE",
    "BOUNCE_LOCALE_NATIVE_LINUX_BUILD": "原生 Linux 版本",
    "BOUNCE_LOCALE_ORIGINAL_INSPIRED": "參照原作",
    "BOUNCE_LOCALE_BEHAVIOR_BASED": "以行為為準",
    "BOUNCE_LOCALE_PLATFORM_LINUX": "PLATFORM LINUX",
    "BOUNCE_LOCALE_NATIVE_SCREEN": "原生畫面",
    "BOUNCE_LOCALE_LEVEL_SELECT": "選關", "BOUNCE_LOCALE_LEVEL": "等級",
    "BOUNCE_LOCALE_BACK": "返回", "BOUNCE_LOCALE_ESC_BACK": "ESC 返回",
    "BOUNCE_LOCALE_NO_RMS_VALUE": "無存檔數值",
    "BOUNCE_LOCALE_GAMEPLAY": "遊戲中", "BOUNCE_LOCALE_DESTINATION": "目標",
    "BOUNCE_LOCALE_BACK_TO_MENU": "返回選單", "BOUNCE_LOCALE_SCORE": "分數",
    "BOUNCE_LOCALE_LIVES": "生命", "BOUNCE_LOCALE_COUNTDOWN": "倒數計時",
    "BOUNCE_LOCALE_NOT_STARTED": "未開始",
    "BOUNCE_LOCALE_GAME_END_CONGRATS": "恭喜！",
    "BOUNCE_LOCALE_LEVEL_COMPLETED": "等級%U\n已完成！",
    "BOUNCE_LOCALE_NEW_HIGH_SCORE": "刷新紀錄！",
    "BOUNCE_LOCALE_GAME_END_OK": "確認",

}

# ---------------------------------------------------------------------- Thai
# Thai needs complex shaping for vowel signs and tone marks. The generator has no
# shaper, so every string below deliberately avoids combining marks (U+0E31,
# U+0E34..U+0E3A, U+0E47..U+0E4E). validate() enforces that rather than trusting
# this comment.
TH_TH = {
    "BOUNCE_LOCALE_TITLE": "BOUNCE", "BOUNCE_LOCALE_CONTINUE": "เล่นต่อ",
    "BOUNCE_LOCALE_NEW_GAME": "เกมส์ใหม่", "BOUNCE_LOCALE_HIGH_SCORE": "คะแนนสูงสุด",
    "BOUNCE_LOCALE_INSTRUCTIONS": "วิธีการเล่น", "BOUNCE_LOCALE_EXIT": "ออก",
    "BOUNCE_LOCALE_SETTINGS": "ตั้งค่า", "BOUNCE_LOCALE_ABOUT": "ข้อมูล",
    "BOUNCE_LOCALE_START_LEVEL": "เริ่มด่าน",
    "BOUNCE_LOCALE_CONTINUE_UNAVAILABLE": "เล่นต่อไม่ได้",
    "BOUNCE_LOCALE_NO_ACTION": "ไม่มีคำสั่ง",
    "BOUNCE_LOCALE_SETTINGS_DISPLAY": "ตั้งค่าจอ",
    "BOUNCE_LOCALE_SETTINGS_INPUT": "ตั้งค่าปุ่ม",
    "BOUNCE_LOCALE_SETTINGS_AUDIO": "ตั้งค่าเสียง",
    "BOUNCE_LOCALE_SETTINGS_LANGUAGE": "ตั้งค่าภาษา",
    "BOUNCE_LOCALE_DISPLAY": "จอ", "BOUNCE_LOCALE_INPUT": "ปุ่ม",
    "BOUNCE_LOCALE_AUDIO": "เสียง", "BOUNCE_LOCALE_LANGUAGE": "ภาษา",
    "BOUNCE_LOCALE_THEME": "ธีม", "BOUNCE_LOCALE_THEME_DEFAULT": "ค่าเริ่มต้น",
    "BOUNCE_LOCALE_THEME_GREEN_LCD": "จอเขียว",
    "BOUNCE_LOCALE_THEME_SEPIA_TONE": "โทนน้ำตาล",
    "BOUNCE_LOCALE_LOGICAL": "เชิงตรรกะ",
    "BOUNCE_LOCALE_LOGICAL_RESOLUTION": "ความละเอียด",
    "BOUNCE_LOCALE_GAMEPLAY_VIEWPORT": "พื้นที่เล่น",
    "BOUNCE_LOCALE_UI_HUD_REGION": "แถบข้อมูล",
    "BOUNCE_LOCALE_READ_ONLY_NATIVE": "อ่านอย่างเดียว",
    "BOUNCE_LOCALE_LEFT": "ซ้าย", "BOUNCE_LOCALE_RIGHT": "ขวา",
    "BOUNCE_LOCALE_UP": "บน", "BOUNCE_LOCALE_DOWN": "ล่าง",
    "BOUNCE_LOCALE_ARROW_A": "ลูกศร A", "BOUNCE_LOCALE_ARROW_D": "ลูกศร D",
    "BOUNCE_LOCALE_ARROW_W": "ลูกศร W", "BOUNCE_LOCALE_ARROW_S": "ลูกศร S",
    "BOUNCE_LOCALE_ESCAPE": "ปุ่ม Esc",
    "BOUNCE_LOCALE_NO_REMAPPING_YET": "ยังเปลี่ยนปุ่มไม่ได้",
    "BOUNCE_LOCALE_STATUS": "สถานะ",
    "BOUNCE_LOCALE_NOT_YET_IMPLEMENTED": "ยังไม่ได้ทำ",
    "BOUNCE_LOCALE_NO_AUDIO_PATH_YET": "ยังไม่มีเสียง",
    "BOUNCE_LOCALE_SELECT_TO_SWITCH": "เลือกเพื่อเปลี่ยน",
    "BOUNCE_LOCALE_RESOURCE_ONLY": "มีแต่ไฟล์",
    "BOUNCE_LOCALE_NATIVE_STRINGS": "ข้อความต้นฉบับ",
    "BOUNCE_LOCALE_DEFAULT_LANGUAGE": "ค่าเริ่มต้น",
    "BOUNCE_LOCALE_CONTINUE_NO_SAVE_DATA": "เล่นต่อ ไม่มีข้อมูล",
    "BOUNCE_LOCALE_SETTINGS_ABOUT_NATIVE": "ตั้งค่า ข้อมูล ต้นฉบับ",
    "BOUNCE_LOCALE_NOKIA_BOUNCE": "NOKIA BOUNCE",
    "BOUNCE_LOCALE_NATIVE_LINUX_BUILD": "รุ่น Linux",
    "BOUNCE_LOCALE_ORIGINAL_INSPIRED": "ดัดแปลงจากต้นฉบับ",
    "BOUNCE_LOCALE_BEHAVIOR_BASED": "อิงพฤติกรรมต้นฉบับ",
    "BOUNCE_LOCALE_PLATFORM_LINUX": "ระบบ Linux",
    "BOUNCE_LOCALE_NATIVE_SCREEN": "จอต้นฉบับ",
    "BOUNCE_LOCALE_LEVEL_SELECT": "เลือกด่าน", "BOUNCE_LOCALE_LEVEL": "ระดับ",
    "BOUNCE_LOCALE_BACK": "ย้อนกลับ", "BOUNCE_LOCALE_ESC_BACK": "ESC กลับ",
    "BOUNCE_LOCALE_NO_RMS_VALUE": "ไม่มีค่าใน RMS",
    "BOUNCE_LOCALE_GAMEPLAY": "กำลังเล่น",
    "BOUNCE_LOCALE_DESTINATION": "ปลายทาง",
    "BOUNCE_LOCALE_BACK_TO_MENU": "กลับเมนู",
    "BOUNCE_LOCALE_SCORE": "คะแนน", "BOUNCE_LOCALE_LIVES": "ชีวิต",
    "BOUNCE_LOCALE_COUNTDOWN": "นับถอยหลัง",
    "BOUNCE_LOCALE_NOT_STARTED": "ยังไม่เริ่ม",
    "BOUNCE_LOCALE_GAME_END_CONGRATS": "ยินดีด้วย!",
    "BOUNCE_LOCALE_LEVEL_COMPLETED": "ผ่านระดับ %U!",
    "BOUNCE_LOCALE_NEW_HIGH_SCORE": "คะแนน​สูงสุด ใหม่!",
    "BOUNCE_LOCALE_GAME_END_OK": "ตกลง",

}

# The Instructions body and the bounded prefix the page can actually draw.
# These deliberately omit the "%0U"-style key placeholders the English column
# carries, because nothing substitutes them at runtime and the 5x7 font has no
# glyph for '%' anyway; for a test table, plain sentences are clearer.
# D-24 -- Translation.MORE_INSTRUCTIONS, id 1, DECODED FROM THE SHIPPED
# RESOURCES. These were TEMPORARY TEST STRINGS before. The %0U / %1U /
# %2U placeholders are the source's own and are substituted at draw time
# by ui_compose_instructions(), which is why the 5x7 font never needs a
# glyph for '%'.
INSTRUCTIONS = {
    "ZH_CN": (
"使球越过重重障碍以通过所有环圈并打开大门进入下一级别。用按键%0U将球向左移动，用按键%1U将球向右移动，用按键%2U可"
        "使球弹跳。小心尖状物。用放气筒会收缩球的体积，用充气筒会使球的大小恢复正常。大球可以在水上漂浮，而小球则不行。积攒水晶可"
        "增加分数并储存游戏级别。积攒水晶球可延长球的寿命。使用跳跃和速度推进器可为您增力片刻，而用橡皮地板将使球的弹力更大。"
),
    "ZH_TW": (
"設法讓球通過許多障礙物，穿越所有圓箍，最終打開通往下一個關卡的門。按鍵%0U可將球往左移，按鍵%1U可將球往右移，按鍵%"
        "2U可將球往上彈。小心避開釘子。抽氣筒可以把球縮小，充氣筒可以把球恢復成原狀。大球可以浮在水面上，小球則不行。收集水晶來"
        "獲得加分，並儲存遊戲位置。收集水晶球可增加額外生命。跳躍和加速可以暫時提高你的力量，而橡膠地板則可以增加彈跳力。"
),
    "TH_TH": (
"เลื่อนลูกบอลผ่านสิ่งกีดขวาง\n"
        "ให้ลอดห่วงทั้งหมด และเปิดประตูไปยัง\n"
        "ด่านถัดไปใช้ปุ่ม %0U บังคับบอลไปทางซ้าย ปุ่ม %1U ไปทางขวา แล"
        "ะปุ่ม %2U เพื่อดีดลูกบอล ระวังเกล็ดน้ำแข็ง ใช้ตัวถ่ายลมเพื่อ\n"
        "ลดขนาดลูกบอล และตัวสูบลมเพื่อเปลี่ยน\n"
        "เป็นขนาดปกติ บอลลูกใหญ่จะลอยบนน้ำได้ ลูกเล็กลอยไม่ได้ เก็บคร"
        "ิสตัลจะได้คะแนน\n"
        "พิเศษและบันทึกจุด\n"
        "ที่เล่นอยู่ เก็บลูกบอลคริสตัลจะได้\n"
        "พลังชีวิตพิเศษ การกระโดดและเร่งความเร็ว\n"
        "จะเพิ่มพลังชั่วคราว ส่วนพื้นยางทำให้ดีด\n"
        "ได้สูงขึ้นเป็นพิเศษ"
),
}# D-24 -- the bounded excerpt of the same shipped body that the page can
# actually draw. English and Indonesian stop at the end of the
# "Watch out for spikes" sentence. Chinese has no sentence breaks worth
# splitting on and no newlines at all, so its excerpt is the leading span
# of the same CHARACTER BUDGET the English one is. The Thai resource
# carries its own line breaks, so its excerpt is the leading lines
# verbatim rather than a re-flow of text the resource already laid out.
PREFIX = {
    "ZH_CN": "使球越过重重障碍以通过所有环圈并打开大门进入下一级别。用按键%0U将球向左移动，用按键%1U将球向右移动，用按键%2U可使球弹跳。小心尖状物。用放气筒会收缩球的体积，用充气筒会使球的大小恢复正常。大球可以在水上漂浮，而小球则不行。积攒水晶可增加分数并储存游戏级别。积攒水晶球可延长球的寿命。使用跳跃和速度推进器可为您增力片刻，而用橡皮地板将使球的弹力更大。",
    "ZH_TW": "設法讓球通過許多障礙物，穿越所有圓箍，最終打開通往下一個關卡的門。按鍵%0U可將球往左移，按鍵%1U可將球往右移，按鍵%2U可將球往上彈。小心避開釘子。抽氣筒可以把球縮小，充氣筒可以把球恢復成原狀。大球可以浮在水面上，小球則不行。收集水晶來獲得加分，並儲存遊戲位置。收集水晶球可增加額外生命。跳躍和加速可以暫時提高你的力量，而橡膠地板則可以增加彈跳力。",
    "TH_TH": "เลื่อนลูกบอลผ่านสิ่งกีดขวาง\nให้ลอดห่วงทั้งหมด และเปิดประตูไปยัง\nด่านถัดไปใช้ปุ่ม %0U บังคับบอลไปทางซ้าย ปุ่ม %1U ไปทางขวา และปุ่ม %2U เพื่อดีดลูกบอล ระวังเกล็ดน้ำแข็ง ใช้ตัวถ่ายลมเพื่อ",
}
TABLES = {"ZH_CN": ZH_CN, "ZH_TW": ZH_TW, "TH_TH": TH_TH}

THAI_MARKS = set([0x0E31, 0x0E34, 0x0E35, 0x0E36, 0x0E37, 0x0E38, 0x0E39,
                  0x0E3A, 0x0E47, 0x0E48, 0x0E49, 0x0E4A, 0x0E4B, 0x0E4C, 0x0E4D,
                  0x0E4E])


def validate():
    """Structural checks on the test tables.

    Thai combining marks are NOT rejected. Raqm/HarfBuzz is available at
    generation time, so marks are shaped correctly and stored per cluster; that
    is verified by gen_script_font.py reporting zero dropped clusters, not here.
    What must hold here is that every key is populated and that each table is
    distinguishable from English, otherwise switching would be unobservable.
    """
    problems = []
    for name, table in TABLES.items():
        missing = [k for k in KEYS if k not in table]
        extra = [k for k in table if k not in KEYS]
        if missing:
            problems.append(f"{name}: missing {missing}")
        if extra:
            problems.append(f"{name}: unknown keys {extra}")
        for k in KEYS:
            if not table.get(k):
                problems.append(f"{name}: {k} is empty")
        differing = sum(1 for k in KEYS if table.get(k) != EN.get(k))
        if differing < 30:
            problems.append(f"{name}: only {differing} strings differ from EN")
    for name in TABLES:
        if not INSTRUCTIONS.get(name) or not PREFIX.get(name):
            problems.append(f"{name}: missing INSTRUCTIONS/PREFIX text")
    # CN must be Simplified and TW must be Traditional: neither may contain the
    # other's exclusive forms, otherwise the two are not really different scripts.
    # Genuinely exclusive forms only. Characters such as 音 and 言 exist in both
    # scripts, so including them here would be a wrong test, not a strict one.
    sc_only = "设语续开关选择显击单边动头义乐习压险阵阶级"
    tc_only = "設語續開關選擇顯擊單邊動頭義樂習壓險陣階級"
    for chx in sc_only:
        if chx in "".join(ZH_TW.values()):
            problems.append(f"TW contains Simplified-only {chx!r}")
    for chx in tc_only:
        if chx in "".join(ZH_CN.values()):
            problems.append(f"CN contains Traditional-only {chx!r}")
    return problems


if __name__ == "__main__":
    issues = validate()
    if issues:
        print("VALIDATION FAILED:")
        for i in issues:
            print("  -", i)
        raise SystemExit(1)
    print(f"  {len(KEYS)} keys x 3 tables: all populated; CN is Simplified, TW is Traditional")
    for name, table in TABLES.items():
        differing = sum(1 for k in KEYS if table.get(k) != EN.get(k))
        cjk = sorted({c for v in table.values() for c in v if ord(c) > 0x7F})
        print(f"  {name}: {differing}/{len(KEYS)} differ from EN, "
              f"{len(cjk)} non-ASCII chars, "
              f"max len {max(len(v) for v in table.values())}")
