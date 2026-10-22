# AntiAutoRotate (FFT: The Ivalice Chronicles)

A lightweight quality-of-life mod for **FINAL FANTASY TACTICS - The Ivalice Chronicles** (Steam / PC) that stops the battle camera from permanently auto-rotating after attacks, spells, and turn changes.

---

## Features
- **Blocks Permanent Battle Auto-Rotation:** Your chosen camera orientation stays locked across allied turns, enemy turns, and action resolutions.
- **Suppresses Post-Action Tile Snaps:** Prevents the camera from getting yanked across the map to target tiles when an action concludes.
- **Two Modding Formats:** Available both as a standalone **Ultimate ASI Loader** plugin (`.asi`) and as a **Reloaded-II** mod (`.dll`).
- **Zero Overhead:** Built with synchronous MinHook memory detours (0% CPU impact, no background polling loops).

---

## Known Limitation / Motion Sickness Warning
> **Please read before using:**  
> While this mod completely prevents the game from **permanently** altering your camera perspective after an action, **the game engine will still temporarily swing the camera during an attack or ability animation to frame the action**, and then immediately snap back to your chosen angle once the animation ends.  
> 
> Because the camera swings into the attack and then snaps back to your view, this rapid back-and-forth movement might cause motion sickness for sensitive players.

---

## Installation

Choose **Option 1 (ASI Loader)** or **Option 2 (Reloaded-II)** depending on your preferred mod setup:

### Option 1: Ultimate ASI Loader (AntiAutoRotate.asi)
1. Download 64-bit Ultimate ASI Loader (`dinput8-x64.zip`).
2. Extract `dinput8.dll` directly into your game's root directory (where `FFT_enhanced.exe` is located).
3. Download `AntiAutoRotate.asi` from the Releases tab.
4. In your game directory, create a folder named `scripts` if one does not already exist.
5. Place `AntiAutoRotate.asi` inside the `scripts/` folder:

```text
FINAL FANTASY TACTICS/
├── FFT_enhanced.exe
├── dinput8.dll               (Ultimate ASI Loader)
└── scripts/
    ├── AntiAutoRotate.asi    (This mod)
    └── freecam.asi           (Optional: Original FreeCam mod)
```

---

### Option 2: Reloaded-II (AntiAutoRotate-ReloadedII.zip)
1. Download and set up Reloaded-II with `FFT_enhanced.exe`.
2. Download `AntiAutoRotate-ReloadedII.zip` from the Releases tab.
3. Drag and drop `AntiAutoRotate-ReloadedII.zip` directly into the Reloaded-II application window (or extract the archive into your `Reloaded-II/Mods/` directory).
4. Select `FFT_enhanced.exe` in Reloaded-II and check the box to enable **AntiAutoRotate**.
5. Launch the game through Reloaded-II.

---

## Uninstallation
- **For ASI Loader:** Delete `AntiAutoRotate.asi` from your game's `scripts/` folder.
- **For Reloaded-II:** Uncheck the mod in your Reloaded-II launcher, or right-click the mod and select **Delete Mod**.

---

## Building from Source

This project uses CMake and MinHook. You can build either the ASI loader version or the Reloaded-II version:

### Build ASI Plugin (.asi)
```bash
cmake -B build-asi -S .
cmake --build build-asi --config Release
```
Output: `build-asi/Release/AntiAutoRotate.asi`

### Build Reloaded-II Mod (.dll)
```bash
cmake -B build-reloaded -S Reloaded-II
cmake --build build-reloaded --config Release
```
Output: `build-reloaded/Release/AntiAutoRotate.dll`

---

## Credits & Acknowledgements
- **The Original FreeCam Author:** This project was developed by reverse-engineering the binary hooks and memory offsets uncovered by the author of the original *Free Cam* mod for *The Ivalice Chronicles*. Massive credit to their groundwork mapping the game's camera functions (`SetAngle`, `RecenterOnTile`, and `FinishRotate`).
- **Gemini:** Helped reverse-engineer the disassembly, debug rotation tracking, and assist with C++ implementation.
- **MinHook:** The minimalist x86/x64 API hooking library by TsudaKageyu.
