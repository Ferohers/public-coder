#include "common.h"

Handle GetDialogItemHandle(DialogPtr dlg, short itemNo) {
    DialogItemType type;
    Handle itemH;
    Rect box;
    GetDialogItem(dlg, itemNo, &type, &itemH, &box);
    return itemH;
}

void SetDialogItemTextC(DialogPtr dlg, short itemNo, const char* text) {
    Str255 pstr;
    ConvertCtoPstr(text, pstr);
    SetDialogItemText(GetDialogItemHandle(dlg, itemNo), pstr);
}

void GetDialogItemTextC(DialogPtr dlg, short itemNo, char* text) {
    Str255 pstr;
    GetDialogItemText(GetDialogItemHandle(dlg, itemNo), pstr);
    ConvertPtoCstr(pstr, text);
}

void AdjustCursor(Point mouseLoc) {
    if (gApp.isBusy) {
        CursHandle watch = GetCursor(watchCursor);
        if (watch) SetCursor(*watch);
        return;
    }
    if (!gApp.inputTe) return;
    Rect inputRect = (**gApp.inputTe).viewRect;
    if (PtInRect(mouseLoc, &inputRect)) {
        if (gApp.ibeamCursor && *gApp.ibeamCursor) {
            SetCursor(*gApp.ibeamCursor);
        }
    } else {
        SetCursor(&qd.arrow);
    }
}

void AppSetBusy(AppEnv* env, Boolean busy) {
    if (!env) return;
    env->isBusy = busy;
    if (busy) {
        CursHandle watch = GetCursor(watchCursor);
        if (watch) SetCursor(*watch);
        AppUpdateStatus(env, env->settings.language == 1 ? "L\x9Ftfen Bekleyin . . ." : "Please wait . . .");
    } else {
        InitCursor();
        AppUpdateStatus(env, env->lang->statusReady);
    }
}

void AppAppendTextColored(AppEnv* env, const char* msg, const RGBColor* color) {
    AppAppendTextFragmentColored(env, msg, color, true);
}

void AppAppendTextFragmentColored(AppEnv* env, const char* msg, const RGBColor* color, Boolean addReturn) {
    if (!env->transcriptTe || !msg || !msg[0]) return;

    int inputLen = (int)strlen(msg);
    int currentLen = (**env->transcriptTe).teLength;

    /* Prevent memory boundary faults by truncating transcript at 26KB */
    if (currentLen + inputLen > 26000) {
        TESetSelect(0, 8000, env->transcriptTe);
        TEDelete(env->transcriptTe);

        TESetSelect(0, 0, env->transcriptTe);
        {
            int trimLen = strlen(env->lang->transcriptTrim) + 2;
            char* trimMsg = (char*)malloc(trimLen + 1);
            if (trimMsg) {
                strcpy(trimMsg, env->lang->transcriptTrim);
                strcat(trimMsg, "\r\r");
                TEInsert((Ptr)trimMsg, trimLen, env->transcriptTe);
                free(trimMsg);
            }
        }
        currentLen = (**env->transcriptTe).teLength;
    }

    TESetSelect(currentLen, currentLen, env->transcriptTe);

    /* Set text color, font, size, and plain face when inserting */
    if (color) {
        TextStyle ts;
        ts.tsFont = AppCurrentFontID(env);
        ts.tsFace = 0;
        ts.tsSize = (short)env->settings.fontSize;
        ts.tsColor = *color;
        TESetStyle(doFont + doSize + doFace + doColor, &ts, false, env->transcriptTe);
    }

    TEInsert((Ptr)msg, inputLen, env->transcriptTe);
    if (addReturn) {
        TEInsert((Ptr)"\r", 1, env->transcriptTe);
    }

    /* Reset to default color, font, size, and plain face after insertion */
    if (color) {
        TextStyle ts;
        ts.tsFont = AppCurrentFontID(env);
        ts.tsFace = 0;
        ts.tsSize = (short)env->settings.fontSize;
        ts.tsColor = kSystemColor;
        TESetStyle(doFont + doSize + doFace + doColor, &ts, false, env->transcriptTe);
    }

    AppSyncTranscriptMetrics(env);
    ScrollToBottom();
    TEUpdate(&(**env->transcriptTe).viewRect, env->transcriptTe);
}

void AppAppendFileChangeSummaryColored(AppEnv* env, const char* summary) {
    const char* lineStart;

    if (!env || !summary || !summary[0]) return;

    lineStart = summary;
    while (*lineStart) {
        const char* lineEnd;
        const char* plus;
        const char* minus;
        char path[80];
        char addPart[24];
        char removePart[24];
        long len;

        lineEnd = strchr(lineStart, '\n');
        if (!lineEnd) lineEnd = lineStart + strlen(lineStart);
        if (lineEnd <= lineStart) {
            if (*lineEnd == '\n') lineStart = lineEnd + 1;
            else break;
            continue;
        }

        plus = strchr(lineStart, '+');
        minus = strchr(lineStart, '-');
        if (plus && minus && plus < lineEnd && minus < lineEnd) {
            len = plus - lineStart;
            while (len > 0 && lineStart[len - 1] == ' ') len--;
            if (len > (long)sizeof(path) - 1) len = sizeof(path) - 1;
            BlockMoveData(lineStart, path, len);
            path[len] = '\0';

            len = minus - plus;
            while (len > 0 && plus[len - 1] == ' ') len--;
            if (len > (long)sizeof(addPart) - 1) len = sizeof(addPart) - 1;
            BlockMoveData(plus, addPart, len);
            addPart[len] = '\0';

            len = lineEnd - minus;
            if (len > (long)sizeof(removePart) - 1) len = sizeof(removePart) - 1;
            BlockMoveData(minus, removePart, len);
            removePart[len] = '\0';

            AppAppendTextFragmentColored(env, path, &kSystemColor, false);
            AppAppendTextFragmentColored(env, " ", &kSystemColor, false);
            AppAppendTextFragmentColored(env, addPart, &kAddedColor, false);
            AppAppendTextFragmentColored(env, " ", &kSystemColor, false);
            AppAppendTextFragmentColored(env, removePart, &kRemovedColor, true);
        } else {
            char plain[128];
            len = lineEnd - lineStart;
            if (len > (long)sizeof(plain) - 1) len = sizeof(plain) - 1;
            BlockMoveData(lineStart, plain, len);
            plain[len] = '\0';
            if (strstr(plain, "http://") || strstr(plain, "https://") ||
                strstr(plain, "Download:") || strstr(plain, "\xDCndirme:")) {
                AppAppendTextColored(env, plain, &kLinkColor);
            } else {
                AppAppendTextColored(env, plain, &kSystemColor);
            }
        }

        if (*lineEnd == '\n') lineStart = lineEnd + 1;
        else break;
    }
}

const char* AppHTTPBody(const char* response) {
    const char* payload;

    payload = strstr(response, "\r\n\r\n");
    if (payload) return payload + 4;

    payload = strstr(response, "\n\n");
    if (payload) return payload + 2;

    return response;
}

Boolean AppPromptProjectName(AppEnv* env, char* outName, short maxLen) {
    DialogPtr dlg;
    char rawName[64];
    short item;
    Boolean done;
    Boolean accepted;

    if (!outName || maxLen <= 1) return false;
    outName[0] = '\0';

    dlg = GetNewDialog(132, 0, (WindowPtr)-1);
    if (!dlg) {
        AppSanitizeProjectName(env ? env->rootName : "MacApp", outName, maxLen);
        return true;
    }

    AppSanitizeProjectName(env ? env->rootName : "MacApp", rawName, sizeof(rawName));
    SetDialogItemTextC(dlg, 4, rawName);
    SelectDialogItemText(dlg, 4, 0, 32767);

    done = false;
    accepted = false;
    do {
        ModalDialog(nil, &item);
        if (item == 1) {
            GetDialogItemTextC(dlg, 4, rawName);
            AppSanitizeProjectName(rawName, outName, maxLen);
            accepted = outName[0] != '\0';
            done = true;
        } else if (item == 2) {
            done = true;
        }
    } while (!done);

    DisposeDialog(dlg);
    return accepted;
}

pascal void DialogOkFrameProc(DialogRef dlg, DialogItemIndex itemNo) {
    (void)itemNo;  /* required by UserItemUPP signature but not used */
    DialogItemType type;
    Handle itemH;
    Rect box;
    GetDialogItem(dlg, 1, &type, &itemH, &box);
    InsetRect(&box, -4, -4);
    PenSize(3, 3);
    FrameRoundRect(&box, 16, 16);
    PenNormal();
}

IconRef GetAppResourceIcon(OSType creator, SInt16 resID, OSType iconType) {
    ProcessSerialNumber psn = {0, kCurrentProcess};
    ProcessInfoRec infoRec;
    FSSpec appSpec;
    IconRef iconRef = NULL;
    OSErr err;
    
    infoRec.processInfoLength = sizeof(ProcessInfoRec);
    infoRec.processName = NULL;
    infoRec.processAppSpec = &appSpec;
    
    err = GetProcessInformation(&psn, &infoRec);
    if (err == noErr) {
        err = RegisterIconRefFromResource(creator, iconType, &appSpec, resID, &iconRef);
        if (err != noErr) {
            char debugBuf[256];
            sprintf(debugBuf, "RegisterIconRefFromResource failed: creator '%c%c%c%c', type '%c%c%c%c', res %d, err %d\n",
                    (char)(creator>>24), (char)(creator>>16), (char)(creator>>8), (char)creator,
                    (char)(iconType>>24), (char)(iconType>>16), (char)(iconType>>8), (char)iconType,
                    resID, err);
            LogDebug(debugBuf);
        }
    }
    return iconRef;
}

void AppUpdateStatus(AppEnv* env, const char* msg) {
    if (!env->window) return;
    
    Rect portRect;
    strncpy(env->status, msg, sizeof(env->status) - 1);
    env->status[sizeof(env->status) - 1] = '\0';
    SetPort(env->window);

    GetWindowPortBounds(env->window, &portRect);
    
    /* Wipe the top status strip only. */
    Rect eraseRect = portRect;
    eraseRect.top = 0;
    eraseRect.bottom = 31;
    EraseRect(&eraseRect);
    /* Draw themed header background before drawing controls */
    {
        Rect headerRect = eraseRect;
        headerRect.bottom = 32;
        DrawThemeWindowHeader(&headerRect, IsWindowHilited(env->window) ? kThemeStateActive : kThemeStateInactive);
    }

    DrawControls(env->window);
    AppDrawDecorations(env);
}

void AppClearInput(AppEnv* env) {
    Rect redrawRect;

    if (!env->window || !env->inputTe) return;

    TESetText("", 0, env->inputTe);
    TESetSelect(0, 0, env->inputTe);

    SetPort(env->window);
    redrawRect = (**env->inputTe).viewRect;
    InsetRect(&redrawRect, -UI_TEXT_INSET, -UI_TEXT_INSET);
    EraseRect(&redrawRect);

    TEUpdate(&(**env->inputTe).viewRect, env->inputTe);
    AppDrawDecorations(env);
}

void AppClearTranscript(AppEnv* env) {
    Rect redrawRect;

    if (!env->window || !env->transcriptTe) return;

#if ENABLE_CLASSIC_NETWORK
    {
        char resp[64];
        SendClearRequest(resp, sizeof(resp));
    }
#endif

    TESetText("", 0, env->transcriptTe);
    TESetSelect(0, 0, env->transcriptTe);
    AppSyncTranscriptMetrics(env);
    AdjustScrollbar();

    SetPort(env->window);
    redrawRect = (**env->transcriptTe).viewRect;
    InsetRect(&redrawRect, -UI_TEXT_INSET, -UI_TEXT_INSET);
    EraseRect(&redrawRect);

    TEUpdate(&(**env->transcriptTe).viewRect, env->transcriptTe);
    DrawControls(env->window);
    AppDrawDecorations(env);
}

char* AppCopySizedText(const char* text, long len) {
    char* copy;

    if (!text || len < 0) return NULL;
    copy = (char*)malloc(len + 1);
    if (!copy) return NULL;
    if (len > 0) BlockMoveData(text, copy, len);
    copy[len] = '\0';
    return copy;
}

Boolean AppExtractToolPath(const char* tagStart, char* outPath, short maxLen) {
    const char* key;
    const char* start;
    const char* end;
    long len;

    if (!tagStart || !outPath || maxLen <= 1) return false;
    outPath[0] = '\0';
    key = strstr(tagStart, "path=\"");
    if (!key) return false;
    start = key + 6;
    end = strchr(start, '"');
    if (!end || end <= start) return false;
    len = end - start;
    if (len >= maxLen) return false;
    BlockMoveData(start, outPath, len);
    outPath[len] = '\0';
    if (!AppIsSafeRelativePath(outPath)) {
        outPath[0] = '\0';
        return false;
    }
    return true;
}

long AppExtractToolRepeatCount(const char* tagStart) {
    const char* tagEnd;
    const char* key;
    const char* countKey;
    long value;

    if (!tagStart) return 1;
    tagEnd = strchr(tagStart, '>');
    if (!tagEnd) return 1;

    key = strstr(tagStart, "repeat=\"");
    countKey = strstr(tagStart, "count=\"");
    if (!key || key > tagEnd) key = countKey;
    if (!key || key > tagEnd) return 1;

    key = strchr(key, '"');
    if (!key || key > tagEnd) return 1;
    value = atol(key + 1);
    if (value < 1) value = 1;
    if (value > 2000) value = 2000;
    return value;
}

void AppEncodeURLPath(const char* src, char* out, long maxLen) {
    static const char hex[] = "0123456789ABCDEF";
    long i;
    long j;

    if (!out || maxLen <= 1) return;
    out[0] = '\0';
    if (!src) return;

    j = 0;
    for (i = 0; src[i] && j < maxLen - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        Boolean safe = false;
        if (c >= 'A' && c <= 'Z') safe = true;
        else if (c >= 'a' && c <= 'z') safe = true;
        else if (c >= '0' && c <= '9') safe = true;
        else if (c == '/' || c == '.' || c == '_' || c == '-') safe = true;

        if (safe) {
            out[j++] = (char)c;
        } else {
            if (j + 3 >= maxLen) break;
            out[j++] = '%';
            out[j++] = hex[(c >> 4) & 0x0F];
            out[j++] = hex[c & 0x0F];
        }
    }
    out[j] = '\0';
}

void AppLaunchURL(AppEnv* env, const char* url) {
    ICInstance inst;
    OSStatus err;
    long start = 0;
    long end = strlen(url);

    err = ICStart(&inst, 'ALTN');
    if (err != noErr) {
        char errBuf[64];
        sprintf(errBuf, "ICStart error: %d", (int)err);
        AppUpdateStatus(env, errBuf);
        return;
    }

    err = ICFindConfigFile(inst, 0, nil);
    if (err != noErr) {
        char errBuf[64];
        sprintf(errBuf, "ICFindConfig error: %d", (int)err);
        AppUpdateStatus(env, errBuf);
        ICStop(inst);
        return;
    }

    err = ICLaunchURL(inst, "\p", url, end, &start, &end);
    if (err != noErr) {
        char errBuf[128];
        if (err == -5012) {
            if (env->settings.language == 1) {
                sprintf(errBuf, "Varsay\xDDlan taray\xDD\x8D\xDD a\x8D\xDDk olmal\xDD" "d\xDDr. (Hata: %d)", (int)err);
            } else {
                sprintf(errBuf, "Default browser must be opened. (Error: %d)", (int)err);
            }
        } else {
            if (env->settings.language == 1) {
                sprintf(errBuf, "Taray\xDD\x8D\xDD ba\xDFlatma hatas\xDD: %d", (int)err);
            } else {
                sprintf(errBuf, "Browser launch error: %d", (int)err);
            }
        }
        AppUpdateStatus(env, errBuf);
    }
    ICStop(inst);
}

void AppShowTooltip(ControlHandle btn, const char* text) {
    HMMessageRecord msg;
    Rect hotRect = (**btn).contrlRect;
    Point ptTopLeft = { hotRect.top, hotRect.left };
    Point ptBotRight = { hotRect.bottom, hotRect.right };
    Point tip = { hotRect.bottom + 2, (hotRect.left + hotRect.right) / 2 };
    
    msg.hmmHelpType = khmmString;
    ConvertCtoPstr(text, msg.u.hmmString);
    
    LocalToGlobal(&ptTopLeft);
    LocalToGlobal(&ptBotRight);
    LocalToGlobal(&tip);
    
    SetRect(&hotRect, ptTopLeft.h, ptTopLeft.v, ptBotRight.h, ptBotRight.v);
    HMShowBalloon(&msg, tip, &hotRect, NULL, 0, 0, 0);
}

void AppClearHistory(AppEnv* env) {
    int16_t vRefNum;
    int32_t dirID;
    FSSpec spec;
    OSErr err;
    int16_t refNum;
    (void)env;  /* env reserved for future per-window history; not needed yet */
    
    err = FindFolder(PREFS_VOL_REF, kPreferencesFolderType, true, &vRefNum, &dirID);
    if (err != noErr) return;
    
    err = FSMakeFSSpec(vRefNum, dirID, "\pChatHistory.txt", &spec);
    if (err != noErr) return;
    
    err = FSpOpenDF(&spec, fsWrPerm, &refNum);
    if (err != noErr) return;
    
    SetEOF(refNum, 0);
    FSClose(refNum);
}
