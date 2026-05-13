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
    bool       active           = false;
    EGameBinds queue[4]         = {};
    int        queueSize        = 0;
    int        currentIdx       = 0;
    std::chrono::steady_clock::time_point startTime;
    // If >= 0, this is the index where airborne fallback entries begin.
    // At that point we compare current Y to startY; if the delta is below bumpThreshold,
    // the player likely returned to ground after a small bump, so we retry the ground mount.
    int        airborneStartIdx = -1;
    EGameBinds groundBind       = {};
    float      startY           = 0.0f;
};
extern CooldownCheck g_CooldownCheck;
