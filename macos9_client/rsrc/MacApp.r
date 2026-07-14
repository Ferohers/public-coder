#include "Types.r"
#include "Dialogs.r"
#include "Processes.r"
#include "icons.r"
#include "mode_icons.r"


/* Main Window Definition */
resource 'WIND' (128) {
    { 80, 80, 530, 580 },
    documentProc,
    visible,
    goAway,
    0,
    "AltanAI",
    centerMainScreen
};

/* Folder Panel Window (floating, non-resizable) */
resource 'WIND' (129) {
    { 60, 200, 360, 440 },
    noGrowDocProc,
    visible,
    goAway,
    0,
    "Dosyalar",
    centerMainScreen
};

/* Menu Bar and Menus */
resource 'MBAR' (128) {
    { 128, 129, 130 }
};

resource 'MENU' (128) {
    128, textMenuProc;
    allEnabled, enabled;
    apple;
    {
        "About AltanAI...", noIcon, noKey, noMark, plain
    }
};

resource 'MENU' (129) {
    129, textMenuProc;
    allEnabled, enabled;
    "Dosya";
    {
        "Ayarlar...", noIcon, noKey, noMark, plain;
        "Temizle", noIcon, noKey, noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "\x82\xDDk", noIcon, "Q", noMark, plain
    }
};

resource 'MENU' (130) {
    130, textMenuProc;
    allEnabled, enabled;
    "D\x9Fzenle";
    {
        "Geri Al", noIcon, "Z", noMark, plain;
        "-", noIcon, noKey, noMark, plain;
        "Kes", noIcon, "X", noMark, plain;
        "Kopyala", noIcon, "C", noMark, plain;
        "Yap\x8D\xDFt\xDDr", noIcon, "V", noMark, plain;
        "T\x9Fm\x9Fn\x9F Se\x8D", noIcon, "A", noMark, plain;
        "Temizle", noIcon, noKey, noMark, plain
    }
};

/* Settings Dialog Definition */
resource 'DLOG' (129) {
    { 100, 100, 430, 450 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    129,
    "Settings",
    0
};

resource 'DITL' (129) {
    {
        /* 1: OK Button */
        { 290, 240, 310, 320 },
        Button { enabled, "OK" };

        /* 2: Cancel Button */
        { 290, 140, 310, 220 },
        Button { enabled, "Cancel" };

        /* 3: Host label */
        { 20, 20, 35, 110 },
        StaticText { disabled, "Server Host:" };

        /* 4: Host input */
        { 17, 120, 37, 320 },
        EditText { enabled, "tr.iz.ci" };

        /* 5: Port label */
        { 55, 20, 70, 110 },
        StaticText { disabled, "Server Port:" };

        /* 6: Port input */
        { 52, 120, 72, 200 },
        EditText { enabled, "8080" };

        /* 7: Username label */
        { 90, 20, 105, 110 },
        StaticText { disabled, "Username:" };

        /* 8: Username input */
        { 87, 120, 107, 320 },
        EditText { enabled, "altan" };

        /* 9: Password label */
        { 125, 20, 140, 110 },
        StaticText { disabled, "Password:" };

        /* 10: Password input */
        { 122, 120, 142, 320 },
        EditText { enabled, "altan123@" };

        /* 11: Font label */
        { 160, 20, 175, 110 },
        StaticText { disabled, "Chat Font:" };

        /* 12: Font input */
        { 157, 120, 177, 320 },
        EditText { enabled, "Monaco" };

        /* 13: Size label */
        { 195, 20, 210, 110 },
        StaticText { disabled, "Font Size:" };

        /* 14: Size input */
        { 192, 120, 212, 200 },
        EditText { enabled, "11" };

        /* 15: Language label */
        { 232, 20, 247, 110 },
        StaticText { disabled, "Language:" };

        /* 16: English radio button */
        { 229, 120, 247, 205 },
        RadioButton { enabled, "English" };

        /* 17: Turkce radio button — octal escapes: \237=u umlaut (0x9F), \215=c cedilla (0x8D) */
        { 229, 215, 247, 320 },
        RadioButton { enabled, "T\237rk\215e" };

        /* 18: Frame decoration for OK button */
        { 286, 236, 314, 324 },
        UserItem { enabled };
    }
};

/* About Dialog Definition */
resource 'DLOG' (130) {
    { 120, 120, 260, 440 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    130,
    "About AltanAI",
    0
};

resource 'DITL' (130) {
    {
        /* 1: OK Button */
        { 105, 230, 125, 300 },
        Button { enabled, "OK" };

        /* 2: About Text */
        { 15, 20, 90, 300 },
        StaticText { disabled, "AltanAI\rPlain text bridge client\r\rConnects to the Docker AI bridge." };
        
        /* 3: Frame decoration for OK button */
        { 101, 226, 129, 304 },
        UserItem { enabled };
    }
};

/* New Project Dialog */
resource 'DLOG' (132) {
    { 140, 140, 275, 460 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    132,
    "New Project",
    0
};

resource 'DITL' (132) {
    {
        /* 1: OK Button */
        { 100, 230, 120, 300 },
        Button { enabled, "OK" };

        /* 2: Cancel Button */
        { 100, 140, 120, 220 },
        Button { enabled, "Cancel" };

        /* 3: Name label */
        { 24, 20, 39, 110 },
        StaticText { disabled, "App Name:" };

        /* 4: Name input */
        { 21, 120, 41, 295 },
        EditText { enabled, "MacApp" };
    }
};

/* History Chooser Dialog */
resource 'DLOG' (131) {
    { 120, 120, 440, 430 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    131,
    "History",
    0
};

/* Small one-field prompt dialog used by the editor for Find and Go To Line. */
resource 'DLOG' (133) {
    { 140, 140, 245, 480 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    133,
    "Editor",
    0
};

resource 'DITL' (133) {
    {
        { 72, 250, 92, 320 },
        Button { enabled, "OK" };

        { 72, 170, 92, 240 },
        Button { enabled, "Cancel" };

        { 18, 18, 34, 315 },
        StaticText { disabled, "Value:" };

        { 40, 18, 60, 315 },
        EditText { enabled, "" };
    }
};

resource 'DITL' (131) {
    {
        /* 1: Cancel Button */
        { 282, 170, 302, 290 },
        Button { enabled, "Cancel" };

        /* 2-11: Chat buttons */
        { 18, 20, 38, 290 },
        Button { enabled, "Chat 1" };
        { 41, 20, 61, 290 },
        Button { enabled, "Chat 2" };
        { 64, 20, 84, 290 },
        Button { enabled, "Chat 3" };
        { 87, 20, 107, 290 },
        Button { enabled, "Chat 4" };
        { 110, 20, 130, 290 },
        Button { enabled, "Chat 5" };
        { 133, 20, 153, 290 },
        Button { enabled, "Chat 6" };
        { 156, 20, 176, 290 },
        Button { enabled, "Chat 7" };
        { 179, 20, 199, 290 },
        Button { enabled, "Chat 8" };
        { 202, 20, 222, 290 },
        Button { enabled, "Chat 9" };
        { 225, 20, 245, 290 },
        Button { enabled, "Chat 10" };

        /* 12: Note */
        { 254, 20, 269, 290 },
        StaticText { disabled, "Click a chat to open it." };

        /* 13: Clear History Button */
        { 282, 20, 302, 150 },
        Button { enabled, "Clear History" };
    }
};

/* Confirmation Dialog (Save changes?) */
resource 'DLOG' (140) {
    { 120, 150, 240, 490 },
    dBoxProc,
    visible,
    noGoAway,
    0,
    140,
    "Save changes?",
    0
};

resource 'DITL' (140) {
    {
        /* 1: Save Button */
        { 80, 240, 100, 320 },
        Button { enabled, "Save" };

        /* 2: Cancel Button */
        { 80, 40, 100, 120 },
        Button { enabled, "Cancel" };

        /* 3: Don't Save Button */
        { 80, 140, 100, 220 },
        Button { enabled, "Don't Save" };

        /* 4: Prompt message */
        { 15, 20, 65, 320 },
        StaticText { disabled, "Save changes before closing?" }
    }
};

/* Application SIZE resource */
resource 'SIZE' (0) {
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
#ifdef TARGET_API_MAC_CARBON
    isHighLevelEventAware,
#else
    notHighLevelEventAware,
#endif
    onlyLocalHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
#ifdef TARGET_API_MAC_CARBON
    2048 * 1024, /* Carbon needs more memory */
    2048 * 1024
#else
    1536 * 1024, /* Classic PPC memory allocation */
    1536 * 1024
#endif
};
