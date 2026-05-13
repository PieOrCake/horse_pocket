#include <windows.h>
#include "globals.h"
#include "Config.h"
#include "Mount.h"
#include "Settings.h"
#include <imgui.h>
#include <cstring>

AddonAPI_t*          APIDefs        = nullptr;
std::atomic<RTAPI::RealTimeData*> g_RTAPI = nullptr;
CooldownCheck        g_CooldownCheck{};

static AddonDefinition_t AddonDef{};

static void OnKeybind(const char* aIdentifier, bool aIsRelease) {
    if (aIsRelease) return;
    if (strcmp(aIdentifier, "HP_SMART_MOUNT") == 0) Mount_OnKeybind();
}

static void OnRender() {
    Mount_FrameTick();
}

static void OnOptionsRender() {
    Settings_Render();
}

static void OnAddonLoaded(void* aEventArgs) {
    if (!aEventArgs) return;
    if (*static_cast<uint32_t*>(aEventArgs) == RTAPI_SIG)
        g_RTAPI.store(static_cast<RTAPI::RealTimeData*>(APIDefs->DataLink_Get(DL_RTAPI)));
}

static void OnAddonUnloaded(void* aEventArgs) {
    if (!aEventArgs) return;
    if (*static_cast<uint32_t*>(aEventArgs) == RTAPI_SIG)
        g_RTAPI.store(nullptr);
}

void AddonLoad(AddonAPI_t* aApi) {
    APIDefs = aApi;
    ImGui::SetCurrentContext((ImGuiContext*)APIDefs->ImguiContext);
    ImGui::SetAllocatorFunctions(
        (void* (*)(size_t, void*))APIDefs->ImguiMalloc,
        (void(*)(void*, void*))APIDefs->ImguiFree
    );

    g_RTAPI.store(static_cast<RTAPI::RealTimeData*>(APIDefs->DataLink_Get(DL_RTAPI)));

    APIDefs->Events_Subscribe(EV_ADDON_LOADED,   OnAddonLoaded);
    APIDefs->Events_Subscribe(EV_ADDON_UNLOADED, OnAddonUnloaded);

    APIDefs->GUI_Register(RT_Render,        OnRender);
    APIDefs->GUI_Register(RT_OptionsRender, OnOptionsRender);

    APIDefs->InputBinds_RegisterWithString("HP_SMART_MOUNT", OnKeybind, "(null)");

    Config_Load();

    APIDefs->Log(LOGL_INFO, "HorsePocket", "Loaded");
}

void AddonUnload() {
    APIDefs->Events_Unsubscribe(EV_ADDON_LOADED,   OnAddonLoaded);
    APIDefs->Events_Unsubscribe(EV_ADDON_UNLOADED, OnAddonUnloaded);
    APIDefs->GUI_Deregister(OnRender);
    APIDefs->GUI_Deregister(OnOptionsRender);
    APIDefs->InputBinds_Deregister("HP_SMART_MOUNT");
    Config_Save();
}

extern "C" __declspec(dllexport) AddonDefinition_t* GetAddonDef() {
    AddonDef.Signature   = 0xB07E5EA1;
    AddonDef.APIVersion  = NEXUS_API_VERSION;
    AddonDef.Name        = "Horse Pocket";
    AddonDef.Version     = { 0, 1, 0, 0 };
    AddonDef.Author      = "PieOrCake.7635";
    AddonDef.Description = "Smart mount selection based on terrain. Requires RTAPI.";
    AddonDef.Load        = AddonLoad;
    AddonDef.Unload      = AddonUnload;
    AddonDef.Flags       = AF_None;
    AddonDef.Provider    = UP_GitHub;
    AddonDef.UpdateLink  = "https://github.com/PieOrCake/horse_pocket";
    return &AddonDef;
}

BOOL WINAPI DllMain(HINSTANCE, DWORD, LPVOID) { return TRUE; }
