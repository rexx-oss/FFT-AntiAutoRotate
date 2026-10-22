# AntiAutoRotate (FFT: The Ivalice Chronicles)

A lightweight quality-of-life plugin for **FINAL FANTASY TACTICS - The Ivalice Chronicles** (Steam / PC) that stops the battle camera from permanently auto-rotating after attacks, spells, and turn changes.

---

## Features
- **Blocks Permanent Battle Auto-Rotation:** Your chosen camera orientation stays locked across allied turns, enemy turns, and action resolutions.
- **Suppresses Post-Action Tile Snaps:** Prevents the camera from getting yanked to target tiles when an action concludes.
- **Zero Overhead:** Built with synchronous MinHook memory detours (0% CPU impact, no background polling loops).
- **FreeCam Co-Existence:** Fully compatible with the original FreeCam mod—both can run together in the same folder.

---

## Known Limitation / Motion Sickness Warning
> **Please read before using:**  
> While this mod completely prevents the game from **permanently** altering your camera perspective after an action, **the game engine will still temporarily swing the camera during an attack or ability animation to frame the action**, and then immediately snap back to your chosen angle once the animation ends.  
> 
> Because the camera swings into the attack and then snaps back to your view, this rapid back-and-forth movement might cause motion sickness for sensitive players.

---

## Credits & Acknowledgements
- **The Original FreeCam Author:** This project was developed through reverse-engineering the binary hooks and memory offsets uncovered by the author of the original *Free Cam* mod for *The Ivalice Chronicles*. Massive credit to their groundwork mapping the game's camera functions (`SetAngle`, `RecenterOnTile`, and `FinishRotate`).
- **MinHook:** The minimalist x86/x64 API hooking library by TsudaKageyu.

---

## Installation

### 1. Prerequisites
This mod requires **Ultimate ASI Loader** (64-bit):
1. Download `dinput8-x64.zip` from Ultimate ASI Loader Releases.
2. Extract `dinput8.dll` directly into your game's root directory (where `FFT_enhanced.exe` is located).

### 2. Install AntiAutoRotate
1. Download `AntiAutoRotate.asi` from the Releases tab.
2. In your game directory, create a folder named `scripts` if one does not already exist.
3. Place `AntiAutoRotate.asi` inside the `scripts/` folder:

```text
FINAL FANTASY TACTICS/
├── FFT_enhanced.exe
├── dinput8.dll               (Ultimate ASI Loader)
└── scripts/
    ├── AntiAutoRotate.asi    (This mod)
    └── freecam.asi           (Optional: Original FreeCam mod)
```

*(If you also use the original FreeCam mod, simply place `freecam.asi` in the same `scripts/` folder—Ultimate ASI Loader will load both automatically without conflict).*

---

## Uninstallation
Delete `AntiAutoRotate.asi` from your `scripts/` folder.

---

## Building from Source
This project uses CMake and MinHook:
```bash
cmake -B build -S .
cmake --build build --config Release
```
