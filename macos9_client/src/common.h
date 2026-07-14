#ifndef COMMON_H
#define COMMON_H

#include <MacTypes.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Menus.h>
#include <TextEdit.h>
#include <Controls.h>
#include <ControlDefinitions.h>
#include <Dialogs.h>
#include <Events.h>
#include <Appearance.h>
#include <Balloons.h>
#include <Sound.h>
#include <Files.h>
#include <Folders.h>
#include <Resources.h>
#include <Processes.h>
#include <ToolUtils.h>
#include <LowMem.h>
#include <DateTimeUtils.h>
#include <AppleEvents.h>
#include <Navigation.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "network.h"
#include <InternetConfig.h>

/* UTF-8 to Mac Roman decoder for AI responses - Safe implementation */


/* Layout Configuration */
#define UI_MARGIN 16
#define UI_TEXT_INSET 5
#define UI_GAP 12
#define UI_SCROLL_WIDTH 16
#define UI_INPUT_HEIGHT 56
#define UI_PENDING_STRIP_HEIGHT 34
#define UI_BTN_WIDTH 84
#define UI_BTN_HEIGHT 28
#define UI_MODE_WIDTH 84
#define UI_MODE_HEIGHT UI_BTN_HEIGHT
#define UI_TOOL_BTN_SIZE 24
#define UI_FOOTER_HEIGHT 16
#define UI_STATUS_HEIGHT 8
#define UI_MIN_WIDTH 420
#define UI_MIN_HEIGHT 340
#define UI_FOLDER_WIN_WIDTH 250
#define UI_FOLDER_WIN_HEIGHT 440
#define UI_FILES_WIN_WIDTH 336
#define UI_FILES_WIN_HEIGHT 260
#define UI_MAX_PROJECT_ITEMS 128
#define UI_EDITOR_WIN_WIDTH 420
#define UI_EDITOR_WIN_HEIGHT 420
#define UI_EDITOR_MAX_TABS 12
#define UI_EDITOR_MAX_FILE_BYTES 32000
#define UI_EDITOR_GUTTER_WIDTH 42
#define UI_EDITOR_TOOL_HEIGHT 34
#define UI_EDITOR_TAB_HEIGHT 24
#define UI_EDITOR_TAB_WIDTH 86
#define UI_EDITOR_TAB_GAP 2
#define UI_EDITOR_TAB_MIN_WIDTH 58
#define UI_EDITOR_TAB_ARROW_WIDTH 16
#define UI_MAX_UNDO_FILES 8
#define UI_MAX_CONTEXT_BYTES 12000
#define UI_MAX_BUILD_SNAPSHOT_BYTES 60000
#define PREFS_MAGIC 0x4D394149UL
#define PREFS_VERSION 4
#define APP_CREATOR 'ALTN'
#ifndef ENABLE_CLASSIC_NETWORK
#define ENABLE_CLASSIC_NETWORK 0
#endif
#if ENABLE_CLASSIC_NETWORK
#define APP_BUILD_LABEL "Build " __DATE__ " " __TIME__ " network enabled"
#else
#define APP_BUILD_LABEL "Build " __DATE__ " " __TIME__ " network disabled"
#endif
#define PREFS_VOL_REF ((int16_t)-32768)

/* ===========================================================================
   UI String Tables — English [0] and Turkish [1]
   Turkish Mac Roman positions (non-standard, match our UTF8ToMacRoman table):
     \xF0=\u0131 (i), \xDC=\u015F (s), \xBE=\u011F (g), \xD0=\u0130 (I)
     \xAD=\u015E (S), \xB0=\u011E (G)
   Standard Mac Roman also used: \x82=C, \x8D=c, \x85=O, \x9B=o, \x86=U, \xA0=u
   aiLangPrefix is valid UTF-8 — prepended to each prompt sent to the server.
   =========================================================================*/
typedef struct {
    const char* menuFile;        /* File menu title */
    const char* menuSettings;    /* Settings... item */
    const char* menuRemoteCompile; /* Remote compile toggle item */
    const char* menuNewProject;  /* Create starter project item */
    const char* menuBuildProject; /* Build Project item */
    const char* menuFixBuild;    /* Fix Build item */
    const char* menuShowArtifact; /* Show last artifact item */
    const char* menuApplyAI;     /* Apply pending AI changes item */
    const char* menuRejectAI;    /* Reject pending AI changes item */
    const char* menuClear;       /* Clear item */
    const char* menuQuit;        /* Quit item */
    const char* menuEdit;        /* Edit menu title */
    const char* menuUndo;        /* Undo item */
    const char* menuCut;         /* Cut item */
    const char* menuCopy;        /* Copy item */
    const char* menuPaste;       /* Paste item */
    const char* menuSelectAll;   /* Select All item */
    const char* menuEraseSel;    /* Erase selection item */
    const char* ttClear;         /* Clear button tooltip */
    const char* ttHistory;       /* History button tooltip */
    const char* ttSettings;      /* Settings button tooltip */
    const char* ttBuildProject;  /* Build action tooltip */
    const char* ttApplyAI;       /* Apply pending AI changes tooltip */
    const char* ttRejectAI;      /* Reject pending AI changes tooltip */
    const char* statusReady;     /* "Ready" */
    const char* statusCleared;   /* "Cleared" */
    const char* statusWaiting;   /* "Waiting for reply..." */
    const char* statusError;     /* "Error" */
    const char* statusBridgeErr; /* "Bridge error" */
    const char* statusConnFail;  /* "Connection failed" */
    const char* statusNoHist;    /* "No history" */
    const char* statusHistClrd;  /* "History cleared" */
    const char* histNewChat;     /* session separator in history file */
    const char* histWhen;        /* "When: " prefix for timestamp */
    const char* transcriptTrim;  /* truncation notice in chat window */
    const char* welcomeMsg;      /* startup hint shown in transcript */
    const char* aiLangPrefix;    /* UTF-8 prepended to every prompt (may be empty) */
    const char* dlgLangLabel;    /* "Language:" label in Settings dialog */
    const char* btnSend;         /* Send button label (has leading spaces for icon) */
    const char* buildPrefix;     /* prefix: "Build " or "Derlenme Zamani: " */
    const char* buildNetEnabled;  /* network enabled */
    const char* buildNetDisabled; /* network disabled */
    const char* lblYou;          /* "You" or "Sen" */
    const char* lblAI;           /* "AI" */
    const char* lblSystem;       /* "System" or "Sistem" */
    const char* lblError;        /* "Error" or "Hata" */
    const char* lblBridgeError;  /* "Bridge Error" or "Kopru Hatasi" */
    const char* filesWindowTitle;
    const char* filesHeader;
    const char* filesChoose;
    const char* filesNone;
    const char* folderWindowTitle;
    const char* folderNone;
    const char* folderChooseRoot;
    const char* folderNoSubfolders;
    const char* btnChooseFolder;
    const char* btnCancel;
    const char* statusFolderSelected;
    const char* statusFolderCleared;
    const char* statusBuildMode;
    const char* statusPlanMode;
    const char* statusBuildStarted;
    const char* modeBuild;
    const char* modePlan;
    const char* editorSave;
    const char* editorFind;
    const char* editorLine;
    const char* editorNext;
} UIStrings;

static const UIStrings kStrings[2] = {
    { /* [0] English */
        "File",
        "Settings...",
        "Remote Compile",
        "Create Starter Project",
        "Build Project",
        "Fix Build",
        "Show Artifact",
        "Apply AI Changes",
        "Reject AI Changes",
        "Clear",
        "Quit",
        "Edit",
        "Undo",
        "Cut",
        "Copy",
        "Paste",
        "Select All",
        "Clear",
        "Clear Chat Window",
        "View Saved History",
        "Connection Settings",
        "Build Project",
        "Apply Pending AI Changes",
        "Reject Pending AI Changes",
        "Ready",
        "Cleared",
        "Please wait . . .",
        "Error",
        "Bridge error",
        "Connection failed",
        "No history",
        "History cleared",
        "---- New chat ----",
        "When: ",
        "[Transcript trimmed]...",
        "Type a message and press Return.",
        "",   /* no AI language prefix in English */
        "Language:",
        "      Send",  /* 6 leading spaces push text right of the 16px icon */
        "Build ",
        "network enabled",
        "network disabled",
        "You",
        "AI",
        "System",
        "Error",
        "Bridge Error",
        "Files",
        "File List",
        "Choose a folder to list files.",
        "No files in this folder.",
        "Folders",
        "No folder selected",
        "Choose a folder root.",
        "No subfolders.",
        "Choose...",
        "Cancel",
        "Folder selected",
        "Folder cleared",
        "Build Mode",
        "Plan Mode",
        "Build started",
        "  Build",
        "  Plan",
        "Save",
        "Find",
        "Line",
        "Next"
    },
    { /* [1] Turkish — Mac OS Turkish standard mapping (ğ=0xDB, ı=0xDD, ş=0xDF, etc.) */
        "Dosya",
        "Ayarlar...",
        "Uzak Derleme",
        "Yeni Proje Olustur",
        "Projeyi Derle",
        "Derlemeyi D\x9Fzelt",
        "Ciktiyi Goster",
        "AI De\xDBi\xDFikliklerini Uygula",
        "AI De\xDBi\xDFikliklerini Reddet",
        "Temizle",
        "\x82\xDDk",                /* Cik (C=0x82, i=0xDD) */
        "D\x9Fzenle",               /* Duzenle (u=0x9F) */
        "Geri Al",
        "Kes",
        "Kopyala",
        "Yap\xDD\xDFt\xDDr",       /* Yapistir (i=0xDD, s=0xDF) */
        "T\x9Fm\x9Fn\x9F Se\x8D",
        "Temizle",
        "Sohbeti Temizle",
        "Ge\x8Dmi\xDFi G\x9Ar",    /* Gecmisi Gor (c=0x8D, s=0xDF, o=0x9A) */
        "Ba\xDBlant\xDD Ayarlar\xDD",  /* Baglanti Ayarlari (g=0xDB, i=0xDD) */
        "Projeyi Derle",
        "Bekleyen AI De\xDBi\xDFikliklerini Uygula",
        "Bekleyen AI De\xDBi\xDFikliklerini Reddet",
        "Haz\xDDr",                 /* Hazir (i=0xDD) */
        "Temizlendi",
        "L\x9Ftfen Bekleyin . . .",   /* Yanit bekleniyor... (i=0xDD) */
        "Hata",
        "K\x9Apr\x9F hatas\xDD",   /* Kopru hatasi (o=0x9A, u=0x9F, i=0xDD) */
        "Ba\xBElant\xDD hatas\xDD", /* Baglanti hatasi (g=0xDB, i=0xDD) */
        "Ge\x8Dmi\xDF yok",        /* Gecmis yok (c=0x8D, s=0xDF) */
        "Ge\x8Dmi\xDF silindi",    /* Gecmis silindi */
        "---- Yeni sohbet ----",
        "Ne zaman: ",
        "[Kopya kesildi]...",
        "Bir mesaj yaz\xDDp Return'e bas\xDDn.", /* yazip... basin (i=0xDD) */
        "L\xC3\xBCtfen T\xC3\xBCrk\xC3\xA7" "e yan\xC4\xB1t ver. ",
        "Dil:",
        "      G\x9Ander",  /* G + o-umlaut (0x9A) + nder; 6 leading spaces to clear icon */
        "Derlenme Zaman\xDD: ",
        "a\xDB etkin",
        "a\xDB devre d\xDD\xDF\xDD",
        "Sen",
        "AI",
        "Sistem",
        "Hata",
        "K\x9Apr\x9F Hatas\xDD",
        "Dosyalar",
        "Dosyalar",
        "Dosyalar\xDD listelemek i\x8Din klas\x9Ar se\x8Din.",
        "Bu klas\x9Arde dosya yok.",
        "Klas\x9Arler",
        "Klas\x9Ar se\x8Dilmedi",
        "Bir k\x9Ak klas\x9Ar se\x8Din.",
        "Alt klas\x9Ar yok.",
        "Se\x8D...",
        "\xDCptal",
        "Klas\x9Ar se\x8Dildi",
        "Klas\x9Ar temizlendi",
        "Kodlama Modu",
        "Planlama Modu",
        "Derleme ba\xDFlat\xDDld\xDD",
        "  Kodla",
        " Planla",
        "Kaydet",
        "Bul",
        "Sat\xDDr",
        "Di\xDB" "erini Bul"
    }
};

/* Override Toolbox LoWord/HiWord to avoid linkage lookup */
#undef LoWord
#undef HiWord
#define LoWord(x) ((short)(x))
#define HiWord(x) ((short)((x) >> 16))

/* Application State */
typedef struct {
    UInt32 magic;
    UInt16 version;
    UInt16 size;
    ChatSettings settings;
} AppPrefsRecord;

typedef struct {
    char name[64];
    long size;
    Boolean isFolder;
} ProjectItem;

typedef struct {
    Boolean inUse;
    Boolean hadOldFile;
    char path[64];
    char* oldText;
    char* newText;
} UndoFileChange;

typedef struct {
    Boolean inUse;
    Boolean dirty;
    char path[64];
} EditorTab;

typedef struct {
    WindowPtr window;
    WindowPtr filesWindow;
    WindowPtr editorWindow;
    ControlHandle scrollbar;
    ControlHandle sendBtn;
    ControlHandle modeBtn;
    ControlHandle clearBtn;
    ControlHandle historyBtn;
    ControlHandle settingsBtn;
    ControlHandle buildActionBtn;
    ControlHandle applyPendingBtn;
    ControlHandle rejectPendingBtn;
    ControlHandle chooseFolderBtn;
    ControlHandle cancelFolderBtn;
    ControlHandle filesScrollbar;
    ControlHandle editorSaveBtn;
    ControlHandle editorSearchBtn;
    ControlHandle editorGoLineBtn;
    ControlHandle editorNextBtn;
    ControlHandle editorScrollbar;
    TEHandle transcriptTe;
    TEHandle inputTe;
    TEHandle editorTe;
    TEHandle activeTe;
    CursHandle ibeamCursor;
    UserItemUPP okFrameUPP;
    UserItemUPP aboutFrameUPP;
    ControlActionUPP scrollActionUPP;
    ControlActionUPP editorScrollActionUPP;
    ControlActionUPP filesScrollActionUPP;
    ControlActionUPP sendTrackActionUPP;
    char status[256];
    Boolean loadingHistory;
    Boolean historySessionStarted;
    ChatSettings settings;
    FSSpec rootSpec;
    long rootDirID;
    char rootName[64];
    Boolean hasRootFolder;
    ProjectItem projectItems[UI_MAX_PROJECT_ITEMS];
    short projectItemCount;
    short filesScrollOffset;
    EditorTab editorTabs[UI_EDITOR_MAX_TABS];
    short editorTabCount;
    short activeEditorTab;
    short editorTabScroll;
    Boolean hasUndoChange;
    Boolean undoTransactionOpen;
    short undoChangeCount;
    char undoTimestamp[32];
    char undoMessage[260];
    UndoFileChange undoChanges[UI_MAX_UNDO_FILES];
    char* pendingAIChangeText;
    char pendingAIChangeSummary[512];
    char lastBuildResult[1400];
    char lastArtifactPath[160];
    char lastSearchNeedle[64];
    IconRef clearIcon;
    IconRef historyIcon;
    IconRef settingsIcon;
    IconRef buildIcon;
    IconRef planIcon;
    IconRef docIcon;
    IconRef folderIcon;
    const UIStrings* lang;  /* points to kStrings[settings.language] */
    Boolean isBusy;
} AppEnv;

static AppEnv gApp;

/* Chat message colors - ICQ-style blue/green */
static const RGBColor kUserColor = { 0x0000, 0x0000, 0xBBBB };   /* Blue for user */
static const RGBColor kAIColor = { 0x0000, 0x8888, 0x0000 };     /* Green for AI */
static const RGBColor kSystemColor = { 0x0000, 0x0000, 0x0000 }; /* Black for system */
static const RGBColor kAddedColor = { 0x0000, 0x8888, 0x0000 };
static const RGBColor kRemovedColor = { 0xBBBB, 0x0000, 0x0000 };
static const RGBColor kLinkColor = { 0x0000, 0x0000, 0xEEEE };    /* Classic Netscape/IE Blue Link */


/* Global Function Prototypes */
long UTF8ToMacRoman(const char* utf8, char* mac, long maxMac);
long MacRomanToUTF8(const char* mac, char* utf8, long maxUtf8);
const char* GetMacErrorMessage(long err);
void LogDebug(const char* msg);
void ConvertCtoPstr(const char* src, StringPtr dst);
void ConvertPtoCstr(ConstStringPtr src, char* dst);
char AppLowerASCII(char c);
Boolean AppContainsASCIIInsensitive(const char* haystack, const char* needle);
Boolean AppEqualASCIIInsensitive(const char* a, const char* b);
Boolean AppIsFilenameStop(char c);
const char* AppFindASCIIInsensitivePtr(const char* haystack, const char* needle);
void AppRemoveRange(char* text, char* start, char* end);
unsigned long AppHashLineRange(const char* text, long start, long end);
void AppModalSettings(AppEnv* env);
void AppModalHistory(AppEnv* env);
void AppModalAbout(AppEnv* env);
char* AppFindHistoryMarker(char* cursor, int* outLen);
char* AppFindHistoryWhen(char* marker, int maxScan, int* outLen);
Boolean AppIsSafeRelativePath(const char* path);
OSErr AppMakeRelativeFSSpec(AppEnv* env, const char* path, FSSpec* outSpec);
OSErr AppMakeRelativeFSSpecWithParents(AppEnv* env, const char* path, FSSpec* outSpec);
OSErr AppCreateRootFile(AppEnv* env, const char* fileName);
char* AppCopyClassicText(const char* text);
OSErr AppWriteRootFile(AppEnv* env, const char* fileName, const char* text);
OSErr AppAppendRootFile(AppEnv* env, const char* fileName, const char* text);
OSErr AppPatchRootFile(AppEnv* env, const char* fileName, const char* oldChunk, const char* newChunk, long* added, long* removed);
OSErr AppPreviewPatchRootFile(AppEnv* env, const char* fileName, const char* oldChunk, const char* newChunk, long* added, long* removed);
OSErr AppDeleteRootFile(AppEnv* env, const char* fileName);
OSErr AppReadRootFile(AppEnv* env, const char* fileName, char** outBuffer, long* outLen, long maxBytes);
OSErr AppReadRootFileSlice(AppEnv* env, const char* fileName, long offset, char** outBuffer, long* outLen, long maxBytes);
Boolean AppLooksTextFile(const char* name);
Boolean AppShouldUploadBuildFile(const char* path, long size);
void AppChooseFolder(AppEnv* env);
void AppClearProjectFolder(AppEnv* env);
OSErr AppScanProjectFolder(AppEnv* env);
OSErr AppScanProjectFolderLevel(AppEnv* env, long dirID, const char* prefix, short depth);
void AppSanitizeProjectName(const char* raw, char* out, short maxLen);
void AppCreatorFromProjectName(const char* appName, char* out);
Boolean AppRootFileExists(AppEnv* env, const char* path);
Boolean AppPromptRequestsRead(const char* prompt);
Boolean AppPromptRequestsWrite(const char* prompt);
Boolean AppPromptRequestsCodeChange(const char* prompt);
Boolean AppPromptAllowsFullCode(const char* prompt);
Boolean AppExtractPromptFilename(const char* prompt, char* outName, short maxLen);
Boolean AppFindMentionedProjectFile(AppEnv* env, const char* prompt, char* outName, short maxLen);
long AppGetTotalProjectTextSize(AppEnv* env);
void AppAppendProjectFileContents(AppEnv* env, const char* prompt, char* out, long maxLen);
void AppBuildProjectSnapshot(AppEnv* env, char* out, long maxLen);
void AppAppendProjectFileSummaries(AppEnv* env, char* out, long maxLen);
void AppBuildProjectContext(AppEnv* env, char* out, long maxLen);
void AppAppendContextLine(char* out, long maxLen, const char* text);
void AppAppendContextItem(char* out, long maxLen, const ProjectItem* item);
long AppAppendContextFileExcerpt(AppEnv* env, const ProjectItem* item, char* out, long maxLen, long budget);
void AppExecuteAIFileOps(AppEnv* env, char* responseText, char* summary, long maxSummary);
void AppAppendBuildResultSummary(AppEnv* env, char* summary, long maxSummary, const char* text, long len);
void AppStoreLastBuildResult(AppEnv* env, const char* text, long len);
void AppClearUndoChange(AppEnv* env);
void AppClearPendingAIChanges(AppEnv* env);
void AppStorePendingAIChanges(AppEnv* env, const char* text, const char* summary);
Boolean AppToolSummaryHasFailure(const char* summary);
Boolean AppToolSummaryHasPreview(const char* summary);
Boolean AppHasEditToolBlock(const char* text);
Boolean AppApplyPendingAIChanges(AppEnv* env);
Boolean AppRejectPendingAIChanges(AppEnv* env);
void AppSaveUndoBackupFile(AppEnv* env, const char* path, const char* oldText, long oldLen, Boolean hadOldFile);
void AppStoreUndoChange(AppEnv* env, const char* path, const char* oldText, long oldLen, Boolean hadOldFile, const char* newText, long newLen, const char* message);
Boolean AppUndoLastAIChange(AppEnv* env);
void AppComputeSimpleLineStats(const char* oldText, long oldLen, const char* newText, long newLen, long* added, long* removed);
long AppCountTextLines(const char* text, long len);
void AppAppendFileStatLine(char* summary, long maxSummary, const char* path, long added, long removed);
void AppStripFencedCodeBlocks(char* text);
void AppShortenToolResponse(char* text);
void AppShortenLongResponse(char* text);
Boolean AppMentionsToolWithoutBlock(const char* text);
char* AppBuildRepeatedAppendContent(const char* content, long contentLen, long repeatCount, long* outLen);
char* AppBuildAppendedText(const char* oldText, long oldLen, const char* content, long contentLen, long* outLen);
short AppCollectLineHashes(const char* text, long len, unsigned long* hashes, short maxLines);
short AppLineHashLCS(const unsigned long* oldHashes, short oldCount, const unsigned long* newHashes, short newCount);
void AppComputeLineStats(const char* oldText, long oldLen, const char* newText, long newLen, long* added, long* removed);
void AppAppendPreviewStatLine(AppEnv* env, char* summary, long maxSummary, const char* path, long added, long removed);
Boolean AppReadHistory(char** outBuffer, long* outLen);
void AppShowHistory(AppEnv* env);
void AppAppendHistoryText(AppEnv* env, const char* text);
void AppAppendHistoryLine(AppEnv* env, const char* text);
void AppBeginHistorySession(AppEnv* env);
void AppCurrentTimestamp(AppEnv* env, char* out, int maxLen);
void AppGetBuildDateString(AppEnv* env, char* out, int maxLen);
void AppOpenEditorFile(AppEnv* env, const char* path);
void AppLoadEditorTab(AppEnv* env, short tabIndex);
short AppEditorVisibleTabCapacity(AppEnv* env);
short AppEditorTabStartX(AppEnv* env);
void AppClampEditorTabScroll(AppEnv* env);
void AppEnsureActiveEditorTabVisible(AppEnv* env);
void AppEditorLeftArrowRect(AppEnv* env, Rect* outRect);
void AppEditorRightArrowRect(AppEnv* env, Rect* outRect);
short AppEditorTabWidth(AppEnv* env);
void AppEditorTabRect(AppEnv* env, short index, Rect* outRect);
void AppEditorTabCloseRect(const Rect* tabRect, Rect* outRect);
Boolean AppCloseEditorTab(AppEnv* env, short tabIndex);
void AppDrawEditorWindow(AppEnv* env);
void AppDrawEditorToolButtons(AppEnv* env);
void AppDrawEditorTabs(AppEnv* env);
void AppDrawEditorLineNumbers(AppEnv* env);
void AppSyncEditorMetrics(AppEnv* env);
void AppScrollEditorTo(AppEnv* env, int targetOffset);
void AppAdjustEditorScrollbar(AppEnv* env);
void AppSaveEditorFile(AppEnv* env);
void AppMarkEditorDirty(AppEnv* env);
Boolean AppPromptEditorValue(const char* title, const char* label, char* out, short maxLen);
void AppFindInEditor(AppEnv* env);
void AppFindNextInEditor(AppEnv* env);
void AppGoToEditorLine(AppEnv* env);
short AppCurrentFontID(AppEnv* env);
void AppUseConfiguredFont(AppEnv* env);
void AppRestyleTextEdit(AppEnv* env, TEHandle te);
void AppApplyTextStyle(AppEnv* env);
pascal void EditorScrollbarAction(ControlHandle control, short part);
void AppSetEditorTitle(AppEnv* env);
short AppPromptSaveChanges(AppEnv* env, const char* path);
void AppDrawEditorToolButton(ControlHandle button, const char* label, short kind);
Boolean AppPendingChangesTouchPath(AppEnv* env, const char* path);
Boolean AppActiveEditorHasUnsavedTouchedFile(AppEnv* env);
void AppReloadActiveEditorIfClean(AppEnv* env);
void AppCreateWindow(AppEnv* env);
void AppCreateProjectWindows(AppEnv* env);
void AppCreateEditorWindow(AppEnv* env);
void AppResizeWidgets(AppEnv* env, short w, short h);
void AppResizeEditorWidgets(AppEnv* env);
void AppDrawDecorations(AppEnv* env);
void AppDrawSendIcon(AppEnv* env);
void AppRedrawSendButton(AppEnv* env);
void AppRedrawModeButton(AppEnv* env);
void AppDrawPendingStrip(AppEnv* env);
void AppDrawPendingActionIcons(AppEnv* env);
void AppUpdatePendingControls(AppEnv* env);
void AppDrawModeLabel(AppEnv* env);
void AppUpdateModeButton(AppEnv* env);
void AppSetIconTextButtonContent(ControlHandle button, IconRef iconRef);
void AppDrawProjectWindow(AppEnv* env, WindowPtr window);
void AppResizeFilesWidgets(AppEnv* env);
void AppDrawFilesWindow(AppEnv* env);
void AppDrawFilesList(AppEnv* env);
short AppVisibleFileRows(AppEnv* env);
void AppClampFilesScroll(AppEnv* env);
void AppShowProjectFile(AppEnv* env, const char* path);
pascal void ScrollbarAction(ControlHandle control, short part);
pascal void FilesScrollbarAction(ControlHandle control, short part);
pascal void SendButtonTrackAction(ControlHandle control, short part);
void ScrollTranscriptTo(int targetOffset);
void AdjustScrollbar(void);
void ScrollToBottom(void);
void AppSyncTranscriptMetrics(AppEnv* env);
void AppDrawPendingActionIcon(ControlHandle button, Boolean accept);
void AppHandleMouseDown(AppEnv* env, EventRecord* event);
void AppHandleKeyDown(AppEnv* env, EventRecord* event);
void AppHandleUpdate(AppEnv* env, EventRecord* event);
void AppHandleNullEvent(AppEnv* env);
void AppHandleMenuChoice(AppEnv* env, long choice);
void AppHandleActivate(AppEnv* env, EventRecord* event);
void AppQuit(AppEnv* env);
void AppDoSend(AppEnv* env);
void AppSendSyntheticPrompt(AppEnv* env, const char* prompt);
void AppDoCreateStarterProject(AppEnv* env);
void AppDoBuildProject(AppEnv* env);
void AppDoFixBuild(AppEnv* env);
void AppDoShowArtifact(AppEnv* env);
void AppInstallLocalizedMenus(const UIStrings* L);
void AppUpdateRemoteCompileMenu(AppEnv* env);
void AppAppendPlainMenuItem(MenuHandle menu, const char* text, short cmdChar);
Handle GetDialogItemHandle(DialogPtr dlg, short itemNo);
void SetDialogItemTextC(DialogPtr dlg, short itemNo, const char* text);
void GetDialogItemTextC(DialogPtr dlg, short itemNo, char* text);
void AdjustCursor(Point mouseLoc);
void AppSetBusy(AppEnv* env, Boolean busy);
void AppAppendTextColored(AppEnv* env, const char* msg, const RGBColor* color);
void AppAppendTextFragmentColored(AppEnv* env, const char* msg, const RGBColor* color, Boolean addReturn);
void AppAppendFileChangeSummaryColored(AppEnv* env, const char* summary);
const char* AppHTTPBody(const char* response);
Boolean AppPromptProjectName(AppEnv* env, char* outName, short maxLen);
pascal void DialogOkFrameProc(DialogRef dlg, DialogItemIndex itemNo);
IconRef GetAppResourceIcon(OSType creator, SInt16 resID, OSType iconType);
void AppUpdateStatus(AppEnv* env, const char* msg);
void AppClearInput(AppEnv* env);
void AppClearTranscript(AppEnv* env);
char* AppCopySizedText(const char* text, long len);
Boolean AppExtractToolPath(const char* tagStart, char* outPath, short maxLen);
long AppExtractToolRepeatCount(const char* tagStart);
void AppEncodeURLPath(const char* src, char* out, long maxLen);
void AppLaunchURL(AppEnv* env, const char* url);
void AppShowTooltip(ControlHandle btn, const char* text);
void AppClearHistory(AppEnv* env);
void AppInitToolbox(void);
void AppLoadPrefs(AppEnv* env);
void AppSavePrefs(AppEnv* env);
void AppApplyLanguage(AppEnv* env);
void AppNormalizeSettings(ChatSettings* settings);
int main(void);

#endif /* COMMON_H */