//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "RHandles.h"
#include "RRenderQueue.h"

#include "Core/Math/FMatrix4.h"

class RRenderRoute;
class IRenderBackend;

struct RRenderContext
{
    ERenderLayer layer = ERenderLayer::Opaque;
    uint16_t depthBucket{0};
};

struct RRenderProxy
{
    virtual ~RRenderProxy() = default;
    virtual void RecordToRoute(RRenderRoute& route, const RRenderContext& ctx) = 0;
};

struct RMeshProxy : public RRenderProxy
{
    RMeshHandle mesh;
    RShaderHandle shader;
    RMaterialHandle material;
    FMatrix4 transform;

    void RecordToRoute(RRenderRoute& route, const RRenderContext &ctx) override;
};

struct RLightProxy : public RRenderProxy
{
    FVector3 position;
    FVector3 color{1,1,1};
    float intensity{1.f};

    void RecordToRoute(RRenderRoute& route, const RRenderContext &ctx) override;
};
