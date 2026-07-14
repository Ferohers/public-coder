#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef ENABLE_CLASSIC_NETWORK
#define ENABLE_CLASSIC_NETWORK 0
#endif

#if ENABLE_CLASSIC_NETWORK
#include <Files.h>
#include <OpenTransport.h>
#include <OpenTransportProviders.h>
#endif

#include "network.h"

ChatSettings gSettings;

#if ENABLE_CLASSIC_NETWORK
static const char base64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static int gOTInitialized = 0;

/* Base64 Authentication Encoder */
static void Base64Encode(const char* src, char* dst) {
    int len = strlen(src);
    int j = 0;
    int i;
    unsigned char a3[3];
    unsigned char a4[4];

    while (len > 0) {
        int chunk = len < 3 ? len : 3;
        a3[0] = 0; a3[1] = 0; a3[2] = 0;
        for (i = 0; i < chunk; i++) {
            a3[i] = src[j++];
        }

        a4[0] = (a3[0] & 0xfc) >> 2;
        a4[1] = ((a3[0] & 0x03) << 4) + ((a3[1] & 0xf0) >> 4);
        a4[2] = ((a3[1] & 0x0f) << 2) + ((a3[2] & 0xc0) >> 6);
        a4[3] = a3[2] & 0x3f;

        for (i = 0; i < chunk + 1; i++) {
            *dst++ = base64_table[a4[i]];
        }
        while (chunk++ < 3) {
            *dst++ = '=';
        }
        len -= 3;
    }
    *dst = '\0';
}

/* Connects to remote endpoint and performs HTTP POST using Open Transport. */
static long DoHTTPPost(const char* host, int port,
                       const char* path, const char* body,
                       char* outBuf, long maxLen) {
    OSStatus err;
    EndpointRef ep;
    TCall connectCall;
    DNSAddress dnsAddr;
    char dnsName[256];
    char* request;
    int bodyLen;
    long totalSent;
    OTResult bytesSent;
    OTResult bytesRecv;
    char* cursor;
    char userpass[256];
    char encoded_auth[512];
    char auth_header[600];
    char logBuf[512];
    long requestLen;
    int httpStatus;
    char* statusPtr;
    OTFlags flags;
    OTByteCount addrLen;

    if (outBuf && maxLen > 0) {
        outBuf[0] = '\0';
    }

    if (!gOTInitialized) {
        err = InitOpenTransport();
        if (err != noErr) {
            sprintf(logBuf, "OT init failed: %ld (%s)", err, GetMacErrorMessage(err));
            LogDebug(logBuf);
            return (long)err;
        }
        gOTInitialized = 1;
    }

    err = noErr;
    ep = OTOpenEndpoint(OTCreateConfiguration(kTCPName), 0, NULL, &err);
    if (!ep) {
        sprintf(logBuf, "Endpoint open failed: %ld (%s)", err, GetMacErrorMessage(err));
        LogDebug(logBuf);
        return err ? (long)err : -1;
    }

    err = OTSetBlocking(ep);
    if (err != noErr) {
        sprintf(logBuf, "SetBlocking failed: %ld (%s)", err, GetMacErrorMessage(err));
        LogDebug(logBuf);
        OTCloseProvider(ep);
        return (long)err;
    }

    err = OTBind(ep, NULL, NULL);
    if (err != noErr) {
        sprintf(logBuf, "Bind failed: %ld (%s)", err, GetMacErrorMessage(err));
        LogDebug(logBuf);
        OTCloseProvider(ep);
        return (long)err;
    }

    sprintf(dnsName, "%s:%d", host, port);
    memset(&dnsAddr, 0, sizeof(dnsAddr));
    addrLen = OTInitDNSAddress(&dnsAddr, dnsName);

    memset(&connectCall, 0, sizeof(connectCall));
    connectCall.addr.maxlen = sizeof(dnsAddr);
    connectCall.addr.len = addrLen;
    connectCall.addr.buf = (UInt8*)&dnsAddr;

    err = OTConnect(ep, &connectCall, NULL);
    if (err != noErr) {
        sprintf(logBuf, "Connect failed: %ld (%s)", err, GetMacErrorMessage(err));
        LogDebug(logBuf);
        OTCloseProvider(ep);
        return (long)err;
    }

    sprintf(userpass, "%s:%s", gSettings.username, gSettings.password);
    Base64Encode(userpass, encoded_auth);
    sprintf(auth_header, "Authorization: Basic %s\r\n", encoded_auth);

    bodyLen = strlen(body);
    request = (char*)malloc(bodyLen + 1024);
    if (!request) {
        LogDebug("Log: Request malloc failed");
        OTCloseProvider(ep);
        return -1;
    }

    sprintf(request,
        "POST %s HTTP/1.0\r\n"
        "Host: %s:%d\r\n"
        "%s"
        "Content-Type: text/plain\r\n"
        "Accept: text/plain\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n"
        "\r\n"
        "%s",
        path, host, port,
        auth_header,
        bodyLen, body);

    totalSent = 0;
    requestLen = strlen(request);
    while (totalSent < requestLen) {
        bytesSent = OTSnd(ep, request + totalSent, requestLen - totalSent, 0);
        if (bytesSent < 0) {
            sprintf(logBuf, "Send failed: %ld (%s)", bytesSent, GetMacErrorMessage(bytesSent));
            LogDebug(logBuf);
            free(request);
            OTCloseProvider(ep);
            return bytesSent;
        }
        totalSent += bytesSent;
    }
    free(request);

    cursor = outBuf;
    maxLen--;

    while (maxLen > 0) {
        flags = 0;
        bytesRecv = OTRcv(ep, cursor, maxLen, &flags);

        if (bytesRecv <= 0) {
            if (bytesRecv < 0) {
                if (cursor == outBuf) {
                    sprintf(logBuf, "Receive failed: %ld (%s)", bytesRecv, GetMacErrorMessage(bytesRecv));
                    LogDebug(logBuf);
                }
            }
            break;
        }

        cursor += bytesRecv;
        maxLen -= bytesRecv;
    }

    *cursor = '\0';
    OTCloseProvider(ep);

    httpStatus = 0;
    if (strncmp(outBuf, "HTTP/", 5) == 0) {
        statusPtr = strchr(outBuf, ' ');
        if (statusPtr) {
            httpStatus = atoi(statusPtr + 1);
        }
    }

    if (httpStatus >= 300 || (httpStatus > 0 && httpStatus < 200)) {
        return httpStatus;
    }

    return 0;
}

long SendAuthenticatedPrompt(const char* prompt, char* outBuffer, long maxLen) {
    return DoHTTPPost(gSettings.host, gSettings.port, "/api/chat",
                      prompt, outBuffer, maxLen);
}

long SendBuildRequest(const char* body, char* outBuffer, long maxLen) {
    return DoHTTPPost(gSettings.host, gSettings.port, "/api/build",
                      body ? body : "", outBuffer, maxLen);
}

long SendClearRequest(char* outBuffer, long maxLen) {
    return DoHTTPPost(gSettings.host, gSettings.port, "/api/clear",
                      "", outBuffer, maxLen);
}
#else
long SendAuthenticatedPrompt(const char* prompt, char* outBuffer, long maxLen) {
    (void)prompt;
    if (outBuffer && maxLen > 0) {
        strncpy(outBuffer,
            "Classic networking is disabled: OpenTransportAppPPC.o is missing.",
            maxLen - 1);
        outBuffer[maxLen - 1] = '\0';
    }
    return -9001;
}

long SendBuildRequest(const char* body, char* outBuffer, long maxLen) {
    (void)body;
    if (outBuffer && maxLen > 0) {
        strncpy(outBuffer,
            "Classic networking is disabled: OpenTransportAppPPC.o is missing.",
            maxLen - 1);
        outBuffer[maxLen - 1] = '\0';
    }
    return -9001;
}

long SendClearRequest(char* outBuffer, long maxLen) {
    (void)outBuffer;
    if (outBuffer && maxLen > 0) {
        strncpy(outBuffer,
            "Classic networking is disabled: OpenTransportAppPPC.o is missing.",
            maxLen - 1);
        outBuffer[maxLen - 1] = '\0';
    }
    return -9001;
}
#endif
