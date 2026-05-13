#include "Mount.h"
#include "Config.h"
#include "globals.h"
#include <chrono>
#include <windows.h>
#include <cstring>

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

// Appends a mount + optional cooldown-fallback to the queue. Returns new size.
static int AppendToQueue(EGameBinds* queue, int size, const std::string& mountName,
                         const std::string& fallbackName) {
    const MountInfo* m = Mount_FindByName(mountName);
    if (!m || size >= 4) return size;
    queue[size++] = m->gameBind;

    if (Mount_NeedsCooldownFallback(mountName) && !fallbackName.empty()) {
        const MountInfo* fb = Mount_FindByName(fallbackName);
        if (fb && size < 4) queue[size++] = fb->gameBind;
    }
    return size;
}

void Mount_OnKeybind() {
    if (!g_MumbleLink) return;

    // Dismount if already mounted
    if (g_MumbleLink->Context.MountIndex != Mumble::EMountIndex::None) {
        PressMount(EGameBinds_SpumoniToggle);
        return;
    }

    // Primary terrain detection: MumbleLink Y position
    float y             = g_MumbleLink->AvatarPosition.Y;
    bool  isUnderwater  = (y < -1.2f);
    bool  isWaterSurface = (y >= -1.2f && y <= -1.0f);
    bool  isAirborne    = false;

    // RTAPI override when available (adds accurate airborne detection)
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
    bool isGroundScenario = false;

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
        mountName       = &g_Config.mountGround;
        fallbackName    = &g_Config.fallbackGround;
        isGroundScenario = true;
    }

    // Build the fallback queue:
    //   [terrain mount] → [terrain cooldown fallback?]
    //   → [airborne mount?] → [airborne cooldown fallback?]   (ground scenario only)
    EGameBinds queue[4] = {};
    int queueSize = 0;

    queueSize = AppendToQueue(queue, queueSize, *mountName, *fallbackName);

    // Airborne mount appended as last resort only for ground scenario and only when
    // RTAPI is not available (if RTAPI is available, airborne is already detected above)
    if (isGroundScenario && (!rtapi || rtapi->GameBuild == 0)) {
        queueSize = AppendToQueue(queue, queueSize,
                                  g_Config.mountAirborne, g_Config.fallbackAirborne);
    }

    if (queueSize == 0) return;

    // Press the first mount immediately
    PressMount(queue[0]);

    // Arm the sequence checker if there are fallbacks to try
    if (queueSize > 1) {
        EnterCriticalSection(&s_cs);
        g_CooldownCheck.active     = true;
        memcpy(g_CooldownCheck.queue, queue, queueSize * sizeof(EGameBinds));
        g_CooldownCheck.queueSize  = queueSize;
        g_CooldownCheck.currentIdx = 0;
        g_CooldownCheck.startTime  = std::chrono::steady_clock::now();
        LeaveCriticalSection(&s_cs);
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

    // Any mount activated → done
    if (g_MumbleLink->Context.MountIndex != Mumble::EMountIndex::None) {
        g_CooldownCheck.active = false;
        LeaveCriticalSection(&s_cs);
        return;
    }

    auto elapsed = std::chrono::steady_clock::now() - g_CooldownCheck.startTime;
    if (elapsed < std::chrono::milliseconds(g_Config.retryDelayMs)) {
        LeaveCriticalSection(&s_cs);
        return;
    }

    // Timer expired — advance to next mount in queue
    g_CooldownCheck.currentIdx++;
    if (g_CooldownCheck.currentIdx >= g_CooldownCheck.queueSize) {
        g_CooldownCheck.active = false;
        LeaveCriticalSection(&s_cs);
        return;
    }

    EGameBinds nextBind       = g_CooldownCheck.queue[g_CooldownCheck.currentIdx];
    g_CooldownCheck.startTime = std::chrono::steady_clock::now();
    LeaveCriticalSection(&s_cs);

    PressMount(nextBind);
}
