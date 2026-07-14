# AltanAI: Classic Mac OS 9 AI Code Studio & Co-pilot

AltanAI is a native, native-compiled Classic Mac OS 9 coding co-pilot and IDE. It allows you to write, modify, refactor, and build Mac OS 9 applications directly from a vintage Macintosh environment using modern Large Language Models (LLMs) and containerized Retro68 compiler chains.

---

## Architecture and System Flow

The project is split into two primary components:
1. **The Classic Client (AltanAI):** A C-based application written for the Classic Macintosh Toolbox. It runs natively inside Mac OS 9 (or emulators like SheepShaver) and communicates over Open Transport TCP/IP sockets.
2. **The Compile & AI Bridge (Backend):** A lightweight Python-based Flask service running in a Docker container on the host macOS or a remote server. It coordinates prompt completions with high-capacity LLMs (e.g., Nvidia Nemotron) and manages the Retro68 gcc-cross-compilation pipeline.

```mermaid
sequenceDiagram
    participant Emulator as SheepShaver / Mac OS 9 (AltanAI Client)
    participant Bridge as Python Flask Bridge (Docker Container)
    participant LLM as Nvidia Nemotron AI (Hosted LLM)
    participant Compiler as GCC Toolchain (Retro68 Compiler)

    Emulator->>Bridge: 1. Send Prompt + Local Context (TCP/IP)
    activate Bridge
    Bridge->>LLM: 2. Fetch Completion (with System Prompts)
    activate LLM
    LLM-->>Bridge: 3. Return XML Tool Blocks (PATCH / WRITE)
    deactivate LLM
    Bridge-->>Emulator: 4. Clean Payload Response
    deactivate Bridge
    activate Emulator
    Note over Emulator: User previews & clicks (✓) to apply changes locally.
    deactivate Emulator
    Emulator->>Bridge: 5. Trigger Build / Send Code Snapshot
    activate Bridge
    Bridge->>Compiler: 6. Run CMake / build_macos9.sh
    activate Compiler
    Compiler-->>Bridge: 7. Generate Binaries (.bin / .dsk / .APPL)
    deactivate Compiler
    Bridge-->>Emulator: 8. Return Compile Log & Binary Link
    deactivate Bridge
```

---

## Design Choices & Legacy Limitations

Creating a modern developer tool for a 25-year-old operating system required addressing major platform constraints:

### 1. The 32KB TextEdit Buffer Boundary
In Classic Mac OS, standard text editing, viewing, and layout are managed by the ROM-based **TextEdit Manager**. TextEdit stores the text length in a signed 16-bit integer (`short`), capping any single text block at **32,767 bytes** (approx. 1000 lines of code).
* **The Impact:** Large monolithic source files exceed this limit and crash the editor.
* **The Solution:** The codebase was refactored from a single monolithic file into 15 small, highly modular C files (under 38KB each).
* **The Patch Protocol:** We enforce **`<ALTANAI_PATCH>`** blocks rather than **`<ALTANAI_WRITE>`** blocks. By sending code diffs instead of full files, network payloads are kept under 1KB, staying safe from buffer overflows.

### 2. Memory Partition & Heap Fragmentation
Mac OS 9 lacks modern virtual memory paging and is highly susceptible to heap fragmentation. The client app runs in a partition defined by its `SIZE` resource (set to **512KB**).
* **The Impact:** If the client loads a 250KB file into memory and attempts to modify it, it must allocate another 250KB buffer to construct the output, immediately triggering an out-of-memory crash (`memFullErr`).
* **The Solution:** We enforce strict client-side read boundaries of 30,000 bytes. This ensures that memory consumption during edit transactions remains lightweight.

### 3. API Token Capacity Matching
The remote bridge uses **`MAX_COMPLETION_TOKENS=4096`** (raised from 1,024). 
* **The Impact:** A 1,024-token limit frequently cut off larger code generations mid-stream. The resulting broken closing XML tags (`</ALTANAI_WRITE>`) were rejected by the client as malformed.
* **The Solution:** By raising the limit to 4,096 tokens, the AI has enough space to write complete, valid tool blocks.

### 4. Progress States & UI Responsiveness
Because Classic Mac OS uses **cooperative multitasking**, a single blocking network thread or compilation freeze can lock up the entire system.
* **The Solution:** We implemented busy state overrides (`isBusy`). While the client waits for compilation or prompt completions, it overrides the cursor to a spinning watch (`watchCursor`), ignores mouse clicks/keypresses, and plays a `SysBeep(1)` to prevent double-build corruption.

---

## Setup & Installation

### A. Building the Client (Host macOS)
1. Install [Retro68](https://github.com/autc04/Retro68).
2. Configure CMake in the client folder:
   ```bash
   cmake -S macos9_client -B macos9_client/build_ppc \
     -DCMAKE_TOOLCHAIN_FILE=retroppc.toolchain.cmake \
     -U OPEN_TRANSPORT_APP_PPC
   cmake --build macos9_client/build_ppc
   ```
3. Mount the generated `AltanAI.dsk` image inside your emulator (e.g., SheepShaver).

### B. Launching the Docker Bridge
1. Copy `docker/.env.example` to `docker/.env` and add your LLM API Key:
   ```text
   PROVIDER_API_KEY=your_key_here
   MAX_COMPLETION_TOKENS=4096
   ```
2. Start the service:
   ```bash
   cd docker
   docker compose up -d --build
   ```
3. Connect the AltanAI client by typing the bridge host IP and port (`8080`) into the settings panel inside the emulator.

---

## Visual Tour

### 1. Main Chat Interface
An ICQ-style chat interface displaying server response logs.
![File Menu Commands](screenshot/Screenshot%202026-07-14%20at%2020.40.50.jpg)


### 2. File Menu Commands
A Classic drop-down menu managing connection settings, remote compile triggers, project builders, and direct shortcuts to apply/reject AI code changes.
![Main Chat Interface](screenshot/Screenshot%202026-07-14%20at%2017.42.00.jpg)

### 3. Local File Manager
Scans local virtual disk folders and tracks files using the Macintosh File Manager.
![Local File Manager](screenshot/Screenshot%202026-07-14%20at%2020.27.19.jpg)
![File List](screenshot/Screenshot%202026-07-14%20at%2020.27.23.jpg)

### 4. Text Editor Tab Views
Multi-tab text editor with line numbers, search tools, and dirty flags.
![Text Editor Empty](screenshot/Screenshot%202026-07-14%20at%2020.27.57.jpg)
![Text Editor Editing](screenshot/Screenshot%202026-07-14%20at%2020.30.34.jpg)

### 5. Dialog Panels
Modal settings, history managers, and product details panels.
![Dialog Panels](screenshot/Screenshot%202026-07-14%20at%2020.31.39.jpg)

### 6. Busy State event blocking
Watch cursor loading indicators and event blocks.
![Busy State indicator](screenshot/Screenshot%202026-07-14%20at%2020.32.02.jpg)

### 7. Interactive AI Code Application
Green checkmark (✓) prompt change verification indicators.
![AI Code Application](screenshot/Screenshot%202026-07-14%20at%2020.33.23.jpg)


