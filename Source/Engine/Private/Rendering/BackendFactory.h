//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <memory>

#include "EGraphicsAPI.h"
#include "ESurfaceAPI.h"
#include "IRenderBackend.h"
#include "Core/Memory/SmartPointers.h"

class BackendFactory
{
public:
    static TUniquePtr<IPlatformSurface> MakeSurfaceBackend(ESurfaceAPI api);
    static TUniquePtr<IRenderBackend> MakeRenderBackend(EGraphicsAPI api);
};