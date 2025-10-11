//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "RObjects.h"
#include "RHandles.h"

struct FMatrix4;

class IRenderBackend
{
public:
    virtual ~IRenderBackend() = default;

    // Life cycle
    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    // Frame control
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    // Resource creation
    virtual RMeshHandle CreateMesh(const RMesh& meshData) = 0;
    virtual RTextureHandle CreateTexture(const RTexture& textureData) = 0;
    virtual RShaderHandle CreateShader(const RShader& shaderData) = 0;
    virtual RFramebufferHandle CreateFramebuffer(const RFramebuffer& framebufferData) = 0;

    // Resource destruction
    virtual void DestroyMesh(RMeshHandle handle) = 0;
    virtual void DestroyTexture(RTextureHandle handle) = 0;
    virtual void DestroyShader(RShaderHandle handle) = 0;

    // Framebuffer / Targets
    virtual RFramebufferHandle createFramebuffer(int width, int height) = 0;
    virtual void destroyFramebuffer(RFramebufferHandle handle) = 0;
    virtual void bindFramebuffer(RFramebufferHandle handle) = 0;
    virtual void unbindFramebuffer() = 0;

    // Drawing
    virtual void BindShader(RShaderHandle shader) = 0;
    virtual void BindTexture(RTextureHandle texture, uint32_t slot) = 0;
    virtual void SetTransform(const FMatrix4& transform) = 0;
    virtual void DrawMesh(RMeshHandle mesh) = 0;

    // Rendering
    virtual void SubmitMesh(RMeshHandle mesh, RShaderHandle shader, const FMatrix4& transform) = 0;
};
