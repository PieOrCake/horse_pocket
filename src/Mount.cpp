#include "Mount.h"
#include "Config.h"
#include "globals.h"
#include <chrono>

const MountInfo MOUNT_TABLE[] = {
    { "Raptor",       GB_SpumoniMAM01,    Mumble::EMountIndex::Raptor       },
    { "Springer",     GB_SpumoniMAM02,    Mumble::EMountIndex::Springer     },
    { "Skimmer",      GB_SpumoniMAM03,    Mumble::EMountIndex::Skimmer      },
    { "Jackal",       GB_SpumoniMAM04,    Mumble::EMountIndex::Jackal       },
    { "Griffon",      GB_SpumoniMAM05,    Mumble::EMountIndex::Griffon      },
    { "RollerBeetle", GB_SpumoniMAM06,    Mumble::EMountIndex::RollerBeetle },
    { "Warclaw",      GB_SpumoniMAM07,    Mumble::EMountIndex::Warclaw      },
    { "Skyscale",     GB_SpumoniMAM08,    Mumble::EMountIndex::Skyscale     },
    { "Skiff",        GB_MasteryAccess02, Mumble::EMountIndex::Skiff        },
    { "SiegeTurtle",  GB_SpumoniMAM09,    Mumble::EMountIndex::SiegeTurtle  },
};
const int MOUNT_TABLE_SIZE = sizeof(MOUNT_TABLE) / sizeof(MOUNT_TABLE[0]);

const MountInfo* Mount_FindByName(const std::string& name) {
    for (int i = 0; i < MOUNT_TABLE_SIZE; i++) {
        if (name == MOUNT_TABLE[i].name) return &MOUNT_TABLE[i];
    }
    return nullptr;
}

static void PressMount(EGameBinds bind) {
    APIDefs->GameBinds_PressAsync(bind);
    APIDefs->GameBinds_ReleaseAsync(bind);
}

static bool NeedsCooldownCheck(const std::string& name) {
    return name == "Warclaw" || name == "Skyscale";
}

void Mount_OnKeybind() {
    if (!g_RTAPI || g_RTAPI->GameBuild == 0) {
        APIDefs->GUI_SendAlert("Horse Pocket: RTAPI not installed");
        return;
    }
    if (g_RTAPI->GameState != RTAPI::EGameState::Gameplay) return;

    // Dismount if already mounted
    if (g_RTAPI->MountIndex != 0) {
        APIDefs->GameBinds_PressAsync(GB_SpumoniToggle);
        APIDefs->GameBinds_ReleaseAsync(GB_SpumoniToggle);
        return;
    }

    // Determine terrain from CharacterState bitmask
    auto state = static_cast<uint32_t>(g_RTAPI->CharacterState);
    const std::string* mountName    = nullptr;
    const std::string* fallbackName = nullptr;

    auto flying     = static_cast<uint32_t>(RTAPI::ECharacterState::IsFlying);
    auto gliding    = static_cast<uint32_t>(RTAPI::ECharacterState::IsGliding);
    auto underwater = static_cast<uint32_t>(RTAPI::ECharacterState::IsUnderwater);
    auto swimming   = static_cast<uint32_t>(RTAPI::ECharacterState::IsSwimming);

    if ((state & flying) || (state & gliding)) {
        mountName    = &g_Config.mountAirborne;
        fallbackName = &g_Config.fallbackAirborne;
    } else if (state & underwater) {
        mountName    = &g_Config.mountUnderwater;
        fallbackName = &g_Config.fallbackUnderwater;
    } else if (state & swimming) {
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
            g_CooldownCheck.active        = true;
            g_CooldownCheck.expectedMount = static_cast<uint32_t>(mount->mountIndex);
            g_CooldownCheck.fallbackBind  = fallback->gameBind;
            g_CooldownCheck.startTime     = std::chrono::steady_clock::now();
        }
    }
}

void Mount_FrameTick() {
    if (!g_CooldownCheck.active) return;
    if (!g_RTAPI || g_RTAPI->GameBuild == 0) {
        g_CooldownCheck.active = false;
        return;
    }
    // Mounted successfully — cancel
    if (g_RTAPI->MountIndex == g_CooldownCheck.expectedMount) {
        g_CooldownCheck.active = false;
        return;
    }
    auto elapsed = std::chrono::steady_clock::now() - g_CooldownCheck.startTime;
    if (elapsed >= std::chrono::milliseconds(300)) {
        PressMount(g_CooldownCheck.fallbackBind);
        g_CooldownCheck.active = false;
    }
}
