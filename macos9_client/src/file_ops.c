#include "common.h"

Boolean AppIsSafeRelativePath(const char* path) {
    const char* p;
    short componentLen;

    if (!path || !path[0]) return false;
    if (path[0] == '/' || path[0] == ':' || strchr(path, ':')) return false;

    p = path;
    componentLen = 0;
    while (*p) {
        if (*p == '/') {
            if (componentLen <= 0) return false;
            componentLen = 0;
            p++;
            continue;
        }
        if (*p == '.') {
            if ((p == path || p[-1] == '/') &&
                    (p[1] == '\0' || p[1] == '/' ||
                    (p[1] == '.' && (p[2] == '\0' || p[2] == '/')))) {
                return false;
            }
        }
        componentLen++;
        if (componentLen > 31) return false;
        p++;
    }

    return componentLen > 0;
}

OSErr AppMakeRelativeFSSpec(AppEnv* env, const char* path, FSSpec* outSpec) {
    long dirID;
    const char* p;
    OSErr err;

    if (!env || !env->hasRootFolder || !path || !outSpec) return paramErr;
    if (!AppIsSafeRelativePath(path)) return paramErr;

    dirID = env->rootDirID;
    p = path;
    while (*p) {
        char component[32];
        Str255 pName;
        const char* slash;
        long len;

        slash = strchr(p, '/');
        len = slash ? (slash - p) : (long)strlen(p);
        if (len <= 0 || len > 31) return paramErr;
        BlockMoveData(p, component, len);
        component[len] = '\0';
        ConvertCtoPstr(component, pName);

        err = FSMakeFSSpec(env->rootSpec.vRefNum, dirID, pName, outSpec);
        if (!slash) return err;
        if (err != noErr) return err;

        {
            CInfoPBRec pb;
            Str255 infoName;
            memset(&pb, 0, sizeof(pb));
            BlockMoveData(pName, infoName, pName[0] + 1);
            pb.hFileInfo.ioNamePtr = infoName;
            pb.hFileInfo.ioVRefNum = env->rootSpec.vRefNum;
            pb.hFileInfo.ioFDirIndex = 0;
            pb.hFileInfo.ioDirID = dirID;
            err = PBGetCatInfoSync(&pb);
            if (err != noErr) return err;
            if (!(pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask)) return dirNFErr;
            dirID = pb.dirInfo.ioDrDirID;
        }

        p = slash + 1;
    }

    return paramErr;
}

OSErr AppMakeRelativeFSSpecWithParents(AppEnv* env, const char* path, FSSpec* outSpec) {
    long dirID;
    const char* p;
    OSErr err;

    if (!env || !env->hasRootFolder || !path || !outSpec) return paramErr;
    if (!AppIsSafeRelativePath(path)) return paramErr;

    dirID = env->rootDirID;
    p = path;
    while (*p) {
        char component[32];
        Str255 pName;
        const char* slash;
        long len;

        slash = strchr(p, '/');
        len = slash ? (slash - p) : (long)strlen(p);
        if (len <= 0 || len > 31) return paramErr;
        BlockMoveData(p, component, len);
        component[len] = '\0';
        ConvertCtoPstr(component, pName);

        err = FSMakeFSSpec(env->rootSpec.vRefNum, dirID, pName, outSpec);
        if (!slash) return err;

        if (err == fnfErr) {
            long createdDirID = 0;
            err = FSpDirCreate(outSpec, smSystemScript, &createdDirID);
            if (err != noErr && err != dupFNErr) return err;
            if (createdDirID != 0) {
                dirID = createdDirID;
                p = slash + 1;
                continue;
            }
        } else if (err != noErr) {
            return err;
        }

        if (err == noErr || err == dupFNErr) {
            CInfoPBRec pb;
            Str255 infoName;
            memset(&pb, 0, sizeof(pb));
            BlockMoveData(pName, infoName, pName[0] + 1);
            pb.hFileInfo.ioNamePtr = infoName;
            pb.hFileInfo.ioVRefNum = env->rootSpec.vRefNum;
            pb.hFileInfo.ioFDirIndex = 0;
            pb.hFileInfo.ioDirID = dirID;
            err = PBGetCatInfoSync(&pb);
            if (err != noErr) return err;
            if (!(pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask)) return dirNFErr;
            dirID = pb.dirInfo.ioDrDirID;
        }

        p = slash + 1;
    }

    return paramErr;
}

OSErr AppCreateRootFile(AppEnv* env, const char* fileName) {
    FSSpec spec;
    OSErr err;
    short refNum;

    if (!env || !env->hasRootFolder || !fileName || !fileName[0]) return paramErr;
    if (!AppIsSafeRelativePath(fileName)) return paramErr;

    err = AppMakeRelativeFSSpecWithParents(env, fileName, &spec);
    if (err == noErr) return dupFNErr;
    if (err != fnfErr) return err;

    err = FSpCreate(&spec, APP_CREATOR, 'TEXT', smSystemScript);
    if (err != noErr && err != dupFNErr) return err;
    err = FSpOpenDF(&spec, fsWrPerm, &refNum);
    if (err == noErr) {
        SetEOF(refNum, 0);
        FSClose(refNum);
    }
    AppScanProjectFolder(env);
    return err;
}

char* AppCopyClassicText(const char* text) {
    long srcLen;
    char* out;
    long i;
    long j;

    if (!text) return NULL;
    srcLen = strlen(text);
    out = (char*)malloc(srcLen + 1);
    if (!out) return NULL;

    i = 0;
    j = 0;
    while (i < srcLen) {
        if (text[i] == '\r') {
            out[j++] = '\r';
            i++;
            if (i < srcLen && text[i] == '\n') i++;
        } else if (text[i] == '\n') {
            out[j++] = '\r';
            i++;
        } else {
            out[j++] = text[i++];
        }
    }
    out[j] = '\0';
    return out;
}

OSErr AppWriteRootFile(AppEnv* env, const char* fileName, const char* text) {
    FSSpec spec;
    OSErr err;
    short refNum;
    long bytesToWrite;
    char* classicText;

    if (!env || !env->hasRootFolder || !fileName || !fileName[0] || !text) return paramErr;
    if (!AppIsSafeRelativePath(fileName)) return paramErr;

    classicText = AppCopyClassicText(text);
    if (!classicText) return memFullErr;

    err = AppMakeRelativeFSSpecWithParents(env, fileName, &spec);
    if (err == fnfErr) {
        err = FSpCreate(&spec, APP_CREATOR, 'TEXT', smSystemScript);
    }
    if (err != noErr && err != dupFNErr) {
        free(classicText);
        return err;
    }

    err = FSpOpenDF(&spec, fsWrPerm, &refNum);
    if (err != noErr) {
        free(classicText);
        return err;
    }

    SetFPos(refNum, fsFromStart, 0);
    SetEOF(refNum, 0);
    bytesToWrite = strlen(classicText);
    if (bytesToWrite > 0) {
        err = FSWrite(refNum, &bytesToWrite, classicText);
    }
    FSClose(refNum);
    free(classicText);
    AppScanProjectFolder(env);
    return err;
}

OSErr AppAppendRootFile(AppEnv* env, const char* fileName, const char* text) {
    FSSpec spec;
    OSErr err;
    short refNum;
    long fileLen;
    long bytesToWrite;
    char* classicText;

    if (!env || !env->hasRootFolder || !fileName || !fileName[0] || !text) return paramErr;
    if (!AppIsSafeRelativePath(fileName)) return paramErr;

    classicText = AppCopyClassicText(text);
    if (!classicText) return memFullErr;

    err = AppMakeRelativeFSSpecWithParents(env, fileName, &spec);
    if (err == fnfErr) {
        err = FSpCreate(&spec, APP_CREATOR, 'TEXT', smSystemScript);
    }
    if (err != noErr && err != dupFNErr) {
        free(classicText);
        return err;
    }

    err = FSpOpenDF(&spec, fsWrPerm, &refNum);
    if (err != noErr) {
        free(classicText);
        return err;
    }

    err = GetEOF(refNum, &fileLen);
    if (err == noErr) {
        SetFPos(refNum, fsFromLEOF, 0);
        if (fileLen > 0 && classicText[0] && classicText[0] != '\r') {
            char cr = '\r';
            long one = 1;
            FSWrite(refNum, &one, &cr);
        }
        bytesToWrite = strlen(classicText);
        if (bytesToWrite > 0) {
            err = FSWrite(refNum, &bytesToWrite, classicText);
        }
    }

    FSClose(refNum);
    free(classicText);
    AppScanProjectFolder(env);
    return err;
}

OSErr AppPatchRootFile(AppEnv* env, const char* fileName, const char* oldChunk,
                              const char* newChunk, long* added, long* removed) {
    char* oldFile;
    char* classicOld;
    char* classicNew;
    char* newFile;
    char* first;
    char* second;
    long oldLen;
    long oldChunkLen;
    long newChunkLen;
    long prefixLen;
    long suffixLen;
    long newLen;
    OSErr err;

    if (added) *added = 0;
    if (removed) *removed = 0;
    if (!env || !fileName || !oldChunk || !newChunk) return paramErr;

    oldFile = NULL;
    oldLen = 0;
    err = AppReadRootFile(env, fileName, &oldFile, &oldLen, 30000);
    if (err != noErr || !oldFile) return err;

    classicOld = AppCopyClassicText(oldChunk);
    classicNew = AppCopyClassicText(newChunk);
    if (!classicOld || !classicNew) {
        if (classicOld) free(classicOld);
        if (classicNew) free(classicNew);
        free(oldFile);
        return memFullErr;
    }

    oldChunkLen = strlen(classicOld);
    newChunkLen = strlen(classicNew);
    if (oldChunkLen <= 0) {
        free(classicOld);
        free(classicNew);
        free(oldFile);
        return paramErr;
    }

    first = strstr(oldFile, classicOld);
    second = first ? strstr(first + oldChunkLen, classicOld) : NULL;
    if (!first || second) {
        free(classicOld);
        free(classicNew);
        free(oldFile);
        return paramErr;
    }

    prefixLen = first - oldFile;
    suffixLen = oldLen - prefixLen - oldChunkLen;
    newLen = prefixLen + newChunkLen + suffixLen;
    newFile = (char*)malloc(newLen + 1);
    if (!newFile) {
        free(classicOld);
        free(classicNew);
        free(oldFile);
        return memFullErr;
    }

    BlockMoveData(oldFile, newFile, prefixLen);
    BlockMoveData(classicNew, newFile + prefixLen, newChunkLen);
    BlockMoveData(first + oldChunkLen, newFile + prefixLen + newChunkLen, suffixLen);
    newFile[newLen] = '\0';

    AppComputeSimpleLineStats(oldFile, oldLen, newFile, newLen, added, removed);
    err = AppWriteRootFile(env, fileName, newFile);

    free(newFile);
    free(classicOld);
    free(classicNew);
    free(oldFile);
    return err;
}

OSErr AppPreviewPatchRootFile(AppEnv* env, const char* fileName, const char* oldChunk,
                                     const char* newChunk, long* added, long* removed) {
    char* oldFile;
    char* classicOld;
    char* classicNew;
    char* newFile;
    char* first;
    char* second;
    long oldLen;
    long oldChunkLen;
    long newChunkLen;
    long prefixLen;
    long suffixLen;
    long newLen;
    OSErr err;

    if (added) *added = 0;
    if (removed) *removed = 0;
    if (!env || !fileName || !oldChunk || !newChunk) return paramErr;

    oldFile = NULL;
    oldLen = 0;
    err = AppReadRootFile(env, fileName, &oldFile, &oldLen, 30000);
    if (err != noErr || !oldFile) return err;

    classicOld = AppCopyClassicText(oldChunk);
    classicNew = AppCopyClassicText(newChunk);
    if (!classicOld || !classicNew) {
        if (classicOld) free(classicOld);
        if (classicNew) free(classicNew);
        free(oldFile);
        return memFullErr;
    }

    oldChunkLen = strlen(classicOld);
    newChunkLen = strlen(classicNew);
    if (oldChunkLen <= 0) {
        free(classicOld);
        free(classicNew);
        free(oldFile);
        return paramErr;
    }

    first = strstr(oldFile, classicOld);
    second = first ? strstr(first + oldChunkLen, classicOld) : NULL;
    if (!first || second) {
        free(classicOld);
        free(classicNew);
        free(oldFile);
        return paramErr;
    }

    prefixLen = first - oldFile;
    suffixLen = oldLen - prefixLen - oldChunkLen;
    newLen = prefixLen + newChunkLen + suffixLen;
    newFile = (char*)malloc(newLen + 1);
    if (!newFile) {
        free(classicOld);
        free(classicNew);
        free(oldFile);
        return memFullErr;
    }

    BlockMoveData(oldFile, newFile, prefixLen);
    BlockMoveData(classicNew, newFile + prefixLen, newChunkLen);
    BlockMoveData(first + oldChunkLen, newFile + prefixLen + newChunkLen, suffixLen);
    newFile[newLen] = '\0';

    AppComputeSimpleLineStats(oldFile, oldLen, newFile, newLen, added, removed);

    free(newFile);
    free(classicOld);
    free(classicNew);
    free(oldFile);
    return noErr;
}

OSErr AppDeleteRootFile(AppEnv* env, const char* fileName) {
    FSSpec spec;
    OSErr err;

    if (!env || !env->hasRootFolder || !fileName || !fileName[0]) return paramErr;
    if (!AppIsSafeRelativePath(fileName)) return paramErr;

    err = AppMakeRelativeFSSpec(env, fileName, &spec);
    if (err != noErr) return err;
    err = FSpDelete(&spec);
    AppScanProjectFolder(env);
    return err;
}

OSErr AppReadRootFile(AppEnv* env, const char* fileName, char** outBuffer, long* outLen, long maxBytes) {
    FSSpec spec;
    OSErr err;
    short refNum;
    long fileLen;
    long readLen;
    char* buffer;

    if (outBuffer) *outBuffer = NULL;
    if (outLen) *outLen = 0;
    if (!env || !env->hasRootFolder || !fileName || !fileName[0] || !outBuffer || !outLen) return paramErr;
    if (!AppIsSafeRelativePath(fileName)) return paramErr;

    err = AppMakeRelativeFSSpec(env, fileName, &spec);
    if (err != noErr) return err;

    err = FSpOpenDF(&spec, fsRdPerm, &refNum);
    if (err != noErr) return err;

    err = GetEOF(refNum, &fileLen);
    if (err != noErr) {
        FSClose(refNum);
        return err;
    }

    readLen = fileLen;
    if (readLen > maxBytes) readLen = maxBytes;
    buffer = (char*)malloc(readLen + 1);
    if (!buffer) {
        FSClose(refNum);
        return memFullErr;
    }

    err = FSRead(refNum, &readLen, buffer);
    FSClose(refNum);
    if (err != noErr && err != eofErr) {
        free(buffer);
        return err;
    }

    buffer[readLen] = '\0';
    *outBuffer = buffer;
    *outLen = readLen;
    return noErr;
}

OSErr AppReadRootFileSlice(AppEnv* env, const char* fileName, long offset,
                                  char** outBuffer, long* outLen, long maxBytes) {
    FSSpec spec;
    OSErr err;
    short refNum;
    long fileLen;
    long readLen;
    char* buffer;

    if (outBuffer) *outBuffer = NULL;
    if (outLen) *outLen = 0;
    if (!env || !env->hasRootFolder || !fileName || !fileName[0] || !outBuffer || !outLen) return paramErr;
    if (!AppIsSafeRelativePath(fileName)) return paramErr;
    if (offset < 0 || maxBytes <= 0) return paramErr;

    err = AppMakeRelativeFSSpec(env, fileName, &spec);
    if (err != noErr) return err;

    err = FSpOpenDF(&spec, fsRdPerm, &refNum);
    if (err != noErr) return err;

    err = GetEOF(refNum, &fileLen);
    if (err != noErr) {
        FSClose(refNum);
        return err;
    }
    if (offset > fileLen) offset = fileLen;

    readLen = fileLen - offset;
    if (readLen > maxBytes) readLen = maxBytes;
    buffer = (char*)malloc(readLen + 1);
    if (!buffer) {
        FSClose(refNum);
        return memFullErr;
    }

    SetFPos(refNum, fsFromStart, offset);
    err = FSRead(refNum, &readLen, buffer);
    FSClose(refNum);
    if (err != noErr && err != eofErr) {
        free(buffer);
        return err;
    }

    buffer[readLen] = '\0';
    *outBuffer = buffer;
    *outLen = readLen;
    return noErr;
}

Boolean AppLooksTextFile(const char* name) {
    const char* ext;
    if (!name) return false;
    ext = strrchr(name, '.');
    if (!ext) return true;
    ext++;
    if (AppContainsASCIIInsensitive(ext, "txt")) return true;
    if (AppContainsASCIIInsensitive(ext, "c")) return true;
    if (AppContainsASCIIInsensitive(ext, "h")) return true;
    if (AppContainsASCIIInsensitive(ext, "html")) return true;
    if (AppContainsASCIIInsensitive(ext, "htm")) return true;
    if (AppContainsASCIIInsensitive(ext, "js")) return true;
    if (AppContainsASCIIInsensitive(ext, "css")) return true;
    if (AppContainsASCIIInsensitive(ext, "json")) return true;
    if (AppContainsASCIIInsensitive(ext, "py")) return true;
    if (AppContainsASCIIInsensitive(ext, "md")) return true;
    return false;
}

Boolean AppShouldUploadBuildFile(const char* path, long size) {
    const char* ext;

    if (!path || !path[0] || size <= 0 || size > 30000) return false;
    if (strchr(path, '"')) return false;
    if (AppContainsASCIIInsensitive(path, "cmakelists.txt")) return true;

    ext = strrchr(path, '.');
    if (!ext) return false;
    ext++;
    if (AppEqualASCIIInsensitive(ext, "c")) return true;
    if (AppEqualASCIIInsensitive(ext, "h")) return true;
    if (AppEqualASCIIInsensitive(ext, "r")) return true;
    if (AppEqualASCIIInsensitive(ext, "rez")) return true;
    if (AppEqualASCIIInsensitive(ext, "rsrc")) return true;
    return false;
}

void AppChooseFolder(AppEnv* env) {
    NavDialogOptions options;
    NavReplyRecord reply;
    OSErr err;

    memset(&reply, 0, sizeof(reply));
    err = NavGetDefaultDialogOptions(&options);
    if (err == noErr) {
        ConvertCtoPstr("AltanAI", options.clientName);
        ConvertCtoPstr(env->lang->folderWindowTitle, options.windowTitle);
        ConvertCtoPstr(env->lang->btnChooseFolder, options.actionButtonLabel);
        ConvertCtoPstr(env->lang->btnCancel, options.cancelButtonLabel);
        ConvertCtoPstr(env->lang->folderChooseRoot, options.message);
        options.dialogOptionFlags &= ~kNavAllowMultipleFiles;
    } else {
        memset(&options, 0, sizeof(options));
        options.version = kNavDialogOptionsVersion;
    }

    err = NavChooseFolder(NULL, &reply, &options, NULL, NULL, NULL);
    if (err == noErr && reply.validRecord) {
        FSSpec spec;
        AEKeyword keyword;
        DescType typeCode;
        Size actualSize;

        memset(&spec, 0, sizeof(spec));
        err = AEGetNthPtr(&reply.selection, 1, typeFSS, &keyword, &typeCode,
            &spec, sizeof(spec), &actualSize);
        if (err == noErr) {
            env->rootSpec = spec;
            ConvertPtoCstr(spec.name, env->rootName);
            env->hasRootFolder = true;
            err = AppScanProjectFolder(env);
            if (err == noErr) {
                AppUpdateStatus(env, env->lang->statusFolderSelected);
                if (env->filesWindow) {
                    ShowWindow(env->filesWindow);
                    AppDrawFilesWindow(env);
                }
            } else {
                AppUpdateStatus(env, "Folder scan failed");
            }
        }
    } else if (err != noErr) {
        AppUpdateStatus(env, "Folder choose failed");
    } else {
        AppUpdateStatus(env, "Folder choose canceled");
    }

    NavDisposeReply(&reply);

    if (env->filesWindow) AppDrawFilesWindow(env);
}

void AppClearProjectFolder(AppEnv* env) {
    env->hasRootFolder = false;
    env->rootDirID = 0;
    env->rootName[0] = '\0';
    env->projectItemCount = 0;
    env->filesScrollOffset = 0;
    AppUpdateStatus(env, env->lang->statusFolderCleared);
    if (env->filesWindow) AppDrawFilesWindow(env);
}

OSErr AppScanProjectFolder(AppEnv* env) {
    CInfoPBRec pb;
    Str255 name;
    OSErr err;

    if (!env->hasRootFolder) return fnfErr;

    memset(&pb, 0, sizeof(pb));
    BlockMoveData(env->rootSpec.name, name, env->rootSpec.name[0] + 1);
    pb.hFileInfo.ioNamePtr = name;
    pb.hFileInfo.ioVRefNum = env->rootSpec.vRefNum;
    pb.hFileInfo.ioFDirIndex = 0;
    pb.hFileInfo.ioDirID = env->rootSpec.parID;
    err = PBGetCatInfoSync(&pb);
    if (err != noErr) return err;
    if (!(pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask)) return dirNFErr;

    env->rootDirID = pb.dirInfo.ioDrDirID;
    env->projectItemCount = 0;
    env->filesScrollOffset = 0;

    return AppScanProjectFolderLevel(env, env->rootDirID, "", 0);
}

OSErr AppScanProjectFolderLevel(AppEnv* env, long dirID, const char* prefix, short depth) {
    CInfoPBRec pb;
    Str255 name;
    OSErr err;
    short index;

    if (!env || !env->hasRootFolder) return fnfErr;
    if (depth > 4) return noErr;

    for (index = 1; index < 512 && env->projectItemCount < UI_MAX_PROJECT_ITEMS; index++) {
        ProjectItem* item;
        char leaf[64];
        char path[64];
        Boolean isFolder;
        long childDirID;

        memset(&pb, 0, sizeof(pb));
        name[0] = 0;
        pb.hFileInfo.ioNamePtr = name;
        pb.hFileInfo.ioVRefNum = env->rootSpec.vRefNum;
        pb.hFileInfo.ioFDirIndex = index;
        pb.hFileInfo.ioDirID = dirID;
        err = PBGetCatInfoSync(&pb);
        if (err == fnfErr) break;
        if (err != noErr) return err;

        ConvertPtoCstr(name, leaf);
        if (prefix && prefix[0]) {
            if (strlen(prefix) + strlen(leaf) + 2 > sizeof(path)) continue;
            strcpy(path, prefix);
            strcat(path, "/");
            strcat(path, leaf);
        } else {
            strncpy(path, leaf, sizeof(path) - 1);
            path[sizeof(path) - 1] = '\0';
        }

        isFolder = (pb.hFileInfo.ioFlAttrib & kioFlAttribDirMask) != 0;
        childDirID = isFolder ? pb.dirInfo.ioDrDirID : 0;

        item = &env->projectItems[env->projectItemCount];
        strncpy(item->name, path, sizeof(item->name) - 1);
        item->name[sizeof(item->name) - 1] = '\0';
        item->isFolder = isFolder;
        item->size = isFolder ? 0 : pb.hFileInfo.ioFlLgLen;
        env->projectItemCount++;

        if (isFolder && env->projectItemCount < UI_MAX_PROJECT_ITEMS) {
            err = AppScanProjectFolderLevel(env, childDirID, path, depth + 1);
            if (err != noErr) return err;
        }
    }

    return noErr;
}

void AppSanitizeProjectName(const char* raw, char* out, short maxLen) {
    short i;
    short j;

    if (!out || maxLen <= 1) return;
    out[0] = '\0';
    if (!raw || !raw[0]) raw = "MacApp";

    j = 0;
    for (i = 0; raw[i] && j < maxLen - 1; i++) {
        char c = raw[i];
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                (c >= '0' && c <= '9')) {
            if (j == 0 && c >= '0' && c <= '9') out[j++] = 'A';
            out[j++] = c;
        } else if ((c == '_' || c == '-') && j > 0) {
            out[j++] = '_';
        }
    }
    if (j == 0) {
        strcpy(out, "MacApp");
    } else {
        out[j] = '\0';
    }
}

void AppCreatorFromProjectName(const char* appName, char* out) {
    short i;

    if (!out) return;
    for (i = 0; i < 4; i++) {
        char c = (appName && appName[i]) ? appName[i] : 'X';
        if (c >= 'a' && c <= 'z') c = (char)(c - ('a' - 'A'));
        if (!((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))) c = 'X';
        out[i] = c;
    }
    out[4] = '\0';
}

Boolean AppRootFileExists(AppEnv* env, const char* path) {
    FSSpec spec;

    if (!env || !path || !path[0]) return false;
    return AppMakeRelativeFSSpec(env, path, &spec) == noErr;
}
