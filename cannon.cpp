// ════════════════════════════════════════════════════════════════════════
//  cannon.cpp — TP to nearest cannon, auto-fire loop
// ════════════════════════════════════════════════════════════════════════
#include "cannon.h"
#include "mem.h"
#include "offsets.h"
#include "entities.h"
#include <cmath>
#include <limits>
#include <thread>
#include <chrono>
#include <atomic>

static std::atomic<bool> g_fire_running{ false };

static float Dist(const FVector& a, const FVector& b) {
    float dx = a.x-b.x, dy = a.y-b.y, dz = a.z-b.z;
    return sqrtf(dx*dx + dy*dy + dz*dz);
}

// Write player location → nearest cannon's location (TP)
bool Cannon_TPToNearest(const FVector& player_loc,
                        const std::vector<CannonEntry>& cannons,
                        uintptr_t local_root_comp) {
    if (cannons.empty()) return false;
    const CannonEntry* best = nullptr;
    float best_d = std::numeric_limits<float>::max();
    for (auto& c : cannons) {
        float d = Dist(player_loc, c.loc);
        if (d < best_d) { best_d = d; best = &c; }
    }
    if (!best) return false;

    // Offset slightly above cannon to land inside it
    FVector dest = best->loc;
    dest.z += 80.f;
    WPM<FVector>(local_root_comp + off::USceneComp_RelLoc, dest);
    return true;
}

// Auto-fire: repeatedly toggle bCanFire on each cannon in list
// Runs on its own thread; stop with Cannon_StopFire()
static void FireLoop(std::vector<CannonEntry> cannons) {
    while (g_fire_running) {
        for (auto& c : cannons) {
            if (!g_fire_running) break;
            WPM<bool>(c.actor + off::ACannonActor_bCanFire, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(120));
            WPM<bool>(c.actor + off::ACannonActor_bCanFire, false);
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
        }
    }
}

void Cannon_StartFire(const std::vector<CannonEntry>& cannons) {
    if (g_fire_running) return;
    g_fire_running = true;
    std::thread(FireLoop, cannons).detach();
}

void Cannon_StopFire() { g_fire_running = false; }
