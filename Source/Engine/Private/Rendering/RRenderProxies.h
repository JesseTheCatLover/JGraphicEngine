//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "RHandles.h"
#include "Core/Math/FMatrix4.h"

class IRenderBackend;

struct RRenderProxy {
    virtual ~RRenderProxy() = default;
    virtual void Submit(IRenderBackend* backend) = 0;
};

struct RMeshProxy : public RRenderProxy {
    RMeshHandle mesh;
    RShaderHandle shader;
    FMatrix4 transform;

    void Submit(IRenderBackend* backend) override;

};

struct RLightProxy : public RRenderProxy {
    FVector3 position;
    FVector3 color;
    float intensity;

    void Submit(IRenderBackend* backend) override;
};
