#include "common.h"

void AppDoSend(AppEnv* env) {
    int inputLen;
    Handle textH;
    char* prompt;
    char timestamp[32];
    char header[128];
    Boolean allowFullCode;
#if ENABLE_CLASSIC_NETWORK
    char* networkPrompt;
    char* response;
    char* macPayload;
    char* opsUtf8;
    char* fileReadBuffer;
    char* pendingOps;
    long err;
    long fileReadLen;
    long networkPromptLen;
    const char* payload;
    const char* pendingMsg;
    char localOpsMac[512];
    char mentionedName[64];
    char toolSummary[512];
    Boolean previewPending;
    short oldWorkMode;
#endif

    if (!env->inputTe || !env->transcriptTe) return;

    inputLen = (**env->inputTe).teLength;
    if (inputLen == 0) return;

    textH = (**env->inputTe).hText;
    if (!textH || !*textH) return;

    prompt = (char*)malloc(inputLen + 1);
    if (!prompt) return;

    HLock(textH);
    memcpy(prompt, *textH, inputLen);
    HUnlock(textH);
    prompt[inputLen] = '\0';

    /* Convert Mac OS 9 \r line endings to \n for the bridge */
    {
        int i;
        for (i = 0; i < inputLen; i++) {
            if (prompt[i] == '\r') prompt[i] = '\n';
        }
    }
    allowFullCode = AppPromptAllowsFullCode(prompt);

    AppClearInput(env);

    /* ICQ-style user message with timestamp and blue color */
    AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
    sprintf(header, "%s [%s]:", env->lang->lblYou, timestamp);
    AppAppendTextColored(env, header, &kUserColor);
    AppAppendTextColored(env, prompt, &kUserColor);

    AppBeginHistorySession(env);
    AppAppendHistoryLine(env, header);
    AppAppendHistoryLine(env, prompt);

#if !ENABLE_CLASSIC_NETWORK
    AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
    sprintf(header, "%s [%s]:", env->lang->lblSystem, timestamp);
    AppAppendTextColored(env, header, &kSystemColor);
    AppAppendTextColored(env, "Networking is disabled because this Retro68 toolchain is missing OpenTransportAppPPC.o.", &kSystemColor);
    AppAppendTextColored(env, "Install Apple Universal Interfaces/Libraries 3.x and rebuild to enable Open Transport.", &kSystemColor);
    AppAppendHistoryLine(env, header);
    AppAppendHistoryLine(env, "Networking disabled - missing OpenTransportAppPPC.o.");
    AppUpdateStatus(env, "Send OK");
    free(prompt);
    return;
#else
    localOpsMac[0] = '\0';
    mentionedName[0] = '\0';
    toolSummary[0] = '\0';
    fileReadBuffer = NULL;
    pendingOps = NULL;
    fileReadLen = 0;
    opsUtf8 = NULL;
    previewPending = false;

    if (env->hasRootFolder && (AppPromptRequestsRead(prompt) || AppPromptRequestsWrite(prompt)) &&
            AppFindMentionedProjectFile(env, prompt, mentionedName, sizeof(mentionedName))) {
        OSErr readErr = AppReadRootFile(env, mentionedName, &fileReadBuffer, &fileReadLen, 12000);
        if (readErr == noErr) {
            char line[180];
            sprintf(line, "Local file operation: read %ld bytes from %s.\n", fileReadLen, mentionedName);
            strncat(localOpsMac, line, sizeof(localOpsMac) - strlen(localOpsMac) - 1);
        } else {
            char line[180];
            sprintf(line, "Local file operation failed: could not read %s. Mac OS error %d.\n",
                mentionedName, readErr);
            strncat(localOpsMac, line, sizeof(localOpsMac) - strlen(localOpsMac) - 1);
        }
    }

    response = (char*)malloc(32768);
    macPayload = NULL;
    networkPrompt = NULL;
    payload = NULL;

    if (!response) {
        SysBeep(1);
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        sprintf(header, "%s [%s]:", env->lang->lblError, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        AppAppendTextColored(env, "Out of memory.", &kSystemColor);
        AppUpdateStatus(env, "Error");
        free(prompt);
        if (fileReadBuffer) free(fileReadBuffer);
        return;
    }
    response[0] = '\0';

    macPayload = (char*)malloc(32768);
    if (!macPayload) {
        SysBeep(1);
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        sprintf(header, "%s [%s]:", env->lang->lblError, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        AppAppendTextColored(env, "Out of memory.", &kSystemColor);
        AppUpdateStatus(env, "Error");
        free(prompt);
        if (fileReadBuffer) free(fileReadBuffer);
        free(response);
        return;
    }
    macPayload[0] = '\0';

    char* contextMac = (char*)malloc(UI_MAX_CONTEXT_BYTES);
    char* contextUtf8 = NULL;
    char* utf8Prompt = NULL;
    if (!contextMac) {
        SysBeep(1);
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        sprintf(header, "%s [%s]:", env->lang->lblError, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        AppAppendTextColored(env, "Out of memory.", &kSystemColor);
        AppUpdateStatus(env, env->lang->statusError);
        free(prompt);
        if (fileReadBuffer) free(fileReadBuffer);
        free(macPayload);
        free(response);
        return;
    }
    AppBuildProjectContext(env, contextMac, UI_MAX_CONTEXT_BYTES);
    AppAppendProjectFileContents(env, prompt, contextMac, UI_MAX_CONTEXT_BYTES);
    AppAppendContextLine(contextMac, UI_MAX_CONTEXT_BYTES, "--- User Prompt ---");

    contextUtf8 = (char*)malloc(strlen(contextMac) * 3 + 1);
    utf8Prompt = (char*)malloc(strlen(prompt) * 3 + 1);
    if (localOpsMac[0]) {
        opsUtf8 = (char*)malloc(strlen(localOpsMac) * 3 + 1);
    }
    if (!contextUtf8 || !utf8Prompt || (localOpsMac[0] && !opsUtf8)) {
        SysBeep(1);
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        sprintf(header, "%s [%s]:", env->lang->lblError, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        AppAppendTextColored(env, "Out of memory.", &kSystemColor);
        AppUpdateStatus(env, "Error");
        free(prompt);
        if (contextMac) free(contextMac);
        if (contextUtf8) free(contextUtf8);
        if (utf8Prompt) free(utf8Prompt);
        if (opsUtf8) free(opsUtf8);
        if (fileReadBuffer) free(fileReadBuffer);
        free(macPayload);
        free(response);
        return;
    }
    MacRomanToUTF8(contextMac, contextUtf8, strlen(contextMac) * 3 + 1);
    MacRomanToUTF8(prompt, utf8Prompt, strlen(prompt) * 3 + 1);
    if (opsUtf8) MacRomanToUTF8(localOpsMac, opsUtf8, strlen(localOpsMac) * 3 + 1);

    networkPromptLen = strlen(env->lang->aiLangPrefix) + strlen(contextUtf8) +
        strlen(utf8Prompt) + 2;
    if (opsUtf8) networkPromptLen += strlen(opsUtf8) + 64;
    if (fileReadBuffer) networkPromptLen += fileReadLen + strlen(mentionedName) + 96;

    networkPrompt = (char*)malloc(networkPromptLen);
    if (!networkPrompt) {
        SysBeep(1);
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        sprintf(header, "%s [%s]:", env->lang->lblError, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        AppAppendTextColored(env, "Out of memory.", &kSystemColor);
        AppUpdateStatus(env, env->lang->statusError);
        free(prompt);
        free(contextMac);
        free(contextUtf8);
        free(utf8Prompt);
        if (opsUtf8) free(opsUtf8);
        if (fileReadBuffer) free(fileReadBuffer);
        free(macPayload);
        free(response);
        return;
    }

    strcpy(networkPrompt, env->lang->aiLangPrefix);
    strcat(networkPrompt, contextUtf8);
    if (opsUtf8) {
        strcat(networkPrompt, "\n--- Local File Operations ---\n");
        strcat(networkPrompt, opsUtf8);
    }
    if (fileReadBuffer) {
        strcat(networkPrompt, "\n--- Local File Content: ");
        strcat(networkPrompt, mentionedName);
        strcat(networkPrompt, " ---\n");
        strncat(networkPrompt, fileReadBuffer, fileReadLen);
        strcat(networkPrompt, "\n--- End Local File Content ---\n");
    }
    strcat(networkPrompt, utf8Prompt);
    free(contextMac);
    free(contextUtf8);
    free(utf8Prompt);
    if (opsUtf8) free(opsUtf8);
    if (fileReadBuffer) free(fileReadBuffer);
    free(prompt);

    AppSetBusy(env, true);
    err = SendAuthenticatedPrompt(networkPrompt, response, 32768);
    free(networkPrompt);

    if (err != 0) {
        SysBeep(1);
        AppSetBusy(env, false);
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        if (err > 0) {
            payload = AppHTTPBody(response);
            UTF8ToMacRoman(payload, macPayload, 32768);
            sprintf(header, "%s [%s]:", env->lang->lblBridgeError, timestamp);
            AppAppendTextColored(env, header, &kSystemColor);
            AppAppendTextColored(env, macPayload, &kSystemColor);
            AppAppendHistoryLine(env, header);
            AppAppendHistoryLine(env, macPayload);
            AppUpdateStatus(env, env->lang->statusBridgeErr);
        } else {
            sprintf(header, "%s [%s]: %s", env->lang->lblError, timestamp, GetMacErrorMessage(err));
            AppAppendTextColored(env, header, &kSystemColor);
            AppAppendHistoryLine(env, header);
            AppUpdateStatus(env, env->lang->statusConnFail);
        }
        free(response);
        free(macPayload);
        return;
    }

    /* ICQ-style AI response with timestamp and green color */
    payload = AppHTTPBody(response);
    UTF8ToMacRoman(payload, macPayload, 32768);
    if (env->settings.workMode == 1 && AppHasEditToolBlock(macPayload)) {
        pendingOps = AppCopySizedText(macPayload, strlen(macPayload));
        AppClearPendingAIChanges(env);
        oldWorkMode = env->settings.workMode;
        env->settings.workMode = 0;
        AppExecuteAIFileOps(env, macPayload, toolSummary, sizeof(toolSummary));
        env->settings.workMode = oldWorkMode;
        if (pendingOps && AppToolSummaryHasPreview(toolSummary) &&
                !AppToolSummaryHasFailure(toolSummary)) {
            AppStorePendingAIChanges(env, pendingOps, toolSummary);
            free(pendingOps);
            pendingOps = NULL;
            previewPending = true;
            pendingMsg = env->settings.language == 1 ?
                "Bekleyen AI de\xDBi\xDFiklikleri var. Onaylamak i\x8Din tik, reddetmek i\x8Din X.\n" :
                "Pending AI changes. Use the tick to apply or X to reject.\n";
            strncat(toolSummary, pendingMsg, sizeof(toolSummary) - strlen(toolSummary) - 1);
        } else {
            if (pendingOps) {
                free(pendingOps);
                pendingOps = NULL;
            }
            strncat(toolSummary, "No pending change stored.\n",
                sizeof(toolSummary) - strlen(toolSummary) - 1);
        }
    } else {
        AppExecuteAIFileOps(env, macPayload, toolSummary, sizeof(toolSummary));
        if (env->settings.workMode == 0 && toolSummary[0]) {
            strncat(toolSummary,
                env->settings.language == 1 ?
                    "Plan modunda aksiyon alamaz ve de\xDBi\xDFiklik yapamazs\xDDn\xDDz. Kodla moduna ge\x8D" "erek aksiyon alabilirsin.\n" :
                    "Plan mode is preview only. Switch to Build mode to approve changes.\n",
                sizeof(toolSummary) - strlen(toolSummary) - 1);
        }
    }
    if (!toolSummary[0] && AppMentionsToolWithoutBlock(macPayload)) {
        strncat(toolSummary, "No file change: malformed or missing ALTANAI tool block.\n",
            sizeof(toolSummary) - strlen(toolSummary) - 1);
    }
    if (toolSummary[0]) AppShortenToolResponse(macPayload);
    else if (!allowFullCode) AppShortenLongResponse(macPayload);
    AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
    sprintf(header, "%s [%s]:", env->lang->lblAI, timestamp);
    AppAppendTextColored(env, header, &kAIColor);
    AppAppendTextColored(env, macPayload, &kAIColor);
    if (toolSummary[0]) AppAppendFileChangeSummaryColored(env, toolSummary);
    AppAppendHistoryLine(env, header);
    AppAppendHistoryLine(env, macPayload);
    if (toolSummary[0]) AppAppendHistoryLine(env, toolSummary);
    if (previewPending) {
        AppUpdateStatus(env, env->settings.language == 1 ?
            "Bekleyen AI de\xDBi\xDFiklikleri var" : "Pending AI changes");
        env->isBusy = false;
        InitCursor();
    } else {
        AppSetBusy(env, false);
    }

    free(response);
    free(macPayload);
#endif
}

void AppSendSyntheticPrompt(AppEnv* env, const char* prompt) {
    long len;

    if (!env || !env->inputTe || !prompt || !prompt[0]) return;
    len = strlen(prompt);
    TESetText(prompt, len, env->inputTe);
    TESetSelect(len, len, env->inputTe);
    TECalText(env->inputTe);
    env->activeTe = env->inputTe;
    AppDoSend(env);
}

void AppDoCreateStarterProject(AppEnv* env) {
    char appName[32];
    char creator[5];
    char* cmakeText;
    char* mainText;
    char* rsrcText;
    char summary[256];
    char timestamp[32];
    char header[128];
    OSErr err;

    if (!env || !env->hasRootFolder) {
        AppUpdateStatus(env, env ? env->lang->folderNone : "No folder selected");
        return;
    }

    if (AppRootFileExists(env, "CMakeLists.txt") ||
            AppRootFileExists(env, "src/main.c") ||
            AppRootFileExists(env, "rsrc/App.r")) {
        AppUpdateStatus(env, env->settings.language == 1 ?
            "Proje dosyalari zaten var" : "Starter project files already exist");
        return;
    }

    if (!AppPromptProjectName(env, appName, sizeof(appName))) {
        AppUpdateStatus(env, env->settings.language == 1 ?
            "Proje olusturma iptal edildi" : "Project creation canceled");
        return;
    }

    cmakeText = (char*)malloc(1600);
    mainText = (char*)malloc(3600);
    rsrcText = (char*)malloc(1600);
    if (!cmakeText || !mainText || !rsrcText) {
        if (cmakeText) free(cmakeText);
        if (mainText) free(mainText);
        if (rsrcText) free(rsrcText);
        AppUpdateStatus(env, env->lang->statusError);
        return;
    }

    AppCreatorFromProjectName(appName, creator);

    sprintf(cmakeText,
        "cmake_minimum_required(VERSION 3.10)\n"
        "project(%s C)\n"
        "\n"
        "set(CMAKE_C_STANDARD 90)\n"
        "set(CMAKE_C_FLAGS \"${CMAKE_C_FLAGS} -Wno-multichar\")\n"
        "\n"
        "add_application(%s\n"
        "    TYPE APPL\n"
        "    CREATOR %s\n"
        "    src/main.c\n"
        "    rsrc/App.r\n"
        ")\n"
        "\n"
        "target_link_libraries(%s\n"
        "    InterfaceLib\n"
        "    retrocrt\n"
        ")\n",
        appName, appName, creator, appName);

    sprintf(mainText,
        "#include <MacTypes.h>\n"
        "#include <Quickdraw.h>\n"
        "#include <Fonts.h>\n"
        "#include <Windows.h>\n"
        "#include <Dialogs.h>\n"
        "#include <Events.h>\n"
        "#include <Menus.h>\n"
        "#include <ToolUtils.h>\n"
        "\n"
        "static short gClicks = 0;\n"
        "\n"
        "static void DrawMainWindow(WindowPtr window) {\n"
        "    Rect content;\n"
        "\n"
        "    SetPort(window);\n"
        "    SetRect(&content, 0, 0, 340, 180);\n"
        "    EraseRect(&content);\n"
        "    MoveTo(24, 64);\n"
        "    if (gClicks == 0) {\n"
        "        DrawString(\"\\pHello World (Click window to change text)\");\n"
        "    } else {\n"
        "        DrawString(\"\\pHahaha\");\n"
        "    }\n"
        "}\n"
        "\n"
        "int main(void) {\n"
        "    Rect bounds;\n"
        "    Rect dirtyRect;\n"
        "    WindowPtr window;\n"
        "    WindowPtr whichWindow;\n"
        "    EventRecord event;\n"
        "    short part;\n"
        "\n"
        "    MaxApplZone();\n"
        "#if !TARGET_API_MAC_CARBON\n"
        "    InitGraf(&qd.thePort);\n"
        "    InitFonts();\n"
        "    InitWindows();\n"
        "    InitMenus();\n"
        "    TEInit();\n"
        "    InitDialogs(NULL);\n"
        "#endif\n"
        "    InitCursor();\n"
        "\n"
        "    SetRect(&bounds, 80, 80, 260, 420);\n"
        "    window = NewCWindow(NULL, &bounds, \"\\p%s\", true, documentProc,\n"
        "        (WindowPtr)-1L, true, 0L);\n"
        "\n"
        "    while (1) {\n"
        "        if (WaitNextEvent(everyEvent, &event, 30, NULL)) {\n"
        "            if (event.what == mouseDown) {\n"
        "                part = FindWindow(event.where, &whichWindow);\n"
        "                if (part == inGoAway && whichWindow == window) {\n"
        "                    if (TrackGoAway(window, event.where)) ExitToShell();\n"
        "                } else if (part == inContent && whichWindow == window) {\n"
        "                    SelectWindow(window);\n"
        "                    SetPort(window);\n"
        "                    gClicks++;\n"
        "                    SetRect(&dirtyRect, 0, 0, 340, 180);\n"
        "                    InvalRect(&dirtyRect);\n"
        "                }\n"
        "            } else if (event.what == updateEvt) {\n"
        "                BeginUpdate((WindowPtr)event.message);\n"
        "                DrawMainWindow((WindowPtr)event.message);\n"
        "                EndUpdate((WindowPtr)event.message);\n"
        "            }\n"
        "        }\n"
        "    }\n"
        "    return 0;\n"
        "}\n",
        appName);

    strcpy(rsrcText,
        "#include \"Types.r\"\n"
        "#include \"Processes.r\"\n"
        "\n"
        "resource 'SIZE' (-1) {\n"
        "    reserved,\n"
        "    acceptSuspendResumeEvents,\n"
        "    reserved,\n"
        "    canBackground,\n"
        "    doesActivateOnFGSwitch,\n"
        "    backgroundAndForeground,\n"
        "    dontGetFrontClicks,\n"
        "    ignoreChildDiedEvents,\n"
        "    is32BitCompatible,\n"
        "    notHighLevelEventAware,\n"
        "    localAndRemoteHLEvents,\n"
        "    notStationeryAware,\n"
        "    dontUseTextEditServices,\n"
        "    reserved,\n"
        "    reserved,\n"
        "    reserved,\n"
        "    512 * 1024,\n"
        "    512 * 1024\n"
        "};\n");

    err = AppWriteRootFile(env, "CMakeLists.txt", cmakeText);
    if (err == noErr) err = AppWriteRootFile(env, "src/main.c", mainText);
    if (err == noErr) err = AppWriteRootFile(env, "rsrc/App.r", rsrcText);

    if (err == noErr) {
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        sprintf(header, "%s [%s]:", env->lang->lblSystem, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        AppAppendTextColored(env, env->settings.language == 1 ?
            "Yeni Mac OS 9 projesi olusturuldu." :
            "Created starter Mac OS 9 project.", &kSystemColor);
        summary[0] = '\0';
        AppAppendFileStatLine(summary, sizeof(summary), "CMakeLists.txt",
            AppCountTextLines(cmakeText, strlen(cmakeText)), 0);
        AppAppendFileStatLine(summary, sizeof(summary), "src/main.c",
            AppCountTextLines(mainText, strlen(mainText)), 0);
        AppAppendFileStatLine(summary, sizeof(summary), "rsrc/App.r",
            AppCountTextLines(rsrcText, strlen(rsrcText)), 0);
        AppAppendFileChangeSummaryColored(env, summary);
        AppAppendHistoryLine(env, header);
        AppAppendHistoryLine(env, "Created starter Mac OS 9 project.");
        AppAppendHistoryLine(env, summary);
        AppUpdateStatus(env, env->lang->statusReady);
        if (env->filesWindow) AppDrawFilesWindow(env);
    } else {
        AppUpdateStatus(env, env->settings.language == 1 ?
            "Proje olusturulamadi" : "Could not create starter project");
    }

    free(cmakeText);
    free(mainText);
    free(rsrcText);
}

void AppDoBuildProject(AppEnv* env) {
    if (!env || !env->hasRootFolder) {
        AppUpdateStatus(env, env->lang->folderNone);
        return;
    }
    if (!env->settings.remoteCompileEnabled) {
        AppUpdateStatus(env, env->settings.language == 1 ?
            "Uzak derleme kapal\xDD" : "Remote compile is off");
        return;
    }

#if ENABLE_CLASSIC_NETWORK
    {
        char* response;
        char* macPayload;
        char* buildBody;
        char toolSummary[512];
        char timestamp[32];
        char header[128];
        const char* payload;
        long err;

        response = (char*)malloc(32768);
        macPayload = (char*)malloc(32768);
        buildBody = (char*)malloc(UI_MAX_BUILD_SNAPSHOT_BYTES);
        if (!response || !macPayload || !buildBody) {
            SysBeep(1);
            AppUpdateStatus(env, env->lang->statusError);
            if (response) free(response);
            if (macPayload) free(macPayload);
            if (buildBody) free(buildBody);
            return;
        }
        AppSetBusy(env, true);
        response[0] = '\0';
        macPayload[0] = '\0';
        buildBody[0] = '\0';
        toolSummary[0] = '\0';
        AppBuildProjectSnapshot(env, buildBody, UI_MAX_BUILD_SNAPSHOT_BYTES);

        err = SendBuildRequest(buildBody[0] ? buildBody : env->rootName, response, 32768);
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        if (err != 0) {
            AppSetBusy(env, false);
            if (err > 0) {
                payload = AppHTTPBody(response);
                UTF8ToMacRoman(payload, macPayload, 32768);
                sprintf(header, "%s [%s]:", env->lang->lblBridgeError, timestamp);
                AppAppendTextColored(env, header, &kSystemColor);
                AppAppendTextColored(env, macPayload, &kSystemColor);
                AppAppendHistoryLine(env, header);
                AppAppendHistoryLine(env, macPayload);
                AppUpdateStatus(env, env->lang->statusBridgeErr);
            } else {
                sprintf(header, "%s [%s]: %s", env->lang->lblError, timestamp, GetMacErrorMessage(err));
                AppAppendTextColored(env, header, &kSystemColor);
                AppAppendHistoryLine(env, header);
                AppUpdateStatus(env, env->lang->statusConnFail);
            }
            free(response);
            free(macPayload);
            free(buildBody);
            return;
        }

        payload = AppHTTPBody(response);
        UTF8ToMacRoman(payload, macPayload, 32768);
        AppExecuteAIFileOps(env, macPayload, toolSummary, sizeof(toolSummary));
        if (toolSummary[0]) AppShortenToolResponse(macPayload);
        else AppShortenLongResponse(macPayload);

        sprintf(header, "%s [%s]:", env->lang->lblSystem, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        if (macPayload[0]) {
            AppAppendTextColored(env, macPayload, &kSystemColor);
            AppAppendHistoryLine(env, header);
            AppAppendHistoryLine(env, macPayload);
        } else {
            AppAppendTextColored(env, env->lang->statusBuildStarted, &kSystemColor);
            AppAppendHistoryLine(env, header);
            AppAppendHistoryLine(env, env->lang->statusBuildStarted);
        }
        if (toolSummary[0]) {
            AppAppendFileChangeSummaryColored(env, toolSummary);
            AppAppendHistoryLine(env, toolSummary);
        }
        AppSetBusy(env, false);

        free(response);
        free(macPayload);
        free(buildBody);
    }
#else
    {
        char timestamp[32];
        char header[128];
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        sprintf(header, "%s [%s]:", env->lang->lblSystem, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        AppAppendTextColored(env,
            "Build requires Classic networking and the bridge /api/build endpoint.",
            &kSystemColor);
        AppAppendHistoryLine(env, header);
        AppAppendHistoryLine(env,
            "Build requires Classic networking and the bridge /api/build endpoint.");
        AppUpdateStatus(env, env->lang->statusConnFail);
    }
#endif
}

void AppDoFixBuild(AppEnv* env) {
    const char* fixPromptEn =
        "Fix the last build errors using the attached build result. Prefer PATCH. Do not show full code.";
    const char* fixPromptTr =
        "Son derleme hatalarini ekli derleme sonucuna gore duzelt. PATCH tercih et. Tam kod gosterme.";
    const char* appPromptEn =
        "The last build succeeded but produced no downloadable Mac OS app artifact. Convert this project to a Retro68 APPL app that creates .dsk/.bin/.APPL outputs. Prefer PATCH. Do not show full code.";
    const char* appPromptTr =
        "Son derleme basarili ama indirilebilir Mac OS uygulama cikarmadi. Projeyi .dsk/.bin/.APPL uretecek Retro68 APPL uygulamasina cevir. PATCH tercih et. Tam kod gosterme.";
    const char* prompt;

    if (!env || !env->hasRootFolder) {
        AppUpdateStatus(env, env ? env->lang->folderNone : "No folder selected");
        return;
    }
    if (!env->lastBuildResult[0]) {
        AppUpdateStatus(env, "No build result to fix");
        return;
    }

    if (AppContainsASCIIInsensitive(env->lastBuildResult, "status: ok") &&
            (AppContainsASCIIInsensitive(env->lastBuildResult, "output: none") ||
             AppContainsASCIIInsensitive(env->lastBuildResult, "No downloadable Mac OS artifact") ||
             AppContainsASCIIInsensitive(env->lastBuildResult, "No Mac app artifact"))) {
        prompt = env->settings.language == 1 ? appPromptTr : appPromptEn;
    } else {
        prompt = env->settings.language == 1 ? fixPromptTr : fixPromptEn;
    }

    env->settings.workMode = 1;
    AppNormalizeSettings(&env->settings);
    AppUpdateModeButton(env);
    AppSavePrefs(env);
    AppUpdateStatus(env, env->lang->statusWaiting);
    AppSendSyntheticPrompt(env, prompt);
}

void AppDoShowArtifact(AppEnv* env) {
    char encoded[260];
    char url[420];
    char timestamp[32];
    char header[128];

    if (!env || !env->lastArtifactPath[0]) {
        AppUpdateStatus(env, env && env->settings.language == 1 ?
            "Derleme ciktisi yok" : "No build artifact yet");
        return;
    }

    AppEncodeURLPath(env->lastArtifactPath, encoded, sizeof(encoded));
    sprintf(url, "Artifact: %s\nDownload: http://%s:%d/api/artifact?path=%s",
        env->lastArtifactPath, env->settings.host, env->settings.port, encoded);
    AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
    sprintf(header, "%s [%s]:", env->lang->lblSystem, timestamp);
    AppAppendTextColored(env, header, &kSystemColor);
    AppAppendTextColored(env, url, &kSystemColor);
    AppAppendHistoryLine(env, header);
    AppAppendHistoryLine(env, url);
    AppUpdateStatus(env, env->lang->statusReady);
}
