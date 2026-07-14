#include "common.h"

void AppExecuteAIFileOps(AppEnv* env, char* responseText, char* summary, long maxSummary) {
    char* cursor;
    short writes;
    short deletes;
    short previews;
    short blocked;
    short failed;
    short builds;

    if (summary && maxSummary > 0) summary[0] = '\0';
    if (!env || !responseText || !summary || maxSummary <= 1) return;

    writes = 0;
    deletes = 0;
    previews = 0;
    blocked = 0;
    failed = 0;
    builds = 0;
    env->undoTransactionOpen = false;

    cursor = responseText;
    while ((cursor = strstr(cursor, "<ALTANAI_BUILD_RESULT")) != NULL) {
        char* tagEnd = strchr(cursor, '>');
        char* close = strstr(cursor, "</ALTANAI_BUILD_RESULT>");
        if (!tagEnd || !close) {
            AppRemoveRange(responseText, cursor, responseText + strlen(responseText));
            failed++;
            continue;
        }

        AppAppendBuildResultSummary(env, summary, maxSummary, tagEnd + 1, close - (tagEnd + 1));
        AppStoreLastBuildResult(env, tagEnd + 1, close - (tagEnd + 1));
        builds++;
        AppRemoveRange(responseText, cursor, close + strlen("</ALTANAI_BUILD_RESULT>"));
    }

    cursor = responseText;
    while ((cursor = strstr(cursor, "<ALTANAI_READ")) != NULL) {
        char path[64];
        char* tagEnd = strstr(cursor, "/>");
        if (!tagEnd) {
            AppRemoveRange(responseText, cursor, responseText + strlen(responseText));
            failed++;
            continue;
        }

        if (AppExtractToolPath(cursor, path, sizeof(path))) {
            char* readText = NULL;
            long readLen = 0;
            if (env->hasRootFolder && AppReadRootFile(env, path, &readText, &readLen, 12000) == noErr) {
                char line[120];
                sprintf(line, "Read %s (%ld bytes).\n", path, readLen);
                strncat(summary, line, maxSummary - strlen(summary) - 1);
            } else {
                failed++;
            }
            if (readText) free(readText);
        } else {
            failed++;
        }
        AppRemoveRange(responseText, cursor, tagEnd + 2);
    }

    cursor = responseText;
    while ((cursor = strstr(cursor, "<ALTANAI_CREATE")) != NULL) {
        char path[64];
        char* tagEnd = strstr(cursor, "/>");
        if (!tagEnd) {
            AppRemoveRange(responseText, cursor, responseText + strlen(responseText));
            failed++;
            continue;
        }

        if (AppExtractToolPath(cursor, path, sizeof(path))) {
            if (env->settings.workMode && env->hasRootFolder) {
                OSErr createErr = AppCreateRootFile(env, path);
                if (createErr == noErr) {
                    char* newText = NULL;
                    long newLen = 0;
                    writes++;
                    AppAppendFileStatLine(summary, maxSummary, path, 0, 0);
                    if (AppReadRootFile(env, path, &newText, &newLen, 26000) == noErr) {
                        AppStoreUndoChange(env, path, NULL, 0, false,
                            newText, newLen, responseText);
                    }
                    if (newText) free(newText);
                } else {
                    failed++;
                }
            } else if (env->hasRootFolder) {
                char* existingText = NULL;
                long existingLen = 0;
                if (AppReadRootFile(env, path, &existingText, &existingLen, 1) == fnfErr) {
                    previews++;
                    AppAppendPreviewStatLine(env, summary, maxSummary, path, 0, 0);
                } else {
                    failed++;
                }
                if (existingText) free(existingText);
            } else {
                blocked++;
            }
        } else {
            failed++;
        }
        AppRemoveRange(responseText, cursor, tagEnd + 2);
    }

    cursor = responseText;
    while ((cursor = strstr(cursor, "<ALTANAI_PATCH")) != NULL) {
        char path[64];
        char* tagEnd = strchr(cursor, '>');
        char* close = strstr(cursor, "</ALTANAI_PATCH>");
        if (!tagEnd || !close) {
            AppRemoveRange(responseText, cursor, responseText + strlen(responseText));
            failed++;
            continue;
        }

        if (AppExtractToolPath(cursor, path, sizeof(path))) {
            const char* oldOpenTag = "<ALTANAI_OLD>";
            const char* oldCloseTag = "</ALTANAI_OLD>";
            const char* newOpenTag = "<ALTANAI_NEW>";
            const char* newCloseTag = "</ALTANAI_NEW>";
            char* oldOpen = strstr(tagEnd + 1, oldOpenTag);
            char* oldClose = oldOpen ? strstr(oldOpen + strlen(oldOpenTag), oldCloseTag) : NULL;
            char* newOpen = oldClose ? strstr(oldClose + strlen(oldCloseTag), newOpenTag) : NULL;
            char* newClose = newOpen ? strstr(newOpen + strlen(newOpenTag), newCloseTag) : NULL;

            if (oldOpen && oldClose && newOpen && newClose && newClose <= close) {
                long oldChunkLen = oldClose - (oldOpen + strlen(oldOpenTag));
                long newChunkLen = newClose - (newOpen + strlen(newOpenTag));
                char* oldChunk = (char*)malloc(oldChunkLen + 1);
                char* newChunk = (char*)malloc(newChunkLen + 1);
                if (oldChunk && newChunk) {
                    long added = 0;
                    long removed = 0;
                    BlockMoveData(oldOpen + strlen(oldOpenTag), oldChunk, oldChunkLen);
                    oldChunk[oldChunkLen] = '\0';
                    BlockMoveData(newOpen + strlen(newOpenTag), newChunk, newChunkLen);
                    newChunk[newChunkLen] = '\0';

                    if (env->settings.workMode && env->hasRootFolder) {
                        char* oldText = NULL;
                        char* newText = NULL;
                        long oldLen = 0;
                        long newLen = 0;
                        AppReadRootFile(env, path, &oldText, &oldLen, 26000);
                        if (AppPatchRootFile(env, path, oldChunk, newChunk, &added, &removed) == noErr) {
                            writes++;
                            AppAppendFileStatLine(summary, maxSummary, path, added, removed);
                            if (AppReadRootFile(env, path, &newText, &newLen, 26000) == noErr) {
                                AppStoreUndoChange(env, path, oldText, oldLen, true,
                                    newText, newLen, responseText);
                            }
                        } else {
                            failed++;
                        }
                        if (oldText) free(oldText);
                        if (newText) free(newText);
                    } else if (env->hasRootFolder) {
                        if (AppPreviewPatchRootFile(env, path, oldChunk, newChunk, &added, &removed) == noErr) {
                            previews++;
                            AppAppendPreviewStatLine(env, summary, maxSummary, path, added, removed);
                        } else {
                            failed++;
                        }
                    } else {
                        blocked++;
                    }
                } else {
                    failed++;
                }
                if (oldChunk) free(oldChunk);
                if (newChunk) free(newChunk);
            } else {
                failed++;
            }
        } else {
            failed++;
        }
        AppRemoveRange(responseText, cursor, close + strlen("</ALTANAI_PATCH>"));
    }

    cursor = responseText;
    while ((cursor = strstr(cursor, "<ALTANAI_WRITE")) != NULL) {
        char path[64];
        char* tagEnd = strchr(cursor, '>');
        char* close = strstr(cursor, "</ALTANAI_WRITE>");
        if (!tagEnd || !close) {
            AppRemoveRange(responseText, cursor, responseText + strlen(responseText));
            continue;
        }

        if (AppExtractToolPath(cursor, path, sizeof(path))) {
            long contentLen = close - (tagEnd + 1);
            char* content = (char*)malloc(contentLen + 1);
            if (content) {
                BlockMoveData(tagEnd + 1, content, contentLen);
                content[contentLen] = '\0';
                if (env->settings.workMode && env->hasRootFolder) {
                    char* oldText = NULL;
                    char* newText = NULL;
                    long oldLen = 0;
                    long newLen = 0;
                    long added = 0;
                    long removed = 0;
                    OSErr oldErr = AppReadRootFile(env, path, &oldText, &oldLen, 26000);
                    AppComputeSimpleLineStats(oldText, oldLen, content, contentLen, &added, &removed);
                    if (AppWriteRootFile(env, path, content) == noErr) {
                        writes++;
                        AppAppendFileStatLine(summary, maxSummary, path, added, removed);
                        if (AppReadRootFile(env, path, &newText, &newLen, 26000) == noErr) {
                            AppStoreUndoChange(env, path, oldText, oldLen, oldErr == noErr,
                                newText, newLen, responseText);
                        }
                    }
                    if (oldText) free(oldText);
                    if (newText) free(newText);
                } else if (env->hasRootFolder) {
                    char* oldText = NULL;
                    long oldLen = 0;
                    long added = 0;
                    long removed = 0;
                    AppReadRootFile(env, path, &oldText, &oldLen, 26000);
                    AppComputeSimpleLineStats(oldText, oldLen, content, contentLen, &added, &removed);
                    previews++;
                    AppAppendPreviewStatLine(env, summary, maxSummary, path, added, removed);
                    if (oldText) free(oldText);
                } else {
                    blocked++;
                }
                free(content);
            }
        }
        AppRemoveRange(responseText, cursor, close + strlen("</ALTANAI_WRITE>"));
    }

    cursor = responseText;
    while ((cursor = strstr(cursor, "<ALTANAI_APPEND")) != NULL) {
        char path[64];
        char* tagEnd = strchr(cursor, '>');
        char* close = strstr(cursor, "</ALTANAI_APPEND>");
        char* removeEnd;
        if (!tagEnd) {
            cursor += 15;
            continue;
        }
        if (!close) close = responseText + strlen(responseText);
        removeEnd = close;

        if (AppExtractToolPath(cursor, path, sizeof(path))) {
            long contentLen = close - (tagEnd + 1);
            long repeatCount = AppExtractToolRepeatCount(cursor);
            long expandedLen = 0;
            char* content = AppBuildRepeatedAppendContent(tagEnd + 1, contentLen,
                repeatCount, &expandedLen);
            if (content) {
                if (env->settings.workMode && env->hasRootFolder) {
                    char* oldText = NULL;
                    char* newText = NULL;
                    char* proposedText = NULL;
                    long oldLen = 0;
                    long newLen = 0;
                    long proposedLen = 0;
                    long added = 0;
                    long removed = 0;
                    OSErr oldErr = AppReadRootFile(env, path, &oldText, &oldLen, 26000);
                    proposedText = AppBuildAppendedText(oldText, oldLen, content, expandedLen,
                        &proposedLen);
                    AppComputeSimpleLineStats(oldText, oldLen, proposedText, proposedLen,
                        &added, &removed);
                    if (AppAppendRootFile(env, path, content) == noErr) {
                        writes++;
                        AppAppendFileStatLine(summary, maxSummary, path, added, removed);
                        if (AppReadRootFile(env, path, &newText, &newLen, 26000) == noErr) {
                            AppStoreUndoChange(env, path, oldText, oldLen, oldErr == noErr,
                                newText, newLen, responseText);
                        }
                    }
                    if (oldText) free(oldText);
                    if (newText) free(newText);
                    if (proposedText) free(proposedText);
                } else if (env->hasRootFolder) {
                    char* oldText = NULL;
                    char* proposedText = NULL;
                    long oldLen = 0;
                    long proposedLen = 0;
                    long added = 0;
                    long removed = 0;
                    AppReadRootFile(env, path, &oldText, &oldLen, 26000);
                    proposedText = AppBuildAppendedText(oldText, oldLen, content, expandedLen,
                        &proposedLen);
                    AppComputeSimpleLineStats(oldText, oldLen, proposedText, proposedLen,
                        &added, &removed);
                    previews++;
                    AppAppendPreviewStatLine(env, summary, maxSummary, path, added, removed);
                    if (oldText) free(oldText);
                    if (proposedText) free(proposedText);
                } else {
                    blocked++;
                }
                free(content);
            }
        }
        if (*removeEnd) removeEnd += strlen("</ALTANAI_APPEND>");
        AppRemoveRange(responseText, cursor, removeEnd);
    }

    cursor = responseText;
    while ((cursor = strstr(cursor, "<ALTANAI_DELETE")) != NULL) {
        char path[64];
        char* tagEnd = strstr(cursor, "/>");
        if (!tagEnd) {
            AppRemoveRange(responseText, cursor, responseText + strlen(responseText));
            continue;
        }

        if (AppExtractToolPath(cursor, path, sizeof(path))) {
            if (env->settings.workMode && env->hasRootFolder) {
                char* oldText = NULL;
                long oldLen = 0;
                long removed = 0;
                OSErr oldErr = AppReadRootFile(env, path, &oldText, &oldLen, 26000);
                removed = AppCountTextLines(oldText, oldLen);
                if (AppDeleteRootFile(env, path) == noErr) {
                    deletes++;
                    AppAppendFileStatLine(summary, maxSummary, path, 0, removed);
                    AppStoreUndoChange(env, path, oldText, oldLen, oldErr == noErr,
                        NULL, 0, responseText);
                }
                if (oldText) free(oldText);
            } else if (env->hasRootFolder) {
                char* oldText = NULL;
                long oldLen = 0;
                long removed = 0;
                if (AppReadRootFile(env, path, &oldText, &oldLen, 26000) == noErr) {
                    removed = AppCountTextLines(oldText, oldLen);
                    previews++;
                    AppAppendPreviewStatLine(env, summary, maxSummary, path, 0, removed);
                } else {
                    failed++;
                }
                if (oldText) free(oldText);
            } else {
                blocked++;
            }
        }
        AppRemoveRange(responseText, cursor, tagEnd + 2);
    }

    if (env->filesWindow && (writes || deletes)) AppDrawFilesWindow(env);
    if (writes || deletes || previews || builds) AppStripFencedCodeBlocks(responseText);
    if (blocked) {
        char line[64];
        sprintf(line, "%d blocked.\n", blocked);
        strncat(summary, line, maxSummary - strlen(summary) - 1);
    }
    if (failed) {
        strncat(summary, "Could not safely apply edit.\n", maxSummary - strlen(summary) - 1);
    }
}

void AppAppendBuildResultSummary(AppEnv* env, char* summary, long maxSummary, const char* text, long len) {
    char line[360];
    char oneLine[220];
    char outputPath[160];
    char artifactLine[240];
    long i;
    long room;
    short emitted;
    Boolean statusOk;

    char statusVal[32];
    int warnVal = 0;
    int errVal = 0;
    char outVal[160];

    if (!summary || !text || len <= 0 || maxSummary <= 1 || !env) return;

    while (len > 0 && (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')) {
        text++;
        len--;
    }
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' ||
            text[len - 1] == '\r' || text[len - 1] == '\n')) {
        len--;
    }
    if (len <= 0) return;

    room = maxSummary - strlen(summary) - 1;
    if (room <= 0) return;

    /* Initialize values for parsing */
    strcpy(statusVal, "unknown");
    strcpy(outVal, "none");

    /* Parse status */
    {
        const char* p = AppFindASCIIInsensitivePtr(text, "status:");
        if (p) {
            p += 7;
            while (p < text + len && (*p == ' ' || *p == '\t')) p++;
            i = 0;
            while (p < text + len && *p != ';' && *p != ' ' && *p != '\r' && *p != '\n' && i < 30) {
                statusVal[i++] = *p++;
            }
            statusVal[i] = '\0';
        }
    }
    /* Parse warnings */
    {
        const char* p = AppFindASCIIInsensitivePtr(text, "warnings:");
        if (p) {
            p += 9;
            warnVal = atoi(p);
        }
    }
    /* Parse errors */
    {
        const char* p = AppFindASCIIInsensitivePtr(text, "errors:");
        if (p) {
            p += 7;
            errVal = atoi(p);
        }
    }
    /* Parse output */
    {
        const char* p = AppFindASCIIInsensitivePtr(text, "output:");
        if (p) {
            p += 7;
            while (p < text + len && (*p == ' ' || *p == '\t')) p++;
            i = 0;
            while (p < text + len && *p != '\r' && *p != '\n' && *p != ' ' && i < 150) {
                outVal[i++] = *p++;
            }
            outVal[i] = '\0';
        }
    }

    if (env->settings.language == 1) {
        char trStatus[32];
        char trOutput[160];

        if (AppEqualASCIIInsensitive(statusVal, "ok")) {
            strcpy(trStatus, "ok");
        } else if (AppEqualASCIIInsensitive(statusVal, "failed")) {
            strcpy(trStatus, "ba\xDF" "ar\xDDs\xDDz");
        } else {
            strcpy(trStatus, statusVal);
        }

        if (AppEqualASCIIInsensitive(outVal, "none")) {
            strcpy(trOutput, "yok");
        } else {
            strcpy(trOutput, outVal);
        }

        sprintf(line, "Derleme: durum: %s; uyar\xDDlar: %d; hatalar: %d; \x8D\xDDkt\xDD: %s\n",
            trStatus, warnVal, errVal, trOutput);
    } else {
        sprintf(line, "Build: status: %s; warnings: %d; errors: %d; output: %s\n",
            statusVal, warnVal, errVal, outVal);
    }
    strncat(summary, line, room);

    outputPath[0] = '\0';
    statusOk = AppContainsASCIIInsensitive(statusVal, "ok");
    if (!AppEqualASCIIInsensitive(outVal, "none") && !AppContainsASCIIInsensitive(outVal, "not found")) {
        strcpy(outputPath, outVal);
    }

    if (outputPath[0]) {
        if (env->settings.language == 1) {
            sprintf(artifactLine, "Dosya: %s\n\xDCndirme: /api/artifact?path=%s\n",
                outputPath, outputPath);
        } else {
            sprintf(artifactLine, "Artifact: %s\nDownload: /api/artifact?path=%s\n",
                outputPath, outputPath);
        }
        strncat(summary, artifactLine, maxSummary - strlen(summary) - 1);
    } else if (statusOk) {
        if (env->settings.language == 1) {
            strncat(summary,
                "Mac uygulama dosyas\xDD bulunamad\xDD. add_application kullan\xDDn.\n",
                maxSummary - strlen(summary) - 1);
        } else {
            strncat(summary,
                "No Mac app artifact found. Use Create Starter Project or Retro68 add_application.\n",
                maxSummary - strlen(summary) - 1);
        }
    }

    emitted = 0;
    i = 0;
    while (i < len && emitted < 4) {
        long start;
        long lineLen;

        while (i < len && (text[i] == '\r' || text[i] == '\n')) i++;
        start = i;
        while (i < len && text[i] != '\r' && text[i] != '\n') i++;
        lineLen = i - start;
        if (lineLen <= 0) continue;
        if (lineLen > (long)sizeof(oneLine) - 1) lineLen = sizeof(oneLine) - 1;
        BlockMoveData(text + start, oneLine, lineLen);
        oneLine[lineLen] = '\0';

        if (AppContainsASCIIInsensitive(oneLine, "Imported build snapshot") ||
                AppContainsASCIIInsensitive(oneLine, "No CMakeLists") ||
                AppContainsASCIIInsensitive(oneLine, "error") ||
                AppContainsASCIIInsensitive(oneLine, "fatal") ||
                AppContainsASCIIInsensitive(oneLine, "warning") ||
                AppContainsASCIIInsensitive(oneLine, "CMake Error")) {
            room = maxSummary - strlen(summary) - 1;
            if (room <= 0) break;

            if (env->settings.language == 1 && AppContainsASCIIInsensitive(oneLine, "status:")) {
                char locStatus[32];
                int locWarn = 0;
                int locErr = 0;
                char locOut[160];
                char locLine[360];
                long k;

                strcpy(locStatus, "unknown");
                strcpy(locOut, "none");

                /* Parse from oneLine */
                const char* pS = AppFindASCIIInsensitivePtr(oneLine, "status:");
                if (pS) {
                    pS += 7;
                    while (*pS == ' ' || *pS == '\t') pS++;
                    k = 0;
                    while (*pS && *pS != ';' && *pS != ' ' && k < 30) {
                        locStatus[k++] = *pS++;
                    }
                    locStatus[k] = '\0';
                }
                const char* pW = AppFindASCIIInsensitivePtr(oneLine, "warnings:");
                if (pW) locWarn = atoi(pW + 9);
                const char* pE = AppFindASCIIInsensitivePtr(oneLine, "errors:");
                if (pE) locErr = atoi(pE + 7);
                const char* pO = AppFindASCIIInsensitivePtr(oneLine, "output:");
                if (pO) {
                    pO += 7;
                    while (*pO == ' ' || *pO == '\t') pO++;
                    k = 0;
                    while (*pO && *pO != '\r' && *pO != '\n' && *pO != ' ' && k < 150) {
                        locOut[k++] = *pO++;
                    }
                    locOut[k] = '\0';
                }

                if (AppEqualASCIIInsensitive(locStatus, "ok")) strcpy(locStatus, "ok");
                else if (AppEqualASCIIInsensitive(locStatus, "failed")) strcpy(locStatus, "ba\xDF" "ar\xDDs\xDDz");

                if (AppEqualASCIIInsensitive(locOut, "none")) strcpy(locOut, "yok");

                sprintf(locLine, "durum: %s; uyar\xDDlar: %d; hatalar: %d; \x82\xDDkt\xDD: %s",
                    locStatus, locWarn, locErr, locOut);
                strncat(summary, locLine, room);
            } else if (env->settings.language == 1 && AppContainsASCIIInsensitive(oneLine, "Imported build snapshot")) {
                char trLine[360];
                const char* filesPtr = strchr(oneLine, ':');
                if (filesPtr) {
                    filesPtr++;
                    while (*filesPtr == ' ' || *filesPtr == '\t') filesPtr++;
                    sprintf(trLine, "\xDC\x8D" "e aktar\xDDlan derleme dosyas\xDD: %s", filesPtr);
                } else {
                    sprintf(trLine, "\xDC\x8D" "e aktar\xDDlan derleme dosyas\xDD:");
                }
                strncat(summary, trLine, room);
            } else {
                strncat(summary, oneLine, room);
            }
            strncat(summary, "\n", maxSummary - strlen(summary) - 1);
            emitted++;
        }
    }
}

void AppStoreLastBuildResult(AppEnv* env, const char* text, long len) {
    long copyLen;
    const char* outputPtr;
    const char* outputEnd;
    long outputLen;

    if (!env) return;
    env->lastBuildResult[0] = '\0';
    env->lastArtifactPath[0] = '\0';
    if (!text || len <= 0) return;

    while (len > 0 && (*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')) {
        text++;
        len--;
    }
    while (len > 0 && (text[len - 1] == ' ' || text[len - 1] == '\t' ||
            text[len - 1] == '\r' || text[len - 1] == '\n')) {
        len--;
    }
    if (len <= 0) return;

    copyLen = len;
    if (copyLen > (long)sizeof(env->lastBuildResult) - 1) {
        copyLen = sizeof(env->lastBuildResult) - 1;
    }
    BlockMoveData(text, env->lastBuildResult, copyLen);
    env->lastBuildResult[copyLen] = '\0';

    outputPtr = AppFindASCIIInsensitivePtr(env->lastBuildResult, "output:");
    if (!outputPtr) return;
    outputPtr += strlen("output:");
    while (*outputPtr == ' ' || *outputPtr == '\t') outputPtr++;
    outputEnd = outputPtr;
    while (*outputEnd && *outputEnd != '\r' && *outputEnd != '\n' &&
            *outputEnd != ';') {
        outputEnd++;
    }
    outputLen = outputEnd - outputPtr;
    while (outputLen > 0 && outputPtr[outputLen - 1] == ' ') outputLen--;
    if (outputLen <= 0 || outputLen >= (long)sizeof(env->lastArtifactPath)) return;

    BlockMoveData(outputPtr, env->lastArtifactPath, outputLen);
    env->lastArtifactPath[outputLen] = '\0';
    if (AppEqualASCIIInsensitive(env->lastArtifactPath, "none") ||
            AppContainsASCIIInsensitive(env->lastArtifactPath, "not found")) {
        env->lastArtifactPath[0] = '\0';
    }
}

void AppClearUndoChange(AppEnv* env) {
    short i;

    if (!env) return;
    for (i = 0; i < UI_MAX_UNDO_FILES; i++) {
        if (env->undoChanges[i].oldText) free(env->undoChanges[i].oldText);
        if (env->undoChanges[i].newText) free(env->undoChanges[i].newText);
        env->undoChanges[i].oldText = NULL;
        env->undoChanges[i].newText = NULL;
        env->undoChanges[i].path[0] = '\0';
        env->undoChanges[i].hadOldFile = false;
        env->undoChanges[i].inUse = false;
    }
    env->hasUndoChange = false;
    env->undoTransactionOpen = false;
    env->undoChangeCount = 0;
    env->undoTimestamp[0] = '\0';
    env->undoMessage[0] = '\0';
}

void AppClearPendingAIChanges(AppEnv* env) {
    if (!env) return;
    if (env->pendingAIChangeText) {
        free(env->pendingAIChangeText);
        env->pendingAIChangeText = NULL;
    }
    env->pendingAIChangeSummary[0] = '\0';
    AppUpdatePendingControls(env);
}

void AppStorePendingAIChanges(AppEnv* env, const char* text, const char* summary) {
    char* copy;

    if (!env || !text || !text[0]) return;
    copy = AppCopySizedText(text, strlen(text));
    if (!copy) return;

    AppClearPendingAIChanges(env);
    env->pendingAIChangeText = copy;
    if (summary) {
        strncpy(env->pendingAIChangeSummary, summary, sizeof(env->pendingAIChangeSummary) - 1);
        env->pendingAIChangeSummary[sizeof(env->pendingAIChangeSummary) - 1] = '\0';
    }
    AppUpdatePendingControls(env);
}

Boolean AppToolSummaryHasFailure(const char* summary) {
    if (!summary) return false;
    if (strstr(summary, "Could not safely apply edit.")) return true;
    if (strstr(summary, "blocked.")) return true;
    return false;
}

Boolean AppToolSummaryHasPreview(const char* summary) {
    if (!summary) return false;
    return strstr(summary, "Preview: ") != NULL || strstr(summary, "\x85nizleme: ") != NULL;
}

Boolean AppHasEditToolBlock(const char* text) {
    if (!text || !text[0]) return false;
    if (strstr(text, "<ALTANAI_CREATE")) return true;
    if (strstr(text, "<ALTANAI_PATCH")) return true;
    if (strstr(text, "<ALTANAI_WRITE")) return true;
    if (strstr(text, "<ALTANAI_APPEND")) return true;
    if (strstr(text, "<ALTANAI_DELETE")) return true;
    return false;
}

Boolean AppApplyPendingAIChanges(AppEnv* env) {
    char* opsCopy;
    char summary[512];
    char timestamp[32];
    char header[128];
    const char* appliedMsg;
    short oldMode;

    if (!env || !env->pendingAIChangeText || !env->pendingAIChangeText[0]) {
        AppUpdateStatus(env, env && env->settings.language == 1 ?
            "Bekleyen AI de\xDBi\xDFikli\xDBi yok" : "No pending AI changes");
        return false;
    }
    if (AppActiveEditorHasUnsavedTouchedFile(env)) {
        AppUpdateStatus(env, env->settings.language == 1 ?
            "Once acik editor dosyasini kaydedin" :
            "Save open editor file before applying AI changes");
        return false;
    }

    opsCopy = AppCopySizedText(env->pendingAIChangeText, strlen(env->pendingAIChangeText));
    if (!opsCopy) {
        AppUpdateStatus(env, env->lang->statusError);
        return false;
    }

    summary[0] = '\0';
    oldMode = env->settings.workMode;
    env->settings.workMode = 1;
    AppExecuteAIFileOps(env, opsCopy, summary, sizeof(summary));
    env->settings.workMode = oldMode;

    free(opsCopy);

    if (!summary[0] || AppToolSummaryHasFailure(summary)) {
        AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
        sprintf(header, "%s [%s]:", env->lang->lblSystem, timestamp);
        AppAppendTextColored(env, header, &kSystemColor);
        AppAppendTextColored(env, "Could not safely apply edit.", &kSystemColor);
        if (summary[0]) AppAppendFileChangeSummaryColored(env, summary);
        AppAppendHistoryLine(env, header);
        AppAppendHistoryLine(env, "Could not safely apply edit.");
        if (summary[0]) AppAppendHistoryLine(env, summary);
        AppUpdateStatus(env, "Could not safely apply edit");
        return false;
    }

    AppClearPendingAIChanges(env);

    appliedMsg = env->settings.language == 1 ?
        "Bekleyen AI de\xDBi\xDFiklikleri uyguland\xDD." :
        "Applied pending AI changes.";

    AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
    sprintf(header, "%s [%s]:", env->lang->lblSystem, timestamp);
    AppAppendTextColored(env, header, &kSystemColor);
    AppAppendTextColored(env, appliedMsg, &kSystemColor);
    AppAppendHistoryLine(env, header);
    AppAppendHistoryLine(env, appliedMsg);
    if (summary[0]) {
        AppAppendFileChangeSummaryColored(env, summary);
        AppAppendHistoryLine(env, summary);
    }
    AppReloadActiveEditorIfClean(env);
    AppUpdateStatus(env, appliedMsg);
    return true;
}

Boolean AppRejectPendingAIChanges(AppEnv* env) {
    char timestamp[32];
    char header[128];
    const char* rejectedMsg;

    if (!env || !env->pendingAIChangeText || !env->pendingAIChangeText[0]) {
        AppUpdateStatus(env, env && env->settings.language == 1 ?
            "Bekleyen AI de\xDBi\xDFikli\xDBi yok" : "No pending AI changes");
        return false;
    }

    AppClearPendingAIChanges(env);
    rejectedMsg = env->settings.language == 1 ?
        "Bekleyen AI de\xDBi\xDFiklikleri reddedildi." :
        "Rejected pending AI changes.";

    AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
    sprintf(header, "%s [%s]:", env->lang->lblSystem, timestamp);
    AppAppendTextColored(env, header, &kSystemColor);
    AppAppendTextColored(env, rejectedMsg, &kSystemColor);
    AppAppendHistoryLine(env, header);
    AppAppendHistoryLine(env, rejectedMsg);
    AppUpdateStatus(env, rejectedMsg);
    return true;
}

void AppSaveUndoBackupFile(AppEnv* env, const char* path,
                                  const char* oldText, long oldLen, Boolean hadOldFile) {
    int16_t vRefNum;
    int32_t dirID;
    FSSpec spec;
    Str255 pName;
    OSErr err;
    short refNum;
    char header[220];
    char* oldCopy;
    char* backup;
    char* classicBackup;
    long backupLen;
    long bytesToWrite;

    if (!env || !path || !path[0]) return;

    err = FindFolder(PREFS_VOL_REF, kPreferencesFolderType, true, &vRefNum, &dirID);
    if (err != noErr) return;

    ConvertCtoPstr("AltanAI Undo Backup", pName);
    err = FSMakeFSSpec(vRefNum, dirID, pName, &spec);
    if (err == fnfErr) err = FSpCreate(&spec, APP_CREATOR, 'TEXT', smSystemScript);
    if (err != noErr && err != dupFNErr) return;

    oldCopy = NULL;
    if (oldText && oldLen > 0) oldCopy = AppCopySizedText(oldText, oldLen);
    if (!oldCopy) oldCopy = AppCopySizedText("", 0);
    if (!oldCopy) return;

    sprintf(header,
        "AltanAI Last AI Backup\rPath: %s\rWhen: %s\rHad old file: %s\r--- Old Content ---\r",
        path,
        env->undoTimestamp[0] ? env->undoTimestamp : "unknown",
        hadOldFile ? "yes" : "no");

    backupLen = strlen(header) + strlen(oldCopy) + 2;
    backup = (char*)malloc(backupLen + 1);
    if (!backup) {
        free(oldCopy);
        return;
    }
    strcpy(backup, header);
    strcat(backup, oldCopy);
    strcat(backup, "\r");

    classicBackup = AppCopyClassicText(backup);
    free(backup);
    free(oldCopy);
    if (!classicBackup) return;

    err = FSpOpenDF(&spec, fsWrPerm, &refNum);
    if (err == noErr) {
        SetFPos(refNum, fsFromStart, 0);
        SetEOF(refNum, 0);
        bytesToWrite = strlen(classicBackup);
        if (bytesToWrite > 0) FSWrite(refNum, &bytesToWrite, classicBackup);
        FSClose(refNum);
    }
    free(classicBackup);
}

void AppStoreUndoChange(AppEnv* env, const char* path,
                               const char* oldText, long oldLen, Boolean hadOldFile,
                               const char* newText, long newLen, const char* message) {
    short i;
    UndoFileChange* slot;

    if (!env || !path || !path[0]) return;

    if (!env->undoTransactionOpen) {
        AppClearUndoChange(env);
        env->undoTransactionOpen = true;
        AppCurrentTimestamp(env, env->undoTimestamp, sizeof(env->undoTimestamp));
        if (message) {
            strncpy(env->undoMessage, message, sizeof(env->undoMessage) - 1);
            env->undoMessage[sizeof(env->undoMessage) - 1] = '\0';
        }
    }

    slot = NULL;
    for (i = 0; i < env->undoChangeCount; i++) {
        if (env->undoChanges[i].inUse && strcmp(env->undoChanges[i].path, path) == 0) {
            slot = &env->undoChanges[i];
            break;
        }
    }
    if (!slot) {
        if (env->undoChangeCount >= UI_MAX_UNDO_FILES) {
            AppSaveUndoBackupFile(env, path, oldText, oldLen, hadOldFile);
            return;
        }
        slot = &env->undoChanges[env->undoChangeCount++];
        memset(slot, 0, sizeof(*slot));
        slot->inUse = true;
        slot->hadOldFile = hadOldFile;
        strncpy(slot->path, path, sizeof(slot->path) - 1);
        slot->path[sizeof(slot->path) - 1] = '\0';
        if (hadOldFile && oldText) slot->oldText = AppCopySizedText(oldText, oldLen);
    }

    if (slot->newText) {
        free(slot->newText);
        slot->newText = NULL;
    }
    if (newText) slot->newText = AppCopySizedText(newText, newLen);

    AppSaveUndoBackupFile(env, path, oldText, oldLen, hadOldFile);
    env->hasUndoChange = true;
}

Boolean AppUndoLastAIChange(AppEnv* env) {
    OSErr err;
    short i;

    if (!env || !env->hasUndoChange || env->undoChangeCount <= 0) return false;

    for (i = env->undoChangeCount - 1; i >= 0; i--) {
        UndoFileChange* change = &env->undoChanges[i];
        if (!change->inUse || !change->path[0]) continue;

        if (change->hadOldFile) {
            if (!change->oldText) return false;
            err = AppWriteRootFile(env, change->path, change->oldText);
        } else {
            err = AppDeleteRootFile(env, change->path);
            if (err == fnfErr) err = noErr;
        }
        if (err != noErr) return false;
    }

    AppClearUndoChange(env);
    if (env->filesWindow) AppDrawFilesWindow(env);
    AppReloadActiveEditorIfClean(env);
    return true;
}
