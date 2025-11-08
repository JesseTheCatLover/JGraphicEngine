//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include "RHandles.h"

struct RShader;
struct RTexture;
struct RMesh;

class IRenderDevice
{
public:
    virtual ~IRenderDevice() = default;

    virtual RMeshHandle CreateMesh (const RMesh& data) = 0;
    virtual void DestroyMesh (RMeshHandle h) = 0;

    virtual RTextureHandle CreateTexture (const RTexture& data) = 0;
    virtual void DestroyTexture(RTextureHandle h) = 0;

    virtual RShaderHandle CreateShader  (const RShader& data) = 0;
    virtual void DestroyShader (RShaderHandle h) = 0;

    // virtual RMaterialHandle CreateMaterial(const RMaterialDesc& d) = 0;
    // Queue a lambda to run on the render thread.
    virtual void Enqueue(std::function<void()> fn) = 0;
};
