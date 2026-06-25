//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>

struct FHierarchySnapshot
{
    using ID = std::uint64_t;

    ID id = 0;
    ID parentID = 0;
    std::string name;

    bool bHasChildren = false;
    bool bVisible = false;
    bool bLocked = false;
};