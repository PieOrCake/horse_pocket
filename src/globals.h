#pragma once
#include <windows.h>
#include <atomic>
#include <chrono>
#include <cstdint>
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"
#include "RTAPI.hpp"

extern AddonAPI*                          APIDefs;
extern Mumble::Data*                      g_MumbleLink;
extern std::atomic<RTAPI::RealTimeData*>  g_RTAPI;

// Queue of mount binds to try in sequence until one activates
struct CooldownCheck {
    bool       active     = false;
    EGameBinds queue[4]   = {};
    int        queueSize  = 0;
    int        currentIdx = 0;
    std::chrono::steady_clock::time_point startTime;
};
extern CooldownCheck g_CooldownCheck;
