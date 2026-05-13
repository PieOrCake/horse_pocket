#include <windows.h>
#include "globals.h"
#include "Config.h"
#include "Mount.h"
#include "Settings.h"
#include <imgui.h>
#include <cstring>

AddonAPI*                         APIDefs        = nullptr;
std::atomic<RTAPI::RealTimeData*> g_RTAPI        = nullptr;
CooldownCheck                     g_CooldownCheck{};

static AddonDefinition AddonDef{};

static void OnKeybind(const char* aIdentifier, bool aIsRelease) {
    if (aIsRelease) return;
    if (strcmp(aIdentifier, "HP_SMART_MOUNT") == 0) Mount_OnKeybind();
}

static void OnRender() {
    if (!g_RTAPI.load()) {
        auto* p = static_cast<RTAPI::RealTimeData*>(APIDefs->DataLink.Get(DL_RTAPI));
        if (p) {
            g_RTAPI.store(p);
            APIDefs->Log(ELogLevel_INFO, "HorsePocket", "RTAPI DataLink acquired on frame tick");
        }
    }
    Mount_FrameTick();
}

static void OnOptionsRender() {
    Settings_Render();
}

static void OnAddonLoaded(void* aEventArgs) {
    if (!aEventArgs) return;
    uint32_t sig = *static_cast<uint32_t*>(aEventArgs);
    char buf[64];
    sprintf(buf, "OnAddonLoaded: sig=0x%08X (RTAPI_SIG=0x%08X)", sig, (uint32_t)RTAPI_SIG);
    APIDefs->Log(ELogLevel_INFO, "HorsePocket", buf);
    if (sig == (uint32_t)RTAPI_SIG) {
        auto* p = static_cast<RTAPI::RealTimeData*>(APIDefs->DataLink.Get(DL_RTAPI));
        g_RTAPI.store(p);
        APIDefs->Log(ELogLevel_INFO, "HorsePocket", p
            ? "OnAddonLoaded: RTAPI acquired"
            : "OnAddonLoaded: DataLink.Get still returned null");
    }
}

static void OnAddonUnloaded(void* aEventArgs) {
    if (!aEventArgs) return;
    if (*static_cast<uint32_t*>(aEventArgs) == (uint32_t)RTAPI_SIG)
        g_RTAPI.store(nullptr);
}

void AddonLoad(AddonAPI* aApi) {
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions(
        (void* (*)(size_t, void*))APIDefs->ImguiMalloc,
        (void(*)(void*, void*))APIDefs->ImguiFree
    );

    auto* rtapiAtLoad  = static_cast<RTAPI::RealTimeData*>(APIDefs->DataLink.Get(DL_RTAPI));
    auto* mumbleAtLoad = APIDefs->DataLink.Get("DL_MUMBLE_LINK");
    g_RTAPI.store(rtapiAtLoad);
    char dlbuf[128];
    sprintf(dlbuf, "AddonLoad: RTAPI=%p MumbleLink=%p", (void*)rtapiAtLoad, mumbleAtLoad);
    APIDefs->Log(ELogLevel_INFO, "HorsePocket", dlbuf);

    APIDefs->Events.Subscribe("EV_ADDON_LOADED",   OnAddonLoaded);
    APIDefs->Events.Subscribe("EV_ADDON_UNLOADED", OnAddonUnloaded);

    APIDefs->Renderer.Register(ERenderType_Render,        OnRender);
    APIDefs->Renderer.Register(ERenderType_OptionsRender, OnOptionsRender);

    APIDefs->InputBinds.RegisterWithString("HP_SMART_MOUNT", OnKeybind, "(null)");

    Config_Load();

    APIDefs->Log(ELogLevel_INFO, "HorsePocket", "Loaded");
}

void AddonUnload() {
    APIDefs->Events.Unsubscribe("EV_ADDON_LOADED",   OnAddonLoaded);
    APIDefs->Events.Unsubscribe("EV_ADDON_UNLOADED", OnAddonUnloaded);
    APIDefs->Renderer.Deregister(OnRender);
    APIDefs->Renderer.Deregister(OnOptionsRender);
    APIDefs->InputBinds.Deregister("HP_SMART_MOUNT");
    Config_Save();
}

extern "C" __declspec(dllexport) AddonDefinition* GetAddonDef() {
    AddonDef.Signature   = (signed int)0xB07E5EA1;
    AddonDef.APIVersion  = NEXUS_API_VERSION;
    AddonDef.Name        = "Horse Pocket";
    AddonDef.Version     = { 0, 1, 0, 0 };
    AddonDef.Author      = "PieOrCake.7635";
    AddonDef.Description = "Smart mount selection based on terrain. Requires RTAPI.";
    AddonDef.Load        = AddonLoad;
    AddonDef.Unload      = AddonUnload;
    AddonDef.Flags       = EAddonFlags_None;
    AddonDef.Provider    = EUpdateProvider_GitHub;
    AddonDef.UpdateLink  = "https://github.com/PieOrCake/horse_pocket";
    return &AddonDef;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
