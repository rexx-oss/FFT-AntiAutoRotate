#include <windows.h>
#include <cstdint>
#include <MinHook.h>

uintptr_t g_Base = 0;

// 1. Hook SetAngle (Game + 0x200530)
// Allows manual keyboard 'E' turns, drops all battle auto-turns
typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

void hkSetAngle(int angle) {
    // If you physically pressed 'E', allow the game to rotate 90 degrees
    if ((GetAsyncKeyState('E') & 0x8000) != 0) {
        oSetAngle(angle);
        return;
    }
    // Otherwise, it is battle AI / turn-change auto-rotation -> DROP IT!
    return;
}

// 2. Hook RecenterOnTile (Game + 0x200588)
// Drops the camera from auto-snapping to units/tiles after actions
typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

void hkRecenter(uint64_t param_1) {
    // Drop post-action recentering completely
    return;
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);

    if (MH_Initialize() == MH_OK) {
        // Block auto-angle changes
        MH_CreateHook((void*)(g_Base + 0x200530), (void*)&hkSetAngle, (void**)&oSetAngle);
        MH_EnableHook((void*)(g_Base + 0x200530));

        // Block tile snap-back
        MH_CreateHook((void*)(g_Base + 0x200588), (void*)&hkRecenter, (void**)&oRecenter);
        MH_EnableHook((void*)(g_Base + 0x200588));
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
