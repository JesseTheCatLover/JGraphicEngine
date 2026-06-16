//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

#include "EditorAPI/Scene/FHierarchySnapshot.h"

using ActorID = uint64_t;

struct FHierarchyOutput
{
    const std::vector<FHierarchySnapshot>* snapshot = nullptr;
    bool bHasSnapshot = false;

    ActorID revealActorID = 0;
};