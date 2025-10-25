//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "RObjects.h"
#include "RHandles.h"

class IPlatformSurface;
struct FMatrix4;

class IRenderBackend
{
public:
    enum class EResolveMask   : uint8_t { Color = 1, Depth = 2, Stencil = 4, ColorDepth = 3, All = 7 };
    enum class EResolveFilter : uint8_t { Nearest, Linear };

    virtual ~IRenderBackend() = default;

    // Lifecycle
    virtual bool Initialize(IPlatformSurface* surface) = 0;
    virtual void Shutdown() = 0;

    // Frame control
    virtual void BeginFrame() = 0;
    virtual void EndFrame() = 0;

    // Resource creation / destruction
    virtual RMeshHandle CreateMesh(const RMesh& meshData) = 0;
    virtual void DestroyMesh(RMeshHandle handle) = 0;

    virtual RTextureHandle CreateTexture(const RTexture& textureData) = 0;
    virtual void DestroyTexture(RTextureHandle handle) = 0;

    virtual RShaderHandle CreateShader(const RShader& shaderData) = 0;
    virtual void DestroyShader(RShaderHandle handle) = 0;

    virtual RFramebufferHandle CreateFramebuffer(const RFramebuffer& framebufferData) = 0;
    virtual void DestroyFramebuffer(RFramebufferHandle handle) = 0;

    // Binding
    virtual void BindShader(RShaderHandle shader) = 0;
    virtual void BindTexture(RTextureHandle texture, uint32_t slot) = 0;
    virtual void BindFramebuffer(RFramebufferHandle handle) = 0;
    virtual void UnbindFramebuffer() = 0;
    virtual void ResolveFramebuffer(RFramebufferHandle src, RFramebufferHandle dst,
                                EResolveMask mask = EResolveMask::Color,
                                EResolveFilter filter = EResolveFilter::Nearest) = 0;

    // Rendering
    virtual void SubmitMesh(RMeshHandle mesh, RShaderHandle shader, const FMatrix4& transform) = 0;

    // --- Uniforms (scalars/vectors/matrices) ---
    virtual void SetUniformInt  (RShaderHandle sh, const char* name, int v) = 0;
    virtual void SetUniformFloat(RShaderHandle sh, const char* name, float v) = 0;
    virtual void SetUniformVec2 (RShaderHandle sh, const char* name, const float* v2) = 0;
    virtual void SetUniformVec3 (RShaderHandle sh, const char* name, const float* v3) = 0;
    virtual void SetUniformVec4 (RShaderHandle sh, const char* name, const float* v4) = 0;
    virtual void SetUniformMat4 (RShaderHandle sh, const char* name, const float* mat4) = 0;

    virtual void LinkUniformBlock(RShaderHandle sh, const char* blockName, uint32_t bindingPoint) = 0;
};