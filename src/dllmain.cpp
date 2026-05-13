#include <windows.h>
#include "globals.h"
#include "nexus/Nexus.h"
#include "RTAPI.hpp"
#include <imgui.h>

AddonAPI_t*          APIDefs        = nullptr;
RTAPI::RealTimeData* g_RTAPI        = nullptr;
CooldownCheck        g_CooldownCheck{};

static AddonDefinition_t AddonDef{};

void AddonLoad(AddonAPI_t* aApi)  { APIDefs = aApi; }
void AddonUnload()                {}

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
