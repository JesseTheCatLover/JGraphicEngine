//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

#include "FPanelInputBase.h"

using ActorID = uint64_t;

struct FHierarchyPanelInput : public FPanelInputBase
{
    bool bClickedItem = false;
    ActorID clickedActor = 0;

    bool bClearSelection = false; // background click
};