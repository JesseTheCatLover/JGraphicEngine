//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "GLBackend.h"

GLBackend::GLMesh * GLBackend::FindMesh(RMeshHandle handle)
{
}

GLBackend::GLMesh * GLBackend::FindMesh(RShaderHandle handle)
{
}

bool GLBackend::Initialize()
{
    if (!gladLoadGL((GLADloadfunc)glfwGetProcAddress)))
}

void GLBackend::Shutdown()
{
}

void GLBackend::BeginFrame()
{
}

void GLBackend::EndFrame()
{
}

RMeshHandle GLBackend::CreateMesh(const RMesh &meshData)
{
}

void GLBackend::DestroyMesh(RMeshHandle handle)
{
}

RTextureHandle GLBackend::CreateTexture(const RTexture &textureData)
{
}

void GLBackend::DestroyTexture(RTextureHandle handle)
{
}

RShaderHandle GLBackend::CreateShader(const RShader &shaderData)
{
}

void GLBackend::DestroyShader(RShaderHandle handle)
{
}

RFramebufferHandle GLBackend::CreateFramebuffer(const RFramebuffer &framebufferData)
{
}

void GLBackend::DestroyFramebuffer(RFramebufferHandle handle)
{
}

void GLBackend::BindShader(RShaderHandle shader)
{
}

void GLBackend::BindTexture(RTextureHandle texture, uint32_t slot)
{
}

void GLBackend::BindFramebuffer(RFramebufferHandle handle)
{
}

void GLBackend::UnbindFramebuffer()
{
}

void GLBackend::SubmitMesh(RMeshHandle mesh, RShaderHandle shader, const FMatrix4 &transform)
{
}

// Helpers
static GLuint CompileShader(GLenum type, const std::string& source)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);
    return shader;
}

static GLuint CreateProgram(const std::string& vs, const std::string& fs)
{
    GLuint vertexShader = CompileShader(GL_VERTEX_SHADER, vs);
    GLuint fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fs);
    GLuint program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return program;
}