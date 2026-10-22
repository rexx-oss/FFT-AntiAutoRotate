#include <windows.h>
#include <cstdint>
#include <MinHook.h>

uintptr_t g_Base = 0;

typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

typedef void (*Blend_t)(uint64_t param_1, uint64_t param_2);
Blend_t oBlend = nullptr;

// Manual rotation is ONLY:
// 1. Keyboard 'E'
// 2. Right-Click Drag (VK_RBUTTON)
// 3. Middle-Click Drag (VK_MBUTTON)
// (Left-Click is excluded so confirming attacks NEVER triggers camera rotation!)
bool IsManualRotation() {
    if ((GetAsyncKeyState('E') & 0x8000) != 0) return true;
    if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) return true;
    if ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0) return true;
    return false;
}

// 1. Hook SetAngle (Game + 0x200530)
void hkSetAngle(int angle) {
    if (IsManualRotation()) {
        oSetAngle(angle);
        return;
    }
    // Drop all automated battle camera turns
    return;
}

// 2. Hook RecenterOnTile (Game + 0x200588)
void hkRecenter(uint64_t param_1) {
    // Drop post-action recentering completely
    return;
}

// 3. Hook the REAL Blend function (Game + 0x3A6240)
void hkBlend(uint64_t param_1, uint64_t param_2) {
    // Drop the cinematic ability camera blend completely
    return;
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);

    if (MH_Initialize() == MH_OK) {
        // Block manual vs AI angle changes
        MH_CreateHook((void*)(g_Base + 0x200530), (void*)&hkSetAngle, (void**)&oSetAngle);
        MH_EnableHook((void*)(g_Base + 0x200530));

        // Block tile snap-back
        MH_CreateHook((void*)(g_Base + 0x200588), (void*)&hkRecenter, (void**)&oRecenter);
        MH_EnableHook((void*)(g_Base + 0x200588));

        // Block the real cinematic attack/ability camera blend
        MH_CreateHook((void*)(g_Base + 0x3A6240), (void*)&hkBlend, (void**)&oBlend);
        MH_EnableHook((void*)(g_Base + 0x3A6240));
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID) {
    if (ul_reason_for_call == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, InitThread, NULL, 0, NULL);
    } else if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        MH_Uninitialize();
    }
    return TRUE;
}
