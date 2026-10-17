// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Rendering/RHandles.h"

struct FViewportOutput
{
    RTextureHandle textureHandle{};
    int width = 0;
    int height = 0;

    // Temporary compatibility for the existing ImGui renderer.
    void* nativeTexture = nullptr;
    bool bHasTexture = false;
};