#ifndef NETWORK_H
#define NETWORK_H

typedef struct {
    char host[128];
    int port;
    char username[64];
    char password[64];
    char fontName[32];
    int fontSize;
    int language;   /* 0 = English, 1 = Turkish */
    int workMode;   /* 0 = Plan, 1 = Build */
    int remoteCompileEnabled; /* 0 = off, 1 = on */
} ChatSettings;

extern ChatSettings gSettings;

long SendAuthenticatedPrompt(const char* prompt, char* outBuffer, long maxLen);
long SendBuildRequest(const char* body, char* outBuffer, long maxLen);
long SendClearRequest(char* outBuffer, long maxLen);

/* Shared debug logging callback */
extern void LogDebug(const char* msg);

/* Shared error message converter */
extern const char* GetMacErrorMessage(long err);

#endif
