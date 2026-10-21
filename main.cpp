#include <windows.h>
#include <cstdint>
#include <MinHook.h>

// --- CONFIG ---
bool g_BlockAutoRotate   = true;  // Blocks battle camera auto-turn after actions
bool g_BlockAutoRecenter = true;  // Blocks camera snapping to tile after actions

uintptr_t g_Base = 0;

typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

typedef void (*Recenter_t)(uint64_t param_1);
Recenter_t oRecenter = nullptr;

void LoadConfig() {
    char iniPath[MAX_PATH];
    GetModuleFileNameA(NULL, iniPath, MAX_PATH);
    char* slash = strrchr(iniPath, '\\');
    if (slash) *(slash + 1) = '\0';
    strcat_s(iniPath, "scripts\\freecam.ini");

    if (GetFileAttributesA(iniPath) == INVALID_FILE_ATTRIBUTES) {
        WritePrivateProfileStringA("Camera", "BlockAutoRotate", "1", iniPath);
        WritePrivateProfileStringA("Camera", "BlockAutoRecenter", "1", iniPath);
    }
    g_BlockAutoRotate   = GetPrivateProfileIntA("Camera", "BlockAutoRotate", 1, iniPath) == 1;
    g_BlockAutoRecenter = GetPrivateProfileIntA("Camera", "BlockAutoRecenter", 1, iniPath) == 1;
}

// Hook on SetAngle (Game + 0x200530)
void hkSetAngle(int angle) {
    if ((GetAsyncKeyState('Q') & 0x8000) || (GetAsyncKeyState('E') & 0x8000)) {
        oSetAngle(angle);
        return;
    }
    if (!g_BlockAutoRotate) {
        oSetAngle(angle);
        return;
    }
    // Otherwise: engine-initiated rotation -> drop it
}

// Hook on RecenterOnTile (Game + 0x200588)
void hkRecenter(uint64_t param_1) {
    if (g_BlockAutoRecenter) return;  // drop post-action recentering
    oRecenter(param_1);
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);
    LoadConfig();

    if (MH_Initialize() == MH_OK) {
        void* t1 = (void*)(g_Base + 0x200530);
        if (MH_CreateHook(t1, (void*)&hkSetAngle, (void**)&oSetAngle) == MH_OK)
            MH_EnableHook(t1);

        void* t2 = (void*)(g_Base + 0x200588);
        if (MH_CreateHook(t2, (void*)&hkRecenter, (void**)&oRecenter) == MH_OK)
            MH_EnableHook(t2);
    }
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        CreateThread(NULL, 0, InitThread, NULL, 0, NULL);
    } else if (reason == DLL_PROCESS_DETACH) {
        MH_DisableHook(MH_ALL_HOOKS);
        MH_Uninitialize();
    }
    return TRUE;
}
