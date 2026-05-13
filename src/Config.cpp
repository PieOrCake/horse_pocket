#include "Config.h"
#include "globals.h"
#include <fstream>
#include <filesystem>
#include <nlohmann/json.hpp>

Config g_Config;

static std::string ConfigPath() {
    return std::string(APIDefs->Paths.GetAddonDirectory("horse_pocket")) + "/config.json";
}

void Config_Save() {
    std::string path = ConfigPath();
    std::filesystem::create_directories(std::filesystem::path(path).parent_path());
    nlohmann::json j;
    j["mount_ground"]           = g_Config.mountGround;
    j["mount_water_surface"]    = g_Config.mountWaterSurface;
    j["mount_underwater"]       = g_Config.mountUnderwater;
    j["mount_airborne"]         = g_Config.mountAirborne;
    j["fallback_ground"]        = g_Config.fallbackGround;
    j["fallback_water_surface"] = g_Config.fallbackWaterSurface;
    j["fallback_underwater"]    = g_Config.fallbackUnderwater;
    j["fallback_airborne"]      = g_Config.fallbackAirborne;
    j["retry_delay_ms"]         = g_Config.retryDelayMs;
    std::ofstream f(path);
    if (f.is_open()) f << j.dump(2);
}

void Config_Load() {
    std::ifstream f(ConfigPath());
    if (!f.is_open()) return;
    try {
        auto j = nlohmann::json::parse(f);
        if (j.contains("mount_ground"))           g_Config.mountGround          = j["mount_ground"].get<std::string>();
        if (j.contains("mount_water_surface"))    g_Config.mountWaterSurface    = j["mount_water_surface"].get<std::string>();
        if (j.contains("mount_underwater"))       g_Config.mountUnderwater      = j["mount_underwater"].get<std::string>();
        if (j.contains("mount_airborne"))         g_Config.mountAirborne        = j["mount_airborne"].get<std::string>();
        if (j.contains("fallback_ground"))        g_Config.fallbackGround       = j["fallback_ground"].get<std::string>();
        if (j.contains("fallback_water_surface")) g_Config.fallbackWaterSurface = j["fallback_water_surface"].get<std::string>();
        if (j.contains("fallback_underwater"))    g_Config.fallbackUnderwater   = j["fallback_underwater"].get<std::string>();
        if (j.contains("fallback_airborne"))      g_Config.fallbackAirborne     = j["fallback_airborne"].get<std::string>();
        if (j.contains("retry_delay_ms"))         g_Config.retryDelayMs         = j["retry_delay_ms"].get<int>();
    } catch (...) {
        APIDefs->Log(ELogLevel_WARNING, "HorsePocket", "Failed to parse config.json; using defaults");
    }
}
