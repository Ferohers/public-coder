#include "common.h"

long UTF8ToMacRoman(const char* utf8, char* mac, long maxMac) {
    const unsigned char* s = (const unsigned char*)utf8;
    long di = 0;
    unsigned long cp;

    while (*s && di < maxMac - 1) {
        if (*s < 0x80) {
            mac[di++] = (char)*s++;
        } else if ((*s & 0xE0) == 0xC0) {
            if (s[1] == '\0') {
                mac[di++] = '?';
                break;
            }
            cp = (*s++ & 0x1F) << 6;
            cp |= (*s++ & 0x3F);
            if (cp == 0x0130) mac[di++] = '\xDC';       /* İ */
            else if (cp == 0x0131) mac[di++] = '\xDD';  /* ı */
            else if (cp == 0x015E) mac[di++] = '\xDE';  /* Ş */
            else if (cp == 0x015F) mac[di++] = '\xDF';  /* ş */
            else if (cp == 0x011E) mac[di++] = '\xDA';  /* Ğ */
            else if (cp == 0x011F) mac[di++] = '\xDB';  /* ğ */
            else if (cp == 0x00C4) mac[di++] = '\x80';  /* Ä */
            else if (cp == 0x00C5) mac[di++] = '\x81';  /* Å */
            else if (cp == 0x00C7) mac[di++] = '\x82';  /* Ç */
            else if (cp == 0x00C9) mac[di++] = '\x83';  /* É */
            else if (cp == 0x00D1) mac[di++] = '\x84';  /* Ñ */
            else if (cp == 0x00D6) mac[di++] = '\x85';  /* Ö */
            else if (cp == 0x00DC) mac[di++] = '\x86';  /* Ü */
            else if (cp == 0x00E1) mac[di++] = '\x87';  /* á */
            else if (cp == 0x00E0) mac[di++] = '\x88';  /* à */
            else if (cp == 0x00E2) mac[di++] = '\x89';  /* â */
            else if (cp == 0x00E4) mac[di++] = '\x8A';  /* ä */
            else if (cp == 0x00E3) mac[di++] = '\x8B';  /* ã */
            else if (cp == 0x00E5) mac[di++] = '\x8C';  /* å */
            else if (cp == 0x00E7) mac[di++] = '\x8D';  /* ç */
            else if (cp == 0x00E9) mac[di++] = '\x8E';  /* é */
            else if (cp == 0x00E8) mac[di++] = '\x8F';  /* è */
            else if (cp == 0x00EA) mac[di++] = '\x90';  /* ê */
            else if (cp == 0x00EB) mac[di++] = '\x91';  /* ë */
            else if (cp == 0x00ED) mac[di++] = '\x92';  /* í */
            else if (cp == 0x00EC) mac[di++] = '\x93';  /* ì */
            else if (cp == 0x00EE) mac[di++] = '\x94';  /* î */
            else if (cp == 0x00EF) mac[di++] = '\x95';  /* ï */
            else if (cp == 0x00F1) mac[di++] = '\x96';  /* ñ */
            else if (cp == 0x00F3) mac[di++] = '\x97';  /* ó */
            else if (cp == 0x00F2) mac[di++] = '\x98';  /* ò */
            else if (cp == 0x00F4) mac[di++] = '\x99';  /* ô */
            else if (cp == 0x00F5) mac[di++] = '\x9B';  /* õ */
            else if (cp == 0x00F6) mac[di++] = '\x9A';  /* ö */
            else if (cp == 0x00FC) mac[di++] = '\x9F';  /* ü */
            else if (cp == 0x00FB) mac[di++] = '\xAF';  /* û (standard fallback) */
            else mac[di++] = '?';
        } else if ((*s & 0xF0) == 0xE0) {
            if (s[1] == '\0' || s[2] == '\0') {
                mac[di++] = '?';
                break;
            }
            cp = (*s++ & 0x0F) << 12;
            cp |= (*s++ & 0x3F) << 6;
            cp |= (*s++ & 0x3F);
            if (cp == 0x0130) mac[di++] = '\xDC';
            else if (cp == 0x0131) mac[di++] = '\xDD';
            else if (cp == 0x015E) mac[di++] = '\xDE';
            else if (cp == 0x015F) mac[di++] = '\xDF';
            else if (cp == 0x011E) mac[di++] = '\xDA';
            else if (cp == 0x011F) mac[di++] = '\xDB';
            else mac[di++] = '?';
        } else if ((*s & 0xF8) == 0xF0) {
            if (s[1] == '\0' || s[2] == '\0' || s[3] == '\0') {
                mac[di++] = '?';
                break;
            }
            s += 4; /* skip 4-byte, unmapped */
            mac[di++] = '?';
        } else {
            s++; /* skip continuation or invalid */
            mac[di++] = '?';
        }
    }
    mac[di] = '\0';
    return di;
}

long MacRomanToUTF8(const char* mac, char* utf8, long maxUtf8) {
    const unsigned char* s = (const unsigned char*)mac;
    long di = 0;
    while (*s && di < maxUtf8 - 4) {
        unsigned char c = *s++;
        if (c < 0x80) {
            utf8[di++] = (char)c;
        } else {
            unsigned long cp = 0;
            switch (c) {
                case 0xDC: cp = 0x0130; break; /* İ */
                case 0xDD: cp = 0x0131; break; /* ı */
                case 0xDE: cp = 0x015E; break; /* Ş */
                case 0xDF: cp = 0x015F; break; /* ş */
                case 0xDA: cp = 0x011E; break; /* Ğ */
                case 0xDB: cp = 0x011F; break; /* ğ */
                case 0x82: cp = 0x00C7; break; /* Ç */
                case 0x8D: cp = 0x00E7; break; /* ç */
                case 0x9B: cp = 0x00F5; break; /* õ */
                case 0x9A: cp = 0x00F6; break; /* ö */
                case 0x86: cp = 0x00DC; break; /* Ü */
                case 0x9F: cp = 0x00FC; break; /* ü */
                /* Standard Mac Roman fallbacks */
                case 0x80: cp = 0x00C4; break; /* Ä */
                case 0x81: cp = 0x00C5; break; /* Å */
                case 0x83: cp = 0x00C9; break; /* É */
                case 0x84: cp = 0x00D1; break; /* Ñ */
                case 0x87: cp = 0x00E1; break; /* á */
                case 0x88: cp = 0x00E0; break; /* à */
                case 0x89: cp = 0x00E2; break; /* â */
                case 0x8A: cp = 0x00E4; break; /* ä */
                case 0x8B: cp = 0x00E3; break; /* ã */
                case 0x8C: cp = 0x00E5; break; /* å */
                case 0x8E: cp = 0x00E9; break; /* é */
                case 0x8F: cp = 0x00E8; break; /* è */
                case 0x90: cp = 0x00EA; break; /* ê */
                case 0x91: cp = 0x00EB; break; /* ë */
                case 0x92: cp = 0x00ED; break; /* í */
                case 0x93: cp = 0x00EC; break; /* ì */
                case 0x94: cp = 0x00EE; break; /* î */
                case 0x95: cp = 0x00EF; break; /* ï */
                case 0x96: cp = 0x00F1; break; /* ñ */
                case 0x97: cp = 0x00F3; break; /* ó */
                case 0x98: cp = 0x00F2; break; /* ò */
                case 0x99: cp = 0x00F4; break; /* ô */
                case 0x9C: cp = 0x00FA; break; /* ú */
                case 0x9D: cp = 0x00F9; break; /* ù */
                case 0x9E: cp = 0x00FB; break; /* û */
                case 0xBF: cp = 0x00F8; break; /* ø */
                default: cp = c; break;
            }
            if (cp < 0x80) {
                utf8[di++] = (char)cp;
            } else if (cp < 0x800) {
                utf8[di++] = (char)(0xC0 | (cp >> 6));
                utf8[di++] = (char)(0x80 | (cp & 0x3F));
            } else if (cp < 0x10000) {
                utf8[di++] = (char)(0xE0 | (cp >> 12));
                utf8[di++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                utf8[di++] = (char)(0x80 | (cp & 0x3F));
            } else {
                utf8[di++] = (char)(0xF0 | (cp >> 18));
                utf8[di++] = (char)(0x80 | ((cp >> 12) & 0x3F));
                utf8[di++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                utf8[di++] = (char)(0x80 | (cp & 0x3F));
            }
        }
    }
    utf8[di] = '\0';
    return di;
}

const char* GetMacErrorMessage(long err) {
    switch (err) {
        case 0: return "noErr (Success)";
        case -50: return "paramErr (Error in parameter list)";
        case -108: return "memFullErr (Not enough room in heap zone)";
        case -3200: return "kOTOutOfMemoryErr (Open Transport out of memory)";
        case -3201: return "kOTNotFoundErr (Open Transport/TCP not found or disabled)";
        case -3202: return "kOTBadNameErr (Bad Open Transport endpoint/configuration name)";
        case -3203: return "kOTBadAddressErr (Bad Open Transport address)";
        case -3210: return "kOTSndErr (Open Transport send failed)";
        case -3214: return "kOTBlockingErr (Open Transport would block)";
        case -3150: return "kOTBadAddressErr (Bad address format or family)";
        case -3151: return "kOTBadOptionErr (Bad option specification)";
        case -3152: return "kOTAccessErr (Permission denied)";
        case -3153: return "kOTBadReferenceErr (Bad provider reference)";
        case -3154: return "kOTNoAddressErr (Address not bound)";
        case -3155: return "kOTOutStateErr (Interface in wrong state)";
        case -3156: return "kOTBadSequenceErr (Bad sequence number)";
        case -3157: return "kOTSysErrorErr (OS system error)";
        case -3158: return "kOTBadDataErr (Bad data structure)";
        case -3159: return "kOTBufferOverflowErr (Buffer overflow)";
        case -3160: return "kOTFlowErr (Flow control error / Blocking)";
        case -3161: return "kOTNoDataErr (No data available)";
        case -3162: return "kOTNoDisconnectErr (No disconnect indication found)";
        case -3163: return "kOTNoOrderReleaseErr (No orderly release indication found)";
        case -3164: return "kOTBadProviderErr (Bad provider type)";
        case -3165: return "kOTBadResErr (Bad resource)";
        case -3166: return "kOTStateChangeErr (State is changing)";
        case -3167: return "kOTStructureTypeErr (Bad structure type)";
        case -3168: return "kOTNotSupportedErr (Feature not supported)";
        case -3169: return "kOTNonBlockingErr (Non-blocking operation would block)";
        case -3170: return "kOTProviderMismatchErr (Provider mismatch)";
        case -3211: return "kOTAddressBusyErr (Address is already in use)";
        case -3212: return "kOTResAddressErr (Reserved address)";
        case -3213: return "kOTResPortErr (Reserved port)";
        default: return "Unknown OS/OT Error";
    }
}

void LogDebug(const char* msg) {
    if (!gApp.transcriptTe || !msg || !msg[0]) return;

    AppAppendTextColored(&gApp, msg, &kSystemColor);

    /* Force immediate screen refresh safely without BeginUpdate/EndUpdate */
    if (gApp.window) {
        SetPort(gApp.window);
        
        Rect bounds;
        GetWindowPortBounds(gApp.window, &bounds);
        
        EraseRect(&bounds);
        /* Draw themed header background before controls and text views */
        {
            Rect headerRect = bounds;
            headerRect.top = 0;
            headerRect.bottom = 32;
            DrawThemeWindowHeader(&headerRect, IsWindowHilited(gApp.window) ? kThemeStateActive : kThemeStateInactive);
        }

        if (gApp.transcriptTe) {
            TEUpdate(&(**gApp.transcriptTe).viewRect, gApp.transcriptTe);
        }
        if (gApp.inputTe) {
            TEUpdate(&(**gApp.inputTe).viewRect, gApp.inputTe);
        }

        DrawControls(gApp.window);
        AppDrawDecorations(&gApp);
    }
}

void ConvertCtoPstr(const char* src, StringPtr dst) {
    int len = strlen(src);
    if (len > 255) len = 255;
    dst[0] = len;
    memcpy(dst + 1, src, len);
}

void ConvertPtoCstr(ConstStringPtr src, char* dst) {
    int len = src[0];
    memcpy(dst, src + 1, len);
    dst[len] = '\0';
}

char AppLowerASCII(char c) {
    if (c >= 'A' && c <= 'Z') return (char)(c + ('a' - 'A'));
    return c;
}

Boolean AppContainsASCIIInsensitive(const char* haystack, const char* needle) {
    long i;
    long j;
    long hayLen;
    long needleLen;

    if (!haystack || !needle || !needle[0]) return false;
    hayLen = strlen(haystack);
    needleLen = strlen(needle);
    if (needleLen > hayLen) return false;

    for (i = 0; i <= hayLen - needleLen; i++) {
        for (j = 0; j < needleLen; j++) {
            if (AppLowerASCII(haystack[i + j]) != AppLowerASCII(needle[j])) break;
        }
        if (j == needleLen) return true;
    }
    return false;
}

Boolean AppEqualASCIIInsensitive(const char* a, const char* b) {
    if (!a || !b) return false;
    while (*a && *b) {
        if (AppLowerASCII(*a) != AppLowerASCII(*b)) return false;
        a++;
        b++;
    }
    return *a == '\0' && *b == '\0';
}

Boolean AppIsFilenameStop(char c) {
    unsigned char uc = (unsigned char)c;
    if (c == '\0' || c == ' ' || c == '\t' || c == '\r' || c == '\n') return true;
    if (c == '"' || c == '\'' || c == ',' || c == ';' || c == ')' || c == '(') return true;
    if (c == '<' || c == '>' || c == '[' || c == ']') return true;
    if (uc < 32) return true;
    return false;
}

const char* AppFindASCIIInsensitivePtr(const char* haystack, const char* needle) {
    long i;
    long j;
    long hayLen;
    long needleLen;

    if (!haystack || !needle || !needle[0]) return NULL;
    hayLen = strlen(haystack);
    needleLen = strlen(needle);
    if (needleLen > hayLen) return NULL;

    for (i = 0; i <= hayLen - needleLen; i++) {
        for (j = 0; j < needleLen; j++) {
            if (AppLowerASCII(haystack[i + j]) != AppLowerASCII(needle[j])) break;
        }
        if (j == needleLen) return haystack + i;
    }
    return NULL;
}

void AppRemoveRange(char* text, char* start, char* end) {
    if (!text || !start || !end || end < start) return;
    memmove(start, end, strlen(end) + 1);
}

unsigned long AppHashLineRange(const char* text, long start, long end) {
    unsigned long hash;
    long i;

    hash = 2166136261UL;
    for (i = start; i < end; i++) {
        hash ^= (unsigned char)text[i];
        hash *= 16777619UL;
    }
    return hash;
}
