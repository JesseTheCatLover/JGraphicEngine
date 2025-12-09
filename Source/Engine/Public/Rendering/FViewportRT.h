//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "RHandles.h"

struct FViewportRT
{
    RFramebufferHandle fbo;
    RTextureHandle color;
    int width = 0;
    int height = 0;
};
