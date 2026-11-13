// ════════════════════════════════════════════════════════════════════════
//  mem.cpp — RPM/WPM + sig-scan + module base
// ════════════════════════════════════════════════════════════════════════
#include "mem.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <vector>
#include <cstring>

HANDLE g_proc   = nullptr;
DWORD  g_pid    = 0;
uintptr_t g_base = 0;

bool Mem_Init(const wchar_t* exe_name) {
    PROCESSENTRY32W pe{ sizeof(pe) };
    HANDLE snap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    for (BOOL ok = Process32FirstW(snap, &pe); ok; ok = Process32NextW(snap, &pe)) {
        if (!_wcsicmp(pe.szExeFile, exe_name)) { g_pid = pe.th32ProcessID; break; }
    }
    CloseHandle(snap);
    if (!g_pid) return false;

    g_proc = OpenProcess(PROCESS_VM_READ | PROCESS_VM_WRITE |
                         PROCESS_VM_OPERATION | PROCESS_QUERY_INFORMATION,
                         FALSE, g_pid);
    if (!g_proc) return false;

    HMODULE mods[512]; DWORD needed;
    EnumProcessModulesEx(g_proc, mods, sizeof(mods), &needed, LIST_MODULES_64BIT);
    for (unsigned i = 0; i < needed / sizeof(HMODULE); i++) {
        wchar_t name[MAX_PATH];
        GetModuleBaseNameW(g_proc, mods[i], name, MAX_PATH);
        if (!_wcsicmp(name, exe_name)) { g_base = (uintptr_t)mods[i]; break; }
    }
    return g_base != 0;
}

template<typename T> T RPM(uintptr_t addr) {
    T v{}; ReadProcessMemory(g_proc, (LPCVOID)addr, &v, sizeof(T), nullptr); return v;
}
template<typename T> void WPM(uintptr_t addr, const T& v) {
    WriteProcessMemory(g_proc, (LPVOID)addr, &v, sizeof(T), nullptr);
}

// IDA-style sig scan over mapped region
uintptr_t SigScan(const char* pattern, const char* mask) {
    SYSTEM_INFO si; GetSystemInfo(&si);
    MEMORY_BASIC_INFORMATION mbi{};
    uintptr_t addr = (uintptr_t)si.lpMinimumApplicationAddress;
    size_t plen = strlen(mask);
    while (addr < (uintptr_t)si.lpMaximumApplicationAddress) {
        if (!VirtualQueryEx(g_proc, (LPCVOID)addr, &mbi, sizeof(mbi))) break;
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE))) {
            std::vector<char> buf(mbi.RegionSize);
            SIZE_T read;
            if (ReadProcessMemory(g_proc, (LPCVOID)addr, buf.data(), mbi.RegionSize, &read)) {
                for (size_t i = 0; i + plen < read; i++) {
                    bool ok = true;
                    for (size_t j = 0; j < plen && ok; j++)
                        if (mask[j] == 'x' && buf[i+j] != pattern[j]) ok = false;
                    if (ok) return addr + i;
                }
            }
        }
        addr += mbi.RegionSize;
    }
    return 0;
}

// explicit instantiations
template uintptr_t RPM<uintptr_t>(uintptr_t);
template float     RPM<float>(uintptr_t);
template int32_t   RPM<int32_t>(uintptr_t);
template void      WPM<float>(uintptr_t, const float&);
template void      WPM<uintptr_t>(uintptr_t, const uintptr_t&);
