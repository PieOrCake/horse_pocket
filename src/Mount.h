#pragma once
#include <string>
#include "nexus/Nexus.h"
#include "mumble/Mumble.h"

struct MountInfo {
    const char*         name;
    EGameBinds          gameBind;
    Mumble::EMountIndex mountIndex;
};

extern const MountInfo MOUNT_TABLE[];
extern const int       MOUNT_TABLE_SIZE;

const MountInfo* Mount_FindByName(const std::string& name);

// Called on keybind press (key-down only)
void Mount_OnKeybind();

// Called each render frame to process pending cooldown check
void Mount_FrameTick();
