#include <windows.h>
#include <cstdint>
#include <MinHook.h>

uintptr_t g_Base = 0;

typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

// Detects manual camera input (Keyboard 'E' OR any mouse drag/movement)
bool IsManualInput() {
    // 1. Keyboard 'E' (manual 90-degree turn)
    if ((GetAsyncKeyState('E') & 0x8000) != 0) return true;

    // 2. Mouse buttons held down (Right, Middle, or Left click drag)
    if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) return true;
    if ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0) return true;
    if ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0) return true;

    // 3. Mouse cursor movement delta
    POINT cur;
    if (GetCursorPos(&cur)) {
        static POINT last = cur;
        if (cur.x != last.x || cur.y != last.y) {
            last = cur;
            return true;
        }
    }

    return false;
}

// Hook SetAngle (Game + 0x200530)
// Drops battle AI auto-rotations while preserving manual keyboard and mouse rotation
void hkSetAngle(int angle) {
    if (IsManualInput()) {
        oSetAngle(angle);
        return;
    }
    // Drop all automated battle camera turns
    return;
}

// Hook RecenterOnTile (Game + 0x200588)
// Drops camera auto-snapping to units/tiles after actions
void hkRecenter(uint64_t param_1) {
    return;
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);

    if (MH_Initialize() == MH_OK) {
        MH_CreateHook((void*)(g_Base + 0x200530), (void*)&hkSetAngle, (void**)&oSetAngle);
        MH_EnableHook((void*)(g_Base + 0x200530));

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
