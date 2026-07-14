import re
import sys
from pathlib import Path


def safe_name(raw: str) -> str:
    name = re.sub(r"[^A-Za-z0-9_ -]+", "", raw.strip())
    name = re.sub(r"\s+", "", name)
    if not name:
        name = "NewMacApp"
    if not re.match(r"^[A-Za-z]", name):
        name = "App" + name
    return name[:24]


def write_text(path: Path, text: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(text, encoding="mac_roman", errors="replace", newline="\n")


def main() -> int:
    workspace = Path(sys.argv[1] if len(sys.argv) > 1 else "/app/workspace")
    app_name = safe_name(sys.argv[2] if len(sys.argv) > 2 else "NewMacApp")
    root = workspace / app_name
    creator = (app_name.upper() + "XXXX")[:4]

    if root.exists() and any(root.iterdir()):
        print(f"Project already exists and is not empty: {root}")
        return 2

    write_text(
        root / "CMakeLists.txt",
        f"""cmake_minimum_required(VERSION 3.10)
project({app_name} C)

set(CMAKE_C_STANDARD 90)
set(CMAKE_C_FLAGS "${{CMAKE_C_FLAGS}} -Wno-multichar")

add_application({app_name}
    TYPE APPL
    CREATOR {creator}
    src/main.c
    rsrc/App.r
)

target_link_libraries({app_name}
    InterfaceLib
    retrocrt
)
""",
    )

    write_text(
        root / "src" / "main.c",
        f"""#include <MacTypes.h>
#include <Quickdraw.h>
#include <Fonts.h>
#include <Windows.h>
#include <Dialogs.h>
#include <Events.h>
#include <Menus.h>
#include <ToolUtils.h>

static short gClicks = 0;

static void DrawMainWindow(WindowPtr window) {{
    Rect content;

    SetPort(window);
    SetRect(&content, 0, 0, 340, 180);
    EraseRect(&content);
    MoveTo(24, 64);
    if (gClicks == 0) {{
        DrawString("\\pHello World (Click window to change text)");
    }} else {{
        DrawString("\\pHahaha");
    }}
}}

int main(void) {{
    Rect bounds;
    Rect dirtyRect;
    WindowPtr window;
    WindowPtr whichWindow;
    EventRecord event;
    short part;

    MaxApplZone();
#if !TARGET_API_MAC_CARBON
    InitGraf(&qd.thePort);
    InitFonts();
    InitWindows();
    InitMenus();
    TEInit();
    InitDialogs(NULL);
#endif
    InitCursor();

    SetRect(&bounds, 80, 80, 260, 420);
    window = NewCWindow(NULL, &bounds, "\\p{app_name}", true, documentProc,
        (WindowPtr)-1L, true, 0L);

    while (1) {{
        if (WaitNextEvent(everyEvent, &event, 30, NULL)) {{
            if (event.what == mouseDown) {{
                part = FindWindow(event.where, &whichWindow);
                if (part == inGoAway && whichWindow == window) {{
                    if (TrackGoAway(window, event.where)) ExitToShell();
                }} else if (part == inContent && whichWindow == window) {{
                    SelectWindow(window);
                    SetPort(window);
                    gClicks++;
                    SetRect(&dirtyRect, 0, 0, 340, 180);
                    InvalRect(&dirtyRect);
                }}
            }} else if (event.what == updateEvt) {{
                BeginUpdate((WindowPtr)event.message);
                DrawMainWindow((WindowPtr)event.message);
                EndUpdate((WindowPtr)event.message);
            }}
        }}
    }}
    return 0;
}}
""",
    )

    write_text(
        root / "rsrc" / "App.r",
        f"""#include "Types.r"
#include "Processes.r"

resource 'SIZE' (-1) {{
    reserved,
    acceptSuspendResumeEvents,
    reserved,
    canBackground,
    doesActivateOnFGSwitch,
    backgroundAndForeground,
    dontGetFrontClicks,
    ignoreChildDiedEvents,
    is32BitCompatible,
    notHighLevelEventAware,
    localAndRemoteHLEvents,
    notStationeryAware,
    dontUseTextEditServices,
    reserved,
    reserved,
    reserved,
    512 * 1024,
    512 * 1024
}};
""",
    )

    print(f"Created {app_name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
