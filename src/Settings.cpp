#include "Settings.h"
#include "Config.h"
#include "globals.h"
#include <imgui.h>
#include <string>

static const char* MOUNTS_GROUND[]        = { "Raptor", "Springer", "Skimmer", "Jackal", "Griffon", "RollerBeetle", "Warclaw", "Skyscale", "SiegeTurtle" };
static const char* MOUNTS_WATER_SURFACE[] = { "Skimmer", "Skiff" };
static const char* MOUNTS_UNDERWATER[]    = { "Skimmer", "SiegeTurtle" };
static const char* MOUNTS_AIRBORNE[]      = { "Griffon", "Skyscale" };
static const int   COUNT_GROUND           = 9;
static const int   COUNT_WATER_SURFACE    = 2;
static const int   COUNT_UNDERWATER       = 2;
static const int   COUNT_AIRBORNE         = 2;

static bool NeedsFallback(const std::string& name) {
    return name == "Warclaw" || name == "Skyscale";
}

static int FindIndex(const char** list, int count, const std::string& name) {
    for (int i = 0; i < count; i++) {
        if (name == list[i]) return i;
    }
    return 0;
}

// Returns true if any value changed
static bool RenderScenarioRow(const char* label, const char** list, int count,
                               std::string& selection, std::string& fallback) {
    bool changed = false;
    int idx = FindIndex(list, count, selection);
    if (ImGui::Combo(label, &idx, list, count)) {
        selection = list[idx];
        changed = true;
    }
    if (NeedsFallback(selection)) {
        const char* fbList[8];
        int fbCount = 0;
        for (int i = 0; i < count; i++) {
            if (selection != list[i]) fbList[fbCount++] = list[i];
        }
        int fbIdx = 0;
        for (int i = 0; i < fbCount; i++) {
            if (fallback == fbList[i]) { fbIdx = i; break; }
        }
        std::string fbLabel = std::string("Cooldown fallback##") + label;
        ImGui::Indent();
        if (ImGui::Combo(fbLabel.c_str(), &fbIdx, fbList, fbCount)) {
            fallback = fbList[fbIdx];
            changed = true;
        }
        ImGui::Unindent();
    } else {
        if (!fallback.empty()) {
            fallback.clear();
            changed = true;
        }
    }
    return changed;
}

void Settings_Render() {
    ImGui::TextDisabled("Keybind");
    ImGui::Separator();
    ImGui::Text("Smart Mount");
    ImGui::SameLine();
    ImGui::TextDisabled("(HP_SMART_MOUNT)");
    ImGui::TextDisabled("Assign in Nexus Settings -> Keybinds -> Horse Pocket.");
    ImGui::TextDisabled("Use the same key as your GW2 Mount/Dismount for a transparent override.");
    ImGui::Spacing();

    ImGui::TextDisabled("Mount Selection");
    ImGui::Separator();

    bool changed = false;
    changed |= RenderScenarioRow("Ground##hp",        MOUNTS_GROUND,        COUNT_GROUND,        g_Config.mountGround,        g_Config.fallbackGround);
    changed |= RenderScenarioRow("Water Surface##hp", MOUNTS_WATER_SURFACE, COUNT_WATER_SURFACE, g_Config.mountWaterSurface,  g_Config.fallbackWaterSurface);
    changed |= RenderScenarioRow("Underwater##hp",    MOUNTS_UNDERWATER,    COUNT_UNDERWATER,    g_Config.mountUnderwater,    g_Config.fallbackUnderwater);
    changed |= RenderScenarioRow("Airborne##hp",      MOUNTS_AIRBORNE,      COUNT_AIRBORNE,      g_Config.mountAirborne,      g_Config.fallbackAirborne);

    if (changed) Config_Save();
}
