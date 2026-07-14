#include "common.h"

void AppOpenEditorFile(AppEnv* env, const char* path) {
    short i;

    if (!env || !path || !path[0]) return;
    if (!AppLooksTextFile(path)) {
        AppUpdateStatus(env, "Cannot edit binary file");
        return;
    }
    if (env->activeEditorTab >= 0 &&
            env->activeEditorTab < env->editorTabCount &&
            env->editorTabs[env->activeEditorTab].dirty) {
        AppUpdateStatus(env, "Save current tab before switching");
        return;
    }

    for (i = 0; i < env->editorTabCount; i++) {
        if (env->editorTabs[i].inUse && strcmp(env->editorTabs[i].path, path) == 0) {
            AppLoadEditorTab(env, i);
            return;
        }
    }

    if (env->editorTabCount >= UI_EDITOR_MAX_TABS) {
        AppUpdateStatus(env, "Too many editor tabs");
        return;
    }

    i = env->editorTabCount++;
    env->editorTabs[i].inUse = true;
    env->editorTabs[i].dirty = false;
    strncpy(env->editorTabs[i].path, path, sizeof(env->editorTabs[i].path) - 1);
    env->editorTabs[i].path[sizeof(env->editorTabs[i].path) - 1] = '\0';
    AppLoadEditorTab(env, i);
}

void AppLoadEditorTab(AppEnv* env, short tabIndex) {
    char* buffer;
    long len;
    OSErr err;

    if (!env || !env->editorTe || tabIndex < 0 || tabIndex >= env->editorTabCount) return;
    buffer = NULL;
    len = 0;
    err = AppReadRootFile(env, env->editorTabs[tabIndex].path, &buffer, &len, UI_EDITOR_MAX_FILE_BYTES);
    if (err != noErr || !buffer) {
        AppUpdateStatus(env, "Could not open editor file");
        if (buffer) free(buffer);
        return;
    }

    env->activeEditorTab = tabIndex;
    env->editorTabs[tabIndex].dirty = false;
    AppClampEditorTabScroll(env);
    AppEnsureActiveEditorTabVisible(env);
    SetPort(env->editorWindow);
    TESetText(buffer, len, env->editorTe);
    TESetSelect(0, 0, env->editorTe);
    AppRestyleTextEdit(env, env->editorTe);
    AppSyncEditorMetrics(env);
    AppAdjustEditorScrollbar(env);
    free(buffer);
    ShowWindow(env->editorWindow);
    SelectWindow(env->editorWindow);
    env->activeTe = env->editorTe;
    TEActivate(env->editorTe);
    AppSetEditorTitle(env);
    AppDrawEditorWindow(env);
    AppUpdateStatus(env, env->editorTabs[tabIndex].path);
}

short AppEditorVisibleTabCapacity(AppEnv* env) {
    Rect portRect;
    short available;
    short capacity;

    if (!env || !env->editorWindow) return 1;
    GetWindowPortBounds(env->editorWindow, &portRect);
    available = portRect.right - UI_SCROLL_WIDTH - 24;
    if (env->editorTabCount > 0 &&
            env->editorTabCount * (UI_EDITOR_TAB_MIN_WIDTH + UI_EDITOR_TAB_GAP) > available) {
        available -= (UI_EDITOR_TAB_ARROW_WIDTH * 2) + 6;
    }
    capacity = available / (UI_EDITOR_TAB_MIN_WIDTH + UI_EDITOR_TAB_GAP);
    if (capacity < 1) capacity = 1;
    if (capacity > env->editorTabCount && env->editorTabCount > 0) capacity = env->editorTabCount;
    return capacity;
}

short AppEditorTabStartX(AppEnv* env) {
    Rect portRect;
    short available;

    if (!env || !env->editorWindow) return 8;
    GetWindowPortBounds(env->editorWindow, &portRect);
    available = portRect.right - UI_SCROLL_WIDTH - 24;
    if (env->editorTabCount > 0 &&
            env->editorTabCount * (UI_EDITOR_TAB_MIN_WIDTH + UI_EDITOR_TAB_GAP) > available) {
        return 8 + UI_EDITOR_TAB_ARROW_WIDTH + 4;
    }
    return 8;
}

void AppClampEditorTabScroll(AppEnv* env) {
    short capacity;
    short maxScroll;

    if (!env) return;
    capacity = AppEditorVisibleTabCapacity(env);
    maxScroll = env->editorTabCount - capacity;
    if (maxScroll < 0) maxScroll = 0;
    if (env->editorTabScroll < 0) env->editorTabScroll = 0;
    if (env->editorTabScroll > maxScroll) env->editorTabScroll = maxScroll;
}

void AppEnsureActiveEditorTabVisible(AppEnv* env) {
    short capacity;
    short maxScroll;

    if (!env || env->activeEditorTab < 0) return;
    capacity = AppEditorVisibleTabCapacity(env);
    if (env->activeEditorTab >= 0) {
        if (env->activeEditorTab < env->editorTabScroll) {
            env->editorTabScroll = env->activeEditorTab;
        } else if (env->activeEditorTab >= env->editorTabScroll + capacity) {
            env->editorTabScroll = env->activeEditorTab - capacity + 1;
        }
    }
    maxScroll = env->editorTabCount - capacity;
    if (maxScroll < 0) maxScroll = 0;
    if (env->editorTabScroll > maxScroll) env->editorTabScroll = maxScroll;
    if (env->editorTabScroll < 0) env->editorTabScroll = 0;
}

void AppEditorLeftArrowRect(AppEnv* env, Rect* outRect) {
    (void)env;
    if (!outRect) return;
    SetRect(outRect, 8, UI_EDITOR_TOOL_HEIGHT + 8,
        8 + UI_EDITOR_TAB_ARROW_WIDTH, UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT - 1);
}

void AppEditorRightArrowRect(AppEnv* env, Rect* outRect) {
    Rect portRect;

    if (!outRect) return;
    if (!env || !env->editorWindow) {
        SetRect(outRect, 0, 0, 0, 0);
        return;
    }
    GetWindowPortBounds(env->editorWindow, &portRect);
    SetRect(outRect, portRect.right - UI_SCROLL_WIDTH - UI_EDITOR_TAB_ARROW_WIDTH - 10,
        UI_EDITOR_TOOL_HEIGHT + 8,
        portRect.right - UI_SCROLL_WIDTH - 10,
        UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT - 1);
}

short AppEditorTabWidth(AppEnv* env) {
    Rect portRect;
    short capacity;
    short available;
    short width;

    if (!env || !env->editorWindow) return UI_EDITOR_TAB_WIDTH;
    GetWindowPortBounds(env->editorWindow, &portRect);
    capacity = AppEditorVisibleTabCapacity(env);
    available = portRect.right - UI_SCROLL_WIDTH - 24;
    if (env->editorTabCount > capacity) {
        available -= (UI_EDITOR_TAB_ARROW_WIDTH * 2) + 6;
    }
    width = (available - ((capacity - 1) * UI_EDITOR_TAB_GAP)) / capacity;
    if (width > UI_EDITOR_TAB_WIDTH) width = UI_EDITOR_TAB_WIDTH;
    if (width < UI_EDITOR_TAB_MIN_WIDTH) width = UI_EDITOR_TAB_MIN_WIDTH;
    return width;
}

void AppEditorTabRect(AppEnv* env, short index, Rect* outRect) {
    short width;
    short left;
    short relative;
    short capacity;

    if (!outRect) return;
    AppClampEditorTabScroll(env);
    capacity = AppEditorVisibleTabCapacity(env);
    relative = index - env->editorTabScroll;
    if (relative < 0 || relative >= capacity) {
        SetRect(outRect, 0, 0, 0, 0);
        return;
    }
    width = AppEditorTabWidth(env);
    left = AppEditorTabStartX(env) + (relative * (width + UI_EDITOR_TAB_GAP));
    SetRect(outRect, left, UI_EDITOR_TOOL_HEIGHT + 5,
        left + width, UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT + 2);
}

void AppEditorTabCloseRect(const Rect* tabRect, Rect* outRect) {
    if (!tabRect || !outRect) return;
    /* Move close box left by 6px to avoid touching the right slope edge */
    SetRect(outRect, tabRect->right - 22, tabRect->top + 5,
        tabRect->right - 12, tabRect->top + 15);
}

Boolean AppCloseEditorTab(AppEnv* env, short tabIndex) {
    short i;
    short nextTab;
    Boolean closingActive;

    if (!env || tabIndex < 0 || tabIndex >= env->editorTabCount) return false;
    if (env->editorTabs[tabIndex].dirty) {
        short action;
        if (env->activeEditorTab != tabIndex) {
            AppLoadEditorTab(env, tabIndex);
        }
        action = AppPromptSaveChanges(env, env->editorTabs[tabIndex].path);
        if (action == 1) { /* Save */
            AppSaveEditorFile(env);
            if (env->editorTabs[tabIndex].dirty) {
                /* Save failed, abort closing */
                return false;
            }
        } else if (action == 3) { /* Don't Save */
            /* Discard: allow closing */
        } else { /* Cancel */
            return false;
        }
    }
    closingActive = (tabIndex == env->activeEditorTab);

    for (i = tabIndex; i < env->editorTabCount - 1; i++) {
        env->editorTabs[i] = env->editorTabs[i + 1];
    }
    if (env->editorTabCount > 0) {
        memset(&env->editorTabs[env->editorTabCount - 1], 0, sizeof(EditorTab));
        env->editorTabCount--;
    }

    if (env->editorTabCount <= 0) {
        env->activeEditorTab = -1;
        env->editorTabScroll = 0;
        if (env->editorTe) {
            TESetText("", 0, env->editorTe);
            TESetSelect(0, 0, env->editorTe);
            AppSyncEditorMetrics(env);
            AppAdjustEditorScrollbar(env);
        }
        AppSetEditorTitle(env);
        AppDrawEditorWindow(env);
        AppUpdateStatus(env, "Closed editor tab");
        return true;
    }

    nextTab = tabIndex;
    if (nextTab >= env->editorTabCount) nextTab = env->editorTabCount - 1;
    if (!closingActive) {
        if (env->activeEditorTab > tabIndex) env->activeEditorTab--;
        AppClampEditorTabScroll(env);
        AppDrawEditorWindow(env);
        AppUpdateStatus(env, "Closed editor tab");
        return true;
    }

    AppLoadEditorTab(env, nextTab);
    AppUpdateStatus(env, "Closed editor tab");
    return true;
}

void AppDrawEditorWindow(AppEnv* env) {
    Rect portRect;
    Str255 pstr;

    if (!env || !env->editorWindow) return;
    SetPort(env->editorWindow);
    GetWindowPortBounds(env->editorWindow, &portRect);
    EraseRect(&portRect);

    /* Draw themed toolbar header background at top */
    Rect toolRect = portRect;
    toolRect.top = 0;
    toolRect.bottom = UI_EDITOR_TOOL_HEIGHT;
    DrawThemeWindowHeader(&toolRect, IsWindowHilited(env->editorWindow) ? kThemeStateActive : kThemeStateInactive);

    DrawControls(env->editorWindow);
    AppDrawEditorToolButtons(env);
    AppDrawEditorTabs(env);

    if (env->editorTe && env->activeEditorTab >= 0) {
        AppAdjustEditorScrollbar(env);

        /* Draw themed 3D frame enclosing both line gutter and text edit area */
        Rect editorFrame;
        Rect scrollRect;
        scrollRect = (**env->editorScrollbar).contrlRect;
        SetRect(&editorFrame, 8, UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT + 6,
            scrollRect.left - 2, portRect.bottom - 14);
        DrawThemeEditTextFrame(&editorFrame, IsWindowHilited(env->editorWindow) ? kThemeStateActive : kThemeStateInactive);

        TEUpdate(&(**env->editorTe).viewRect, env->editorTe);
        AppDrawEditorLineNumbers(env);
    } else {
        TextFont(systemFont);
        TextSize(12);
        TextFace(0);
        if (env->settings.language == 1) {
            ConvertCtoPstr("A\x8D\xDDk dosya yok", pstr);
        } else {
            ConvertCtoPstr("No file open", pstr);
        }
        MoveTo(16, UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT + 36);
        DrawString(pstr);
    }
}

void AppDrawEditorToolButtons(AppEnv* env) {
    GrafPtr oldPort;

    if (!env || !env->editorWindow) return;
    GetPort(&oldPort);
    SetPort(env->editorWindow);
    AppDrawEditorToolButton(env->editorSaveBtn, env->lang->editorSave, 0);
    AppDrawEditorToolButton(env->editorSearchBtn, env->lang->editorFind, 1);
    AppDrawEditorToolButton(env->editorGoLineBtn, env->lang->editorLine, 2);
    AppDrawEditorToolButton(env->editorNextBtn, env->lang->editorNext, 3);
    SetPort(oldPort);
}

void AppDrawEditorTabs(AppEnv* env) {
    GrafPtr oldPort;
    Rect portRect;
    Rect bandRect;
    Rect tabRect;
    Rect closeRect;
    Str255 pstr;
    short i;
    short tabWidth;
    char label[48];
    short labelLen;
    RGBColor oldColor;
    RGBColor closeColor = { 0x8888, 0x0000, 0x0000 };
    Boolean hasHidden;
    ThemeTabStyle tabStyle;

    if (!env || !env->editorWindow) return;
    GetPort(&oldPort);
    SetPort(env->editorWindow);
    GetWindowPortBounds(env->editorWindow, &portRect);
    AppClampEditorTabScroll(env);

    /* Erase the tab band area */
    SetRect(&bandRect, 0, UI_EDITOR_TOOL_HEIGHT + 2,
        portRect.right, UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT + 4);
    EraseRect(&bandRect);
    GetForeColor(&oldColor);
    tabWidth = AppEditorTabWidth(env);
    hasHidden = env->editorTabCount > AppEditorVisibleTabCapacity(env);

    /* Scroll arrows for overflow tabs */
    if (hasHidden) {
        Rect arrowRect;
        RGBColor arrowBg = { 0xEEEE, 0xEEEE, 0xEEEE };

        AppEditorLeftArrowRect(env, &arrowRect);
        RGBForeColor(&arrowBg);
        PaintRoundRect(&arrowRect, 4, 4);
        RGBForeColor(&oldColor);
        FrameRoundRect(&arrowRect, 4, 4);
        MoveTo(arrowRect.left + 9, arrowRect.top + 3);
        LineTo(arrowRect.left + 5, (arrowRect.top + arrowRect.bottom) / 2);
        LineTo(arrowRect.left + 9, arrowRect.bottom - 3);

        AppEditorRightArrowRect(env, &arrowRect);
        RGBForeColor(&arrowBg);
        PaintRoundRect(&arrowRect, 4, 4);
        RGBForeColor(&oldColor);
        FrameRoundRect(&arrowRect, 4, 4);
        MoveTo(arrowRect.right - 9, arrowRect.top + 3);
        LineTo(arrowRect.right - 5, (arrowRect.top + arrowRect.bottom) / 2);
        LineTo(arrowRect.right - 9, arrowRect.bottom - 3);
    }

    /* Draw each tab using Appearance Manager DrawThemeTab */
    for (i = 0; i < env->editorTabCount; i++) {
        AppEditorTabRect(env, i, &tabRect);
        if (tabRect.right <= tabRect.left) continue;

        /* Use Appearance Manager themed tabs */
        tabStyle = (i == env->activeEditorTab) ? kThemeTabFront : kThemeTabNonFront;
        DrawThemeTab(&tabRect, tabStyle, kThemeTabNorth, NULL, 0);

        /* Tab label — Geneva 10 for readability */
        strncpy(label, env->editorTabs[i].path, sizeof(label) - 3);
        label[sizeof(label) - 3] = '\0';
        if (env->editorTabs[i].dirty) strcat(label, " \xA5");  /* bullet for dirty */
        labelLen = strlen(label);
        TextFont(kFontIDGeneva);
        TextSize(10);
        TextFace(i == env->activeEditorTab ? bold : 0);
        while (labelLen > 3) {
            ConvertCtoPstr(label, pstr);
            if (StringWidth(pstr) <= tabWidth - 36) break;
            label[--labelLen] = '\0';
        }
        ConvertCtoPstr(label, pstr);
        MoveTo(tabRect.left + 14, tabRect.top + 14);
        DrawString(pstr);
        TextFace(0);

        /* Close button — small circle with × */
        AppEditorTabCloseRect(&tabRect, &closeRect);
        RGBForeColor(&closeColor);
        FrameOval(&closeRect);
        MoveTo(closeRect.left + 2, closeRect.top + 2);
        LineTo(closeRect.right - 3, closeRect.bottom - 3);
        MoveTo(closeRect.right - 3, closeRect.top + 2);
        LineTo(closeRect.left + 2, closeRect.bottom - 3);
        RGBForeColor(&oldColor);
    }

    /* Baseline separator under all tabs */
    MoveTo(0, UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT + 2);
    LineTo(portRect.right, UI_EDITOR_TOOL_HEIGHT + UI_EDITOR_TAB_HEIGHT + 2);
    RGBForeColor(&oldColor);
    SetPort(oldPort);
}

void AppDrawEditorLineNumbers(AppEnv* env) {
    GrafPtr oldPort;
    Rect viewRect;
    Rect gutterRect;
    Str255 pstr;
    short lineHeight;
    short visibleLines;
    short totalLines;
    short i;
    short lineNo;
    short baseline;
    char num[12];
    Handle textH;
    char* text;
    long len;
    long j;

    if (!env || !env->editorTe || env->activeEditorTab < 0) return;
    GetPort(&oldPort);
    SetPort(env->editorWindow);
    viewRect = (**env->editorTe).viewRect;
    lineHeight = (**env->editorTe).lineHeight;
    if (lineHeight <= 0) lineHeight = 12;
    visibleLines = (viewRect.bottom - viewRect.top) / lineHeight + 1;

    totalLines = 1;
    len = (**env->editorTe).teLength;
    textH = (**env->editorTe).hText;
    HLock(textH);
    text = *textH;
    for (j = 0; j < len; j++) {
        if (text[j] == '\r' || text[j] == '\n') {
            totalLines++;
            if (j + 1 < len && text[j] == '\r' && text[j + 1] == '\n') j++;
        }
    }
    HUnlock(textH);

    /* Gutter bounds: left=9 (inside 3D border), right=50 */
    gutterRect = viewRect;
    gutterRect.left = 9;
    gutterRect.right = UI_EDITOR_GUTTER_WIDTH + 8;
    EraseRect(&gutterRect);
    MoveTo(UI_EDITOR_GUTTER_WIDTH + 8, viewRect.top);
    LineTo(UI_EDITOR_GUTTER_WIDTH + 8, viewRect.bottom);

    TextFont(AppCurrentFontID(env));
    TextSize((short)env->settings.fontSize);
    lineNo = ((**env->editorTe).viewRect.top - (**env->editorTe).destRect.top) / lineHeight + 1;
    if (lineNo < 1) lineNo = 1;
    for (i = 0; i < visibleLines && i < totalLines; i++) {
        baseline = viewRect.top + ((i + 1) * lineHeight) - 3;
        if (baseline > viewRect.bottom - 3) break;
        sprintf(num, "%d", lineNo + i);
        ConvertCtoPstr(num, pstr);
        MoveTo(14, baseline);  /* Center within the 9..50 gutter */
        DrawString(pstr);
    }
    SetPort(oldPort);
}

void AppSyncEditorMetrics(AppEnv* env) {
    int viewHeight;
    int lineHeight;
    int contentHeight;
    int currentOffset;
    FontInfo info;

    if (!env || !env->editorTe) return;

    SetPort((**env->editorTe).inPort);
    TextFont(AppCurrentFontID(env));
    TextSize((short)env->settings.fontSize);
    TextFace(0);
    GetFontInfo(&info);
    lineHeight = info.ascent + info.descent + info.leading;
    if (lineHeight <= 0) lineHeight = 12;
    (**env->editorTe).lineHeight = lineHeight;
    (**env->editorTe).fontAscent = info.ascent;

    TECalText(env->editorTe);
    viewHeight = (**env->editorTe).viewRect.bottom - (**env->editorTe).viewRect.top;
    contentHeight = ((**env->editorTe).nLines + 1) * lineHeight;
    if (contentHeight < viewHeight) contentHeight = viewHeight;

    currentOffset = (**env->editorTe).viewRect.top - (**env->editorTe).destRect.top;
    if (currentOffset < 0) currentOffset = 0;
    (**env->editorTe).destRect.top = (**env->editorTe).viewRect.top - currentOffset;
    (**env->editorTe).destRect.bottom = (**env->editorTe).destRect.top + contentHeight;
}

void AppScrollEditorTo(AppEnv* env, int targetOffset) {
    int currentOffset;
    int delta;
    Rect gutterRect;

    if (!env || !env->editorTe) return;
    currentOffset = (**env->editorTe).viewRect.top - (**env->editorTe).destRect.top;
    delta = targetOffset - currentOffset;
    if (delta != 0) {
        TEScroll(0, -delta, env->editorTe);
        gutterRect = (**env->editorTe).viewRect;
        gutterRect.left = 0;
        gutterRect.right = UI_EDITOR_GUTTER_WIDTH;
        EraseRect(&gutterRect);
        AppDrawEditorLineNumbers(env);
    }
}

void AppAdjustEditorScrollbar(AppEnv* env) {
    int viewHeight;
    int totalHeight;
    int maxScroll;
    int currentOffset;

    if (!env || !env->editorTe || !env->editorScrollbar) return;
    AppSyncEditorMetrics(env);
    viewHeight = (**env->editorTe).viewRect.bottom - (**env->editorTe).viewRect.top;
    totalHeight = (**env->editorTe).destRect.bottom - (**env->editorTe).destRect.top;
    maxScroll = totalHeight - viewHeight;
    if (maxScroll < 0) maxScroll = 0;

    SetControlMinimum(env->editorScrollbar, 0);
    SetControlMaximum(env->editorScrollbar, maxScroll);
    currentOffset = (**env->editorTe).viewRect.top - (**env->editorTe).destRect.top;
    if (currentOffset > maxScroll) {
        AppScrollEditorTo(env, maxScroll);
        currentOffset = maxScroll;
    }
    SetControlValue(env->editorScrollbar, currentOffset);
    if (env->editorWindow) {
        SetPort(env->editorWindow);
        Draw1Control(env->editorScrollbar);
    }
}

void AppSaveEditorFile(AppEnv* env) {
    Handle textH;
    char* text;
    long len;
    OSErr err;

    if (!env || !env->editorTe || env->activeEditorTab < 0 ||
            env->activeEditorTab >= env->editorTabCount) {
        AppUpdateStatus(env, "No editor file");
        return;
    }

    len = (**env->editorTe).teLength;
    textH = (**env->editorTe).hText;
    text = (char*)malloc(len + 1);
    if (!text) {
        AppUpdateStatus(env, "Not enough memory to save");
        return;
    }
    HLock(textH);
    BlockMoveData(*textH, text, len);
    HUnlock(textH);
    text[len] = '\0';

    err = AppWriteRootFile(env, env->editorTabs[env->activeEditorTab].path, text);
    free(text);
    if (err == noErr) {
        env->editorTabs[env->activeEditorTab].dirty = false;
        AppSetEditorTitle(env);
        AppDrawEditorTabs(env);
        if (env->filesWindow) AppDrawFilesWindow(env);
        AppUpdateStatus(env, "Saved editor file");
    } else {
        AppUpdateStatus(env, "Save failed");
    }
}

void AppMarkEditorDirty(AppEnv* env) {
    if (!env || env->activeEditorTab < 0 || env->activeEditorTab >= env->editorTabCount) return;
    env->editorTabs[env->activeEditorTab].dirty = true;
    AppSetEditorTitle(env);
    AppDrawEditorTabs(env);
}

Boolean AppPromptEditorValue(const char* title, const char* label, char* out, short maxLen) {
    DialogPtr dlg;
    short item;
    Boolean accepted;
    Str255 pstr;

    if (!out || maxLen <= 1) return false;
    out[0] = '\0';
    dlg = GetNewDialog(133, 0, (WindowPtr)-1);
    if (!dlg) return false;

    ConvertCtoPstr(title ? title : "Editor", pstr);
    SetWTitle(dlg, pstr);
    SetDialogItemTextC(dlg, 3, label ? label : "Value:");
    SelectDialogItemText(dlg, 4, 0, 32767);

    accepted = false;
    do {
        ModalDialog(nil, &item);
        if (item == 1) {
            GetDialogItemTextC(dlg, 4, out);
            accepted = out[0] != '\0';
            break;
        }
        if (item == 2) break;
    } while (true);

    DisposeDialog(dlg);
    return accepted;
}

void AppFindInEditor(AppEnv* env) {
    char needle[64];
    Handle textH;
    char* text;
    char* copy;
    long len;
    long start;
    char* found;

    if (!env || !env->editorTe) return;
    if (!AppPromptEditorValue("Find", "Find text:", needle, sizeof(needle))) return;
    
    /* Store the search term */
    strncpy(env->lastSearchNeedle, needle, sizeof(env->lastSearchNeedle) - 1);
    env->lastSearchNeedle[sizeof(env->lastSearchNeedle) - 1] = '\0';

    len = (**env->editorTe).teLength;
    textH = (**env->editorTe).hText;
    copy = (char*)malloc(len + 1);
    if (!copy) {
        AppUpdateStatus(env, "Not enough memory to find");
        return;
    }
    HLock(textH);
    BlockMoveData(*textH, copy, len);
    HUnlock(textH);
    copy[len] = '\0';
    text = copy;
    start = (**env->editorTe).selEnd;
    if (start < 0 || start >= len) start = 0;
    found = strstr(text + start, needle);
    if (!found && start > 0) found = strstr(text, needle);
    if (found) {
        long pos = found - text;
        TESetSelect(pos, pos + strlen(needle), env->editorTe);
        AppSyncEditorMetrics(env);
        AppAdjustEditorScrollbar(env);
        TEUpdate(&(**env->editorTe).viewRect, env->editorTe);
        AppDrawEditorLineNumbers(env);
        AppUpdateStatus(env, "Found");
    } else {
        AppUpdateStatus(env, "Not found");
    }
    free(copy);
}

void AppFindNextInEditor(AppEnv* env) {
    Handle textH;
    char* text;
    char* copy;
    long len;
    long start;
    char* found;

    if (!env || !env->editorTe) return;
    if (env->lastSearchNeedle[0] == '\0') {
        /* No search history yet, run standard Find */
        AppFindInEditor(env);
        return;
    }

    len = (**env->editorTe).teLength;
    textH = (**env->editorTe).hText;
    copy = (char*)malloc(len + 1);
    if (!copy) {
        AppUpdateStatus(env, "Not enough memory to find");
        return;
    }
    HLock(textH);
    BlockMoveData(*textH, copy, len);
    HUnlock(textH);
    copy[len] = '\0';
    text = copy;
    start = (**env->editorTe).selEnd;
    if (start < 0 || start >= len) start = 0;
    found = strstr(text + start, env->lastSearchNeedle);
    if (!found && start > 0) found = strstr(text, env->lastSearchNeedle);
    if (found) {
        long pos = found - text;
        TESetSelect(pos, pos + strlen(env->lastSearchNeedle), env->editorTe);
        AppSyncEditorMetrics(env);
        AppAdjustEditorScrollbar(env);
        TEUpdate(&(**env->editorTe).viewRect, env->editorTe);
        AppDrawEditorLineNumbers(env);
        AppUpdateStatus(env, "Found next");
    } else {
        AppUpdateStatus(env, "Not found");
    }
    free(copy);
}

void AppGoToEditorLine(AppEnv* env) {
    char value[16];
    long target;
    long line;
    long pos;
    long len;
    Handle textH;
    char* text;

    if (!env || !env->editorTe) return;
    if (!AppPromptEditorValue("Go To Line", "Line number:", value, sizeof(value))) return;
    target = atol(value);
    if (target < 1) target = 1;
    len = (**env->editorTe).teLength;
    textH = (**env->editorTe).hText;
    HLock(textH);
    text = *textH;
    line = 1;
    pos = 0;
    while (pos < len && line < target) {
        if (text[pos] == '\r' || text[pos] == '\n') line++;
        pos++;
    }
    TESetSelect(pos, pos, env->editorTe);
    AppSyncEditorMetrics(env);
    AppAdjustEditorScrollbar(env);
    TEUpdate(&(**env->editorTe).viewRect, env->editorTe);
    AppDrawEditorLineNumbers(env);
    HUnlock(textH);
}

short AppCurrentFontID(AppEnv* env) {
    Str255 fontP;
    short fontID = 4; /* Monaco */

    if (env->settings.fontName[0]) {
        ConvertCtoPstr(env->settings.fontName, fontP);
        GetFNum(fontP, &fontID);
    }

    return fontID;
}

void AppUseConfiguredFont(AppEnv* env) {
    TextFont(AppCurrentFontID(env));
    TextSize((short)env->settings.fontSize);
    TextFace(0); /* Reset to plain style to prevent pollution */
}

void AppRestyleTextEdit(AppEnv* env, TEHandle te) {
    FontInfo info;
    short fontID;

    if (!te) return;

    /* Deselect so stale selection rectangles are not redrawn */
    TESetSelect(0, 0, te);

    fontID = AppCurrentFontID(env);
    SetPort((**te).inPort);
    TextFont(fontID);
    TextSize((short)env->settings.fontSize);
    TextFace(0); /* Reset GrafPort text face to plain */
    GetFontInfo(&info);

    (**te).txFont = fontID;
    (**te).txSize = (short)env->settings.fontSize;
    (**te).txFace = 0; /* Reset TextEdit record text face */
    (**te).lineHeight = info.ascent + info.descent + info.leading;
    (**te).fontAscent = info.ascent;

    /* For styled TextEdit (transcript), update all style runs to the new font/size/plain face */
    if (TEGetStyleHandle(te)) {
        TextStyle ts;
        ts.tsFont = fontID;
        ts.tsSize = (short)env->settings.fontSize;
        ts.tsFace = 0;
        ts.tsColor = kSystemColor;
        TESetSelect(0, (**te).teLength, te);
        TESetStyle(doFont + doSize + doFace, &ts, true, te);
        TESetSelect(0, 0, te);
    }

    TECalText(te);
    if (te == env->transcriptTe) {
        AppSyncTranscriptMetrics(env);
    } else if (te == env->editorTe) {
        AppSyncEditorMetrics(env);
    }
}

void AppApplyTextStyle(AppEnv* env) {
    Rect portRect;

    AppNormalizeSettings(&env->settings);
    AppRestyleTextEdit(env, env->transcriptTe);
    AppRestyleTextEdit(env, env->inputTe);
    AppRestyleTextEdit(env, env->editorTe);

    if (env->window) {
        SetPort(env->window);
        GetWindowPortBounds(env->window, &portRect);
        AppResizeWidgets(env, portRect.right - portRect.left, portRect.bottom - portRect.top);
        InvalRect(&portRect);
    }
    if (env->editorWindow) {
        SetPort(env->editorWindow);
        GetWindowPortBounds(env->editorWindow, &portRect);
        AppResizeEditorWidgets(env);
        InvalRect(&portRect);
    }
}

pascal void EditorScrollbarAction(ControlHandle control, short part) {
    int current;
    int step;
    int page;
    int maxScroll;

    if (!gApp.editorTe) return;
    current = GetControlValue(control);
    step = (**gApp.editorTe).lineHeight;
    if (step <= 0) step = 12;
    page = (**gApp.editorTe).viewRect.bottom - (**gApp.editorTe).viewRect.top - step;

    if (part == kControlUpButtonPart) {
        current -= step;
    } else if (part == kControlDownButtonPart) {
        current += step;
    } else if (part == kControlPageUpPart) {
        current -= page;
    } else if (part == kControlPageDownPart) {
        current += page;
    }

    maxScroll = GetControlMaximum(control);
    if (current < 0) current = 0;
    if (current > maxScroll) current = maxScroll;

    SetControlValue(control, current);
    AppScrollEditorTo(&gApp, current);
}

void AppSetEditorTitle(AppEnv* env) {
    char title[96];
    Str255 ptitle;

    if (!env || !env->editorWindow) return;
    if (env->activeEditorTab >= 0 && env->activeEditorTab < env->editorTabCount) {
        sprintf(title, "%s%s", env->editorTabs[env->activeEditorTab].path,
            env->editorTabs[env->activeEditorTab].dirty ? " *" : "");
    } else {
        strcpy(title, "Editor");
    }
    ConvertCtoPstr(title, ptitle);
    SetWTitle(env->editorWindow, ptitle);
}

short AppPromptSaveChanges(AppEnv* env, const char* path) {
    DialogPtr dlg;
    short itemHit = 2; /* Default to Cancel */
    char message[256];

    if (!env) return 2;

    dlg = GetNewDialog(140, nil, (WindowPtr)-1L);
    if (!dlg) return 2;

    SetPort(dlg);

    if (env->settings.language == 1) {
        sprintf(message, "\"%s\" dosyas\xDDndaki de\xDBi\xDFiklikler kaydedilsin mi?", path);
        SetDialogItemTextC(dlg, 1, "Kaydet");
        SetDialogItemTextC(dlg, 2, "\xDCptal");
        SetDialogItemTextC(dlg, 3, "Kaydetme");
    } else {
        sprintf(message, "Save changes to \"%s\" before closing?", path);
        SetDialogItemTextC(dlg, 1, "Save");
        SetDialogItemTextC(dlg, 2, "Cancel");
        SetDialogItemTextC(dlg, 3, "Don't Save");
    }
    SetDialogItemTextC(dlg, 4, message);

    {
        Rect dlgRect;
        short dlgWidth, dlgHeight;
        short screenWidth, screenHeight;
        short x, y;

        GetWindowPortBounds(dlg, &dlgRect);
        dlgWidth = dlgRect.right - dlgRect.left;
        dlgHeight = dlgRect.bottom - dlgRect.top;

        screenWidth = qd.screenBits.bounds.right - qd.screenBits.bounds.left;
        screenHeight = qd.screenBits.bounds.bottom - qd.screenBits.bounds.top;

        x = (screenWidth - dlgWidth) / 2;
        y = (screenHeight - dlgHeight) / 2;

        MoveWindow(dlg, x, y, false);
    }

    ShowWindow(dlg);
    SelectWindow(dlg);

    ModalDialog(nil, &itemHit);

    DisposeDialog(dlg);
    if (env->editorWindow && IsWindowVisible(env->editorWindow)) {
        SetPort(env->editorWindow);
    } else {
        SetPort(env->window);
    }

    return itemHit;
}

void AppDrawEditorToolButton(ControlHandle button, const char* label, short kind) {
    Rect r;
    Rect icon;
    Rect mark;
    Str255 pstr;
    FontInfo info;
    short textTop;
    RGBColor oldFore;

    if (!button) return;
    r = (**button).contrlRect;
    SetRect(&icon, r.left + 7, r.top + 5, r.left + 21, r.top + 19);

    GetForeColor(&oldFore);
    RGBForeColor(&kSystemColor);
    PenNormal();
    if (kind == 0) {
        FrameRect(&icon);
        mark = icon;
        InsetRect(&mark, 3, 2);
        FrameRect(&mark);
        MoveTo(icon.left + 3, icon.bottom - 4);
        LineTo(icon.right - 3, icon.bottom - 4);
        MoveTo(icon.right - 4, icon.top + 2);
        LineTo(icon.right - 4, icon.top + 5);
    } else if (kind == 1) {
        SetRect(&icon, r.left + 8, r.top + 5, r.left + 19, r.top + 16);
        FrameOval(&icon);
        MoveTo(icon.right - 1, icon.bottom - 1);
        LineTo(icon.right + 5, icon.bottom + 5);
    } else if (kind == 2) {
        MoveTo(icon.left + 3, icon.top + 2);
        LineTo(icon.right - 2, icon.top + 2);
        MoveTo(icon.left + 3, icon.top + 6);
        LineTo(icon.right - 2, icon.top + 6);
        MoveTo(icon.left + 3, icon.top + 10);
        LineTo(icon.right - 2, icon.top + 10);
        MoveTo(icon.left, icon.top + 2);
        LineTo(icon.left + 1, icon.top + 2);
    } else if (kind == 3) {
        /* Arrow pointing right for Find Next */
        MoveTo(icon.left + 2, icon.top + 7);
        LineTo(icon.right - 2, icon.top + 7);
        MoveTo(icon.right - 5, icon.top + 4);
        LineTo(icon.right - 2, icon.top + 7);
        LineTo(icon.right - 5, icon.top + 10);
    }

    RGBForeColor(&oldFore);
    TextFont(kFontIDGeneva);
    TextSize(10);
    TextFace(bold);
    GetFontInfo(&info);
    ConvertCtoPstr(label, pstr);
    textTop = r.top + ((r.bottom - r.top -
        (info.ascent + info.descent + info.leading)) / 2) + info.ascent;
    MoveTo(r.left + 26, textTop);
    DrawString(pstr);
    TextFace(0);
}

Boolean AppPendingChangesTouchPath(AppEnv* env, const char* path) {
    const char* cursor;
    char needle[96];

    if (!env || !env->pendingAIChangeText || !path || !path[0]) return false;
    sprintf(needle, "path=\"%s\"", path);
    if (strstr(env->pendingAIChangeText, needle)) return true;
    sprintf(needle, "filename=\"%s\"", path);
    if (strstr(env->pendingAIChangeText, needle)) return true;

    cursor = env->pendingAIChangeText;
    while ((cursor = strstr(cursor, path)) != NULL) {
        char before = (cursor == env->pendingAIChangeText) ? ' ' : cursor[-1];
        char after = cursor[strlen(path)];
        if ((before == '"' || before == ' ' || before == '=') &&
                (after == '"' || after == ' ' || after == '>' || after == '\r' || after == '\n')) {
            return true;
        }
        cursor++;
    }
    return false;
}

Boolean AppActiveEditorHasUnsavedTouchedFile(AppEnv* env) {
    if (!env || env->activeEditorTab < 0 || env->activeEditorTab >= env->editorTabCount) return false;
    if (!env->editorTabs[env->activeEditorTab].dirty) return false;
    return AppPendingChangesTouchPath(env, env->editorTabs[env->activeEditorTab].path);
}

void AppReloadActiveEditorIfClean(AppEnv* env) {
    short tabIndex;

    if (!env || env->activeEditorTab < 0 || env->activeEditorTab >= env->editorTabCount) return;
    tabIndex = env->activeEditorTab;
    if (env->editorTabs[tabIndex].dirty) {
        AppUpdateStatus(env, "Editor has unsaved changes; not reloaded");
        return;
    }
    AppLoadEditorTab(env, tabIndex);
}
