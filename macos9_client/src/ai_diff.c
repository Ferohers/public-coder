#include "common.h"

void AppComputeSimpleLineStats(const char* oldText, long oldLen,
                                      const char* newText, long newLen,
                                      long* added, long* removed) {
    AppComputeLineStats(oldText, oldLen, newText, newLen, added, removed);
}

long AppCountTextLines(const char* text, long len) {
    long i;
    long lines;

    if (!text || len <= 0) return 0;
    lines = 1;
    for (i = 0; i < len; i++) {
        if (text[i] == '\r') {
            lines++;
            if (i + 1 < len && text[i + 1] == '\n') i++;
        } else if (text[i] == '\n') {
            lines++;
        }
    }
    if (len > 0 && (text[len - 1] == '\r' || text[len - 1] == '\n')) lines--;
    if (lines < 0) lines = 0;
    return lines;
}

void AppAppendFileStatLine(char* summary, long maxSummary,
                                  const char* path, long added, long removed) {
    char line[128];
    long room;

    if (!summary || !path || maxSummary <= 1) return;
    room = maxSummary - strlen(summary) - 1;
    if (room <= 0) return;
    sprintf(line, "%s +%ld -%ld\n", path, added, removed);
    strncat(summary, line, room);
}

void AppStripFencedCodeBlocks(char* text) {
    char* start;

    if (!text) return;
    while ((start = strstr(text, "```")) != NULL) {
        char* end = strstr(start + 3, "```");
        if (end) {
            end += 3;
            while (*end == '\r' || *end == '\n') end++;
            AppRemoveRange(text, start, end);
        } else {
            *start = '\0';
            break;
        }
    }
}

void AppShortenToolResponse(char* text) {
    char* start;
    char* cut;
    long len;

    if (!text) return;
    start = text;
    while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n') start++;
    if (start != text) memmove(text, start, strlen(start) + 1);

    cut = strpbrk(text, "\r\n");
    if (cut) *cut = '\0';

    len = strlen(text);
    if (len > 260) {
        cut = text + 260;
        while (cut > text && *cut != ' ') cut--;
        if (cut <= text) cut = text + 260;
        *cut = '\0';
    }
}

void AppShortenLongResponse(char* text) {
    char* cut;
    long len;

    if (!text) return;
    if (strstr(text, "```")) {
        AppStripFencedCodeBlocks(text);
        len = strlen(text);
        if (len < 1800) {
            strncat(text,
                "\r[Code block hidden. Ask to show full code if needed.]",
                32767 - len);
            return;
        }
    }

    len = strlen(text);
    if (len <= 1800) return;

    cut = text + 1800;
    while (cut > text + 1500 && *cut != ' ' && *cut != '\r' && *cut != '\n') cut--;
    if (cut <= text + 1500) cut = text + 1800;
    *cut = '\0';
    strcat(text, "\r[Response shortened. Ask to show full code if needed.]");
}

Boolean AppMentionsToolWithoutBlock(const char* text) {
    if (!text) return false;
    if (strstr(text, "<ALTANAI_")) return false;
    if (strstr(text, "ALTANAI_")) return true;
    if (strstr(text, "<PATCH") || strstr(text, "</PATCH>")) return true;
    if (strstr(text, "<REPLACE") || strstr(text, "</REPLACE>")) return true;
    return false;
}

char* AppBuildRepeatedAppendContent(const char* content, long contentLen, long repeatCount,
                                           long* outLen) {
    char* body;
    char* repeated;
    long bodyLen;
    long totalLen;
    long i;
    long pos;
    Boolean bodyEndsNewline;

    if (outLen) *outLen = 0;
    if (!content || contentLen < 0 || repeatCount < 1) return NULL;
    if (repeatCount == 1) {
        body = (char*)malloc(contentLen + 1);
        if (!body) return NULL;
        BlockMoveData(content, body, contentLen);
        body[contentLen] = '\0';
        if (outLen) *outLen = contentLen;
        return body;
    }

    while (contentLen > 0 && (*content == '\r' || *content == '\n')) {
        content++;
        contentLen--;
    }
    while (contentLen > 0 && (content[contentLen - 1] == '\r' ||
            content[contentLen - 1] == '\n')) {
        contentLen--;
    }
    bodyLen = contentLen;
    bodyEndsNewline = false;
    totalLen = (bodyLen + 1) * repeatCount;
    repeated = (char*)malloc(totalLen + 1);
    if (!repeated) return NULL;

    pos = 0;
    for (i = 0; i < repeatCount; i++) {
        if (bodyLen > 0) {
            BlockMoveData(content, repeated + pos, bodyLen);
            pos += bodyLen;
            bodyEndsNewline = repeated[pos - 1] == '\r' || repeated[pos - 1] == '\n';
        }
        if (!bodyEndsNewline) repeated[pos++] = '\r';
    }
    repeated[pos] = '\0';
    if (outLen) *outLen = pos;
    return repeated;
}

char* AppBuildAppendedText(const char* oldText, long oldLen, const char* content,
                                  long contentLen, long* outLen) {
    char* combined;

    if (outLen) *outLen = 0;
    if (!content || contentLen < 0) return NULL;
    if (!oldText || oldLen < 0) oldLen = 0;

    combined = (char*)malloc(oldLen + contentLen + 1);
    if (!combined) return NULL;
    if (oldLen > 0 && oldText) BlockMoveData(oldText, combined, oldLen);
    if (contentLen > 0) BlockMoveData(content, combined + oldLen, contentLen);
    combined[oldLen + contentLen] = '\0';
    if (outLen) *outLen = oldLen + contentLen;
    return combined;
}

short AppCollectLineHashes(const char* text, long len,
                                  unsigned long* hashes, short maxLines) {
    long start;
    long i;
    short count;

    if (!text || len <= 0 || !hashes || maxLines <= 0) return 0;
    start = 0;
    count = 0;
    for (i = 0; i <= len && count < maxLines; i++) {
        if (i == len || text[i] == '\r' || text[i] == '\n') {
            if (i > start || i < len) {
                hashes[count++] = AppHashLineRange(text, start, i);
            }
            if (i < len && text[i] == '\r' && i + 1 < len && text[i + 1] == '\n') i++;
            start = i + 1;
        }
    }
    return count;
}

short AppLineHashLCS(const unsigned long* oldHashes, short oldCount,
                            const unsigned long* newHashes, short newCount) {
    short* prev;
    short* curr;
    short i;
    short j;
    short result;

    if (oldCount <= 0 || newCount <= 0) return 0;
    prev = (short*)calloc(newCount + 1, sizeof(short));
    curr = (short*)calloc(newCount + 1, sizeof(short));
    if (!prev || !curr) {
        if (prev) free(prev);
        if (curr) free(curr);
        return 0;
    }

    for (i = 1; i <= oldCount; i++) {
        for (j = 1; j <= newCount; j++) {
            if (oldHashes[i - 1] == newHashes[j - 1]) {
                curr[j] = prev[j - 1] + 1;
            } else {
                curr[j] = prev[j] > curr[j - 1] ? prev[j] : curr[j - 1];
            }
        }
        {
            short* tmp = prev;
            prev = curr;
            curr = tmp;
            memset(curr, 0, (newCount + 1) * sizeof(short));
        }
    }

    result = prev[newCount];
    free(prev);
    free(curr);
    return result;
}

void AppComputeLineStats(const char* oldText, long oldLen,
                                const char* newText, long newLen,
                                long* added, long* removed) {
    enum { kMaxDiffLines = 700 };
    unsigned long* oldHashes;
    unsigned long* newHashes;
    short oldCount;
    short newCount;
    short lcs;

    if (!added || !removed) return;
    *added = 0;
    *removed = 0;

    if (AppCountTextLines(oldText, oldLen) > kMaxDiffLines ||
            AppCountTextLines(newText, newLen) > kMaxDiffLines) {
        long oldLines = AppCountTextLines(oldText, oldLen);
        long newLines = AppCountTextLines(newText, newLen);
        *added = newLines;
        *removed = oldLines;
        return;
    }

    oldHashes = (unsigned long*)malloc(sizeof(unsigned long) * kMaxDiffLines);
    newHashes = (unsigned long*)malloc(sizeof(unsigned long) * kMaxDiffLines);
    if (!oldHashes || !newHashes) {
        long oldLines = AppCountTextLines(oldText, oldLen);
        long newLines = AppCountTextLines(newText, newLen);
        *added = newLines >= oldLines ? newLines - oldLines : 0;
        *removed = oldLines > newLines ? oldLines - newLines : 0;
        if (oldHashes) free(oldHashes);
        if (newHashes) free(newHashes);
        return;
    }

    oldCount = AppCollectLineHashes(oldText, oldLen, oldHashes, kMaxDiffLines);
    newCount = AppCollectLineHashes(newText, newLen, newHashes, kMaxDiffLines);
    lcs = AppLineHashLCS(oldHashes, oldCount, newHashes, newCount);

    *added = newCount - lcs;
    *removed = oldCount - lcs;

    free(oldHashes);
    free(newHashes);
}

void AppAppendPreviewStatLine(AppEnv* env, char* summary, long maxSummary,
                                     const char* path, long added, long removed) {
    char previewPath[80];

    if (!path) return;
    if (env && env->settings.language == 1) {
        sprintf(previewPath, "\x85nizleme: %s", path);
    } else {
        sprintf(previewPath, "Preview: %s", path);
    }
    AppAppendFileStatLine(summary, maxSummary, previewPath, added, removed);
}
