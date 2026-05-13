#pragma once
#include <string>

struct Config {
    std::string mountGround         = "Raptor";
    std::string mountWaterSurface   = "Skimmer";
    std::string mountUnderwater     = "SiegeTurtle";
    std::string mountAirborne       = "Griffon";
    std::string fallbackGround;
    std::string fallbackWaterSurface;
    std::string fallbackUnderwater;
    std::string fallbackAirborne;
    int         retryDelayMs        = 100;
    float       bumpThreshold       = 0.12f;
};

extern Config g_Config;

void Config_Load();
void Config_Save();
