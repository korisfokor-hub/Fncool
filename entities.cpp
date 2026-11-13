// ════════════════════════════════════════════════════════════════════════
//  offsets.h — ALL offsets in one place, re-derive per patch
// ════════════════════════════════════════════════════════════════════════
#pragma once
#include <cstdint>

namespace off {
    // ── sig-scan targets (use SigScan, don't hardcode) ──
    // GWorld:        48 8B 05 ? ? ? ? 48 8B 88 ? ? ? ? 48 85 C9 74 ? 48 8B 01
    // GNames:        48 8B 1D ? ? ? ? 48 85 DB 74 ? EB ? 33 DB 8B 05
    // GObjects:      89 0D ? ? ? ? 48 8B DF 48 89 5C 24

    constexpr uintptr_t GWorld             = 0x0;        // FILL via sig
    constexpr uintptr_t ULevel_Actors      = 0x98;
    constexpr uintptr_t TArray_Data        = 0x00;
    constexpr uintptr_t TArray_Count       = 0x08;

    constexpr uintptr_t AActor_RootComp    = 0x1A8;
    constexpr uintptr_t USceneComp_RelLoc  = 0x11C;      // FVector
    constexpr uintptr_t AActor_NetRole     = 0x0258;
    constexpr uintptr_t AActor_ClassName   = 0x0018;     // FName index

    // Chest / container
    constexpr uintptr_t ABuildingContainer_bOpened = 0x05A8; // bool, re-verify

    // Cannon
    constexpr uintptr_t ACannonActor_bCanFire      = 0x0600; // bool
    constexpr uintptr_t ACannonActor_OccupantPawn  = 0x0608; // AActor*

    // Player
    constexpr uintptr_t APawn_PlayerState          = 0x2A8;
    constexpr uintptr_t APlayerState_TeamIndex     = 0x3B4;

    // Camera (APlayerCameraManager)
    constexpr uintptr_t CamCache_ViewInfo          = 0x2A70; // FMinimalViewInfo
    // FMinimalViewInfo: loc+0x00, rot+0x0C, fov+0x18
}
