//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

enum class EAssetDomain : uint8_t
{
    Engine,
    Project
};

enum class EAssetVisibility : uint8_t
{
    Project,       // Project asset, accessible by user
    EnginePrivate, // Engine asset, not accessible by user
    EnginePublic,  // Engine asset but accessible by user
};
