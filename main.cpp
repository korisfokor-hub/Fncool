// ════════════════════════════════════════════════════════════════════════
//  main.cpp — DLL entry, init chain, hotkey loop
// ════════════════════════════════════════════════════════════════════════
#include "eac_layer.h"
#include "mem.h"
#include "menu.h"
#include <windows.h>
#include <thread>
#include <chrono>

static DWORD WINAPI MainThread(LPVOID base) {
    // 1. EAC evasion first — before any game memory touched
    EAC_Init((HMODULE)base);
    Sleep(1200); // let game finish initialising

    // 2. Attach to process (self, internal DLL)
    // For external build: replace with Mem_Init(L"FortniteClient-Win64-Shipping.exe")
    g_proc = GetCurrentProcess();
    g_base = (uintptr_t)GetModuleHandleW(L"FortniteClient-Win64-Shipping.exe");
    if (!g_base) return 1;

    // 3. Hook Present → overlay
    Menu_Init();

    // 4. Keep thread alive
    while (true) {
        if (GetAsyncKeyState(VK_END) & 1) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // 5. Teardown
    Cannon_StopFire();
    // kiero::shutdown();  // ImGui cleanup
    FreeLibraryAndExitThread((HMODULE)base, 0);
    return 0;
}

BOOL APIENTRY DllMain(HMODULE hmod, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hmod);
        CreateThread(nullptr, 0, MainThread, hmod, 0, nullptr);
    }
    return TRUE;
}
