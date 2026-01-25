//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>

struct FInspectorSnapshot;

struct FInspectorOutput
{
    bool bHasSelection = false;
    bool bHasSnapshot = false;

    // Keep it local to avoid include coupling.
    uint64_t selectedActor = 0;

    // Snapshot is owned elsewhere
    // Keep pointer stable for the frame.
    const FInspectorSnapshot* snapshot = nullptr;

    // Optional: status message to show in panel
    const char* statusText = nullptr;
};