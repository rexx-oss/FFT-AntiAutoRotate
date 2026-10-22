#include <windows.h>
#include <cstdint>
#include <MinHook.h>

// --- DIRECTINPUT8 PROXY FORWARDER ---
// Forwards standard input calls to the real Windows System32 dinput8.dll
typedef HRESULT(WINAPI* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
DirectInput8Create_t oDirectInput8Create = nullptr;

extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(
    HINSTANCE hinst, DWORD dwVersion, REFIID riidltf, LPVOID* ppvOut, LPUNKNOWN punkOuter)
{
    if (!oDirectInput8Create) {
        char sysPath[MAX_PATH];
        GetSystemDirectoryA(sysPath, MAX_PATH);
        strcat_s(sysPath, "\\dinput8.dll");
        HMODULE hReal = LoadLibraryA(sysPath);
        if (hReal) {
            oDirectInput8Create = (DirectInput8Create_t)GetProcAddress(hReal, "DirectInput8Create");
        }
    }
    if (oDirectInput8Create) {
        return oDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
    }
    return E_FAIL;
}

// --- ANTI-AUTOROTATE ENGINE ---
uintptr_t g_Base = 0;

typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

// 1. Hook SetAngle (Game + 0x200530)
void hkSetAngle(int angle) {
    // Check if user is manually pressing keyboard 'E', OR holding any mouse button to drag
    bool isManual = ((GetAsyncKeyState('E') & 0x8000) != 0) ||
                    ((GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0) ||
                    ((GetAsyncKeyState(VK_MBUTTON) & 0x8000) != 0) ||
                    ((GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0);

    if (isManual) {
        oSetAngle(angle);
        return;
    }

    // Otherwise, it is battle AI / turn-change auto-rotation -> DROP IT!
    return;
}

// 2. Hook RecenterOnTile (Game + 0x200588)
void hkRecenter(uint64_t param_1) {
    // Drop post-action recentering completely
    return;
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);

    if (MH_Initialize() == MH_OK) {
        // Block AI angle changes (preserves manual E and mouse drag)
        MH_CreateHook((void*)(g_Base + 0x200530), (void*)&hkSetAngle, (void**)&oSetAngle);
        MH_EnableHook((void*)(g_Base + 0x200530));

        // Block AI tile snapping
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
