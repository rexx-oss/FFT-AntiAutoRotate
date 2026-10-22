#include <windows.h>
#include <cstdint>
#include <MinHook.h>

uintptr_t g_Base = 0;

// Function signatures
typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

typedef uint64_t (*IsActionActive_t)(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4);
IsActionActive_t oIsActionActive = nullptr;

typedef uint64_t (*IsActionActiveCached_t)(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4);
IsActionActiveCached_t oIsActionActiveCached = nullptr;

// Detect manual user camera rotation (Keyboard 'E' OR Mouse Drag)
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
    // Block permanent battle auto-turns
    return;
}

// 2. Hook RecenterOnTile (Game + 0x200588)
void hkRecenter(uint64_t param_1) {
    // Block tile snap-backs
    return;
}

// 3. Hook IsActionActive (Game + 0x0FC598)
// Tell the camera that no action is active so it never switches to the Action Cam
uint64_t hkIsActionActive(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4) {
    return 0; // FALSE
}

// 4. Hook IsActionActiveCached (Game + 0x0F0D4C)
uint64_t hkIsActionActiveCached(uint64_t p1, uint64_t p2, uint64_t p3, uint64_t p4) {
    return 0; // FALSE
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);

    if (MH_Initialize() == MH_OK) {
        // Block battle angle changes
        MH_CreateHook((void*)(g_Base + 0x200530), (void*)&hkSetAngle, (void**)&oSetAngle);
        MH_EnableHook((void*)(g_Base + 0x200530));

        // Block tile snap-back
        MH_CreateHook((void*)(g_Base + 0x200588), (void*)&hkRecenter, (void**)&oRecenter);
        MH_EnableHook((void*)(g_Base + 0x200588));

        // Block Action Camera Switch 1
        MH_CreateHook((void*)(g_Base + 0x0FC598), (void*)&hkIsActionActive, (void**)&oIsActionActive);
        MH_EnableHook((void*)(g_Base + 0x0FC598));

        // Block Action Camera Switch 2
        MH_CreateHook((void*)(g_Base + 0x0F0D4C), (void*)&hkIsActionActiveCached, (void**)&oIsActionActiveCached);
        MH_EnableHook((void*)(g_Base + 0x0F0D4C));
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
