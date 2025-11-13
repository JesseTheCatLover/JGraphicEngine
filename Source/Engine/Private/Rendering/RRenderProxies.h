// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <vector>
#include "RHandles.h"
#include "RCommandQueue.h"

#include "Core/Math/FMatrix4.h"
#include "Core/Math/FVector3.h"

class IRenderSubmission;

struct FRenderContext
{
    ERenderLayer layer = ERenderLayer::Opaque;
    uint16_t depthBucket{0};
};

class RRenderProxy
{
public:
    virtual ~RRenderProxy() = default;

    // Main entry: take context + submission sink, emit draw/light work into it
    virtual void SubmitProxy(IRenderSubmission& submission, const FRenderContext& ctx) const = 0;
};

struct RMeshProxy : public RRenderProxy
{
    RMeshHandle     mesh;
    RShaderHandle   shader;
    RMaterialHandle material;
    FMatrix4        transform;

    void SubmitProxy(IRenderSubmission& submission, const FRenderContext &ctx) const override;
};

struct RLightProxy : public RRenderProxy
{
    FVector3 position;
    FVector3 color{1,1,1};
    float    intensity{1.f};

    void SubmitProxy(IRenderSubmission& submission, const FRenderContext &ctx) const override;
};
