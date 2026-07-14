#include "common.h"

void AppInitToolbox(void) {
#if !TARGET_API_MAC_CARBON
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(nil);
#endif
    InitCursor();
    MaxApplZone();
}

void AppLoadPrefs(AppEnv* env) {
    int16_t vRefNum;
    int32_t dirID;
    FSSpec spec;
    OSErr err;

    /* Set default configuration */
    strcpy(env->settings.host, "127.0.0.1");
    env->settings.port = 8080;
    strcpy(env->settings.username, "admin");
    strcpy(env->settings.password, "password");
    strcpy(env->settings.fontName, "Charcoal");
    env->settings.fontSize = 11;
    env->settings.workMode = 0;
    env->settings.remoteCompileEnabled = 0;

    err = FindFolder(PREFS_VOL_REF, kPreferencesFolderType, true, &vRefNum, &dirID);
    if (err == noErr) {
        err = FSMakeFSSpec(vRefNum, dirID, "\pChatPrefs.txt", &spec);
        if (err == noErr) {
            int16_t refNum;
            err = FSpOpenDF(&spec, fsRdPerm, &refNum);
            if (err == noErr) {
                AppPrefsRecord prefs;
                long bytesToRead = sizeof(AppPrefsRecord);
                memset(&prefs, 0, sizeof(prefs));
                FSRead(refNum, &bytesToRead, &prefs);
                FSClose(refNum);

                if (bytesToRead >= (long)sizeof(AppPrefsRecord) &&
                        prefs.magic == PREFS_MAGIC &&
                        prefs.version == PREFS_VERSION &&
                        prefs.size == sizeof(ChatSettings)) {
                    env->settings = prefs.settings;
                } else if (bytesToRead >= (long)sizeof(AppPrefsRecord) - (long)sizeof(int) &&
                        prefs.magic == PREFS_MAGIC &&
                        prefs.version == 3 &&
                        prefs.size == sizeof(ChatSettings) - sizeof(int)) {
                    env->settings = prefs.settings;
                    env->settings.remoteCompileEnabled = 0;
                }
                /* else: version/size mismatch — use factory defaults set above */
            }
        }
    }

    AppNormalizeSettings(&env->settings);

    /* Set lang pointer so callers can use strings immediately */
    env->lang = &kStrings[env->settings.language];

    /* Mirror current settings into global networking namespace */
    gSettings = env->settings;
}

void AppSavePrefs(AppEnv* env) {
    int16_t vRefNum;
    int32_t dirID;
    FSSpec spec;
    OSErr err;

    /* Mirror current settings from environmental state */
    gSettings = env->settings;

    err = FindFolder(PREFS_VOL_REF, kPreferencesFolderType, true, &vRefNum, &dirID);
    if (err == noErr) {
        err = FSMakeFSSpec(vRefNum, dirID, "\pChatPrefs.txt", &spec);
        if (err == fnfErr) {
            err = FSpCreate(&spec, APP_CREATOR, 'pref', smSystemScript);
        }
        if (err == noErr || err == dupFNErr) {
            int16_t refNum;
            err = FSpOpenDF(&spec, fsWrPerm, &refNum);
            if (err == noErr) {
                AppPrefsRecord prefs;
                long bytesToWrite = sizeof(AppPrefsRecord);
                memset(&prefs, 0, sizeof(prefs));
                prefs.magic = PREFS_MAGIC;
                prefs.version = PREFS_VERSION;
                prefs.size = sizeof(ChatSettings);
                prefs.settings = env->settings;
                SetFPos(refNum, fsFromStart, 0);
                SetEOF(refNum, 0);
                FSWrite(refNum, &bytesToWrite, &prefs);
                FSClose(refNum);
            }
        }
    }
}

void AppApplyLanguage(AppEnv* env) {
    const UIStrings* L;

    if (env->settings.language < 0 || env->settings.language > 1) {
        env->settings.language = 0;
    }
    L = &kStrings[env->settings.language];
    env->lang = L;

    AppInstallLocalizedMenus(L);
    AppUpdateRemoteCompileMenu(env);
    DrawMenuBar();

    /* Update Send button title and force redraw */
    if (env->sendBtn) {
        Str255 btnTitle;
        ConvertCtoPstr(L->btnSend, btnTitle);
        SetControlTitle(env->sendBtn, btnTitle);
    }
    AppUpdateModeButton(env);
    AppRedrawSendButton(env);
    AppRedrawModeButton(env);

    if (env->filesWindow) {
        Str255 title;
        ConvertCtoPstr(L->filesWindowTitle, title);
        SetWTitle(env->filesWindow, title);
        if (env->chooseFolderBtn) {
            ConvertCtoPstr(L->btnChooseFolder, title);
            SetControlTitle(env->chooseFolderBtn, title);
        }
        if (env->cancelFolderBtn) {
            ConvertCtoPstr(L->btnCancel, title);
            SetControlTitle(env->cancelFolderBtn, title);
        }
        AppDrawFilesWindow(env);
    }

    /* Copy the status string so the window has the right initial text on cold startup */
    strncpy(env->status, L->statusReady, sizeof(env->status) - 1);
    env->status[sizeof(env->status) - 1] = '\0';

    /* Update status bar to translated "Ready" when window is live */
    if (env->window) {
        AppUpdateStatus(env, L->statusReady);
    }

    /* Refresh the welcome messages if no chat messages have been sent yet */
    if (env->window && env->transcriptTe && !env->historySessionStarted) {
        char buildBuf[128];
        char dateBuf[64];
        const char* netState = ENABLE_CLASSIC_NETWORK ? L->buildNetEnabled : L->buildNetDisabled;
        
        TESetText("", 0, env->transcriptTe);
        AppGetBuildDateString(env, dateBuf, sizeof(dateBuf));
        sprintf(buildBuf, "%s%s %s %s", L->buildPrefix, dateBuf, __TIME__, netState);
        AppAppendTextColored(env, buildBuf, &kSystemColor);
        AppAppendTextColored(env, L->welcomeMsg, &kSystemColor);
    }
}

void AppNormalizeSettings(ChatSettings* settings) {
    if (!settings->host[0]) strcpy(settings->host, "127.0.0.1");
    if (settings->port <= 0 || settings->port > 65535) settings->port = 8080;
    if (!settings->username[0]) strcpy(settings->username, "admin");
    if (!settings->password[0]) strcpy(settings->password, "password");
    if (!settings->fontName[0]) strcpy(settings->fontName, "Charcoal");
    settings->fontName[sizeof(settings->fontName) - 1] = '\0';
    if (settings->fontSize < 8 || settings->fontSize > 24) settings->fontSize = 11;
    if (settings->language < 0 || settings->language > 1) settings->language = 0;
    if (settings->workMode < 0 || settings->workMode > 1) settings->workMode = 0;
    if (settings->remoteCompileEnabled < 0 || settings->remoteCompileEnabled > 1) {
        settings->remoteCompileEnabled = 0;
    }
}

int main(void) {
    /* Initialize global state */
    memset(&gApp, 0, sizeof(AppEnv));
    strcpy(gApp.status, "Ready");

    AppInitToolbox();

    /* Load system cursor and allocate callbacks once to prevent leaks */
    gApp.ibeamCursor = GetCursor(iBeamCursor);
    gApp.okFrameUPP = NewUserItemUPP(&DialogOkFrameProc);
    gApp.aboutFrameUPP = NewUserItemUPP(&DialogOkFrameProc);
    gApp.scrollActionUPP = NewControlActionUPP(&ScrollbarAction);
    gApp.editorScrollActionUPP = NewControlActionUPP(&EditorScrollbarAction);
    gApp.filesScrollActionUPP = NewControlActionUPP(&FilesScrollbarAction);
    gApp.sendTrackActionUPP = NewControlActionUPP(&SendButtonTrackAction);

    AppLoadPrefs(&gApp);

    /* Construct Menu Bar */
    Handle menuBar = GetNewMBar(128);
    if (menuBar) {
        SetMenuBar(menuBar);
#if !TARGET_API_MAC_CARBON
        AppendResMenu(GetMenuHandle(128), 'DRVR');
#endif
        AppApplyLanguage(&gApp);  /* set menu item text to match language pref */
        DrawMenuBar();
    }

    AppCreateWindow(&gApp);

#if ENABLE_CLASSIC_NETWORK
    {
        char resp[64];
        SendClearRequest(resp, sizeof(resp));
    }
#endif

    {
        char buildBuf[128];
        char dateBuf[64];
        const char* netState = ENABLE_CLASSIC_NETWORK ? gApp.lang->buildNetEnabled : gApp.lang->buildNetDisabled;
        AppGetBuildDateString(&gApp, dateBuf, sizeof(dateBuf));
        sprintf(buildBuf, "%s%s %s %s", gApp.lang->buildPrefix, dateBuf, __TIME__, netState);
        AppAppendTextColored(&gApp, buildBuf, &kSystemColor);
    }
    AppAppendTextColored(&gApp, gApp.lang->welcomeMsg, &kSystemColor);

    /* Main Event Loop */
    EventRecord event;
    for (;;) {
        if (WaitNextEvent(everyEvent, &event, 10, nil)) {
            switch (event.what) {
                case keyDown:
                case autoKey:
                    if (gApp.isBusy) {
                        SysBeep(1);
                    } else {
                        AppHandleKeyDown(&gApp, &event);
                    }
                    break;
                case mouseDown:
                    if (gApp.isBusy) {
                        SysBeep(1);
                    } else {
                        AppHandleMouseDown(&gApp, &event);
                    }
                    break;
                case updateEvt:
                    AppHandleUpdate(&gApp, &event);
                    break;
                case activateEvt:
                    AppHandleActivate(&gApp, &event);
                    break;
            }
        } else {
            AppHandleNullEvent(&gApp);
        }
    }

    return 0;
}
