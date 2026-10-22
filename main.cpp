#include <windows.h>
#include <cstdint>
#include <MinHook.h>

uintptr_t g_Base = 0;
bool g_IsUserRotating = false;
POINT g_LastMousePos = {0, 0};
float g_Sensitivity = 2.5f;
int   g_MaxPitch = 900;
int   g_MinPitch = 200;

typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

typedef void (*CameraUpdate_t)(uint64_t p1, uint64_t p2, uint64_t* p3, uint8_t* p4);
CameraUpdate_t oCameraUpdate = nullptr;

// 1. Hook SetAngle: Drops battle AI auto-rotations while allowing manual E & mouse
void hkSetAngle(int angle) {
    if (g_IsUserRotating || (GetAsyncKeyState('E') & 0x8000)) {
        oSetAngle(angle);
        return;
    }
    // Block battle AI auto-turn after actions
    return;
}

// 2. Hook RecenterOnTile: Drops battle camera snaps
void hkRecenter(uint64_t param_1) {
    return;
}

// 3. Synchronous Per-Frame Camera Hook
void hkCameraUpdate(uint64_t p1, uint64_t p2, uint64_t* p3, uint8_t* p4) {
    if (p1 == 0) return;

    // Run original game update first
    oCameraUpdate(p1, p2, p3, p4);

    HWND fgWnd = GetForegroundWindow();
    char wndTitle[64];
    GetClassNameA(fgWnd, wndTitle, 64);
    if (strcmp(wndTitle, "FFTTIC") != 0) return;

    POINT curMouse;
    GetCursorPos(&curMouse);
    int dx = curMouse.x - g_LastMousePos.x;
    int dy = curMouse.y - g_LastMousePos.y;
    g_LastMousePos = curMouse;

    // Hold Right Mouse Button OR Middle Mouse Button to drag the camera
    bool rightDown  = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    bool middleDown = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
    bool dragging   = rightDown || middleDown;

    if (dragging) {
        g_IsUserRotating = true;

        // --- HORIZONTAL ROTATION ---
        if (dx != 0) {
            // Use the real dword registers
            uint32_t* pCurYaw = (uint32_t*)(g_Base + 0xd3a878);
            uint32_t* pTgtYaw = (uint32_t*)(g_Base + 0xd3a87c);
            uint32_t* pState  = (uint32_t*)(g_Base + 0xd3a880);

            int deltaYaw = (int)((float)dx * g_Sensitivity);
            int newYaw = (((int)*pCurYaw + deltaYaw) % 4096 + 4096) % 4096;

            // Update both Current and Target simultaneously so the engine never desyncs
            *pCurYaw = (uint32_t)newYaw;
            *pTgtYaw = (uint32_t)newYaw;
            *pState  = 2; // Active rotating

            // Call the engine's SetAngle to update its internal matrices
            if (oSetAngle) {
                oSetAngle(newYaw);
            }
        }

        // --- VERTICAL TILT ---
        if (dy != 0) {
            uintptr_t camMgr = *(uintptr_t*)(g_Base + 0x3cc96b8);
            if (camMgr && !IsBadReadPtr((void*)camMgr, 0x25000)) {
                int16_t* pPitch      = (int16_t*)(camMgr + 0x240dc);
                float*   pPitchFloat = (float*)(camMgr + 0x2406c);

                int deltaPitch = (int)((float)dy * (g_Sensitivity * 0.8f));
                int newPitch = (int)*pPitch - deltaPitch;

                if (newPitch > g_MaxPitch) newPitch = g_MaxPitch;
                if (newPitch < g_MinPitch) newPitch = g_MinPitch;

                *pPitch = (int16_t)newPitch;
                *pPitchFloat = (float)newPitch;
            }
        }
    } else {
        if (g_IsUserRotating) {
            uint32_t* pState = (uint32_t*)(g_Base + 0xd3a880);
            *pState = 0; // Return to idle cleanly
            g_IsUserRotating = false;
        }
    }
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);

    if (MH_Initialize() == MH_OK) {
        MH_CreateHook((void*)(g_Base + 0x200530), (void*)&hkSetAngle, (void**)&oSetAngle);
        MH_EnableHook((void*)(g_Base + 0x200530));

        MH_CreateHook((void*)(g_Base + 0x200588), (void*)&hkRecenter, (void**)&oRecenter);
        MH_EnableHook((void*)(g_Base + 0x200588));

        MH_CreateHook((void*)(g_Base + 0xe959d), (void*)&hkCameraUpdate, (void**)&oCameraUpdate);
        MH_EnableHook((void*)(g_Base + 0xe959d));
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
