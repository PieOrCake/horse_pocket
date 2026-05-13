#include "Settings.h"
#include "Config.h"
#include "Mount.h"
#include "globals.h"
#include <imgui.h>
#include <iterator>
#include <string>
#include <vector>

static const char* MOUNTS_GROUND[]        = { "Raptor", "Springer", "Skimmer", "Jackal", "Griffon", "RollerBeetle", "Warclaw", "Skyscale", "SiegeTurtle" };
static const char* MOUNTS_WATER_SURFACE[] = { "Skimmer", "Skiff" };
static const char* MOUNTS_UNDERWATER[]    = { "Skimmer", "SiegeTurtle" };
static const char* MOUNTS_AIRBORNE[]      = { "Griffon", "Skyscale" };
static const int   COUNT_GROUND           = (int)std::size(MOUNTS_GROUND);
static const int   COUNT_WATER_SURFACE    = (int)std::size(MOUNTS_WATER_SURFACE);
static const int   COUNT_UNDERWATER       = (int)std::size(MOUNTS_UNDERWATER);
static const int   COUNT_AIRBORNE         = (int)std::size(MOUNTS_AIRBORNE);

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
    ImGui::SetNextItemWidth(160.0f);
    if (ImGui::Combo(label, &idx, list, count)) {
        selection = list[idx];
        changed = true;
    }
    if (Mount_NeedsCooldownFallback(selection)) {
        std::vector<const char*> fbList;
        fbList.reserve(count);
        for (int i = 0; i < count; i++) {
            if (selection != list[i]) fbList.push_back(list[i]);
        }
        int fbIdx = 0;
        for (int i = 0; i < (int)fbList.size(); i++) {
            if (fallback == fbList[i]) { fbIdx = i; break; }
        }
        std::string fbLabel = std::string("Cooldown fallback##") + label;
        ImGui::Indent();
        ImGui::SetNextItemWidth(160.0f);
        if (ImGui::Combo(fbLabel.c_str(), &fbIdx, fbList.data(), (int)fbList.size())) {
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
    bool changed = false;

    ImGui::TextDisabled("Keybind");
    ImGui::Separator();
    ImGui::Text("Smart Mount");
    ImGui::SameLine();
    ImGui::TextDisabled("(HP_SMART_MOUNT)");
    ImGui::TextDisabled("Use the same key as your GW2 Mount/Dismount for a transparent override.");
    ImGui::Spacing();

    ImGui::TextDisabled("Timing");
    ImGui::Separator();
    ImGui::SetNextItemWidth(200.0f);
    if (ImGui::SliderInt("Retry delay (ms)##hp", &g_Config.retryDelayMs, 100, 2000)) {
        changed = true;
    }
    ImGui::TextDisabled("How long to wait before trying the next mount if the first fails.");
    ImGui::TextDisabled("Increase only if mounts fail to activate reliably.");
    ImGui::Spacing();

    ImGui::TextDisabled("Mount Selection");
    ImGui::Separator();

    changed |= RenderScenarioRow("Ground##hp",        MOUNTS_GROUND,        COUNT_GROUND,        g_Config.mountGround,        g_Config.fallbackGround);
    changed |= RenderScenarioRow("Water Surface##hp", MOUNTS_WATER_SURFACE, COUNT_WATER_SURFACE, g_Config.mountWaterSurface,  g_Config.fallbackWaterSurface);
    changed |= RenderScenarioRow("Underwater##hp",    MOUNTS_UNDERWATER,    COUNT_UNDERWATER,    g_Config.mountUnderwater,    g_Config.fallbackUnderwater);
    changed |= RenderScenarioRow("Airborne##hp",      MOUNTS_AIRBORNE,      COUNT_AIRBORNE,      g_Config.mountAirborne,      g_Config.fallbackAirborne);

    if (changed) Config_Save();
}
