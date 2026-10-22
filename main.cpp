#include <windows.h>
#include <cstdint>
#include <MinHook.h>

uintptr_t g_Base = 0;

typedef void (*CameraUpdate_t)(uint64_t p1, uint64_t p2, uint64_t* p3, uint8_t* p4);
CameraUpdate_t oCameraUpdate = nullptr;

typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

// State tracking
static uint32_t s_LockedYaw = 0;
static bool s_Initialized = false;
static bool s_PlayerStartedRotation = false;

// Check if user is physically giving manual camera commands
bool IsManualInputActive() {
    if ((GetAsyncKeyState('E') & 0x8000) != 0) return true;
    if ((GetAsyncKeyState('Q') & 0x8000) != 0) return true;
    if ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) return true;
    if ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0) return true;
    return false;
}

// 1. Hook SetAngle: Allow only when the player commanded the turn
void hkSetAngle(int angle) {
    if (s_PlayerStartedRotation) {
        oSetAngle(angle);
        return;
    }
    // Block battle AI auto-turns
    return;
}

// 2. Hook RecenterOnTile: Drops post-action tile snaps
void hkRecenter(uint64_t param_1) {
    return;
}

// 3. Main Per-Frame Camera Loop (Game + 0xe959d)
// Actively anchors the camera angle during abilities and animations
void hkCameraUpdate(uint64_t p1, uint64_t p2, uint64_t* p3, uint8_t* p4) {
    if (p1 == 0) return;

    // Run game frame first
    oCameraUpdate(p1, p2, p3, p4);

    HWND fgWnd = GetForegroundWindow();
    char wndTitle[64];
    GetClassNameA(fgWnd, wndTitle, 64);
    if (strcmp(wndTitle, "FFTTIC") != 0) return;

    uint32_t* pCurYaw = (uint32_t*)(g_Base + 0xd3a878);
    uint32_t* pTgtYaw = (uint32_t*)(g_Base + 0xd3a87c);
    uint32_t* pState  = (uint32_t*)(g_Base + 0xd3a880);

    if (!pCurYaw || !pTgtYaw || !pState) return;

    // First-time initialization of current angle
    if (!s_Initialized && *pCurYaw != 0) {
        s_LockedYaw = *pCurYaw;
        s_Initialized = true;
    }

    bool manualPressed = IsManualInputActive();

    if (manualPressed) {
        s_PlayerStartedRotation = true;
    }

    if (s_PlayerStartedRotation) {
        // Player is turning via Q, E, or Mouse!
        // Allow the camera to rotate. When rotation finishes and keys are released:
        if (*pState == 0 && !manualPressed) {
            s_LockedYaw = *pCurYaw; // Lock in the new angle!
            s_PlayerStartedRotation = false;
        }
    } else {
        // An ability, spell, or turn change is playing!
        // Override any engine script attempting to swing the camera:
        if (s_Initialized) {
            *pCurYaw = s_LockedYaw;
            *pTgtYaw = s_LockedYaw;
            *pState  = 0; // Force idle
        }
    }
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);

    if (MH_Initialize() == MH_OK) {
        // Hook main camera loop (anchors camera during abilities)
        MH_CreateHook((void*)(g_Base + 0xe959d), (void*)&hkCameraUpdate, (void**)&oCameraUpdate);
        MH_EnableHook((void*)(g_Base + 0xe959d));

        // Hook SetAngle (blocks AI angle triggers)
        MH_CreateHook((void*)(g_Base + 0x200530), (void*)&hkSetAngle, (void**)&oSetAngle);
        MH_EnableHook((void*)(g_Base + 0x200530));

        // Hook RecenterOnTile (blocks tile snaps)
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
