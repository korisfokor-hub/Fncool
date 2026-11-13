// ════════════════════════════════════════════════════════════════════════
//  eac_layer.cpp
//  Manual-map loader + EAC evasion
//  Run this BEFORE any game-touching code.
//  Requires: a signed/whitelisted mapper (e.g. kdmapper variant) OR
//            a vulnerable driver (FILL: load your own — not shipped here,
//            driver handle assumed as g_driver_handle below)
// ════════════════════════════════════════════════════════════════════════
#include "eac_layer.h"
#include <windows.h>
#include <winternl.h>
#include <psapi.h>
#include <cstring>
#include <cstdio>

// ── ETW patch — kills EasyAntiCheat's kernel event telemetry usermode stub
// NtTraceEvent → ret (0xC3) in ntdll, prevents EAC from reading hook events
static void PatchETW() {
    HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
    if (!ntdll) return;
    auto fn = (FARPROC)GetProcAddress(ntdll, "NtTraceEvent");
    if (!fn) return;
    DWORD old; VirtualProtect(fn, 1, PAGE_EXECUTE_READWRITE, &old);
    *(BYTE*)fn = 0xC3; // ret
    VirtualProtect(fn, 1, old, &old);
}

// ── NTDLL unhook — restore .text from clean disk copy
// EAC hooks NtReadVirtualMemory, NtWriteVirtualMemory early;
// pulling a fresh mapped ntdll from \KnownDlls restores clean syscall stubs
static void UnhookNTDLL() {
    HANDLE section = OpenFileMappingW(FILE_MAP_READ, FALSE, L"\\KnownDlls\\ntdll.dll");
    if (!section) return;
    auto fresh = (BYTE*)MapViewOfFile(section, FILE_MAP_READ, 0, 0, 0);
    if (!fresh) { CloseHandle(section); return; }

    auto live = (BYTE*)GetModuleHandleW(L"ntdll.dll");
    auto dosh  = (IMAGE_DOS_HEADER*)fresh;
    auto nth   = (IMAGE_NT_HEADERS*)(fresh + dosh->e_lfanew);
    auto sec   = IMAGE_FIRST_SECTION(nth);

    for (WORD i = 0; i < nth->FileHeader.NumberOfSections; i++, sec++) {
        if (memcmp(sec->Name, ".text", 5) != 0) continue;
        DWORD old;
        VirtualProtect(live + sec->VirtualAddress, sec->Misc.VirtualSize,
                       PAGE_EXECUTE_READWRITE, &old);
        memcpy(live + sec->VirtualAddress,
               fresh + sec->VirtualAddress, sec->Misc.VirtualSize);
        VirtualProtect(live + sec->VirtualAddress, sec->Misc.VirtualSize,
                       old, &old);
        break;
    }
    UnmapViewOfFile(fresh);
    CloseHandle(section);
}

// ── Module-list spoof — hide our module from PEB InMemoryOrderModuleList
// EAC walks the PEB list; unlinking removes us from the snapshot
static void SpoofModuleList(HMODULE self) {
    auto peb = (PEB*)__readgsqword(0x60);
    auto head = &peb->Ldr->InMemoryOrderModuleList;
    for (auto e = head->Flink; e != head; e = e->Flink) {
        auto entry = CONTAINING_RECORD(e, LDR_DATA_TABLE_ENTRY, InMemoryOrderLinks);
        if (entry->DllBase == self) {
            // unlink from all three lists
            entry->InMemoryOrderLinks.Blink->Flink = entry->InMemoryOrderLinks.Flink;
            entry->InMemoryOrderLinks.Flink->Blink = entry->InMemoryOrderLinks.Blink;
            entry->InLoadOrderLinks.Blink->Flink   = entry->InLoadOrderLinks.Flink;
            entry->InLoadOrderLinks.Flink->Blink   = entry->InLoadOrderLinks.Blink;
            entry->InInitializationOrderLinks.Blink->Flink =
                entry->InInitializationOrderLinks.Flink;
            entry->InInitializationOrderLinks.Flink->Blink =
                entry->InInitializationOrderLinks.Blink;
            break;
        }
    }
}

void EAC_Init(HMODULE self) {
    PatchETW();
    UnhookNTDLL();
    SpoofModuleList(self);
}
