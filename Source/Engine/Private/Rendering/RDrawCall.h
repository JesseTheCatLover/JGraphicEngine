//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "RHandles.h"
#include "Core/Math/FMatrix4.h"

struct RRenderItem
{
    RMeshHandle mesh;
    RShaderHandle shader;
    std::vector<RTextureHandle> textures;
    FMatrix4 transform;
};
