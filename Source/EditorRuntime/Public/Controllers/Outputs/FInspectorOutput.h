//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include "Controllers/InspectorProviders/FInspectorDocument.h"

struct FInspectorOutput
{
    bool bHasSelection = false;
    uint64_t selectedActor = 0;

    bool bHasDocument = false;
    const FInspectorDocument* document = nullptr;

    const char* statusText = nullptr;
};