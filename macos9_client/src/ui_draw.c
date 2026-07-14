#include "common.h"

static const signed char kSendIconPixels[16][16] = {
    {-1,-1,-1,-1,-1,-1,-1,0,0,0,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,0,0,1,0,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,0,0,1,0,-1,-1,-1,-1,-1,-1},
    {0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0},
    {0,2,3,-1,3,-1,3,0,4,0,3,-1,3,-1,3,0},
    {0,5,-1,3,-1,3,-1,0,1,0,-1,3,-1,3,-1,0},
    {0,2,3,-1,3,-1,3,0,0,0,3,-1,3,-1,3,0},
    {0,5,-1,3,-1,3,-1,3,-1,3,-1,3,-1,3,-1,0},
    {0,2,0,0,0,0,3,-1,0,0,0,0,0,0,3,0},
    {0,5,-1,3,-1,3,-1,3,-1,3,-1,3,-1,3,-1,0},
    {0,2,0,0,0,-1,0,0,0,0,3,0,0,0,3,0},
    {0,5,-1,3,-1,3,-1,3,-1,3,-1,3,-1,3,-1,0},
    {0,2,5,2,5,2,5,2,5,2,5,2,5,2,5,0},
    {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1},
    {-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1}
};

static const RGBColor kSendIconColors[] = {
    {0x0000, 0x0000, 0x0000},
    {0x0000, 0xCCCC, 0xCCCC},
    {0x8888, 0x8888, 0x8888},
    {0xFFFF, 0xFFFF, 0x0000},
    {0x0000, 0x0000, 0xBBBB},
    {0xCCCC, 0xCCCC, 0x0000}
};


void AppCreateWindow(AppEnv* env) {
    Rect bounds = { 0, 0, 10, 10 };
    
    env->window = GetNewWindow(128, nil, (WindowPtr)-1L);
    if (!env->window) {
        ExitToShell();
    }
    
    SetPort(env->window);

    /* Load toolbar button icons - using custom resource types to prevent registry collision */
    env->clearIcon = GetAppResourceIcon(APP_CREATOR, 129, 'CLER');
    env->historyIcon = GetAppResourceIcon(APP_CREATOR, 131, 'HIST');
    env->settingsIcon = GetAppResourceIcon(APP_CREATOR, 130, 'SETT');
    env->buildIcon = GetAppResourceIcon('BLDI', 140, 'icns');
    env->planIcon = GetAppResourceIcon('PLNI', 141, 'icns');

    /* System icons for file/folder display */
    GetIconRef(kOnSystemDisk, kSystemIconsCreator, kGenericDocumentIcon, &env->docIcon);
    GetIconRef(kOnSystemDisk, kSystemIconsCreator, kGenericFolderIcon, &env->folderIcon);

    /* Allocate Controls */
    env->scrollbar = NewControl(env->window, &bounds, "\p", true, 0, 0, 0, scrollBarProc, 0L);
    {
        Str255 sendTitle;
        ConvertCtoPstr(env->lang->btnSend, sendTitle);
        env->sendBtn = NewControl(env->window, &bounds, sendTitle, true, 0, 0, 0, kControlBevelButtonNormalBevelProc, 0L);
    }

    env->modeBtn = NewControl(env->window, &bounds, "\pPlan", true, 0, 0, 0,
        kControlBevelButtonNormalBevelProc, 0L);
    AppUpdateModeButton(env);

    env->clearBtn = NewControl(env->window, &bounds, "\p", true, 0, 0, 0, kControlBevelButtonNormalBevelProc, 0L);
    if (env->clearBtn && env->clearIcon) {
        ControlButtonContentInfo content;
        content.contentType = kControlContentIconRef;
        content.u.iconRef = env->clearIcon;
        SetControlData(env->clearBtn, kControlEntireControl, kControlBevelButtonContentTag, sizeof(content), &content);
    }

    env->historyBtn = NewControl(env->window, &bounds, "\p", true, 0, 0, 0, kControlBevelButtonNormalBevelProc, 0L);
    if (env->historyBtn && env->historyIcon) {
        ControlButtonContentInfo content;
        content.contentType = kControlContentIconRef;
        content.u.iconRef = env->historyIcon;
        SetControlData(env->historyBtn, kControlEntireControl, kControlBevelButtonContentTag, sizeof(content), &content);
    }

    env->settingsBtn = NewControl(env->window, &bounds, "\p", true, 0, 0, 0, kControlBevelButtonNormalBevelProc, 0L);
    if (env->settingsBtn && env->settingsIcon) {
        ControlButtonContentInfo content;
        content.contentType = kControlContentIconRef;
        content.u.iconRef = env->settingsIcon;
        SetControlData(env->settingsBtn, kControlEntireControl, kControlBevelButtonContentTag, sizeof(content), &content);
    }

    env->buildActionBtn = NewControl(env->window, &bounds, "\p", true, 0, 0, 0, kControlBevelButtonNormalBevelProc, 0L);
    if (env->buildActionBtn && env->buildIcon) {
        ControlButtonContentInfo content;
        content.contentType = kControlContentIconRef;
        content.u.iconRef = env->buildIcon;
        SetControlData(env->buildActionBtn, kControlEntireControl, kControlBevelButtonContentTag, sizeof(content), &content);
    }
    if (env->buildActionBtn) HideControl(env->buildActionBtn);

    env->applyPendingBtn = NewControl(env->window, &bounds, "\p", true, 0, 0, 0,
        kControlBevelButtonNormalBevelProc, 0L);
    env->rejectPendingBtn = NewControl(env->window, &bounds, "\p", true, 0, 0, 0,
        kControlBevelButtonNormalBevelProc, 0L);
    if (env->applyPendingBtn) HideControl(env->applyPendingBtn);
    if (env->rejectPendingBtn) HideControl(env->rejectPendingBtn);

    /* Allocate TextEdit with configured chat font */
    AppUseConfiguredFont(env);
    env->transcriptTe = TEStyleNew(&bounds, &bounds);
    if (env->transcriptTe) {
        TEAutoView(true, env->transcriptTe);
    }

    AppUseConfiguredFont(env);
    env->inputTe = TENew(&bounds, &bounds);
    if (env->inputTe) {
        TEAutoView(true, env->inputTe);
    }

    env->activeTe = env->inputTe;
    if (env->inputTe) {
        TEActivate(env->inputTe);
    }

    /* Initial Geometry Layout */
    Rect portRect;
    GetWindowPortBounds(env->window, &portRect);
    AppResizeWidgets(env, portRect.right - portRect.left, portRect.bottom - portRect.top);

    AppCreateProjectWindows(env);
    AppCreateEditorWindow(env);
    SelectWindow(env->window);
    SetPort(env->window);
    if (env->inputTe) {
        env->activeTe = env->inputTe;
        TEActivate(env->inputTe);
    }
}

void AppCreateProjectWindows(AppEnv* env) {
    Rect bounds;
    Str255 title;

    SetRect(&bounds, 110, 560, 110 + UI_FILES_WIN_HEIGHT, 560 + UI_FILES_WIN_WIDTH);
    ConvertCtoPstr(env->lang->filesWindowTitle, title);
    env->filesWindow = NewCWindow(nil, &bounds, title, true, documentProc, (WindowPtr)-1L, true, 0L);
    if (env->filesWindow) {
        Str255 chooseTitle;
        Str255 cancelTitle;
        SetPort(env->filesWindow);
        SetRect(&bounds, 210, 6, 296, 28);
        ConvertCtoPstr(env->lang->btnChooseFolder, chooseTitle);
        ConvertCtoPstr(env->lang->btnCancel, cancelTitle);
        env->chooseFolderBtn = NewControl(env->filesWindow, &bounds, chooseTitle, true, 0, 0, 0,
            kControlBevelButtonNormalBevelProc, 0L);
        SetRect(&bounds, 304, 6, 374, 28);
        env->cancelFolderBtn = NewControl(env->filesWindow, &bounds, cancelTitle, true, 0, 0, 0,
            kControlBevelButtonNormalBevelProc, 0L);
        SetRect(&bounds, 0, 0, UI_SCROLL_WIDTH, 80);
        env->filesScrollbar = NewControl(env->filesWindow, &bounds, "\p", true, 0, 0, 0,
            scrollBarProc, 0L);
        AppResizeFilesWidgets(env);
        ShowWindow(env->filesWindow);
    }
}

void AppCreateEditorWindow(AppEnv* env) {
    Rect bounds;
    Str255 title;

    SetRect(&bounds, 80, 620, 80 + UI_EDITOR_WIN_HEIGHT, 620 + UI_EDITOR_WIN_WIDTH);
    ConvertCtoPstr("Editor", title);
    env->editorWindow = NewCWindow(nil, &bounds, title, true, documentProc, (WindowPtr)-1L, true, 0L);
    if (!env->editorWindow) return;

    SetPort(env->editorWindow);
    SetRect(&bounds, 8, 8, 76, 30);
    env->editorSaveBtn = NewControl(env->editorWindow, &bounds, "\p", true, 0, 0, 0,
        kControlBevelButtonNormalBevelProc, 0L);
    SetRect(&bounds, 84, 8, 152, 30);
    env->editorSearchBtn = NewControl(env->editorWindow, &bounds, "\p", true, 0, 0, 0,
        kControlBevelButtonNormalBevelProc, 0L);
    SetRect(&bounds, 160, 8, 228, 30);
    env->editorGoLineBtn = NewControl(env->editorWindow, &bounds, "\p", true, 0, 0, 0,
        kControlBevelButtonNormalBevelProc, 0L);
    SetRect(&bounds, 236, 8, 304, 30);
    env->editorNextBtn = NewControl(env->editorWindow, &bounds, "\p", true, 0, 0, 0,
        kControlBevelButtonNormalBevelProc, 0L);
    SetRect(&bounds, 0, 0, UI_SCROLL_WIDTH, 80);
    env->editorScrollbar = NewControl(env->editorWindow, &bounds, "\p", true, 0, 0, 0,
        scrollBarProc, 0L);

    AppUseConfiguredFont(env);
    env->editorTe = TENew(&bounds, &bounds);
    if (env->editorTe) {
        TEAutoView(true, env->editorTe);
    }
    env->activeEditorTab = -1;
    env->lastSearchNeedle[0] = '\0';
    AppResizeEditorWidgets(env);
    HideWindow(env->editorWindow);
}

void AppResizeWidgets(AppEnv* env, short w, short h) {
    short contentRight;
    short lowerRight;
    short footerTop;
    short inputTop;
    short transcriptBottom;
    short sendTop;
    short inputRight;
    short topButtonTop;
    short toolLeft;
    int currentOffset;
    Boolean hasPending;
    short pendingTop;
    Rect scrollRect;
    Rect transBorder;
    Rect transRect;
    Rect inputBorder;
    Rect inputRect;

    if (!env->transcriptTe || !env->inputTe || !env->scrollbar) return;
    if (!env->sendBtn || !env->clearBtn || !env->historyBtn || !env->settingsBtn ||
            !env->buildActionBtn || !env->applyPendingBtn || !env->rejectPendingBtn) return;

    contentRight = w;
    lowerRight = w - UI_MARGIN;
    footerTop = h - UI_FOOTER_HEIGHT;
    inputTop = footerTop - UI_INPUT_HEIGHT - UI_GAP;
    hasPending = (env->pendingAIChangeText && env->pendingAIChangeText[0]);
    if (hasPending) inputTop -= UI_PENDING_STRIP_HEIGHT;
    transcriptBottom = inputTop - UI_GAP;
    if (hasPending) transcriptBottom -= 4;
    sendTop = inputTop;
    inputRight = lowerRight - UI_BTN_WIDTH - (UI_GAP * 2);
    topButtonTop = 4;

    if (transcriptBottom < UI_MARGIN + 80) transcriptBottom = UI_MARGIN + 80;
    if (inputRight < UI_MARGIN + 160) inputRight = contentRight;

    scrollRect.right = contentRight;
    scrollRect.left = scrollRect.right - UI_SCROLL_WIDTH;
    scrollRect.top = 32;
    scrollRect.bottom = transcriptBottom;

    MoveControl(env->scrollbar, scrollRect.left, scrollRect.top);
    SizeControl(env->scrollbar, scrollRect.right - scrollRect.left, scrollRect.bottom - scrollRect.top);

    transBorder.top = 32;
    transBorder.left = UI_MARGIN;
    transBorder.bottom = transcriptBottom;
    transBorder.right = scrollRect.left;

    transRect = transBorder;
    InsetRect(&transRect, UI_TEXT_INSET, UI_TEXT_INSET);

    currentOffset = (**env->transcriptTe).viewRect.top - (**env->transcriptTe).destRect.top;
    (**env->transcriptTe).viewRect = transRect;
    (**env->transcriptTe).destRect = transRect;
    OffsetRect(&(**env->transcriptTe).destRect, 0, -currentOffset);
    AppSyncTranscriptMetrics(env);

    inputBorder.top = inputTop;
    inputBorder.left = UI_MARGIN;
    inputBorder.bottom = inputTop + UI_INPUT_HEIGHT;
    inputBorder.right = inputRight;

    inputRect = inputBorder;
    InsetRect(&inputRect, UI_TEXT_INSET, UI_TEXT_INSET);

    (**env->inputTe).viewRect = inputRect;
    (**env->inputTe).destRect = inputRect;
    TECalText(env->inputTe);

    MoveControl(env->sendBtn, lowerRight - UI_BTN_WIDTH, sendTop);
    SizeControl(env->sendBtn, UI_BTN_WIDTH, UI_BTN_HEIGHT);

    if (env->modeBtn) {
        MoveControl(env->modeBtn, lowerRight - UI_MODE_WIDTH, sendTop + UI_BTN_HEIGHT);
        SizeControl(env->modeBtn, UI_MODE_WIDTH, UI_MODE_HEIGHT);
    }

    toolLeft = scrollRect.left - ((UI_TOOL_BTN_SIZE + 6) * 3) + 6;
    MoveControl(env->clearBtn, toolLeft, topButtonTop);
    SizeControl(env->clearBtn, UI_TOOL_BTN_SIZE, UI_TOOL_BTN_SIZE);

    MoveControl(env->historyBtn, toolLeft + UI_TOOL_BTN_SIZE + 6, topButtonTop);
    SizeControl(env->historyBtn, UI_TOOL_BTN_SIZE, UI_TOOL_BTN_SIZE);

    MoveControl(env->settingsBtn, toolLeft + ((UI_TOOL_BTN_SIZE + 6) * 2), topButtonTop);
    SizeControl(env->settingsBtn, UI_TOOL_BTN_SIZE, UI_TOOL_BTN_SIZE);

    MoveControl(env->buildActionBtn, scrollRect.right + 100, topButtonTop);
    SizeControl(env->buildActionBtn, UI_TOOL_BTN_SIZE, UI_TOOL_BTN_SIZE);

    pendingTop = inputBorder.top - UI_PENDING_STRIP_HEIGHT + 5;
    MoveControl(env->applyPendingBtn,
        inputBorder.right - (UI_TOOL_BTN_SIZE * 2) - 10,
        pendingTop);
    SizeControl(env->applyPendingBtn, UI_TOOL_BTN_SIZE, UI_TOOL_BTN_SIZE);

    MoveControl(env->rejectPendingBtn,
        inputBorder.right - UI_TOOL_BTN_SIZE - 6,
        pendingTop);
    SizeControl(env->rejectPendingBtn, UI_TOOL_BTN_SIZE, UI_TOOL_BTN_SIZE);

    AdjustScrollbar();
}

void AppResizeEditorWidgets(AppEnv* env) {
    Rect portRect;
    Rect editRect;
    Rect scrollRect;
    int currentOffset;

    if (!env || !env->editorWindow || !env->editorTe) return;

    SetPort(env->editorWindow);
    GetWindowPortBounds(env->editorWindow, &portRect);

    {
        short curX = 8;
        short btnH = 22;
        short btnW;
        Str255 pstr;

        if (env->editorSaveBtn) {
            ConvertCtoPstr(env->lang->editorSave, pstr);
            TextFont(kFontIDGeneva);
            TextSize(10);
            TextFace(bold);
            btnW = StringWidth(pstr) + 26 + 12;
            if (btnW < 68) btnW = 68;
            MoveControl(env->editorSaveBtn, curX, 8);
            SizeControl(env->editorSaveBtn, btnW, btnH);
            curX += btnW + 8;
        }
        if (env->editorSearchBtn) {
            ConvertCtoPstr(env->lang->editorFind, pstr);
            TextFont(kFontIDGeneva);
            TextSize(10);
            TextFace(bold);
            btnW = StringWidth(pstr) + 26 + 12;
            if (btnW < 68) btnW = 68;
            MoveControl(env->editorSearchBtn, curX, 8);
            SizeControl(env->editorSearchBtn, btnW, btnH);
            curX += btnW + 8;
        }
        if (env->editorGoLineBtn) {
            ConvertCtoPstr(env->lang->editorLine, pstr);
            TextFont(kFontIDGeneva);
            TextSize(10);
            TextFace(bold);
            btnW = StringWidth(pstr) + 26 + 12;
            if (btnW < 68) btnW = 68;
            MoveControl(env->editorGoLineBtn, curX, 8);
            SizeControl(env->editorGoLineBtn, btnW, btnH);
            curX += btnW + 8;
        }
        if (env->editorNextBtn) {
            ConvertCtoPstr(env->lang->editorNext, pstr);
            TextFont(kFontIDGeneva);
            TextSize(10);
            TextFace(bold);
            btnW = StringWidth(pstr) + 26 + 12;
            if (btnW < 68) btnW = 68;
            MoveControl(env->editorNextBtn, curX, 8);
            SizeControl(env->editorNextBtn, btnW, btnH);
            curX += btnW + 8;
        }
    }

    SetRect(&scrollRect,
        portRect.right - UI_SCROLL_WIDTH - 4,
        UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT + 8,
        portRect.right - 4,
        portRect.bottom - 16);
    if (scrollRect.bottom < scrollRect.top + 80) scrollRect.bottom = scrollRect.top + 80;
    if (env->editorScrollbar) {
        MoveControl(env->editorScrollbar, scrollRect.left, scrollRect.top);
        SizeControl(env->editorScrollbar, scrollRect.right - scrollRect.left,
            scrollRect.bottom - scrollRect.top);
    }

    SetRect(&editRect, UI_EDITOR_GUTTER_WIDTH + 9,
        UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT + 8,
        scrollRect.left - 4,
        portRect.bottom - 16);
    if (editRect.right < editRect.left + 80) editRect.right = editRect.left + 80;
    if (editRect.bottom < editRect.top + 80) editRect.bottom = editRect.top + 80;

    currentOffset = (**env->editorTe).viewRect.top - (**env->editorTe).destRect.top;
    if (currentOffset < 0) currentOffset = 0;
    (**env->editorTe).viewRect = editRect;
    (**env->editorTe).destRect = editRect;
    (**env->editorTe).destRect.left += 4;
    (**env->editorTe).destRect.right -= 4;
    OffsetRect(&(**env->editorTe).destRect, 0, -currentOffset);
    AppSyncEditorMetrics(env);
    AppAdjustEditorScrollbar(env);
    AppClampEditorTabScroll(env);
}

void AppDrawDecorations(AppEnv* env) {
    Rect portRect;
    Rect transcriptBorder;
    Rect inputBorder;
    Rect headerRect;
    short statusX;
    short statusWidth;
    Str255 statusP;
    ThemeDrawState growState;

    if (!env->window || !env->inputTe || !env->transcriptTe) return;
    
    GetWindowPortBounds(env->window, &portRect);
    growState = IsWindowHilited(env->window) ? kThemeStateActive : kThemeStateInactive;



    transcriptBorder = (**env->transcriptTe).viewRect;
    InsetRect(&transcriptBorder, -UI_TEXT_INSET, -UI_TEXT_INSET);
    DrawThemeEditTextFrame(&transcriptBorder, growState);

    inputBorder = (**env->inputTe).viewRect;
    InsetRect(&inputBorder, -UI_TEXT_INSET, -UI_TEXT_INSET);
    DrawThemeEditTextFrame(&inputBorder, growState);

    Point origin;
    origin.h = portRect.right - 15;
    origin.v = portRect.bottom - 15;
    DrawThemeStandaloneGrowBox(origin, kThemeGrowRight | kThemeGrowDown, false, growState);

    /* Use Geneva 10 bold for the status placard text */
    TextFont(kFontIDGeneva);
    TextSize(10);
    TextFace(bold);
    ConvertCtoPstr(env->status, statusP);
    statusWidth = StringWidth(statusP);
    short toolLeft = (portRect.right - UI_SCROLL_WIDTH) - ((UI_TOOL_BTN_SIZE + 6) * 3);
    statusX = (portRect.right - statusWidth) / 2;
    if (statusX + statusWidth > toolLeft - 10) {
        statusX = toolLeft - 10 - statusWidth;
    }
    if (statusX < UI_MARGIN) statusX = UI_MARGIN;
    MoveTo(statusX, 21);
    DrawString(statusP);
    TextFace(0);

    AppDrawSendIcon(env);
    AppDrawPendingStrip(env);
    AppDrawPendingActionIcons(env);
    AppDrawModeLabel(env);
}

void AppDrawSendIcon(AppEnv* env) {
    Rect btnRect;
    Rect pixelRect;
    RGBColor oldFore;
    short x;
    short y;
    short iconLeft;
    short iconTop;
    signed char colorIndex;

    if (!env->sendBtn) return;

    btnRect = (**env->sendBtn).contrlRect;
    iconLeft = btnRect.left + 10;
    iconTop = btnRect.top + ((btnRect.bottom - btnRect.top - 16) / 2);

    GetForeColor(&oldFore);
    for (y = 0; y < 16; y++) {
        for (x = 0; x < 16; x++) {
            colorIndex = kSendIconPixels[y][x];
            if (colorIndex < 0) continue;
            RGBForeColor(&kSendIconColors[colorIndex]);
            SetRect(&pixelRect, iconLeft + x, iconTop + y, iconLeft + x + 1, iconTop + y + 1);
            PaintRect(&pixelRect);
        }
    }
    RGBForeColor(&oldFore);
}

void AppRedrawSendButton(AppEnv* env) {
    GrafPtr oldPort;

    if (!env || !env->window || !env->sendBtn) return;
    GetPort(&oldPort);
    SetPort(env->window);
    Draw1Control(env->sendBtn);
    AppDrawSendIcon(env);
    SetPort(oldPort);
}

void AppRedrawModeButton(AppEnv* env) {
    GrafPtr oldPort;

    if (!env || !env->window || !env->modeBtn) return;
    GetPort(&oldPort);
    SetPort(env->window);
    Draw1Control(env->modeBtn);
    AppDrawModeLabel(env);
    SetPort(oldPort);
}

void AppDrawPendingStrip(AppEnv* env) {
    Rect inputRect;
    Rect stripRect;
    Str255 pstr;
    char line[140];
    char label[170];
    char* nl;
    short maxRight;

    if (!env || !env->window || !env->pendingAIChangeText ||
            !env->pendingAIChangeText[0] || !env->inputTe) return;

    inputRect = (**env->inputTe).viewRect;
    stripRect.left = inputRect.left - UI_TEXT_INSET;
    stripRect.right = inputRect.right + UI_TEXT_INSET;
    stripRect.top = inputRect.top - UI_TEXT_INSET - UI_PENDING_STRIP_HEIGHT;
    stripRect.bottom = inputRect.top - UI_TEXT_INSET - 2;
    if (stripRect.top < 32) return;

    EraseRect(&stripRect);
    
    /* Draw 3D edit text frame to align perfectly down to the pixel with the fields above/below it */
    DrawThemeEditTextFrame(&stripRect, IsWindowHilited(env->window) ? kThemeStateActive : kThemeStateInactive);

    /* Use Geneva 10 regular for clean legibility */
    TextFont(kFontIDGeneva);
    TextSize(10);
    TextFace(0);

    strncpy(line, env->pendingAIChangeSummary, sizeof(line) - 1);
    line[sizeof(line) - 1] = '\0';
    nl = strchr(line, '\n');
    if (nl) *nl = '\0';
    if (!line[0]) {
        if (env->settings.language == 1) strcpy(line, "Bekleyen AI de\xDBi\xDFiklikleri");
        else strcpy(line, "Pending AI changes");
    }
    if (strncmp(line, "Preview: ", 9) == 0) memmove(line, line + 9, strlen(line + 9) + 1);
    if (strncmp(line, "\x85nizleme: ", 10) == 0) memmove(line, line + 10, strlen(line + 10) + 1);

    if (env->settings.language == 1) {
        sprintf(label, "Bekleyen: %s", line);
    } else {
        sprintf(label, "Pending: %s", line);
    }
    maxRight = stripRect.right - ((UI_TOOL_BTN_SIZE * 2) + 18);
    while (strlen(label) > 12) {
        ConvertCtoPstr(label, pstr);
        if (stripRect.left + 10 + StringWidth(pstr) <= maxRight) break;
        label[strlen(label) - 1] = '\0';
    }
    ConvertCtoPstr(label, pstr);
    MoveTo(stripRect.left + 10, stripRect.top + 18);
    DrawString(pstr);
}

void AppDrawPendingActionIcons(AppEnv* env) {
    if (!env || !env->pendingAIChangeText) return;
    AppDrawPendingActionIcon(env->applyPendingBtn, true);
    AppDrawPendingActionIcon(env->rejectPendingBtn, false);
}

void AppUpdatePendingControls(AppEnv* env) {
    GrafPtr oldPort;
    Boolean hasPending;
    Rect portRect;

    if (!env || !env->window || !env->applyPendingBtn || !env->rejectPendingBtn) return;
    hasPending = (env->pendingAIChangeText && env->pendingAIChangeText[0]);
    GetPort(&oldPort);
    SetPort(env->window);
    GetWindowPortBounds(env->window, &portRect);
    AppResizeWidgets(env, portRect.right - portRect.left, portRect.bottom - portRect.top);
    ScrollToBottom();
    if (hasPending) {
        ShowControl(env->applyPendingBtn);
        ShowControl(env->rejectPendingBtn);
        Draw1Control(env->applyPendingBtn);
        Draw1Control(env->rejectPendingBtn);
        AppDrawPendingStrip(env);
        AppDrawPendingActionIcons(env);
    } else {
        HideControl(env->applyPendingBtn);
        HideControl(env->rejectPendingBtn);
    }
    InvalRect(&portRect);
    SetPort(oldPort);
}

void AppDrawModeLabel(AppEnv* env) {
    /* Drawn natively by Control Manager */
}

void AppUpdateModeButton(AppEnv* env) {
    Str255 title;
    IconRef iconRef;

    if (!env || !env->modeBtn) return;

    ConvertCtoPstr(env->settings.workMode ? env->lang->modeBuild : env->lang->modePlan, title);
    SetControlTitle(env->modeBtn, title);

    iconRef = env->settings.workMode ? env->buildIcon : env->planIcon;
    if (iconRef) {
        AppSetIconTextButtonContent(env->modeBtn, iconRef);
    }

    Draw1Control(env->modeBtn);
}

void AppSetIconTextButtonContent(ControlHandle button, IconRef iconRef) {
    ControlButtonContentInfo content;
    ControlButtonGraphicAlignment align;
    ControlButtonTextPlacement placement;
    Point graphicOffset;

    if (!button || !iconRef) return;

    content.contentType = kControlContentIconRef;
    content.u.iconRef = iconRef;
    SetControlData(button, kControlEntireControl, kControlBevelButtonContentTag,
        sizeof(content), &content);

    align = kControlBevelButtonAlignLeft;
    SetControlData(button, kControlEntireControl, kControlBevelButtonGraphicAlignTag,
        sizeof(align), &align);

    graphicOffset.h = 6;
    graphicOffset.v = 0;
    SetControlData(button, kControlEntireControl, kControlBevelButtonGraphicOffsetTag,
        sizeof(graphicOffset), &graphicOffset);

    placement = kControlBevelButtonPlaceToRightOfGraphic;
    SetControlData(button, kControlEntireControl, kControlBevelButtonTextPlaceTag,
        sizeof(placement), &placement);

    {
        Boolean scaleIcon = true;
        SetControlData(button, kControlEntireControl, kControlBevelButtonScaleIconTag,
            sizeof(scaleIcon), &scaleIcon);
    }
}

void AppDrawProjectWindow(AppEnv* env, WindowPtr window) {
    if (window == env->filesWindow) {
        AppDrawFilesWindow(env);
    }
}

void AppResizeFilesWidgets(AppEnv* env) {
    Rect portRect;
    Rect buttonRect;
    Rect listRect;
    short visibleRows;
    short maxScroll;

    if (!env || !env->filesWindow) return;
    SetPort(env->filesWindow);
    GetWindowPortBounds(env->filesWindow, &portRect);

    if (env->cancelFolderBtn) {
        SetRect(&buttonRect, portRect.right - 88, 6, portRect.right - 18, 28);
        MoveControl(env->cancelFolderBtn, buttonRect.left, buttonRect.top);
        SizeControl(env->cancelFolderBtn, buttonRect.right - buttonRect.left,
            buttonRect.bottom - buttonRect.top);
    }
    if (env->chooseFolderBtn) {
        SetRect(&buttonRect, portRect.right - 182, 6, portRect.right - 96, 28);
        MoveControl(env->chooseFolderBtn, buttonRect.left, buttonRect.top);
        SizeControl(env->chooseFolderBtn, buttonRect.right - buttonRect.left,
            buttonRect.bottom - buttonRect.top);
    }

    /* List rect: edge-to-edge, below header, above grow box area */
    SetRect(&listRect, 0, 34, portRect.right - UI_SCROLL_WIDTH, portRect.bottom - UI_SCROLL_WIDTH);
    if (listRect.bottom < listRect.top + 40) listRect.bottom = listRect.top + 40;

    AppClampFilesScroll(env);
    visibleRows = AppVisibleFileRows(env);
    maxScroll = env->projectItemCount - visibleRows;
    if (maxScroll < 0) maxScroll = 0;
    if (env->filesScrollbar) {
        /* Scrollbar attached to window right edge */
        MoveControl(env->filesScrollbar, portRect.right - UI_SCROLL_WIDTH, listRect.top);
        SizeControl(env->filesScrollbar, UI_SCROLL_WIDTH, listRect.bottom - listRect.top);
        SetControlMinimum(env->filesScrollbar, 0);
        SetControlMaximum(env->filesScrollbar, maxScroll);
        SetControlValue(env->filesScrollbar, env->filesScrollOffset);
    }
}

void AppDrawFilesWindow(AppEnv* env) {
    Rect portRect;
    Rect headerRect;
    Str255 pstr;
    RGBColor oldColor;
    RGBColor gray = { 0xDDDD, 0xDDDD, 0xDDDD };
    Point origin;
    ThemeDrawState growState;

    if (!env->filesWindow) return;

    SetPort(env->filesWindow);
    GetWindowPortBounds(env->filesWindow, &portRect);
    AppClampFilesScroll(env);

    SetRect(&headerRect, 0, 0, portRect.right, 34);

    /* Clip around Choose/Cancel buttons to prevent flicker.
       Classic Mac Toolbox best practice: use region clipping so
       EraseRect+PaintRect never touches control pixels. */
    {
        RgnHandle saveClip = NewRgn();
        RgnHandle clipRgn = NewRgn();
        RgnHandle btnRgn = NewRgn();
        Rect btnRect;

        GetClip(saveClip);
        RectRgn(clipRgn, &headerRect);

        if (env->chooseFolderBtn) {
            btnRect = (**env->chooseFolderBtn).contrlRect;
            RectRgn(btnRgn, &btnRect);
            DiffRgn(clipRgn, btnRgn, clipRgn);
        }
        if (env->cancelFolderBtn) {
            btnRect = (**env->cancelFolderBtn).contrlRect;
            RectRgn(btnRgn, &btnRect);
            DiffRgn(clipRgn, btnRgn, clipRgn);
        }

        SetClip(clipRgn);
        EraseRect(&headerRect);
        GetForeColor(&oldColor);
        RGBForeColor(&gray);
        PaintRect(&headerRect);
        RGBForeColor(&oldColor);
        FrameRect(&headerRect);

        SetClip(saveClip);
        DisposeRgn(saveClip);
        DisposeRgn(clipRgn);
        DisposeRgn(btnRgn);
    }

    TextFont(kFontIDGeneva);
    TextSize(11);
    TextFace(bold);
    ConvertCtoPstr(env->lang->filesHeader, pstr);
    MoveTo(8, 22);
    DrawString(pstr);
    TextFace(0);

    AppDrawFilesList(env);

    DrawControls(env->filesWindow);
    origin.h = portRect.right - 15;
    origin.v = portRect.bottom - 15;
    growState = IsWindowHilited(env->filesWindow) ? kThemeStateActive : kThemeStateInactive;
    DrawThemeStandaloneGrowBox(origin, kThemeGrowRight | kThemeGrowDown, false, growState);
}

void AppDrawFilesList(AppEnv* env) {
    Rect portRect;
    Rect listRect;
    Rect rowRect;
    Rect iconRect;
    Str255 pstr;
    char name[64];
    char sizeText[32];
    short y;
    short i;
    short visibleRows;
    short drawnRows;
    short sizeX;
    short sizeWidth;
    short fileCount;
    short rowHeight = 20;
    RGBColor oldColor;
    RGBColor altTint = { 0xEEEE, 0xEEEE, 0xFFFF };  /* subtle blue-white tint for alternating rows */
    RGBColor sepColor = { 0xCCCC, 0xCCCC, 0xCCCC };
    RGBColor sizeColor = { 0x6666, 0x6666, 0x6666 };

    if (!env->filesWindow) return;
    SetPort(env->filesWindow);
    GetWindowPortBounds(env->filesWindow, &portRect);
    AppClampFilesScroll(env);

    /* List area: edge-to-edge below header, above grow box */
    SetRect(&listRect, 0, 34, portRect.right - UI_SCROLL_WIDTH, portRect.bottom - UI_SCROLL_WIDTH);
    if (listRect.bottom < listRect.top + 40) listRect.bottom = listRect.top + 40;

    EraseRect(&listRect);

    /* Use Geneva 10 — the standard Mac OS 9 Finder list font */
    TextFont(kFontIDGeneva);
    TextSize(10);
    TextFace(0);
    GetForeColor(&oldColor);

    y = listRect.top;
    visibleRows = AppVisibleFileRows(env);
    drawnRows = 0;
    fileCount = 0;

    /* Size column x position — right-aligned */
    sizeX = listRect.right - 70;
    if (sizeX < listRect.left + 100) sizeX = listRect.left + 100;

    for (i = env->filesScrollOffset; i < env->projectItemCount && drawnRows < visibleRows; i++) {
        SetRect(&rowRect, listRect.left, y, listRect.right, y + rowHeight);

        /* Alternating row background */
        if ((drawnRows & 1) != 0) {
            RGBForeColor(&altTint);
            PaintRect(&rowRect);
            RGBForeColor(&oldColor);
        }

        /* 16x16 system icon (file or folder) */
        SetRect(&iconRect, listRect.left + 4, y + 2, listRect.left + 20, y + 18);
        if (env->projectItems[i].isFolder && env->folderIcon) {
            PlotIconRef(&iconRect, kAlignNone, kTransformNone,
                kIconServicesNormalUsageFlag, env->folderIcon);
        } else if (env->docIcon) {
            PlotIconRef(&iconRect, kAlignNone, kTransformNone,
                kIconServicesNormalUsageFlag, env->docIcon);
        }

        /* File name */
        if (env->projectItems[i].isFolder) {
            TextFace(bold);
        }
        strncpy(name, env->projectItems[i].name, sizeof(name) - 1);
        name[sizeof(name) - 1] = '\0';
        while (strlen(name) > 3) {
            ConvertCtoPstr(name, pstr);
            if (StringWidth(pstr) <= sizeX - listRect.left - 30) break;
            name[strlen(name) - 1] = '\0';
        }
        ConvertCtoPstr(name, pstr);
        MoveTo(listRect.left + 24, y + 14);
        DrawString(pstr);
        TextFace(0);

        /* Size text — right-aligned, gray */
        if (env->projectItems[i].isFolder) {
            strcpy(sizeText, "folder");
        } else {
            sprintf(sizeText, "%ld bytes", env->projectItems[i].size);
        }
        ConvertCtoPstr(sizeText, pstr);
        sizeWidth = StringWidth(pstr);
        RGBForeColor(&sizeColor);
        MoveTo(listRect.right - sizeWidth - 6, y + 14);
        DrawString(pstr);
        RGBForeColor(&oldColor);

        /* Row separator line */
        RGBForeColor(&sepColor);
        MoveTo(listRect.left, y + rowHeight - 1);
        LineTo(listRect.right - 1, y + rowHeight - 1);
        RGBForeColor(&oldColor);

        y += rowHeight;
        fileCount++;
        drawnRows++;
    }

    /* Empty state messages */
    TextFont(kFontIDGeneva);
    TextSize(10);
    if (!env->hasRootFolder) {
        ConvertCtoPstr(env->lang->filesChoose, pstr);
        MoveTo(listRect.left + 24, listRect.top + 20);
        DrawString(pstr);
    } else if (fileCount == 0) {
        ConvertCtoPstr(env->lang->filesNone, pstr);
        MoveTo(listRect.left + 24, listRect.top + 20);
        DrawString(pstr);
    }

}

short AppVisibleFileRows(AppEnv* env) {
    Rect portRect;
    short listTop;
    short listBottom;
    short rows;

    if (!env || !env->filesWindow) return 0;
    GetWindowPortBounds(env->filesWindow, &portRect);
    listTop = 34;
    listBottom = portRect.bottom - UI_SCROLL_WIDTH;
    rows = (listBottom - listTop) / 20;
    if (rows < 0) rows = 0;
    return rows;
}

void AppClampFilesScroll(AppEnv* env) {
    short visible;
    short maxOffset;

    if (!env) return;
    visible = AppVisibleFileRows(env);
    maxOffset = env->projectItemCount - visible;
    if (maxOffset < 0) maxOffset = 0;
    if (env->filesScrollOffset < 0) env->filesScrollOffset = 0;
    if (env->filesScrollOffset > maxOffset) env->filesScrollOffset = maxOffset;
}

void AppShowProjectFile(AppEnv* env, const char* path) {
    char* buffer;
    long readLen;
    char header[128];

    if (!env || !path || !path[0]) return;
    if (!AppLooksTextFile(path)) {
        AppUpdateStatus(env, "Cannot view binary file");
        return;
    }

    buffer = NULL;
    readLen = 0;
    if (AppReadRootFile(env, path, &buffer, &readLen, 7000) != noErr || !buffer) {
        AppUpdateStatus(env, "Could not read file");
        if (buffer) free(buffer);
        return;
    }

    sprintf(header, "File: %s (%ld bytes shown)", path, readLen);
    AppAppendTextColored(env, header, &kSystemColor);
    AppAppendTextColored(env, buffer, &kSystemColor);
    if (env->hasRootFolder) {
        short i;
        for (i = 0; i < env->projectItemCount; i++) {
            if (!env->projectItems[i].isFolder && strcmp(env->projectItems[i].name, path) == 0 &&
                    env->projectItems[i].size > readLen) {
                AppAppendTextColored(env,
                    "[File view truncated. Ask AI to inspect this file for more context.]",
                    &kSystemColor);
                break;
            }
        }
    }
    AppUpdateStatus(env, path);
    free(buffer);
}

pascal void ScrollbarAction(ControlHandle control, short part) {
    if (!gApp.transcriptTe) return;
    int current = GetControlValue(control);
    int step = (**gApp.transcriptTe).lineHeight;
    int page = (**gApp.transcriptTe).viewRect.bottom - (**gApp.transcriptTe).viewRect.top - step;

    if (part == kControlUpButtonPart) {
        current -= step;
    } else if (part == kControlDownButtonPart) {
        current += step;
    } else if (part == kControlPageUpPart) {
        current -= page;
    } else if (part == kControlPageDownPart) {
        current += page;
    }

    int maxScroll = GetControlMaximum(control);
    if (current < 0) current = 0;
    if (current > maxScroll) current = maxScroll;

    SetControlValue(control, current);
    ScrollTranscriptTo(current);
}

pascal void FilesScrollbarAction(ControlHandle control, short part) {
    short current;
    short page;
    short maxScroll;

    if (!control) return;
    current = GetControlValue(control);
    page = AppVisibleFileRows(&gApp);
    if (page < 1) page = 1;

    if (part == kControlUpButtonPart) {
        current--;
    } else if (part == kControlDownButtonPart) {
        current++;
    } else if (part == kControlPageUpPart) {
        current -= page;
    } else if (part == kControlPageDownPart) {
        current += page;
    }

    maxScroll = GetControlMaximum(control);
    if (current < 0) current = 0;
    if (current > maxScroll) current = maxScroll;
    gApp.filesScrollOffset = current;
    SetControlValue(control, current);
    AppDrawFilesList(&gApp);
    Draw1Control(control);
}

pascal void SendButtonTrackAction(ControlHandle control, short part) {
    GrafPtr oldPort;

    (void)part;
    if (control != gApp.sendBtn || !gApp.window) return;

    GetPort(&oldPort);
    SetPort(gApp.window);
    AppDrawSendIcon(&gApp);
    SetPort(oldPort);
}

void ScrollTranscriptTo(int targetOffset) {
    if (!gApp.transcriptTe) return;
    int currentOffset = (**gApp.transcriptTe).viewRect.top - (**gApp.transcriptTe).destRect.top;
    int delta = targetOffset - currentOffset;
    if (delta != 0) {
        TEScroll(0, -delta, gApp.transcriptTe);
    }
}

void AdjustScrollbar(void) {
    if (!gApp.transcriptTe || !gApp.scrollbar) return;
    AppSyncTranscriptMetrics(&gApp);
    int viewHeight = (**gApp.transcriptTe).viewRect.bottom - (**gApp.transcriptTe).viewRect.top;
    int totalHeight = (**gApp.transcriptTe).destRect.bottom - (**gApp.transcriptTe).destRect.top;
    int maxScroll = totalHeight - viewHeight;

    if (maxScroll < 0) maxScroll = 0;
    SetControlMinimum(gApp.scrollbar, 0);
    SetControlMaximum(gApp.scrollbar, maxScroll);

    int currentOffset = (**gApp.transcriptTe).viewRect.top - (**gApp.transcriptTe).destRect.top;
    if (currentOffset > maxScroll) {
        ScrollTranscriptTo(maxScroll);
        currentOffset = maxScroll;
    }
    SetControlValue(gApp.scrollbar, currentOffset);
    if (gApp.window) {
        SetPort(gApp.window);
        Draw1Control(gApp.scrollbar);
    }
}

void ScrollToBottom(void) {
    if (!gApp.transcriptTe) return;
    AppSyncTranscriptMetrics(&gApp);
    int viewHeight = (**gApp.transcriptTe).viewRect.bottom - (**gApp.transcriptTe).viewRect.top;
    int totalHeight = (**gApp.transcriptTe).destRect.bottom - (**gApp.transcriptTe).destRect.top;
    int maxScroll = totalHeight - viewHeight;
    if (maxScroll > 0) {
        ScrollTranscriptTo(maxScroll);
    }
    AdjustScrollbar();
}

void AppSyncTranscriptMetrics(AppEnv* env) {
    int viewHeight;
    int lineHeight;
    int contentHeight;
    int currentOffset;
    FontInfo info;

    if (!env || !env->transcriptTe) return;

    SetPort((**env->transcriptTe).inPort);
    TextFont(AppCurrentFontID(env));
    TextSize((short)env->settings.fontSize);
    TextFace(0);
    GetFontInfo(&info);
    lineHeight = info.ascent + info.descent + info.leading;
    if (lineHeight <= 0) lineHeight = 12;
    (**env->transcriptTe).lineHeight = lineHeight;
    (**env->transcriptTe).fontAscent = info.ascent;

    TECalText(env->transcriptTe);
    viewHeight = (**env->transcriptTe).viewRect.bottom - (**env->transcriptTe).viewRect.top;

    contentHeight = ((**env->transcriptTe).nLines + 1) * lineHeight;
    if (contentHeight < viewHeight) contentHeight = viewHeight;

    currentOffset = (**env->transcriptTe).viewRect.top - (**env->transcriptTe).destRect.top;
    (**env->transcriptTe).destRect.bottom = (**env->transcriptTe).destRect.top + contentHeight;
    if (currentOffset < 0) currentOffset = 0;
    (**env->transcriptTe).destRect.top = (**env->transcriptTe).viewRect.top - currentOffset;
    (**env->transcriptTe).destRect.bottom = (**env->transcriptTe).destRect.top + contentHeight;
}

void AppDrawPendingActionIcon(ControlHandle button, Boolean accept) {
    Rect r;
    RGBColor oldFore;
    RGBColor green = { 0x0000, 0x8888, 0x0000 };
    RGBColor red = { 0xBBBB, 0x0000, 0x0000 };
    short cx;
    short cy;

    if (!button) return;
    r = (**button).contrlRect;
    cx = (r.left + r.right) / 2;
    cy = (r.top + r.bottom) / 2;

    GetForeColor(&oldFore);
    RGBForeColor(accept ? &green : &red);
    PenSize(2, 2);
    if (accept) {
        MoveTo(cx - 7, cy);
        LineTo(cx - 2, cy + 5);
        LineTo(cx + 8, cy - 6);
    } else {
        MoveTo(cx - 6, cy - 6);
        LineTo(cx + 6, cy + 6);
        MoveTo(cx + 6, cy - 6);
        LineTo(cx - 6, cy + 6);
    }
    PenNormal();
    RGBForeColor(&oldFore);
}
