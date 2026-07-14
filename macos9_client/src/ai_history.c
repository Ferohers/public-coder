#include "common.h"

Boolean AppReadHistory(char** outBuffer, long* outLen) {
    int16_t vRefNum;
    int32_t dirID;
    FSSpec spec;
    OSErr err;
    int16_t refNum;
    long historyLen;
    long bytesToRead;
    char* buffer;

    err = FindFolder(PREFS_VOL_REF, kPreferencesFolderType, true, &vRefNum, &dirID);
    if (err != noErr) return false;

    err = FSMakeFSSpec(vRefNum, dirID, "\pChatHistory.txt", &spec);
    if (err != noErr) return false;

    err = FSpOpenDF(&spec, fsRdPerm, &refNum);
    if (err != noErr) return false;

    err = GetEOF(refNum, &historyLen);
    if (err != noErr || historyLen <= 0) {
        FSClose(refNum);
        return false;
    }
    if (historyLen > 26000) historyLen = 26000;

    buffer = (char*)malloc(historyLen + 1);
    if (!buffer) {
        FSClose(refNum);
        return false;
    }

    bytesToRead = historyLen;
    err = FSRead(refNum, &bytesToRead, buffer);
    FSClose(refNum);
    if (err != noErr && err != eofErr) {
        free(buffer);
        return false;
    }
    buffer[bytesToRead] = '\0';

    *outBuffer = buffer;
    *outLen = bytesToRead;
    return true;
}

void AppShowHistory(AppEnv* env) {
    AppModalHistory(env);
}

void AppAppendHistoryText(AppEnv* env, const char* text) {
    int16_t vRefNum;
    int32_t dirID;
    FSSpec spec;
    OSErr err;
    int16_t refNum;
    long bytesToWrite;

    if (env->loadingHistory || !text || !text[0]) return;

    err = FindFolder(PREFS_VOL_REF, kPreferencesFolderType, true, &vRefNum, &dirID);
    if (err != noErr) return;

    err = FSMakeFSSpec(vRefNum, dirID, "\pChatHistory.txt", &spec);
    if (err == fnfErr) {
        err = FSpCreate(&spec, APP_CREATOR, 'TEXT', smSystemScript);
    }
    if (err != noErr && err != dupFNErr) return;

    err = FSpOpenDF(&spec, fsWrPerm, &refNum);
    if (err != noErr) return;

    bytesToWrite = strlen(text);
    SetFPos(refNum, fsFromLEOF, 0);
    FSWrite(refNum, &bytesToWrite, text);
    FSClose(refNum);
}

void AppAppendHistoryLine(AppEnv* env, const char* text) {
    AppAppendHistoryText(env, text);
    AppAppendHistoryText(env, "\r");
}

void AppBeginHistorySession(AppEnv* env) {
    char timestamp[32];

    if (env->historySessionStarted) return;

    AppAppendHistoryText(env, "\r");
    AppAppendHistoryText(env, env->lang->histNewChat);
    AppAppendHistoryText(env, "\r");
    AppCurrentTimestamp(env, timestamp, sizeof(timestamp));
    AppAppendHistoryText(env, env->lang->histWhen);
    AppAppendHistoryLine(env, timestamp);
    env->historySessionStarted = true;
}

void AppCurrentTimestamp(AppEnv* env, char* out, int maxLen) {
    static const char* monthsEn[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    static const char* monthsTr[] = {
        "Oca", "\xDE" "ub", "Mar", "Nis", "May", "Haz",
        "Tem", "A\xDB" "u", "Eyl", "Eki", "Kas", "Ara"
    };
    DateTimeRec now;
    const char* monthName = "???";

    if (!out || maxLen <= 0) return;

    GetTime(&now);
    if (now.month >= 1 && now.month <= 12) {
        if (env && env->settings.language == 1) {
            monthName = monthsTr[now.month - 1];
        } else {
            monthName = monthsEn[now.month - 1];
        }
    }

    sprintf(out, "%s %d %02d:%02d", monthName, now.day, now.hour, now.minute);
    out[maxLen - 1] = '\0';
}

void AppGetBuildDateString(AppEnv* env, char* out, int maxLen) {
    const char* compileDate = __DATE__; /* e.g. "Jul 12 2026" */

    if (env && env->settings.language == 1) {
        /* Turkish translation with full month names */
        static const char* monthsEn[] = {
            "Jan", "Feb", "Mar", "Apr", "May", "Jun",
            "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
        };
        static const char* monthsTr[] = {
            "Ocak", "\xDE" "ubat", "Mart", "Nisan", "May\xDDs", "Haziran",
            "Temmuz", "A\xDB" "ustos", "Eyl\x9F" "l", "Ekim", "Kas\xDD" "m", "Aral\xDD" "k"
        };
        char monthPart[4];
        int day = 0;
        int year = 0;
        int i;

        /* __DATE__ is always formatted as "Mmm dd yyyy" */
        if (sscanf(compileDate, "%3s %d %d", monthPart, &day, &year) == 3) {
            const char* monthName = "???";
            for (i = 0; i < 12; i++) {
                if (strcmp(monthPart, monthsEn[i]) == 0) {
                    monthName = monthsTr[i];
                    break;
                }
            }
            sprintf(out, "%d %s %d", day, monthName, year);
            return;
        }
    }

    /* Default to raw compile __DATE__ */
    strncpy(out, compileDate, maxLen - 1);
    out[maxLen - 1] = '\0';
}
