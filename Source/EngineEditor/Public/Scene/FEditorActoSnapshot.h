//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

using ActorID = std::uint64_t;

struct FEditorActorSnapshot
{
    ActorID id = 0;
    ActorID parentID = 0;
    std::string name;

    bool hasChildren = false;
    bool isSelected = false;
    bool isHidden = false;
    bool isLocked = false;
};