#include "Mount.h"
#include "Config.h"
#include "globals.h"
#include <chrono>
#include <windows.h>

static CRITICAL_SECTION s_cs;

static struct CsInit {
    CsInit()  { InitializeCriticalSection(&s_cs); }
    ~CsInit() { DeleteCriticalSection(&s_cs); }
} s_csInit;

const MountInfo MOUNT_TABLE[] = {
    { "Raptor",       EGameBinds_SpumoniMAM01,    Mumble::EMountIndex::Raptor       },
    { "Springer",     EGameBinds_SpumoniMAM02,    Mumble::EMountIndex::Springer     },
    { "Skimmer",      EGameBinds_SpumoniMAM03,    Mumble::EMountIndex::Skimmer      },
    { "Jackal",       EGameBinds_SpumoniMAM04,    Mumble::EMountIndex::Jackal       },
    { "Griffon",      EGameBinds_SpumoniMAM05,    Mumble::EMountIndex::Griffon      },
    { "RollerBeetle", EGameBinds_SpumoniMAM06,    Mumble::EMountIndex::RollerBeetle },
    { "Warclaw",      EGameBinds_SpumoniMAM07,    Mumble::EMountIndex::Warclaw      },
    { "Skyscale",     EGameBinds_SpumoniMAM08,    Mumble::EMountIndex::Skyscale     },
    { "Skiff",        EGameBinds_MasteryAccess02, Mumble::EMountIndex::Skiff        },
    { "SiegeTurtle",  EGameBinds_SpumoniMAM09,    Mumble::EMountIndex::SiegeTurtle  },
};
const int MOUNT_TABLE_SIZE = sizeof(MOUNT_TABLE) / sizeof(MOUNT_TABLE[0]);

const MountInfo* Mount_FindByName(const std::string& name) {
    for (int i = 0; i < MOUNT_TABLE_SIZE; i++) {
        if (name == MOUNT_TABLE[i].name) return &MOUNT_TABLE[i];
    }
    return nullptr;
}

static void PressMount(EGameBinds bind) {
    APIDefs->GameBinds.PressAsync(bind);
    APIDefs->GameBinds.ReleaseAsync(bind);
}

static bool NeedsCooldownCheck(const std::string& name) {
    return name == "Warclaw" || name == "Skyscale";
}

void Mount_OnKeybind() {
    if (!g_MumbleLink) return;

    // Dismount if already mounted
    if (g_MumbleLink->Context.MountIndex != Mumble::EMountIndex::None) {
        APIDefs->GameBinds.PressAsync(EGameBinds_SpumoniToggle);
        APIDefs->GameBinds.ReleaseAsync(EGameBinds_SpumoniToggle);
        return;
    }

    // Primary: derive terrain from MumbleLink Y position
    // Thresholds match GW2Radial's StateObserver
    float y = g_MumbleLink->AvatarPosition.Y;
    bool isUnderwater   = (y < -1.2f);
    bool isWaterSurface = (y >= -1.2f && y <= -1.0f);
    bool isAirborne     = false;

    // RTAPI overrides when available (more accurate, includes airborne detection)
    auto* rtapi = g_RTAPI.load();
    if (rtapi && rtapi->GameBuild != 0) {
        auto state      = static_cast<uint32_t>(rtapi->CharacterState);
        auto flying     = static_cast<uint32_t>(RTAPI::ECharacterState::IsFlying);
        auto gliding    = static_cast<uint32_t>(RTAPI::ECharacterState::IsGliding);
        auto underwater = static_cast<uint32_t>(RTAPI::ECharacterState::IsUnderwater);
        auto swimming   = static_cast<uint32_t>(RTAPI::ECharacterState::IsSwimming);
        isUnderwater    = (state & underwater) != 0;
        isWaterSurface  = (state & swimming)   != 0;
        isAirborne      = ((state & flying) || (state & gliding)) != 0;
    }

    // Select scenario
    const std::string* mountName    = nullptr;
    const std::string* fallbackName = nullptr;

    if (isAirborne) {
        mountName    = &g_Config.mountAirborne;
        fallbackName = &g_Config.fallbackAirborne;
    } else if (isUnderwater) {
        mountName    = &g_Config.mountUnderwater;
        fallbackName = &g_Config.fallbackUnderwater;
    } else if (isWaterSurface) {
        mountName    = &g_Config.mountWaterSurface;
        fallbackName = &g_Config.fallbackWaterSurface;
    } else {
        mountName    = &g_Config.mountGround;
        fallbackName = &g_Config.fallbackGround;
    }

    const MountInfo* mount = Mount_FindByName(*mountName);
    if (!mount) return;

    PressMount(mount->gameBind);

    if (NeedsCooldownCheck(*mountName) && fallbackName && !fallbackName->empty()) {
        const MountInfo* fallback = Mount_FindByName(*fallbackName);
        if (fallback) {
            EnterCriticalSection(&s_cs);
            g_CooldownCheck.active        = true;
            g_CooldownCheck.expectedMount = static_cast<uint32_t>(mount->mountIndex);
            g_CooldownCheck.fallbackBind  = fallback->gameBind;
            g_CooldownCheck.startTime     = std::chrono::steady_clock::now();
            LeaveCriticalSection(&s_cs);
        }
    }
}

void Mount_FrameTick() {
    EnterCriticalSection(&s_cs);
    if (!g_CooldownCheck.active) {
        LeaveCriticalSection(&s_cs);
        return;
    }
    if (!g_MumbleLink) {
        g_CooldownCheck.active = false;
        LeaveCriticalSection(&s_cs);
        return;
    }
    // RTAPI and Mumble both source MountIndex from the GW2 Mumble link — values match
    uint32_t curMount = static_cast<uint32_t>(g_MumbleLink->Context.MountIndex);
    if (curMount == g_CooldownCheck.expectedMount) {
        g_CooldownCheck.active = false;
        LeaveCriticalSection(&s_cs);
        return;
    }
    auto elapsed  = std::chrono::steady_clock::now() - g_CooldownCheck.startTime;
    EGameBinds fb = g_CooldownCheck.fallbackBind;
    bool fire     = (elapsed >= std::chrono::milliseconds(300));
    if (fire) g_CooldownCheck.active = false;
    LeaveCriticalSection(&s_cs);

    if (fire) PressMount(fb);
}
