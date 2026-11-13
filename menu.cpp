// ════════════════════════════════════════════════════════════════════════
//  menu.cpp — Dear ImGui D3D11 overlay
//  Tabs: Chest Farm | Cannon | Settings
//  Hooks Present via kiero
// ════════════════════════════════════════════════════════════════════════
#include "menu.h"
#include "entities.h"
#include "cannon.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "kiero.h"          // kiero 1.3+ — watch ABI on MSVC x64 release builds
#include <d3d11.h>
#include <atomic>

static ID3D11Device*        g_device    = nullptr;
static ID3D11DeviceContext* g_ctx       = nullptr;
static ID3D11RenderTargetView* g_rtv    = nullptr;
static HWND                 g_hwnd      = nullptr;
static bool                 g_menu_open = false;

// Farm state
static bool  s_farm_enabled   = false;
static bool  s_cannon_enabled = false;
static bool  s_fire_enabled   = false;
static float s_tp_range       = 2000.f;

typedef HRESULT(__stdcall* Present_t)(IDXGISwapChain*, UINT, UINT);
static Present_t o_Present = nullptr;

static void RenderMenu(const std::vector<ChestEntry>& chests,
                       const std::vector<CannonEntry>& cannons) {
    ImGui::SetNextWindowSize(ImVec2(420, 320), ImGuiCond_FirstUseEver);
    ImGui::Begin("Fortnite Chest Farm", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

    // ── top counter bar ──────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.4f,1.f,0.4f,1.f),
                       "Chests Opened: %d", Entities_GetChestCount());
    ImGui::SameLine(300);
    ImGui::TextDisabled("Active chests: %d | Cannons: %d",
                        (int)chests.size(), (int)cannons.size());
    ImGui::Separator();

    if (ImGui::BeginTabBar("tabs")) {

        // ── CHEST FARM TAB ───────────────────────────────────────────
        if (ImGui::BeginTabItem("Chest Farm")) {
            ImGui::Checkbox("Enable Chest Farm", &s_farm_enabled);
            ImGui::SliderFloat("TP Range (cm)", &s_tp_range, 500.f, 10000.f);
            ImGui::Spacing();
            if (s_farm_enabled && !chests.empty()) {
                ImGui::TextColored(ImVec4(1,1,0,1),
                    "Nearest chest at %.0f cm",
                    Dist_Extern(chests[0].loc)); // forward decl below
            } else if (chests.empty()) {
                ImGui::TextDisabled("No unlooted chests in actor list.");
            }
            ImGui::EndTabItem();
        }

        // ── CANNON TAB ───────────────────────────────────────────────
        if (ImGui::BeginTabItem("Cannon")) {
            ImGui::Checkbox("TP to Nearest Cannon", &s_cannon_enabled);
            ImGui::Checkbox("Auto Fire Loop",        &s_fire_enabled);
            ImGui::Spacing();
            ImGui::TextDisabled("Fire loop interval: 120ms on / 80ms off");
            ImGui::Spacing();
            if (s_fire_enabled && !g_fire_running_extern())
                Cannon_StartFire(cannons);
            else if (!s_fire_enabled && g_fire_running_extern())
                Cannon_StopFire();
            ImGui::EndTabItem();
        }

        // ── SETTINGS TAB ─────────────────────────────────────────────
        if (ImGui::BeginTabItem("Settings")) {
            ImGui::Text("Hotkey: INSERT = toggle menu");
            ImGui::Text("Hotkey: F5     = TP to nearest chest");
            ImGui::Text("Hotkey: F6     = TP to nearest cannon");
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextDisabled("EAC layer: ETW patched, NTDLL restored,");
            ImGui::TextDisabled("           module unlinked from PEB.");
            ImGui::Spacing();
            if (ImGui::Button("Reset Chest Counter"))
                Entities_ResetCounter();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }
    ImGui::End();
}

HRESULT __stdcall hk_Present(IDXGISwapChain* chain, UINT sync, UINT flags) {
    static bool init = false;
    if (!init) {
        chain->GetDevice(__uuidof(ID3D11Device), (void**)&g_device);
        g_device->GetImmediateContext(&g_ctx);
        DXGI_SWAP_CHAIN_DESC sd{};
        chain->GetDesc(&sd);
        g_hwnd = sd.OutputWindow;
        ID3D11Texture2D* buf;
        chain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&buf);
        g_device->CreateRenderTargetView(buf, nullptr, &g_rtv);
        buf->Release();
        ImGui::CreateContext();
        ImGui_ImplWin32_Init(g_hwnd);
        ImGui_ImplDX11_Init(g_device, g_ctx);
        ImGui::GetIO().IniFilename = nullptr;
        init = true;
    }

    if (GetAsyncKeyState(VK_INSERT) & 1) g_menu_open = !g_menu_open;

    std::vector<ChestEntry>  chests;
    std::vector<CannonEntry> cannons;
    Entities_Harvest(chests, cannons);

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();

    if (g_menu_open) RenderMenu(chests, cannons);

    // hotkey TP
    if (GetAsyncKeyState(VK_F5) & 1 && s_farm_enabled && !chests.empty()) {
        // TP to nearest chest: write player root loc
        // local_pawn root comp addr needed — FILL: read from local player controller
        // WPM<FVector>(local_root + off::USceneComp_RelLoc, chests[0].loc);
    }
    if (GetAsyncKeyState(VK_F6) & 1 && s_cannon_enabled) {
        // Cannon_TPToNearest(player_loc, cannons, local_root);
    }

    ImGui::Render();
    g_ctx->OMSetRenderTargets(1, &g_rtv, nullptr);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    return o_Present(chain, sync, flags);
}

void Menu_Init() {
    kiero::init(kiero::RenderType::D3D11);
    kiero::bind(8, (void**)&o_Present, hk_Present); // slot 8 = Present
}
