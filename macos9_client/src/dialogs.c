#include "common.h"

void AppModalSettings(AppEnv* env) {
    DialogPtr dlg = GetNewDialog(129, 0, (WindowPtr)-1);
    if (!dlg) return;

    /* Bind bold-frame callback — item 18 after adding language row */
    DialogItemType type;
    Handle itemH;
    Rect box;
    GetDialogItem(dlg, 18, &type, &itemH, &box);
    SetDialogItem(dlg, 18, type, (Handle)env->okFrameUPP, &box);

    /* Load current values into text fields */
    char portStr[16];
    char sizeStr[16];
    char oldFontName[32];
    int oldFontSize;
    strcpy(oldFontName, env->settings.fontName);
    oldFontName[sizeof(oldFontName) - 1] = '\0';
    oldFontSize = env->settings.fontSize;
    sprintf(portStr, "%d", env->settings.port);
    sprintf(sizeStr, "%d", env->settings.fontSize);
    SetDialogItemTextC(dlg, 4, env->settings.host);
    SetDialogItemTextC(dlg, 6, portStr);
    SetDialogItemTextC(dlg, 8, env->settings.username);
    SetDialogItemTextC(dlg, 10, env->settings.password);
    SetDialogItemTextC(dlg, 12, env->settings.fontName);
    SetDialogItemTextC(dlg, 14, sizeStr);

    /* Set language radio buttons (items 16=English, 17=Turkce) */
    SetControlValue((ControlHandle)GetDialogItemHandle(dlg, 16),
                    env->settings.language == 0 ? 1 : 0);
    SetControlValue((ControlHandle)GetDialogItemHandle(dlg, 17),
                    env->settings.language == 1 ? 1 : 0);

    SelectDialogItemText(dlg, 4, 0, 32767);

    short item;
    Boolean done = false;
    do {
        ModalDialog(nil, &item);
        if (item == 16) { /* English radio selected */
            SetControlValue((ControlHandle)GetDialogItemHandle(dlg, 16), 1);
            SetControlValue((ControlHandle)GetDialogItemHandle(dlg, 17), 0);
        } else if (item == 17) { /* Turkish radio selected */
            SetControlValue((ControlHandle)GetDialogItemHandle(dlg, 16), 0);
            SetControlValue((ControlHandle)GetDialogItemHandle(dlg, 17), 1);
        } else if (item == 1) { /* OK */
            GetDialogItemTextC(dlg, 4, env->settings.host);
            GetDialogItemTextC(dlg, 6, portStr);
            env->settings.port = atoi(portStr);
            if (env->settings.port <= 0) env->settings.port = 8080;

            GetDialogItemTextC(dlg, 8, env->settings.username);
            GetDialogItemTextC(dlg, 10, env->settings.password);
            GetDialogItemTextC(dlg, 12, env->settings.fontName);
            GetDialogItemTextC(dlg, 14, sizeStr);
            env->settings.fontSize = atoi(sizeStr);

            /* Read language choice from radio buttons */
            env->settings.language =
                GetControlValue((ControlHandle)GetDialogItemHandle(dlg, 17));

            AppNormalizeSettings(&env->settings);
            AppSavePrefs(env);
            AppApplyLanguage(env);   /* live menu + status update */
            if (oldFontSize != env->settings.fontSize ||
                    strcmp(oldFontName, env->settings.fontName) != 0) {
                AppApplyTextStyle(env);
            }
            done = true;
        } else if (item == 2) { /* Cancel */
            done = true;
        }
    } while (!done);

    DisposeDialog(dlg);
}

void AppModalHistory(AppEnv* env) {
    char* history = NULL;
    long historyLen = 0;
    
    if (!AppReadHistory(&history, &historyLen)) {
        AppUpdateStatus(env, env->lang->statusNoHist);
        return;
    }

    char* sessionStarts[10];
    char* sessionEnds[10];
    char sessionTimes[10][32];
    int sessionCount = 0;
    
    char* cursor = history;
    while (sessionCount < 10) {
        int markerLen = 0;
        int whenLen = 0;
        char* found = AppFindHistoryMarker(cursor, &markerLen);
        if (!found) break;
        
        if (sessionCount > 0) {
            sessionEnds[sessionCount - 1] = found;
            while (sessionEnds[sessionCount - 1] > sessionStarts[sessionCount - 1] &&
                   (sessionEnds[sessionCount - 1][-1] == '\r' || sessionEnds[sessionCount - 1][-1] == '\n')) {
                sessionEnds[sessionCount - 1]--;
            }
        }
        
        char* whenPtr = AppFindHistoryWhen(found, 80, &whenLen);
        if (whenPtr) {
            char* tsStart = whenPtr + whenLen;
            char* tsEnd = tsStart;
            while (*tsEnd && *tsEnd != '\r' && *tsEnd != '\n' && (tsEnd - tsStart < 30)) {
                tsEnd++;
            }
            int len = tsEnd - tsStart;
            if (len > 30) len = 30;
            strncpy(sessionTimes[sessionCount], tsStart, len);
            sessionTimes[sessionCount][len] = '\0';
            
            char* bodyStart = tsEnd;
            while (*bodyStart == '\r' || *bodyStart == '\n') bodyStart++;
            sessionStarts[sessionCount] = bodyStart;
        } else {
            sprintf(sessionTimes[sessionCount], "Chat %d", sessionCount + 1);
            char* bodyStart = found + markerLen;
            while (*bodyStart == '\r' || *bodyStart == '\n') bodyStart++;
            sessionStarts[sessionCount] = bodyStart;
        }
        
        sessionCount++;
        cursor = found + markerLen;
    }
    
    if (sessionCount > 0) {
        sessionEnds[sessionCount - 1] = history + historyLen;
        while (sessionEnds[sessionCount - 1] > sessionStarts[sessionCount - 1] &&
               (sessionEnds[sessionCount - 1][-1] == '\r' || sessionEnds[sessionCount - 1][-1] == '\n')) {
            sessionEnds[sessionCount - 1]--;
        }
    }
    
    if (sessionCount <= 0) {
        sessionCount = 1;
        sessionStarts[0] = history;
        sessionEnds[0] = history + historyLen;
        while (sessionEnds[0] > sessionStarts[0] &&
               (sessionEnds[0][-1] == '\r' || sessionEnds[0][-1] == '\n')) {
            sessionEnds[0]--;
        }
        strcpy(sessionTimes[0], "Saved History");
    }
    
    DialogPtr dlg = GetNewDialog(131, 0, (WindowPtr)-1);
    if (!dlg) {
        free(history);
        return;
    }
    
    DialogItemType type;
    Handle itemH;
    Rect box;
    int i;
    for (i = 0; i < 10; i++) {
        short itemID = 2 + i;
        GetDialogItem(dlg, itemID, &type, &itemH, &box);
        if (itemH) {
            if (i < sessionCount) {
                Str255 pStr;
                ConvertCtoPstr(sessionTimes[i], pStr);
                SetControlTitle((ControlHandle)itemH, pStr);
            } else {
                HideControl((ControlHandle)itemH);
            }
        }
    }
    
    short shift = 23 * (10 - sessionCount);
    if (shift > 0) {
        Rect itemBox;
        
        GetDialogItem(dlg, 1, &type, &itemH, &itemBox);
        MoveDialogItem(dlg, 1, itemBox.left, itemBox.top - shift);
        
        GetDialogItem(dlg, 12, &type, &itemH, &itemBox);
        MoveDialogItem(dlg, 12, itemBox.left, itemBox.top - shift);
        
        GetDialogItem(dlg, 13, &type, &itemH, &itemBox);
        MoveDialogItem(dlg, 13, itemBox.left, itemBox.top - shift);
        
        SizeWindow(dlg, 310, 320 - shift, true);
        InvalRect(&dlg->portRect);
    }
    
    short item;
    Boolean done = false;
    int selectedIndex = -1;
    Boolean clearHistoryRequested = false;
    
    do {
        ModalDialog(nil, &item);
        if (item == 1) { /* Cancel */
            done = true;
        } else if (item >= 2 && item <= 11) {
            int index = item - 2;
            if (index < sessionCount) {
                selectedIndex = index;
                done = true;
            }
        } else if (item == 13) { /* Clear History */
            clearHistoryRequested = true;
            done = true;
        }
    } while (!done);
    
    if (clearHistoryRequested) {
        AppClearHistory(env);
        
        env->loadingHistory = true;
        Rect transRect = (**env->transcriptTe).viewRect;
        EraseRect(&transRect);
        TESetText("", 0, env->transcriptTe);
        AppSyncTranscriptMetrics(env);
        env->loadingHistory = false;
        
        ScrollToBottom();
        TEUpdate(&transRect, env->transcriptTe);
        
        AppUpdateStatus(env, env->lang->statusHistClrd);
        SysBeep(1);
        Delay(15, NULL);
        SysBeep(1);
    } else if (selectedIndex >= 0) {
        env->loadingHistory = true;
        char* start = sessionStarts[selectedIndex];
        char* end = sessionEnds[selectedIndex];
        
        Rect transRect = (**env->transcriptTe).viewRect;
        EraseRect(&transRect);
        
        TESetText(start, end - start, env->transcriptTe);
        AppSyncTranscriptMetrics(env);
        env->loadingHistory = false;
        
        ScrollToBottom();
        TEUpdate(&(**env->transcriptTe).viewRect, env->transcriptTe);
        
        char statusMsg[64];
        sprintf(statusMsg, "Loaded: %s", sessionTimes[selectedIndex]);
        AppUpdateStatus(env, statusMsg);
        
        SysBeep(1);
    }
    
    DisposeDialog(dlg);
    free(history);
}

void AppModalAbout(AppEnv* env) {
    DialogPtr dlg = GetNewDialog(130, 0, (WindowPtr)-1);
    if (!dlg) return;

    DialogItemType type;
    Handle itemH;
    Rect box;
    GetDialogItem(dlg, 3, &type, &itemH, &box);
    SetDialogItem(dlg, 3, type, (Handle)env->aboutFrameUPP, &box);

    short item;
    do {
        ModalDialog(nil, &item);
    } while (item != 1);

    DisposeDialog(dlg);
}

char* AppFindHistoryMarker(char* cursor, int* outLen) {
    const char* markers[] = {
        "---- New chat ----",
        "---- Yeni sohbet ----"
    };
    char* best = NULL;
    int bestLen = 0;
    int i;

    if (outLen) *outLen = 0;
    if (!cursor) return NULL;

    for (i = 0; i < 2; i++) {
        char* found = strstr(cursor, markers[i]);
        if (found && (!best || found < best)) {
            best = found;
            bestLen = strlen(markers[i]);
        }
    }

    if (outLen) *outLen = bestLen;
    return best;
}

char* AppFindHistoryWhen(char* marker, int maxScan, int* outLen) {
    const char* labels[] = {
        "When: ",
        "Ne zaman: "
    };
    char* best = NULL;
    int bestLen = 0;
    int i;

    if (outLen) *outLen = 0;
    if (!marker) return NULL;

    for (i = 0; i < 2; i++) {
        char* found = strstr(marker, labels[i]);
        if (found && found < marker + maxScan && (!best || found < best)) {
            best = found;
            bestLen = strlen(labels[i]);
        }
    }

    if (outLen) *outLen = bestLen;
    return best;
}
