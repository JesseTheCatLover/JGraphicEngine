//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>

#include "../IRenderBackend.h"
#include <glad/gl.h>

using jint = uint32_t;

class GLBackend : public IRenderBackend
{
private:
    struct GLMesh
    {
        GLuint vao = 0;
        GLuint vbo = 0;
        GLuint ebo = 0;
        uint32_t indexCount = 0;
    };

    struct GLShader
    {
        GLuint id = 0;
    };

    std::unordered_map<jint, GLMesh> m_Meshes;
    std::unordered_map<jint, GLShader> m_Shaders;
    jint m_NextID = 1;

    GLMesh* FindMesh(RMeshHandle handle);
    GLMesh* FindMesh(RShaderHandle handle);

public:
    bool Initialize() override;
    void Shutdown() override;

    void BeginFrame() override;
    void EndFrame() override;

    // Resource creation / destruction
    RMeshHandle CreateMesh(const RMesh& meshData) override;
    void DestroyMesh(RMeshHandle handle) override;

    RTextureHandle CreateTexture(const RTexture& textureData) override;
    void DestroyTexture(RTextureHandle handle) override;

    RShaderHandle CreateShader(const RShader& shaderData) override;
    void DestroyShader(RShaderHandle handle) override;

    RFramebufferHandle CreateFramebuffer(const RFramebuffer& framebufferData) override;
    void DestroyFramebuffer(RFramebufferHandle handle) override;

    // Binding
    void BindShader(RShaderHandle shader) override;
    void BindTexture(RTextureHandle texture, uint32_t slot) override;
    void BindFramebuffer(RFramebufferHandle handle) override;
    void UnbindFramebuffer() override;

    // Rendering
    void SubmitMesh(RMeshHandle mesh, RShaderHandle shader, const FMatrix4& transform) override;
};
