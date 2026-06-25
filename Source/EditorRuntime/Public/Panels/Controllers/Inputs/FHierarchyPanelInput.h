//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

#include "FPanelInputBase.h"

using ActorID = uint64_t;

struct FHierarchyPanelInput : public FPanelInputBase
{
    bool bClickedItem = false;
    ActorID clickedActor = 0;

    bool bClearSelection = false; // background click

    bool bReparentRequested = false;
    ActorID draggedActor = 0;
    ActorID targetParentActor = 0;

    bool bDeleteRequested = false;
    ActorID targetActorToModify = 0;

    bool bRenameRequested = false;
    std::string newName = "";

    bool bToggleVisibilityRequested = false;
};