#include "common.h"

void AppInstallLocalizedMenus(const UIStrings* L) {
    MenuHandle fileMenu;
    MenuHandle editMenu;
    Str255 pstr;

    if (!L) return;

    fileMenu = GetMenuHandle(129);
    if (fileMenu) {
        DeleteMenu(129);
        DisposeMenu(fileMenu);
    }

    editMenu = GetMenuHandle(130);
    if (editMenu) {
        DeleteMenu(130);
        DisposeMenu(editMenu);
    }

    ConvertCtoPstr(L->menuFile, pstr);
    fileMenu = NewMenu(129, pstr);
    if (fileMenu) {
        AppAppendPlainMenuItem(fileMenu, L->menuSettings, 0);
        AppAppendPlainMenuItem(fileMenu, L->menuRemoteCompile, 0);
        AppAppendPlainMenuItem(fileMenu, L->menuNewProject, 'N');
        AppAppendPlainMenuItem(fileMenu, L->menuBuildProject, 'B');
        AppAppendPlainMenuItem(fileMenu, L->menuFixBuild, 0);
        AppAppendPlainMenuItem(fileMenu, L->menuShowArtifact, 0);
        AppAppendPlainMenuItem(fileMenu, L->menuApplyAI, 'Y');
        AppAppendPlainMenuItem(fileMenu, L->menuRejectAI, 0);
        AppAppendPlainMenuItem(fileMenu, L->menuClear, 0);
        AppendMenu(fileMenu, "\p(-");
        AppAppendPlainMenuItem(fileMenu, L->menuQuit, 'Q');
        InsertMenu(fileMenu, 0);
    }

    ConvertCtoPstr(L->menuEdit, pstr);
    editMenu = NewMenu(130, pstr);
    if (editMenu) {
        AppAppendPlainMenuItem(editMenu, L->menuUndo, 'Z');
        AppendMenu(editMenu, "\p(-");
        AppAppendPlainMenuItem(editMenu, L->menuCut, 'X');
        AppAppendPlainMenuItem(editMenu, L->menuCopy, 'C');
        AppAppendPlainMenuItem(editMenu, L->menuPaste, 'V');
        AppAppendPlainMenuItem(editMenu, L->menuSelectAll, 'A');
        AppAppendPlainMenuItem(editMenu, L->menuEraseSel, 0);
        InsertMenu(editMenu, 0);
    }
}

void AppUpdateRemoteCompileMenu(AppEnv* env) {
    MenuHandle fileMenu;

    if (!env) return;
    fileMenu = GetMenuHandle(129);
    if (!fileMenu) return;
    CheckItem(fileMenu, 2, env->settings.remoteCompileEnabled ? true : false);
}

void AppAppendPlainMenuItem(MenuHandle menu, const char* text, short cmdChar) {
    Str255 pstr;
    short item;

    if (!menu || !text) return;
    AppendMenu(menu, "\pX");
    item = CountMItems(menu);
    ConvertCtoPstr(text, pstr);
    SetMenuItemText(menu, item, pstr);
    if (cmdChar) SetItemCmd(menu, item, cmdChar);
}
