#include <windows.h>
#include <cstdint>
#include <MinHook.h>

// --- CONFIG ---
bool g_BlockAutoRotate = true;  // Blocks battle camera auto-turn after actions

uintptr_t g_Base = 0;

// Game function signature
typedef void (*SetAngle_t)(int angle);
SetAngle_t oSetAngle = nullptr;

void LoadConfig() {
    char iniPath[MAX_PATH];
    GetModuleFileNameA(NULL, iniPath, MAX_PATH);
    char* slash = strrchr(iniPath, '\\');
    if (slash) *(slash + 1) = '\0';
    strcat_s(iniPath, "scripts\\freecam.ini");

    if (GetFileAttributesA(iniPath) == INVALID_FILE_ATTRIBUTES) {
        WritePrivateProfileStringA("Camera", "BlockAutoRotate", "1", iniPath);
    }
    g_BlockAutoRotate = GetPrivateProfileIntA("Camera", "BlockAutoRotate", 1, iniPath) == 1;
}

// Hook on SetAngle (Game + 0x200530)
void hkSetAngle(int angle) {
    // Allow manual rotation (Q / E)
    if ((GetAsyncKeyState('Q') & 0x8000) || (GetAsyncKeyState('E') & 0x8000)) {
        oSetAngle(angle);
        return;
    }

    // Blocking disabled in INI -> allow everything
    if (!g_BlockAutoRotate) {
        oSetAngle(angle);
        return;
    }

    // Otherwise this came from the battle engine (auto-rotate after an action) -> drop it
}

DWORD WINAPI InitThread(LPVOID) {
    g_Base = (uintptr_t)GetModuleHandleA(NULL);
    LoadConfig();

    if (MH_Initialize() == MH_OK) {
        void* target = (void*)(g_Base + 0x200530);
        if (MH_CreateHook(target, (void*)&hkSetAngle, (void**)&oSetAngle) == MH_OK) {
            MH_EnableHook(target);
        }
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
