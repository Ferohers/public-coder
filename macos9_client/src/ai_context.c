#include "common.h"

Boolean AppPromptRequestsRead(const char* prompt) {
    if (!prompt) return false;
    if (AppContainsASCIIInsensitive(prompt, "read")) return true;
    if (AppContainsASCIIInsensitive(prompt, "content")) return true;
    if (AppContainsASCIIInsensitive(prompt, "oku")) return true;
    if (AppContainsASCIIInsensitive(prompt, "ne diyor")) return true;
    if (AppContainsASCIIInsensitive(prompt, "anlat")) return true;
    if (AppContainsASCIIInsensitive(prompt, "i\x8D" "er")) return true;
    return false;
}

Boolean AppPromptRequestsWrite(const char* prompt) {
    if (!prompt) return false;
    if (AppContainsASCIIInsensitive(prompt, "write")) return true;
    if (AppContainsASCIIInsensitive(prompt, "yaz")) return true;
    if (AppContainsASCIIInsensitive(prompt, "i\x8D" "ine")) return true;
    if (AppContainsASCIIInsensitive(prompt, "inside")) return true;
    return false;
}

Boolean AppPromptRequestsCodeChange(const char* prompt) {
    if (!prompt) return false;
    if (AppContainsASCIIInsensitive(prompt, "code")) return true;
    if (AppContainsASCIIInsensitive(prompt, "kod")) return true;
    if (AppContainsASCIIInsensitive(prompt, "fix")) return true;
    if (AppContainsASCIIInsensitive(prompt, "correct")) return true;
    if (AppContainsASCIIInsensitive(prompt, "d\x9Fzelt")) return true;
    if (AppContainsASCIIInsensitive(prompt, "hata")) return true;
    if (AppContainsASCIIInsensitive(prompt, "bug")) return true;
    if (AppContainsASCIIInsensitive(prompt, "implement")) return true;
    return false;
}

Boolean AppPromptAllowsFullCode(const char* prompt) {
    if (!prompt) return false;
    if (AppContainsASCIIInsensitive(prompt, "show full")) return true;
    if (AppContainsASCIIInsensitive(prompt, "full code")) return true;
    if (AppContainsASCIIInsensitive(prompt, "show me the file")) return true;
    if (AppContainsASCIIInsensitive(prompt, "show the file")) return true;
    if (AppContainsASCIIInsensitive(prompt, "tam kod")) return true;
    if (AppContainsASCIIInsensitive(prompt, "butun kod")) return true;
    if (AppContainsASCIIInsensitive(prompt, "dosyayi goster")) return true;
    if (AppContainsASCIIInsensitive(prompt, "dosyayi yaz")) return true;
    return false;
}

Boolean AppExtractPromptFilename(const char* prompt, char* outName, short maxLen) {
    long i;
    long len;

    if (!prompt || !outName || maxLen <= 1) return false;
    outName[0] = '\0';
    len = strlen(prompt);

    for (i = 0; i < len; i++) {
        if (prompt[i] == '.') {
            long start = i;
            long end = i;
            long copyLen;

            while (start > 0 && !AppIsFilenameStop(prompt[start - 1])) start--;
            while (end < len && !AppIsFilenameStop(prompt[end])) end++;

            while (end > start) {
                char c = prompt[end - 1];
                if (c == '.' || c == ':' || c == '!' || c == '?' || c == ',') end--;
                else break;
            }

            if (end <= start || end - start >= maxLen) continue;
            copyLen = end - start;
            BlockMoveData(prompt + start, outName, copyLen);
            outName[copyLen] = '\0';
            if (!AppIsSafeRelativePath(outName)) {
                outName[0] = '\0';
                return false;
            }
            return true;
        }
    }
    return false;
}

Boolean AppFindMentionedProjectFile(AppEnv* env, const char* prompt, char* outName, short maxLen) {
    short i;
    short fileCount;
    short onlyFileIndex;
    const char* ext;
    char spacedName[64];

    if (!env || !prompt || !outName || maxLen <= 1) return false;
    outName[0] = '\0';
    fileCount = 0;
    onlyFileIndex = -1;

    for (i = 0; i < env->projectItemCount; i++) {
        ProjectItem* item = &env->projectItems[i];
        if (item->isFolder) continue;
        fileCount++;
        onlyFileIndex = i;
        if (strstr(prompt, item->name) || AppContainsASCIIInsensitive(prompt, item->name)) {
            strncpy(outName, item->name, maxLen - 1);
            outName[maxLen - 1] = '\0';
            return true;
        }
        {
            const char* leaf = strrchr(item->name, '/');
            leaf = leaf ? leaf + 1 : item->name;
            if (strstr(prompt, leaf) || AppContainsASCIIInsensitive(prompt, leaf)) {
                strncpy(outName, item->name, maxLen - 1);
                outName[maxLen - 1] = '\0';
                return true;
            }
        }
        strncpy(spacedName, item->name, sizeof(spacedName) - 1);
        spacedName[sizeof(spacedName) - 1] = '\0';
        {
            char* p;
            for (p = spacedName; *p; p++) {
                if (*p == '.') *p = ' ';
            }
        }
        if (AppContainsASCIIInsensitive(prompt, spacedName)) {
            strncpy(outName, item->name, maxLen - 1);
            outName[maxLen - 1] = '\0';
            return true;
        }
    }

    if (AppExtractPromptFilename(prompt, outName, maxLen)) {
        char extracted[64];
        strncpy(extracted, outName, sizeof(extracted) - 1);
        extracted[sizeof(extracted) - 1] = '\0';
        for (i = 0; i < env->projectItemCount; i++) {
            ProjectItem* item = &env->projectItems[i];
            const char* leaf;
            if (item->isFolder) continue;
            leaf = strrchr(item->name, '/');
            leaf = leaf ? leaf + 1 : item->name;
            if (strcmp(item->name, extracted) == 0 || strcmp(leaf, extracted) == 0) {
                strncpy(outName, item->name, maxLen - 1);
                outName[maxLen - 1] = '\0';
                return true;
            }
        }
        return true;
    }

    for (i = 0; i < env->projectItemCount; i++) {
        ProjectItem* item = &env->projectItems[i];
        if (item->isFolder) continue;
        ext = strrchr(item->name, '.');
        if (ext && AppContainsASCIIInsensitive(prompt, ext + 1)) {
            strncpy(outName, item->name, maxLen - 1);
            outName[maxLen - 1] = '\0';
            return true;
        }
    }

    if (fileCount == 1 && onlyFileIndex >= 0) {
        strncpy(outName, env->projectItems[onlyFileIndex].name, maxLen - 1);
        outName[maxLen - 1] = '\0';
        return true;
    }

    return false;
}

long AppGetTotalProjectTextSize(AppEnv* env) {
    short i;
    long totalSize = 0;
    if (!env) return 0;
    for (i = 0; i < env->projectItemCount; i++) {
        ProjectItem* item = &env->projectItems[i];
        if (!item->isFolder && AppLooksTextFile(item->name)) {
            totalSize += item->size;
        }
    }
    return totalSize;
}

void AppAppendProjectFileContents(AppEnv* env, const char* prompt, char* out, long maxLen) {
    short i;
    short appended;
    long remainingBudget;
    char mentionedName[64];
    short mentionedIndex;

    if (!env || !env->hasRootFolder || !out || maxLen <= 0) return;
    if (!AppPromptRequestsCodeChange(prompt) && !AppPromptRequestsRead(prompt) &&
            AppGetTotalProjectTextSize(env) >= 20000) {
        return;
    }

    appended = 0;
    remainingBudget = 12000;
    mentionedName[0] = '\0';
    mentionedIndex = -1;
    AppAppendContextLine(out, maxLen, "--- Visible File Contents ---");

    if (AppFindMentionedProjectFile(env, prompt, mentionedName, sizeof(mentionedName))) {
        for (i = 0; i < env->projectItemCount; i++) {
            if (!env->projectItems[i].isFolder && strcmp(env->projectItems[i].name, mentionedName) == 0) {
                mentionedIndex = i;
                break;
            }
        }
    }

    if (mentionedIndex >= 0) {
        long used = AppAppendContextFileExcerpt(env, &env->projectItems[mentionedIndex],
            out, maxLen, remainingBudget);
        if (used > 0) {
            remainingBudget -= used;
            appended++;
        }
    }

    for (i = 0; i < env->projectItemCount && remainingBudget > 512; i++) {
        ProjectItem* item = &env->projectItems[i];
        long used;

        if (i == mentionedIndex) continue;
        if (item->isFolder || !AppLooksTextFile(item->name)) continue;
        used = AppAppendContextFileExcerpt(env, item, out, maxLen, remainingBudget);
        if (used > 0) {
            remainingBudget -= used;
            appended++;
        }
    }

    if (!appended) AppAppendContextLine(out, maxLen, "No readable text/code files were attached.");
}

void AppBuildProjectSnapshot(AppEnv* env, char* out, long maxLen) {
    short i;
    short uploaded;
    char line[160];

    if (!out || maxLen <= 0) return;
    out[0] = '\0';
    if (!env || !env->hasRootFolder) return;

    AppAppendContextLine(out, maxLen, "<<<ALTANAI_BUILD_SNAPSHOT>>>");
    sprintf(line, "ROOT %s", env->rootName);
    AppAppendContextLine(out, maxLen, line);
    uploaded = 0;

    for (i = 0; i < env->projectItemCount; i++) {
        ProjectItem* item = &env->projectItems[i];
        char* buffer;
        char* utf8;
        long readLen;
        long j;
        long need;

        if (item->isFolder) continue;
        if (!AppShouldUploadBuildFile(item->name, item->size)) continue;

        buffer = NULL;
        readLen = 0;
        if (AppReadRootFile(env, item->name, &buffer, &readLen, 30000) != noErr || !buffer) {
            if (buffer) free(buffer);
            continue;
        }

        utf8 = (char*)malloc(readLen * 3 + 1);
        if (!utf8) {
            free(buffer);
            continue;
        }
        MacRomanToUTF8(buffer, utf8, readLen * 3 + 1);
        free(buffer);

        for (j = 0; utf8[j]; j++) {
            if (utf8[j] == '\r') utf8[j] = '\n';
        }

        need = strlen(out) + strlen(item->name) + strlen(utf8) + 80;
        if (need >= maxLen - 64) {
            free(utf8);
            continue;
        }

        sprintf(line, "<<<ALTANAI_FILE path=\"%s\">>>", item->name);
        AppAppendContextLine(out, maxLen, line);
        strncat(out, utf8, maxLen - strlen(out) - 1);
        if (out[0] && out[strlen(out) - 1] != '\n') {
            strncat(out, "\n", maxLen - strlen(out) - 1);
        }
        AppAppendContextLine(out, maxLen, "<<<ALTANAI_END_FILE>>>");
        free(utf8);
        uploaded++;
    }

    sprintf(line, "FILES %d", uploaded);
    AppAppendContextLine(out, maxLen, line);
    AppAppendContextLine(out, maxLen, "<<<ALTANAI_END_SNAPSHOT>>>");
}

void AppAppendProjectFileSummaries(AppEnv* env, char* out, long maxLen) {
    short i;
    short count;

    if (!env || !env->hasRootFolder || !out || maxLen <= 0) return;

    AppAppendContextLine(out, maxLen, "Text File Summaries:");
    count = 0;
    for (i = 0; i < env->projectItemCount && count < 16; i++) {
        ProjectItem* item = &env->projectItems[i];
        char* sample = NULL;
        long sampleLen = 0;
        long totalLines;
        char firstLine[80];
        char line[180];
        long j;
        long k;

        if (item->isFolder || !AppLooksTextFile(item->name)) continue;
        if (AppReadRootFile(env, item->name, &sample, &sampleLen, 900) != noErr || !sample) continue;

        totalLines = AppCountTextLines(sample, sampleLen);
        firstLine[0] = '\0';
        j = 0;
        while (j < sampleLen && (sample[j] == '\r' || sample[j] == '\n' ||
                sample[j] == ' ' || sample[j] == '\t')) {
            j++;
        }
        k = 0;
        while (j < sampleLen && sample[j] != '\r' && sample[j] != '\n' &&
                k < (long)sizeof(firstLine) - 1) {
            firstLine[k++] = sample[j++];
        }
        firstLine[k] = '\0';
        if (!firstLine[0]) strcpy(firstLine, "(empty or whitespace)");

        sprintf(line, "  %s: %ld bytes, about %ld visible lines, starts: %s",
            item->name, item->size, totalLines, firstLine);
        AppAppendContextLine(out, maxLen, line);
        count++;
        free(sample);
    }
    if (count == 0) AppAppendContextLine(out, maxLen, "  No readable text/code file summaries available.");
}

void AppBuildProjectContext(AppEnv* env, char* out, long maxLen) {
    short i;
    char line[160];

    if (!out || maxLen <= 0) return;
    out[0] = '\0';

    AppAppendContextLine(out, maxLen, "--- AltanAI Project Context ---");
    sprintf(line, "Mode: %s", env->settings.workMode ? "Build" : "Plan");
    AppAppendContextLine(out, maxLen, line);

    if (env->hasRootFolder) {
        sprintf(line, "Root Folder: %s", env->rootName);
        AppAppendContextLine(out, maxLen, line);
        sprintf(line, "Classic Root ID: vRefNum=%d dirID=%ld", env->rootSpec.vRefNum, env->rootDirID);
        AppAppendContextLine(out, maxLen, line);
        if (env->settings.workMode) {
            AppAppendContextLine(out, maxLen,
                "Access Policy: Build Mode. The AI may prepare edits only inside this root folder and its descendant folders. The client stores edit blocks as pending changes; files are modified only after the user chooses File > Apply AI Changes.");
            AppAppendContextLine(out, maxLen,
                "Tool Protocol: To change files, append hidden tool blocks only. Prefer PATCH for edits inside existing files: <ALTANAI_PATCH path=\"filename\"><ALTANAI_OLD>exact current text</ALTANAI_OLD><ALTANAI_NEW>replacement text</ALTANAI_NEW></ALTANAI_PATCH>.");
            AppAppendContextLine(out, maxLen,
                "Tool Protocol continued: PATCH applies only when OLD appears exactly once. Use READ only to acknowledge an inspected file, CREATE for empty new files, APPEND for add/append/bottom/end requests, WRITE only for new files with content or explicit whole-file replacement, and DELETE only for explicit deletion. Use relative visible filenames only. Do not emit edit tool blocks in Plan mode.");
            AppAppendContextLine(out, maxLen,
                "Repeat Append Rule: For repeated text/line requests, do not paste the text many times. Use <ALTANAI_APPEND path=\"file\" repeat=\"100\">one line of text</ALTANAI_APPEND>; the client expands and counts the lines.");
            AppAppendContextLine(out, maxLen,
                "Tool Protocol critical: Never merely say you will use ALTANAI_WRITE/PATCH/etc. If changing a file, include the actual hidden XML-like tool block in the response.");
            AppAppendContextLine(out, maxLen,
                "Tool Protocol strict: Do not use <PATCH>, <REPLACE>, filename=, offset=, or length= syntax. Only ALTANAI_* blocks are recognized by the client.");
            AppAppendContextLine(out, maxLen,
                "Build Result Protocol: For build requests, do not use edit tool blocks. If real compiler output is not provided by the app, say build is not configured. If real output is available, return <ALTANAI_BUILD_RESULT>status: ok/failed; warnings: N; errors: N; output: filename or none</ALTANAI_BUILD_RESULT>.");
            AppAppendContextLine(out, maxLen,
                "Chat Output Rule: Never show full code, full file contents, diffs, or fenced code blocks in the visible answer when using a tool block. Reply with one short sentence such as 'Prepared changes for program.c.' Do not say files were updated or saved until the user applies pending changes. The client will show file +added -removed stats.");
            AppAppendContextLine(out, maxLen,
                "Mac OS 9 Compatibility: Created files must be plain classic-safe text/code, use visible relative filenames only, avoid modern OS APIs, and prefer APIs documented in os9docs-master headers.");
            AppAppendContextLine(out, maxLen,
                "Resource Rule: For Classic resource SIZE templates, avoid unsupported dontSaveScreen and enableOptionSwitch flags; use reserved in those positions.");
            AppAppendContextLine(out, maxLen,
                "New Project Rule: When the user asks to create a new app/project, create a dedicated folder named after the app and include a complete buildable Retro68 project: CMakeLists.txt, src/main.c, and any rsrc/*.r files needed. Do not create only a lone .c file.");
            AppAppendContextLine(out, maxLen,
                "File Reading Rule: Mentioned file contents are attached when available. Never guess unseen file contents; say when only the first and last sections are visible.");
            AppAppendContextLine(out, maxLen,
                "Turkish Request Note: In commands like 'program.c yaz gec', 'gec' means proceed/go ahead; it is not file content. Generate the requested file content with an ALTANAI_WRITE block.");
        } else {
            AppAppendContextLine(out, maxLen,
                "Access Policy: Plan Mode. The AI may inspect and plan only. Do not edit, remove, or create files.");
            AppAppendContextLine(out, maxLen,
                "Preview Protocol: The AI may include hidden PATCH/APPEND/WRITE/CREATE/DELETE tool blocks only to preview a proposed change. The client will validate exact matches and show Preview file +added -removed stats without modifying files.");
            AppAppendContextLine(out, maxLen,
                "Repeat Preview Rule: For repeated text/line previews, do not paste the text many times. Use <ALTANAI_APPEND path=\"file\" repeat=\"100\">one line of text</ALTANAI_APPEND>; the client expands and counts the lines.");
            AppAppendContextLine(out, maxLen,
                "Preview Protocol critical: Never merely say you will use ALTANAI_WRITE/PATCH/etc. For a preview, include the actual hidden XML-like tool block.");
            AppAppendContextLine(out, maxLen,
                "Preview Protocol strict: Do not use <PATCH>, <REPLACE>, filename=, offset=, or length= syntax. Only ALTANAI_* blocks are recognized by the client.");
            AppAppendContextLine(out, maxLen,
                "Plan Mode Rule: Prefer PATCH previews for edits inside existing files. Never claim a change was applied in Plan Mode.");
            AppAppendContextLine(out, maxLen,
                "Resource Rule: For Classic resource SIZE templates, avoid unsupported dontSaveScreen and enableOptionSwitch flags; use reserved in those positions.");
            AppAppendContextLine(out, maxLen,
                "New Project Rule: If proposing a new app/project, propose a complete Retro68 project folder with CMakeLists.txt, src/main.c, and needed rsrc/*.r files, not a lone .c file.");
            AppAppendContextLine(out, maxLen,
                "Build Result Protocol: Build requests are read-only. If real compiler output is not provided by the app, say build is not configured. If real output is available, return <ALTANAI_BUILD_RESULT>status: ok/failed; warnings: N; errors: N; output: filename or none</ALTANAI_BUILD_RESULT>.");
        }
        AppAppendContextLine(out, maxLen, "Visible Folder Listing:");
        for (i = 0; i < env->projectItemCount; i++) {
            if (env->projectItems[i].isFolder) AppAppendContextItem(out, maxLen, &env->projectItems[i]);
        }
        AppAppendContextLine(out, maxLen, "Visible File Listing:");
        for (i = 0; i < env->projectItemCount; i++) {
            if (!env->projectItems[i].isFolder) AppAppendContextItem(out, maxLen, &env->projectItems[i]);
        }
        AppAppendProjectFileSummaries(env, out, maxLen);
        if (env->lastBuildResult[0]) {
            AppAppendContextLine(out, maxLen, "--- Last Build Result ---");
            AppAppendContextLine(out, maxLen, env->lastBuildResult);
            AppAppendContextLine(out, maxLen, "--- End Last Build Result ---");
        }
    } else {
        AppAppendContextLine(out, maxLen, "Root Folder: none selected");
        AppAppendContextLine(out, maxLen,
            "Access Policy: No folder root is selected. The AI must not claim file access or perform file changes.");
    }

}

void AppAppendContextLine(char* out, long maxLen, const char* text) {
    long used;
    long room;

    if (!out || !text || maxLen <= 0) return;
    used = strlen(out);
    if (used >= maxLen - 2) return;
    room = maxLen - used - 1;
    strncat(out, text, room);
    used = strlen(out);
    if (used < maxLen - 2) {
        out[used] = '\n';
        out[used + 1] = '\0';
    }
}

void AppAppendContextItem(char* out, long maxLen, const ProjectItem* item) {
    char line[128];

    if (!item) return;
    if (item->isFolder) {
        sprintf(line, "  [folder] %s", item->name);
    } else {
        sprintf(line, "  [file] %s (%ld bytes)", item->name, item->size);
    }
    AppAppendContextLine(out, maxLen, line);
}

long AppAppendContextFileExcerpt(AppEnv* env, const ProjectItem* item, char* out, long maxLen,
                                        long budget) {
    char line[160];
    long used;

    if (!env || !item || item->isFolder || !out || budget <= 512) return 0;
    if (!AppLooksTextFile(item->name)) return 0;

    used = 0;
    if (item->size <= 3600 || budget <= 4200) {
        char* buffer = NULL;
        long readLen = 0;
        long limit = item->size;
        if (limit > budget) limit = budget;
        if (limit <= 0) limit = 1;

        if (AppReadRootFile(env, item->name, &buffer, &readLen, limit) == noErr && buffer) {
            sprintf(line, "--- File: %s (%ld of %ld bytes shown) ---",
                item->name, readLen, item->size);
            AppAppendContextLine(out, maxLen, line);
            AppAppendContextLine(out, maxLen, buffer);
            AppAppendContextLine(out, maxLen, "--- End File ---");
            used = readLen;
            free(buffer);
        }
    } else {
        char* head = NULL;
        char* tail = NULL;
        long headLen = 0;
        long tailLen = 0;
        long tailOffset;
        long partBudget = budget / 2;
        if (partBudget > 1800) partBudget = 1800;
        if (partBudget < 512) partBudget = 512;

        tailOffset = item->size - partBudget;
        if (tailOffset < 0) tailOffset = 0;

        if (AppReadRootFileSlice(env, item->name, 0, &head, &headLen, partBudget) == noErr &&
                AppReadRootFileSlice(env, item->name, tailOffset, &tail, &tailLen, partBudget) == noErr &&
                head && tail) {
            sprintf(line, "--- File: %s (first %ld bytes, last %ld bytes of %ld) ---",
                item->name, headLen, tailLen, item->size);
            AppAppendContextLine(out, maxLen, line);
            AppAppendContextLine(out, maxLen, head);
            AppAppendContextLine(out, maxLen, "--- Middle omitted; ask before guessing unseen content. ---");
            AppAppendContextLine(out, maxLen, tail);
            AppAppendContextLine(out, maxLen, "--- End File ---");
            used = headLen + tailLen;
        }
        if (head) free(head);
        if (tail) free(tail);
    }

    return used;
}
