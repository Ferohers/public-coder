#include "common.h"

void AppHandleMouseDown(AppEnv* env, EventRecord* event) {
    if (!env->transcriptTe || !env->inputTe) return;

    WindowPtr targetWindow;
    short part = FindWindow(event->where, &targetWindow);

    if (part == inMenuBar) {
        long choice = MenuSelect(event->where);
        if (choice != 0) {
            AppHandleMenuChoice(env, choice);
        } else {
            HiliteMenu(0);
        }
        return;
    }

    if (targetWindow == env->filesWindow) {
        if (part == inGoAway) {
            if (TrackGoAway(targetWindow, event->where)) {
                HideWindow(targetWindow);
            }
        } else if (part == inDrag) {
            DragWindow(targetWindow, event->where, &qd.screenBits.bounds);
        } else if (part == inGrow) {
            Rect boundaries = { 200, 200, 32767, 32767 };
            long size = GrowWindow(targetWindow, event->where, &boundaries);
            if (size != 0) {
                Rect bounds;
                SizeWindow(targetWindow, LoWord(size), HiWord(size), true);
                AppResizeFilesWidgets(env);
                AppClampFilesScroll(env);
                SetPort(targetWindow);
                GetWindowPortBounds(targetWindow, &bounds);
                InvalRect(&bounds);
            }
        } else if (part == inContent) {
            SelectWindow(targetWindow);
            {
                Point localPt = event->where;
                ControlHandle control;
                short ctrlPart;
                SetPort(targetWindow);
                GlobalToLocal(&localPt);
                ctrlPart = FindControl(localPt, targetWindow, &control);
                if (ctrlPart) {
                    if (control == env->chooseFolderBtn) {
                        if (TrackControl(env->chooseFolderBtn, localPt, nil)) {
                            AppChooseFolder(env);
                        }
                    } else if (control == env->cancelFolderBtn) {
                        if (TrackControl(env->cancelFolderBtn, localPt, nil)) {
                            AppClearProjectFolder(env);
                        }
                    } else if (control == env->filesScrollbar) {
                        if (ctrlPart == kControlIndicatorPart) {
                            if (TrackControl(env->filesScrollbar, localPt, nil)) {
                                env->filesScrollOffset = GetControlValue(env->filesScrollbar);
                                AppClampFilesScroll(env);
                                AppDrawFilesList(env);
                                Draw1Control(env->filesScrollbar);
                            }
                        } else {
                            TrackControl(env->filesScrollbar, localPt, env->filesScrollActionUPP);
                        }
                    }
                } else {
                    Rect portRect;
                    Rect listRect;
                    Rect rowRect;
                    short y;
                    short i;
                    short rowHeight = 20;
                    short visibleRows;
                    short drawnRows;

                    GetWindowPortBounds(env->filesWindow, &portRect);
                    SetRect(&listRect, 0, 34, portRect.right - UI_SCROLL_WIDTH, portRect.bottom - UI_SCROLL_WIDTH);
                    if (listRect.bottom < listRect.top + 40) listRect.bottom = listRect.top + 40;

                    y = listRect.top;
                    visibleRows = AppVisibleFileRows(env);
                    drawnRows = 0;
                    for (i = env->filesScrollOffset; i < env->projectItemCount && drawnRows < visibleRows; i++) {
                        SetRect(&rowRect, listRect.left, y, listRect.right, y + rowHeight);
                        if (PtInRect(localPt, &rowRect)) {
                            if (env->projectItems[i].isFolder) {
                                AppUpdateStatus(env, env->projectItems[i].name);
                            } else {
                                AppOpenEditorFile(env, env->projectItems[i].name);
                            }
                            break;
                        }
                        y += rowHeight;
                        drawnRows++;
                    }
                }
            }
        }
        return;
    }

    if (targetWindow == env->editorWindow) {
        if (part == inGoAway) {
            if (TrackGoAway(targetWindow, event->where)) {
                HideWindow(targetWindow);
            }
        } else if (part == inDrag) {
            DragWindow(targetWindow, event->where, &qd.screenBits.bounds);
        } else if (part == inGrow) {
            Rect boundaries = { 240, 300, 32767, 32767 };
            long size = GrowWindow(targetWindow, event->where, &boundaries);
            if (size != 0) {
                Rect bounds;
                SizeWindow(targetWindow, LoWord(size), HiWord(size), true);
                AppResizeEditorWidgets(env);
                SetPort(targetWindow);
                GetWindowPortBounds(targetWindow, &bounds);
                InvalRect(&bounds);
            }
        } else if (part == inContent) {
            Point localPt = event->where;
            ControlHandle control;
            short ctrlPart;
            SelectWindow(targetWindow);
            SetPort(targetWindow);
            GlobalToLocal(&localPt);
            ctrlPart = FindControl(localPt, targetWindow, &control);
            if (ctrlPart) {
                if (control == env->editorSaveBtn) {
                    if (TrackControl(control, localPt, nil)) {
                        AppDrawEditorToolButtons(env);
                        AppSaveEditorFile(env);
                    }
                    AppDrawEditorToolButtons(env);
                } else if (control == env->editorSearchBtn) {
                    if (TrackControl(control, localPt, nil)) {
                        AppDrawEditorToolButtons(env);
                        AppFindInEditor(env);
                    }
                    AppDrawEditorToolButtons(env);
                } else if (control == env->editorGoLineBtn) {
                    if (TrackControl(control, localPt, nil)) {
                        AppDrawEditorToolButtons(env);
                        AppGoToEditorLine(env);
                    }
                    AppDrawEditorToolButtons(env);
                } else if (control == env->editorNextBtn) {
                    if (TrackControl(control, localPt, nil)) {
                        AppDrawEditorToolButtons(env);
                        AppFindNextInEditor(env);
                    }
                    AppDrawEditorToolButtons(env);
                } else if (control == env->editorScrollbar) {
                    if (ctrlPart == kControlIndicatorPart) {
                        if (TrackControl(env->editorScrollbar, localPt, nil)) {
                            AppScrollEditorTo(env, GetControlValue(env->editorScrollbar));
                        }
                    } else {
                        TrackControl(env->editorScrollbar, localPt, env->editorScrollActionUPP);
                    }
                }
            } else {
                Rect viewRect;
                Rect arrowRect;
                short i;
                Boolean isShift = (event->modifiers & shiftKey) != 0;

                if (env->editorTabCount > AppEditorVisibleTabCapacity(env)) {
                    AppEditorLeftArrowRect(env, &arrowRect);
                    if (PtInRect(localPt, &arrowRect)) {
                        env->editorTabScroll--;
                        AppClampEditorTabScroll(env);
                        AppDrawEditorWindow(env);
                        return;
                    }
                    AppEditorRightArrowRect(env, &arrowRect);
                    if (PtInRect(localPt, &arrowRect)) {
                        env->editorTabScroll++;
                        AppClampEditorTabScroll(env);
                        AppDrawEditorWindow(env);
                        return;
                    }
                }

                for (i = 0; i < env->editorTabCount; i++) {
                    Rect tabRect;
                    Rect closeRect;
                    AppEditorTabRect(env, i, &tabRect);
                    if (tabRect.right <= tabRect.left) continue;
                    tabRect.top -= 1;
                    tabRect.bottom += 2;
                    if (PtInRect(localPt, &tabRect)) {
                        AppEditorTabCloseRect(&tabRect, &closeRect);
                        if (PtInRect(localPt, &closeRect)) {
                            AppCloseEditorTab(env, i);
                            return;
                        }
                        if (env->activeEditorTab >= 0 &&
                                env->editorTabs[env->activeEditorTab].dirty) {
                            AppUpdateStatus(env, "Save current tab before switching");
                        } else {
                            AppLoadEditorTab(env, i);
                        }
                        return;
                    }
                }

                viewRect = (**env->editorTe).viewRect;
                if (PtInRect(localPt, &viewRect)) {
                    if (env->activeTe != env->editorTe) {
                        if (env->activeTe) TEDeactivate(env->activeTe);
                        TEActivate(env->editorTe);
                        env->activeTe = env->editorTe;
                    }
                    TEClick(localPt, isShift, env->editorTe);
                }
            }
        }
        return;
    }

    if (targetWindow != env->window) return;

    if (part == inGoAway) {
        if (TrackGoAway(env->window, event->where)) {
            AppQuit(env);
        }
    } else if (part == inDrag) {
        DragWindow(env->window, event->where, &qd.screenBits.bounds);
    } else if (part == inGrow) {
        Rect boundaries = { UI_MIN_HEIGHT, UI_MIN_WIDTH, 32767, 32767 };
        long size = GrowWindow(env->window, event->where, &boundaries);
        if (size != 0) {
            short w = LoWord(size);
            short h = HiWord(size);
            SizeWindow(env->window, w, h, true);
            AppResizeWidgets(env, w, h);
            
            SetPort(env->window);
            Rect bounds;
            GetWindowPortBounds(env->window, &bounds);
            InvalRect(&bounds);
        }
    } else if (part == inContent) {
        SelectWindow(env->window);
        Point localPt = event->where;
        GlobalToLocal(&localPt);

        ControlHandle control;
        short ctrlPart = FindControl(localPt, env->window, &control);
        if (ctrlPart) {
            if (control == env->sendBtn) {
                if (TrackControl(env->sendBtn, localPt, env->sendTrackActionUPP)) {
                    AppRedrawSendButton(env);
                    AppDoSend(env);
                }
                AppRedrawSendButton(env);
            } else if (control == env->modeBtn) {
                if (TrackControl(env->modeBtn, localPt, nil)) {
                    env->settings.workMode = env->settings.workMode ? 0 : 1;
                    AppNormalizeSettings(&env->settings);
                    AppUpdateModeButton(env);
                    AppSavePrefs(env);
                    AppUpdateStatus(env, env->settings.workMode ? env->lang->statusBuildMode : env->lang->statusPlanMode);
                    AppRedrawModeButton(env);
                }
            } else if (control == env->clearBtn) {
                if (TrackControl(env->clearBtn, localPt, nil)) {
                    AppClearTranscript(env);
                    AppUpdateStatus(env, env->lang->statusCleared);
                }
            } else if (control == env->historyBtn) {
                if (TrackControl(env->historyBtn, localPt, nil)) {
                    AppShowHistory(env);
                }
            } else if (control == env->settingsBtn) {
                if (TrackControl(env->settingsBtn, localPt, nil)) {
                    AppModalSettings(env);
                }
            } else if (control == env->applyPendingBtn) {
                if (TrackControl(env->applyPendingBtn, localPt, nil)) {
                    AppApplyPendingAIChanges(env);
                }
            } else if (control == env->rejectPendingBtn) {
                if (TrackControl(env->rejectPendingBtn, localPt, nil)) {
                    AppRejectPendingAIChanges(env);
                }
            } else if (control == env->scrollbar) {
                if (ctrlPart == kControlIndicatorPart) {
                    if (TrackControl(env->scrollbar, localPt, nil)) {
                        int pos = GetControlValue(env->scrollbar);
                        ScrollTranscriptTo(pos);
                    }
                } else {
                    TrackControl(env->scrollbar, localPt, env->scrollActionUPP);
                }
            }
        } else {
            Rect transRect = (**env->transcriptTe).viewRect;
            Rect inputRect = (**env->inputTe).viewRect;
            Boolean isShift = (event->modifiers & shiftKey) != 0;

            if (PtInRect(localPt, &transRect)) {
                short pos;
                if (env->activeTe != env->transcriptTe) {
                    TEDeactivate(env->inputTe);
                    TEActivate(env->transcriptTe);
                    env->activeTe = env->transcriptTe;
                }
                TEClick(localPt, isShift, env->transcriptTe);

                pos = (**env->transcriptTe).selStart;
                if (pos >= 0 && pos <= (**env->transcriptTe).teLength) {
                    short lineNum = 0;
                    while (lineNum < (**env->transcriptTe).nLines - 1 &&
                           (**env->transcriptTe).lineStarts[lineNum + 1] <= pos) {
                        lineNum++;
                    }
                    if (lineNum < (**env->transcriptTe).nLines) {
                        long startOffset = (**env->transcriptTe).lineStarts[lineNum];
                        long endOffset = (lineNum < (**env->transcriptTe).nLines - 1) ?
                            (**env->transcriptTe).lineStarts[lineNum + 1] : (**env->transcriptTe).teLength;
                        long lineLen = endOffset - startOffset;
                        if (lineLen > 0 && lineLen < 512) {
                            char lineText[512];
                            char* urlPtr;
                            HLock((**env->transcriptTe).hText);
                            BlockMoveData(*((**env->transcriptTe).hText) + startOffset, lineText, lineLen);
                            HUnlock((**env->transcriptTe).hText);
                            lineText[lineLen] = '\0';

                            /* Search for a URL or download link in this line */
                            urlPtr = strstr(lineText, "http://");
                            if (!urlPtr) urlPtr = strstr(lineText, "https://");
                            if (!urlPtr) {
                                char* apiPtr = strstr(lineText, "/api/artifact");
                                if (apiPtr) {
                                    char fullURL[512];
                                    char portStr[16];
                                    char* nl = strchr(apiPtr, '\n');
                                    if (nl) *nl = '\0';
                                    nl = strchr(apiPtr, '\r');
                                    if (nl) *nl = '\0';

                                    sprintf(portStr, "%d", env->settings.port);
                                    if (env->settings.username[0]) {
                                        sprintf(fullURL, "http://%s:%s@%s:%s%s", 
                                            env->settings.username, env->settings.password,
                                            env->settings.host, portStr, apiPtr);
                                    } else {
                                        sprintf(fullURL, "http://%s:%s%s", env->settings.host, portStr, apiPtr);
                                    }
                                    AppUpdateStatus(env, env->settings.language == 1 ? "Taray\xDD" "c\xDD a\x8D\xDDl\xDDyor..." : "Opening browser...");
                                    AppLaunchURL(env, fullURL);
                                }
                            } else {
                                char url[360];
                                long u = 0;
                                while (*urlPtr && *urlPtr != ' ' && *urlPtr != '\r' && 
                                       *urlPtr != '\n' && u < sizeof(url) - 1) {
                                    url[u++] = *urlPtr++;
                                }
                                url[u] = '\0';
                                AppUpdateStatus(env, env->settings.language == 1 ? "Taray\xDD" "c\xDD a\x8D\xDDl\xDDyor..." : "Opening browser...");
                                AppLaunchURL(env, url);
                            }
                        }
                    }
                }
            } else if (PtInRect(localPt, &inputRect)) {
                if (env->activeTe != env->inputTe) {
                    TEDeactivate(env->transcriptTe);
                    TEActivate(env->inputTe);
                    env->activeTe = env->inputTe;
                }
                TEClick(localPt, isShift, env->inputTe);
            }
        }
    }
}

void AppHandleKeyDown(AppEnv* env, EventRecord* event) {
    char key = event->message & charCodeMask;
    
    if (event->modifiers & cmdKey) {
        long choice = MenuKey(key);
        if (choice != 0) {
            AppHandleMenuChoice(env, choice);
        } else if (env->activeTe) {
            char lowerKey = key;
            if (lowerKey >= 'A' && lowerKey <= 'Z') lowerKey += ('a' - 'A');
            if (lowerKey == 'x' && (env->activeTe == env->inputTe || env->activeTe == env->editorTe)) {
                TECut(env->activeTe);
                TEToScrap();
                TECalText(env->activeTe);
                if (env->activeTe == env->editorTe) {
                    AppMarkEditorDirty(env);
                    AppAdjustEditorScrollbar(env);
                    AppDrawEditorLineNumbers(env);
                }
                TEUpdate(&(**env->activeTe).viewRect, env->activeTe);
            } else if (lowerKey == 'c') {
                TECopy(env->activeTe);
                TEToScrap();
            } else if (lowerKey == 'v' && (env->activeTe == env->inputTe || env->activeTe == env->editorTe)) {
                TEFromScrap();
                TEPaste(env->activeTe);
                TECalText(env->activeTe);
                if (env->activeTe == env->editorTe) {
                    AppMarkEditorDirty(env);
                    AppAdjustEditorScrollbar(env);
                    AppDrawEditorLineNumbers(env);
                }
                TEUpdate(&(**env->activeTe).viewRect, env->activeTe);
            } else if (lowerKey == 'a') {
                TESetSelect(0, (**env->activeTe).teLength, env->activeTe);
                TEUpdate(&(**env->activeTe).viewRect, env->activeTe);
            }
        }
        return;
    }

    if (key == 0x0D || key == 0x03) { /* Return/Enter */
        if (env->activeTe == env->inputTe) {
            AppDoSend(env);
        } else if (env->activeTe == env->editorTe) {
            TEKey(0x0D, env->editorTe);
            AppMarkEditorDirty(env);
            AppAdjustEditorScrollbar(env);
            TEUpdate(&(**env->editorTe).viewRect, env->editorTe);
            AppDrawEditorLineNumbers(env);
        }
        return;
    }

    if (key == 0x09) { /* Tab Switch */
        if (env->activeTe == env->inputTe) {
            TEDeactivate(env->inputTe);
            TEActivate(env->transcriptTe);
            env->activeTe = env->transcriptTe;
        } else {
            TEDeactivate(env->transcriptTe);
            TEActivate(env->inputTe);
            env->activeTe = env->inputTe;
        }
        return;
    }

    if (env->activeTe == env->inputTe || env->activeTe == env->editorTe) {
        TEKey(key, env->activeTe);
        if (env->activeTe == env->editorTe) {
            AppMarkEditorDirty(env);
            AppAdjustEditorScrollbar(env);
            AppDrawEditorLineNumbers(env);
        }
        TEUpdate(&(**env->activeTe).viewRect, env->activeTe);
    }
}

void AppHandleUpdate(AppEnv* env, EventRecord* event) {
    if ((WindowPtr)event->message == env->filesWindow) {
        WindowPtr window = (WindowPtr)event->message;
        BeginUpdate(window);
        AppDrawProjectWindow(env, window);
        EndUpdate(window);
        return;
    }

    if ((WindowPtr)event->message == env->editorWindow) {
        BeginUpdate(env->editorWindow);
        AppDrawEditorWindow(env);
        EndUpdate(env->editorWindow);
        if (env->activeTe == env->editorTe && IsWindowHilited(env->editorWindow)) {
            TEActivate(env->editorTe);
        }
        return;
    }

    if ((WindowPtr)event->message != env->window) return;

    BeginUpdate(env->window);
    SetPort(env->window);

    Rect bounds;
    GetWindowPortBounds(env->window, &bounds);
    EraseRect(&bounds);
    /* Draw themed header background before drawing controls */
    {
        Rect headerRect = bounds;
        headerRect.top = 0;
        headerRect.bottom = 32;
        DrawThemeWindowHeader(&headerRect, IsWindowHilited(env->window) ? kThemeStateActive : kThemeStateInactive);
    }

    if (env->transcriptTe) {
        TEUpdate(&(**env->transcriptTe).viewRect, env->transcriptTe);
    }
    if (env->inputTe) {
        TEUpdate(&(**env->inputTe).viewRect, env->inputTe);
    }

    DrawControls(env->window);
    AppDrawDecorations(env);
    if (env->activeTe && IsWindowHilited(env->window)) {
        TEActivate(env->activeTe);
    }

    EndUpdate(env->window);
}

void AppHandleNullEvent(AppEnv* env) {
    /* lastHovered tracks which toolbar button the mouse is over to avoid
       calling HMShowBalloon on every null event (fires ~60/sec). */
    static ControlHandle lastHovered = NULL;

    if (env->activeTe == env->editorTe && env->editorWindow && IsWindowHilited(env->editorWindow)) {
        SetPort(env->editorWindow);
        TEIdle(env->activeTe);
    } else if (env->activeTe && env->window && IsWindowHilited(env->window)) {
        SetPort(env->window);
        TEIdle(env->activeTe);
    }

    Point mouseLoc;
    GetMouse(&mouseLoc);
    AdjustCursor(mouseLoc);

    if (env->window && IsWindowHilited(env->window)) {
        SetPort(env->window);
        Point localPt = mouseLoc;
        ControlHandle hovered = NULL;

        if (PtInRect(localPt, &(**env->clearBtn).contrlRect)) {
            hovered = env->clearBtn;
        } else if (PtInRect(localPt, &(**env->historyBtn).contrlRect)) {
            hovered = env->historyBtn;
        } else if (PtInRect(localPt, &(**env->settingsBtn).contrlRect)) {
            hovered = env->settingsBtn;
        } else if (env->pendingAIChangeText &&
                PtInRect(localPt, &(**env->applyPendingBtn).contrlRect)) {
            hovered = env->applyPendingBtn;
        } else if (env->pendingAIChangeText &&
                PtInRect(localPt, &(**env->rejectPendingBtn).contrlRect)) {
            hovered = env->rejectPendingBtn;
        }

        if (hovered != lastHovered) {
            lastHovered = hovered;
            if (hovered == env->clearBtn) {
                AppShowTooltip(env->clearBtn, env->lang->ttClear);
            } else if (hovered == env->historyBtn) {
                AppShowTooltip(env->historyBtn, env->lang->ttHistory);
            } else if (hovered == env->settingsBtn) {
                AppShowTooltip(env->settingsBtn, env->lang->ttSettings);
            } else if (hovered == env->applyPendingBtn) {
                AppShowTooltip(env->applyPendingBtn, env->lang->ttApplyAI);
            } else if (hovered == env->rejectPendingBtn) {
                AppShowTooltip(env->rejectPendingBtn, env->lang->ttRejectAI);
            }
        }
    }
}

void AppHandleMenuChoice(AppEnv* env, long choice) {
    short menuID = HiWord(choice);
    short menuItem = LoWord(choice);

    if (menuID == 128) { /* Apple Menu */
        if (menuItem == 1) {
            AppModalAbout(env);
        } else {
            Str255 name;
            GetMenuItemText(GetMenuHandle(128), menuItem, name);
#if !TARGET_API_MAC_CARBON
            /* OpenDeskAcc is a Classic-only Toolbox call; no Carbon header declares it.
               The #if guard means Carbon builds never reach this line. */
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wimplicit-function-declaration"
            OpenDeskAcc(name);
            #pragma GCC diagnostic pop
#endif
        }
    } else if (menuID == 129) { /* File Menu */
        switch (menuItem) {
            case 1: /* Settings */
                AppModalSettings(env);
                break;
            case 2: /* Remote Compile */
                env->settings.remoteCompileEnabled = env->settings.remoteCompileEnabled ? 0 : 1;
                AppNormalizeSettings(&env->settings);
                AppSavePrefs(env);
                AppUpdateRemoteCompileMenu(env);
                AppUpdateStatus(env, env->settings.remoteCompileEnabled ?
                    (env->settings.language == 1 ? "Uzak derleme a\x8D\xDDk" : "Remote compile on") :
                    (env->settings.language == 1 ? "Uzak derleme kapal\xDD" : "Remote compile off"));
                break;
            case 3: /* Create Starter Project */
                AppDoCreateStarterProject(env);
                break;
            case 4: /* Build Project */
                AppDoBuildProject(env);
                break;
            case 5: /* Fix Build */
                AppDoFixBuild(env);
                break;
            case 6: /* Show Artifact */
                AppDoShowArtifact(env);
                break;
            case 7: /* Apply AI Changes */
                AppApplyPendingAIChanges(env);
                break;
            case 8: /* Reject AI Changes */
                AppRejectPendingAIChanges(env);
                break;
            case 9: /* Clear */
                if (env->transcriptTe) {
                    AppClearTranscript(env);
                    AppUpdateStatus(env, env->lang->statusCleared);
                }
                break;
            case 11: /* Quit */
                AppQuit(env);
                break;
        }
    } else if (menuID == 130) { /* Edit Menu */
        if (env->activeTe) {
            switch (menuItem) {
                case 1: /* Undo last AI file change */
                    if (AppUndoLastAIChange(env)) {
                        AppUpdateStatus(env, "Undid last AI change");
                    } else {
                        AppUpdateStatus(env, "No AI change to undo");
                    }
                    break;
                case 3: /* Cut */
                    if (env->activeTe == env->inputTe || env->activeTe == env->editorTe) {
                        TECut(env->activeTe);
                        TEToScrap();
                        TECalText(env->activeTe);
                        if (env->activeTe == env->editorTe) {
                            AppMarkEditorDirty(env);
                            AppAdjustEditorScrollbar(env);
                            AppDrawEditorLineNumbers(env);
                        }
                        TEUpdate(&(**env->activeTe).viewRect, env->activeTe);
                    }
                    break;
                case 4: /* Copy */
                    TECopy(env->activeTe);
                    TEToScrap();
                    break;
                case 5: /* Paste */
                    if (env->activeTe == env->inputTe || env->activeTe == env->editorTe) {
                        TEFromScrap();
                        TEPaste(env->activeTe);
                        TECalText(env->activeTe);
                        if (env->activeTe == env->editorTe) {
                            AppMarkEditorDirty(env);
                            AppAdjustEditorScrollbar(env);
                            AppDrawEditorLineNumbers(env);
                        }
                        TEUpdate(&(**env->activeTe).viewRect, env->activeTe);
                    }
                    break;
                case 6: /* Select All */
                    TESetSelect(0, (**env->activeTe).teLength, env->activeTe);
                    TEUpdate(&(**env->activeTe).viewRect, env->activeTe);
                    break;
                case 7: /* Clear */
                    if (env->activeTe == env->inputTe || env->activeTe == env->editorTe) {
                        TEDelete(env->activeTe);
                        TECalText(env->activeTe);
                        if (env->activeTe == env->editorTe) {
                            AppMarkEditorDirty(env);
                            AppAdjustEditorScrollbar(env);
                            AppDrawEditorLineNumbers(env);
                        }
                        TEUpdate(&(**env->activeTe).viewRect, env->activeTe);
                    }
                    break;
            }
        }
    }
    HiliteMenu(0);
}

void AppHandleActivate(AppEnv* env, EventRecord* event) {
    if ((WindowPtr)event->message == env->filesWindow) {
        return;
    }

    if ((WindowPtr)event->message == env->editorWindow) {
        SetPort(env->editorWindow);
        if ((event->modifiers & activeFlag) != 0) {
            if (env->editorTe) {
                env->activeTe = env->editorTe;
                TEActivate(env->editorTe);
            }
        } else if (env->editorTe) {
            TEDeactivate(env->editorTe);
        }
        return;
    }

    if ((WindowPtr)event->message != env->window) return;

    SetPort(env->window);
    Boolean active = (event->modifiers & activeFlag) != 0;

    if (active) {
        if (env->activeTe) {
            TEActivate(env->activeTe);
        }
    } else {
        if (env->activeTe) {
            TEDeactivate(env->activeTe);
        }
    }

    AppDrawDecorations(env);
}

void AppQuit(AppEnv* env) {
    AppClearUndoChange(env);
    AppClearPendingAIChanges(env);
    if (env->clearIcon) ReleaseIconRef(env->clearIcon);
    if (env->historyIcon) ReleaseIconRef(env->historyIcon);
    if (env->settingsIcon) ReleaseIconRef(env->settingsIcon);
    if (env->buildIcon) ReleaseIconRef(env->buildIcon);
    if (env->planIcon) ReleaseIconRef(env->planIcon);
    if (env->docIcon) ReleaseIconRef(env->docIcon);
    if (env->folderIcon) ReleaseIconRef(env->folderIcon);
    ExitToShell();
}
