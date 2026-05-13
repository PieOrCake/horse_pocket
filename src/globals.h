#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include "nexus/Nexus.h"
#include "RTAPI.hpp"

extern AddonAPI_t*                        APIDefs;
extern std::atomic<RTAPI::RealTimeData*>  g_RTAPI;

struct CooldownCheck {
    bool        active        = false;
    uint32_t    expectedMount = 0;        // Mumble EMountIndex value cast to uint32_t
    EGameBinds  fallbackBind  = GB_SpumoniMAM01;
    std::chrono::steady_clock::time_point startTime;
};
extern CooldownCheck g_CooldownCheck;
