#include <windows.h>
#include <cstdint>
#include <cmath>
#include <MinHook.h>

// --- CONFIGURATION DEFAULTS ---
float g_Sensitivity        = 2.5f;
int   g_MaxPitch           = 900;   // High top-down view (Vanilla max was 550)
int   g_MinPitch           = 200;
bool  g_BlockAutoRotate    = true;  // Blocks battle camera auto-turn after actions
bool  g_BlockAutoRecenter  = true;  // Blocks camera from snapping to tile after actions

uintptr_t g_Base = 0;
bool g_IsUserRotating = false;
POINT g_LastMousePos = {0, 0};

// Game function signatures
typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

typedef void (*CameraUpdate_t)(uint64_t p1, uint64_t p2, uint64_t* p3, uint8_t* p4);
CameraUpdate_t oCameraUpdate = nullptr;

void LoadConfig() {
    char iniPath[MAX_PATH];
    GetModuleFileNameA(NULL, iniPath, MAX_PATH);
    char* slash = strrchr(iniPath, '\\');
    if (slash) *(slash + 1) = '\0';
    strcat_s(iniPath, "scripts\\freecam.ini");

    if (GetFileAttributesA(iniPath) == INVALID_FILE_ATTRIBUTES) {
        WritePrivateProfileStringA("Camera", "Sensitivity", "2.5", iniPath);
        WritePrivateProfileStringA("Camera", "MaxTopDownTilt", "900", iniPath);
        WritePrivateProfileStringA("Camera", "BlockAutoRotate", "1", iniPath);
        WritePrivateProfileStringA("Camera", "BlockAutoRecenter", "1", iniPath);
    }

    char buf[32];
    if (GetPrivateProfileStringA("Camera", "Sensitivity", "2.5", buf, 32, iniPath)) {
        g_Sensitivity = (float)atof(buf);
    }
    g_MaxPitch = GetPrivateProfileIntA("Camera", "MaxTopDownTilt", 900, iniPath);
    g_BlockAutoRotate = GetPrivateProfileIntA("Camera", "BlockAutoRotate", 1, iniPath) == 1;
    g_BlockAutoRecenter = GetPrivateProfileIntA("Camera", "BlockAutoRecenter", 1, iniPath) == 1;
}

// 1. Hook on SetAngle (Game + 0x200530)
void hkSetAngle(int angle) {
    // If the user is dragging with mouse, OR physically pressing 'E', allow it!
    if (g_IsUserRotating || (GetAsyncKeyState('E') & 0x8000)) {
        oSetAngle(angle);
        return;
    }

    // If auto-rotate blocking is disabled in INI, allow it
    if (!g_BlockAutoRotate) {
        oSetAngle(angle);
        return;
    }

    // Otherwise, this call came from the battle engine after an attack -> DROP IT!
}

// 2. Hook on RecenterOnTile (Game + 0x200588)
void hkRecenter(uint64_t param_1) {
    if (g_BlockAutoRecenter) {
        return; // Drop the post-action tile recentering!
    }
    oRecenter(param_1);
}

// 3. Synchronous Per-Frame Camera Hook (Game + 0xe959d)
void hkCameraUpdate(uint64_t p1, uint64_t p2, uint64_t* p3, uint8_t* p4) {
    if (p1 == 0) return;

    // Call the original game update first
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

    // Hold Right Mouse Button OR Middle Mouse Button to drag the camera freely
    bool rightDown  = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    bool middleDown = (GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0;
    bool dragging = rightDown || middleDown;

    if (dragging) {
        g_IsUserRotating = true;

        // --- HORIZONTAL YAW (Smooth circle with strict 0..4095 wrap) ---
        if (dx != 0) {
            uint32_t* pCurYaw  = (uint32_t*)(g_Base + 0x1a750f);
            uint32_t* pTgtYaw  = (uint32_t*)(g_Base + 0xd3a87c);
            uint32_t* pState   = (uint32_t*)(g_Base + 0x1a7510);

            int deltaYaw = (int)((float)dx * g_Sensitivity);
            int newYaw = (((int)*pCurYaw + deltaYaw) % 4096 + 4096) % 4096;

            *pCurYaw = (uint32_t)newYaw;
            *pTgtYaw = (uint32_t)newYaw;
            *pState  = 2; // Active rotating state
        }

        // --- VERTICAL PITCH (Top-Down Unlocked) ---
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
            uint32_t* pState = (uint32_t*)(g_Base + 0x1a7510);
            *pState = 0; // Return to idle cleanly
            g_IsUserRotating = false;
        }
    }
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);
    LoadConfig();

    if (MH_Initialize() == MH_OK) {
        // Hook SetAngle: intercepts game-commanded angle changes
        MH_CreateHook((void*)(g_Base + 0x200530), (void*)&hkSetAngle, (void**)&oSetAngle);
        MH_EnableHook((void*)(g_Base + 0x200530));

        // Hook RecenterOnTile: intercepts battle action camera snaps
        MH_CreateHook((void*)(g_Base + 0x200588), (void*)&hkRecenter, (void**)&oRecenter);
        MH_EnableHook((void*)(g_Base + 0x200588));

        // Hook CameraUpdate: main per-frame loop for smooth mouse controls
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
