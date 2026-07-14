#include <string.h>
#include "json_mini.h"

/* State Machine Constants */
typedef enum {
    STATE_SCANNING,
    STATE_IN_KEY,
    STATE_FOUND_KEY,
    STATE_MATCHED_COLON,
    STATE_IN_VALUE_STRING
} JsonParseState;

/* Finds key inside JSON string and returns a pointer to the value */
const char* json_find_value(const char* json, const char* key) {
    if (!json || !key || !key[0]) return (const char*)0;

    const char* ptr = json;
    size_t keyLen = strlen(key);

    while (*ptr) {
        /* Search for key token enclosed in quotes */
        if (*ptr == '"') {
            const char* start = ++ptr;
            while (*ptr && *ptr != '"') {
                ptr++;
            }
            if (*ptr == '"') {
                size_t matchLen = ptr - start;
                if (matchLen == keyLen && memcmp(start, key, keyLen) == 0) {
                    /* Key matches! Advance past the closing quote and skip whitespace to find colon */
                    ptr++;
                    while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
                        ptr++;
                    }
                    if (*ptr == ':') {
                        ptr++; /* Move past colon */
                        while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
                            ptr++;
                        }
                        return ptr; /* Points to the value start */
                    }
                }
            }
        }
        if (*ptr) ptr++;
    }

    return (const char*)0;
}

/* Safely extracts a string value associated with key from the JSON */
int json_extract_string(const char* json, const char* key, char* out, int maxLen) {
    if (!json || !key || !out || maxLen <= 0) {
        if (out && maxLen > 0) out[0] = '\0';
        return 0;
    }

    const char* val = json_find_value(json, key);
    if (!val || *val != '"') {
        out[0] = '\0';
        return 0;
    }

    val++; /* Move past opening quote */
    int len = 0;

    while (*val && *val != '"' && len < maxLen - 1) {
        if (*val == '\\') {
            val++; /* Skip escape character */
            if (*val == '\0') {
                break; /* Guard: prevent reading past string end */
            }
            /* Map escape code */
            switch (*val) {
                case 'n': out[len++] = '\n'; break;
                case 't': out[len++] = '\t'; break;
                case '\\': out[len++] = '\\'; break;
                case '"': out[len++] = '"'; break;
                default: out[len++] = *val; break;
            }
        } else {
            out[len++] = *val;
        }
        val++;
    }

    out[len] = '\0';
    return len;
}
