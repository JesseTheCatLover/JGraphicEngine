//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

struct FViewportOutput
{
    void* nativeTexture = nullptr;  // backend-agnostic handle
    bool bHasTexture = false;
};