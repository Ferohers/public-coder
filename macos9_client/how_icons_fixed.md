# Classic Mac OS 9 Icon Implementation and Fixes

This document details how the Build and Plan mode icon rendering, registry collision, and color fidelity issues were fixed in the AltanAI client.

---

## 1. Icon Manager Registry Collision (Same Icon Bug)

### The Problem
Initially, the application registered both the Build icon (ID 140) and the Plan icon (ID 141) under the standard `'icns'` icon type parameter inside `RegisterIconRefFromResource`:
```c
RegisterIconRefFromResource(APP_CREATOR, 'icns', &appSpec, resID, &iconRef);
```
In the Classic Mac OS Icon Manager, registrations are internally keyed by `(creator, iconType)`. Because both calls used `APP_CREATOR` (`'ALTN'`) and `'icns'`, their registry keys collided. The second registration overwrote the first, resulting in the Plan icon being returned for both modes.

### The Fix
Instead of registering both under the same owner/type combination, we changed `GetAppResourceIcon` to accept a custom `creator` signature:
```c
static IconRef GetAppResourceIcon(OSType creator, SInt16 resID, OSType iconType) {
    ...
    RegisterIconRefFromResource(creator, iconType, &appSpec, resID, &iconRef);
    ...
}
```
In `main.c`, we register them using unique process-local signatures for each mode while keeping the resource type as standard `'icns'`:
```c
env->buildIcon = GetAppResourceIcon('BLDI', 140, 'icns');
env->planIcon = GetAppResourceIcon('PLNI', 141, 'icns');
```
This isolates the lookup keys in the Icon Manager (`('BLDI', 'icns')` vs `('PLNI', 'icns')`), successfully resolving the collision.

---

## 2. Color Table Distortion (Orange/Pink Icon Bug)

### The Problem
To upscale the icons, a Python script parsed the raw resource forks, decompressed the `is32` packbits pixels, and re-mapped them to the Mac system color palettes to construct `ics8`/`icl8` tables. 

However, parsing binary resource forks with ad-hoc scripts is error-prone. The script matched wrong `'icns'` signatures inside the resource map itself and performed lossy color quantization. In 8-bit/16-bit display modes, this mapping caused the Plan icon to render with corrupted orange and pink colors.

### The Fix
We bypassed Python script generation completely:
1. **Direct SDK Extraction:** We used Apple's official `DeRez` tool to decompile the raw, unmodified 16x16 icon resource forks (`is32`/`ics#`) directly from the original StuffIt archive (`Archive.sit`).
2. **Pure Hex Compilation:** The exact hex bytes from `DeRez` were pasted directly into `rsrc/mode_icons.r`. This guarantees 100% authentic bit-for-bit color fidelity as designed by the original graphic editor, with zero conversion or palette mapping issues.

---

## 3. Icon Sizing (Scaling vs. Redrawing)

### The Problem
The original icons are native 16x16 pixels. On higher resolution screens, displaying them at 16x16 inside the bevel buttons made them look too small.

### The Fix
Instead of modifying the source graphics, we leveraged the native scaling features of the Classic Mac OS Appearance Manager. 

We updated `AppSetIconTextButtonContent` in `main.c` to enable the native icon scaling tag on the Bevel Button controls:
```c
Boolean scaleIcon = true;
SetControlData(button, kControlEntireControl, kControlBevelButtonScaleIconTag,
    sizeof(scaleIcon), &scaleIcon);
```
This instructs the system's control renderer to upscale the original, crisp 16x16 color icon to the ideal size of the button layout. We expanded `UI_MODE_WIDTH` to `90` to align symmetrically with the chat panels, resulting in a perfectly sized and sharp toolbar item.
