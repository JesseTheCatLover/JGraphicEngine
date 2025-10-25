//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "GLBackend.h"
#include "Rendering/IPlatformSurface.h"
#include "Core/Math/FMatrix4.h"
#include <iostream>
#include <string>
#include <cmath>
#include <vector>
#include <algorithm>

#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

std::unordered_map<RMeshHandle, GLBackend::FGLMesh> GLBackend::m_Meshes{};
std::unordered_map<RShaderHandle, GLBackend::FGLShader> GLBackend::m_Shaders{};
std::unordered_map<RTextureHandle, GLBackend::FGLTexture> GLBackend::m_Textures{};
std::unordered_map<RFramebufferHandle, GLBackend::FGLFramebuffer> GLBackend::m_Framebuffers{};

// Helpers
static inline uint32_t NextId(uint32_t &next) { return next++; }

static bool CheckShader(GLuint sh, const char* label)
{
    GLint ok = GL_FALSE; glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len=0; glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len); glGetShaderInfoLog(sh, len, nullptr, log.data());
        std::cerr << "[GLBackend]: Shader compile failed (" << label << "): " << log.data() << "\n";
        return false;
    }
    return true;
}
static bool CheckProgram(GLuint prog)
{
    GLint ok = GL_FALSE; glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len=0; glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<GLchar> log(len); glGetProgramInfoLog(prog, len, nullptr, log.data());
        std::cerr << "[GLBackend]: Program link failed: " << log.data() << "\n";
        return false;
    }
    return true;
}

static GLuint CompileShaderChecked(GLenum type, const std::string& source, const char* label)
{
    GLuint s = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    CheckShader(s, label);
    return s;
}

static inline void GLFormatFrom(ETexFormat fmt, GLint& internal, GLenum& external) {
    switch (fmt) {
        case ETexFormat::RGBA8:    internal = GL_RGBA8;      external = GL_RGBA; break;
        case ETexFormat::RGB8:     internal = GL_RGB8;       external = GL_RGB;  break;
        case ETexFormat::SRGB8_A8: internal = GL_SRGB8_ALPHA8; external = GL_RGBA; break;
        case ETexFormat::SRGB8:    internal = GL_SRGB8;      external = GL_RGB;  break;
        case ETexFormat::R8:       internal = GL_R8;         external = GL_RED;  break;
        case ETexFormat::RG16F:    internal = GL_RG16F;      external = GL_RG;   break;
        case ETexFormat::RGBA16F:  internal = GL_RGBA16F;    external = GL_RGBA; break;
        default:                   internal = GL_RGBA8;      external = GL_RGBA; break;
    }
}

static inline GLuint GetProgramFromHandle(const std::unordered_map<RShaderHandle, GLBackend::FGLShader>& map,
                                          RShaderHandle h) {
    auto it = map.find(h);
    return (it != map.end()) ? it->second.program : 0u;
}

static inline GLint GetUniformLoc(GLuint prog, const char* name) {
    return (prog && name) ? glGetUniformLocation(prog, name) : -1;
}

static inline GLbitfield ToGLMask(IRenderBackend::EResolveMask m) {
    using M = IRenderBackend::EResolveMask;
    GLbitfield bits = 0;
    if ((static_cast<uint8_t>(m) & static_cast<uint8_t>(M::Color))   != 0) bits |= GL_COLOR_BUFFER_BIT;
    if ((static_cast<uint8_t>(m) & static_cast<uint8_t>(M::Depth))   != 0) bits |= GL_DEPTH_BUFFER_BIT;
    if ((static_cast<uint8_t>(m) & static_cast<uint8_t>(M::Stencil)) != 0) bits |= GL_STENCIL_BUFFER_BIT;
    return bits;
}
static inline GLenum ToGLFilter(IRenderBackend::EResolveFilter f) {
    using F = IRenderBackend::EResolveFilter;
    return (f == F::Linear) ? GL_LINEAR : GL_NEAREST;
}

bool GLBackend::Initialize(IPlatformSurface* surface)
{
    if (!surface)
    {
        std::cerr << "[GLBackend]: No valid surface provided!" << std::endl;
        return false;
    }

    m_Surface = surface;

    // Get the platform-specific loader function
    const auto loader = surface->GetProcAddressFunction();
    if (!loader)
    {
        std::cerr << "[GLBackend]: Surface does not provide GetProcAddressFunction!" << std::endl;
        return false;
    }

    // Load GL using that function
    if (!gladLoadGL((GLADloadfunc)loader))
    {
        std::cerr << "[GLBackend]: Failed to load OpenGL functions (GLAD)" << std::endl;
        return false;
    }

    std::cout << "[GLBackend]: OpenGL version" << glGetString(GL_VERSION) << " with renderer "
    << glGetString(GL_RENDERER) << " loaded successfully" << std::endl;

    glEnable(GL_DEPTH_TEST); // TODO: check
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    return true;
}

void GLBackend::Shutdown()
{
    // Clean up meshes
    for (auto &[_, mesh] : m_Meshes)
    {
        glDeleteVertexArrays(1, &mesh.vao);
        glDeleteBuffers(1, &mesh.vbo);
        glDeleteBuffers(1, &mesh.ebo);
    }
    m_Meshes.clear();

    // Clean up shaders
    for (auto &[_, shader] : m_Shaders)
        glDeleteProgram(shader.program);
    m_Shaders.clear();

    // Clean up ...
    for (auto &[_, tex] : m_Textures)
        glDeleteTextures(1, &tex.handle);
    m_Textures.clear();

    for (auto &[_, fb] : m_Framebuffers)
    {
        if (fb.colorAttachment) {
            if (fb.samples == 1 && !fb.bColorIsTexture) {
                glDeleteRenderbuffers(1, &fb.colorAttachment);
            } else {
                glDeleteTextures(1, &fb.colorAttachment);
            }
        }

        if (fb.colorAttachmentMS) {
            if (fb.samples > 1 && fb.bColorIsTexture) glDeleteTextures(1, &fb.colorAttachmentMS);
            else glDeleteRenderbuffers(1, &fb.colorAttachmentMS);
        }

        if (fb.depthStencil) {
            if (fb.bDepthIsTexture && fb.samples == 1) glDeleteTextures(1, &fb.depthStencil);
            else glDeleteRenderbuffers(1, &fb.depthStencil);
        }

        if (fb.fbo) glDeleteFramebuffers(1, &fb.fbo);
    }
    m_Framebuffers.clear();

    std::cout << "[GLBackend]: Shutdown completed" << std::endl;
}

void GLBackend::BeginFrame()
{
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void GLBackend::EndFrame()
{
    //glFlush();
}

RMeshHandle GLBackend::CreateMesh(const RMesh& meshData)
{
    FGLMesh mesh{};
    glGenVertexArrays(1, &mesh.vao);
    glBindVertexArray(mesh.vao);

    glGenBuffers(1, &mesh.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    if (!meshData.vertices.empty())
    {
        glBufferData(GL_ARRAY_BUFFER,
                     meshData.vertices.size() * sizeof(float),
                     meshData.vertices.data(),
                     GL_STATIC_DRAW);
    }

    if (!meshData.indices.empty())
    {
        glGenBuffers(1, &mesh.ebo);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     meshData.indices.size() * sizeof(uint32_t),
                     meshData.indices.data(),
                     GL_STATIC_DRAW);
        mesh.indexCount = static_cast<uint32_t>(meshData.indices.size());
    }
    else
    {
        if (meshData.vertexStride > 0)
            mesh.indexCount = static_cast<uint32_t>((meshData.vertices.size() * sizeof(float)) / meshData.vertexStride);
        else
            mesh.indexCount = static_cast<uint32_t>(meshData.vertices.size() / 3);
    }

    // Attribute layout (0=pos, 1=normal, 2=uv, 3=tangent, etc.)
    const GLsizei stride = (meshData.vertexStride > 0)
        ? static_cast<GLsizei>(meshData.vertexStride)
        : static_cast<GLsizei>(sizeof(float) * 3); // fallback

    // Position
    size_t offset = 0;
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset));
    offset += sizeof(float) * 3;

    if (meshData.bHasNormals)
    {
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset));
        offset += sizeof(float) * 3;
    }

    if (meshData.bHasUVs)
    {
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset));
        offset += sizeof(float) * 2;
    }

    if (meshData.bHasTangents)
    {
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, reinterpret_cast<const void*>(offset));
        offset += sizeof(float) * 3;
    }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    uint32_t id = NextId(m_NextID);
    RMeshHandle handle{ id };
    m_Meshes[handle] = mesh;

    return handle;
}

void GLBackend::DestroyMesh(RMeshHandle handle)
{
    if (auto it = m_Meshes.find(handle); it != m_Meshes.end())
    {
        auto &mesh = it->second;
        if (mesh.vao) glDeleteVertexArrays(1, &mesh.vao);
        if (mesh.vbo) glDeleteBuffers(1, &mesh.vbo);
        if (mesh.ebo) glDeleteBuffers(1, &mesh.ebo);
        m_Meshes.erase(it);
    }
}

GLBackend::FGLMesh* GLBackend::FindMesh(RMeshHandle handle)
{
    if (auto it = m_Meshes.find(handle); it != m_Meshes.end())
        return &it->second;
    return nullptr;
}

RTextureHandle GLBackend::CreateTexture(const RTexture &textureData)
{
    GLuint tex = 0;
    glGenTextures(1, &tex);

    const GLenum target = (textureData.type == ETexType::Tex2D) ? GL_TEXTURE_2D : GL_TEXTURE_CUBE_MAP;
    glBindTexture(target, tex);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    GLint internal = 0; GLenum external = 0;
    if (textureData.bSRGB && (textureData.format == ETexFormat::RGBA8 || textureData.format == ETexFormat::RGB8)) {
        GLFormatFrom(textureData.channels == 4 ? ETexFormat::SRGB8_A8 : ETexFormat::SRGB8, internal, external);
    } else {
        GLFormatFrom(textureData.format, internal, external);
    }

    if (textureData.type == ETexType::Tex2D)
    {
        glTexImage2D(GL_TEXTURE_2D, 0, internal,
                     textureData.width, textureData.height, 0,
                     external, GL_UNSIGNED_BYTE, textureData.data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, textureData.bGenerateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, textureData.bClampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, textureData.bClampToEdge ? GL_CLAMP_TO_EDGE : GL_REPEAT);

        if (textureData.bGenerateMipmaps) glGenerateMipmap(GL_TEXTURE_2D);
    }
    else // CubeMap
    {
        for (int i = 0; i < 6; ++i) {
            const void* face = textureData.faces[i];
            if (face) {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, internal,
                             textureData.width, textureData.height, 0,
                             external, GL_UNSIGNED_BYTE, face);
            }
        }

        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, textureData.bGenerateMipmaps ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        if (textureData.bGenerateMipmaps) glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
    }

    if (textureData.bUseAnisotropy) {
        GLfloat maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        if (maxAniso >= 1.0f)
            glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAniso);
    }

    glBindTexture(target, 0);

    RTextureHandle handle{ NextId(m_NextID) };
    m_Textures[handle] = { tex, target };
    return handle;
}

void GLBackend::DestroyTexture(RTextureHandle handle)
{
    if (auto it = m_Textures.find(handle); it != m_Textures.end()) {
        if (it->second.handle) glDeleteTextures(1, &it->second.handle);
        m_Textures.erase(it);
    }
}

void GLBackend::BindTexture(RTextureHandle texture, uint32_t slot)
{
    auto it = m_Textures.find(texture);
    if (it == m_Textures.end()) return;

    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(it->second.target, it->second.handle);
}

RShaderHandle GLBackend::CreateShader(const RShader &shaderData)
{
    GLuint vs = CompileShaderChecked(GL_VERTEX_SHADER,   shaderData.vertexSource,   "VS");
    GLuint fs = CompileShaderChecked(GL_FRAGMENT_SHADER, shaderData.fragmentSource, "FS");

    GLuint gs = 0;
    if (!shaderData.geometrySource.empty()) {
        gs = CompileShaderChecked(GL_GEOMETRY_SHADER, shaderData.geometrySource, "GS");
    }

    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    if (gs) glAttachShader(prog, gs);

    glLinkProgram(prog);
    CheckProgram(prog);

    glDeleteShader(vs);
    glDeleteShader(fs);
    if (gs) glDeleteShader(gs);

    RShaderHandle h{ NextId(m_NextID) };
    m_Shaders[h] = { prog };
    return h;
}

void GLBackend::DestroyShader(RShaderHandle handle)
{
    if (auto it = m_Shaders.find(handle); it != m_Shaders.end()) {
        if (it->second.program) glDeleteProgram(it->second.program);
        m_Shaders.erase(it);
    }
}

void GLBackend::BindShader(RShaderHandle shader)
{
    auto it = m_Shaders.find(shader);
    glUseProgram(it != m_Shaders.end() ? it->second.program : 0);
}

RFramebufferHandle GLBackend::CreateFramebuffer(const RFramebuffer &framebufferData)
{
   FGLFramebuffer fb{};
    fb.width = framebufferData.width; fb.height = framebufferData.height; fb.samples = std::max(1, framebufferData.samples);
    fb.bColorIsTexture = framebufferData.colorAsTexture;
    fb.bDepthIsTexture = framebufferData.depthAsTexture;

    glGenFramebuffers(1, &fb.fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fb.fbo);

    // Color
    if (fb.samples > 1) {
        // MSAA path
        if (fb.bColorIsTexture) {
            glGenTextures(1, &fb.colorAttachmentMS);
            glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fb.colorAttachmentMS);
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, fb.samples, GL_RGBA8, fb.width, fb.height, GL_TRUE);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, fb.colorAttachmentMS, 0);
        } else {
            GLuint rbo = 0; glGenRenderbuffers(1, &rbo);
            fb.colorAttachmentMS = rbo;
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, fb.samples, GL_RGBA8, fb.width, fb.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
        }
    } else {
        // single-sample
        if (fb.bColorIsTexture) {
            glGenTextures(1, &fb.colorAttachment);
            glBindTexture(GL_TEXTURE_2D, fb.colorAttachment);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, fb.width, fb.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fb.colorAttachment, 0);
        } else {
            GLuint rbo = 0; glGenRenderbuffers(1, &rbo);
            fb.colorAttachment = rbo;
            glBindRenderbuffer(GL_RENDERBUFFER, rbo);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, fb.width, fb.height);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, rbo);
        }
    }

    // Depth-stencil
    if (fb.bDepthIsTexture && fb.samples == 1) {
        glGenTextures(1, &fb.depthStencil);
        glBindTexture(GL_TEXTURE_2D, fb.depthStencil);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, fb.width, fb.height, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, nullptr);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, fb.depthStencil, 0);
    } else {
        glGenRenderbuffers(1, &fb.depthStencil);
        glBindRenderbuffer(GL_RENDERBUFFER, fb.depthStencil);
        if (fb.samples > 1) {
            glRenderbufferStorageMultisample(GL_RENDERBUFFER, fb.samples, GL_DEPTH24_STENCIL8, fb.width, fb.height);
        } else {
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, fb.width, fb.height);
        }
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, fb.depthStencil);
    }

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[GLBackend] FBO incomplete: 0x" << std::hex << status << std::dec << "\n";
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // cleanup
        if (fb.colorAttachment) glDeleteTextures(1, &fb.colorAttachment);
        if (fb.colorAttachmentMS) {
            if (fb.samples>1 && fb.bColorIsTexture) glDeleteTextures(1, &fb.colorAttachmentMS);
            else glDeleteRenderbuffers(1, &fb.colorAttachmentMS);
        }
        if (fb.depthStencil) {
            if (fb.bDepthIsTexture && fb.samples==1) glDeleteTextures(1, &fb.depthStencil);
            else glDeleteRenderbuffers(1, &fb.depthStencil);
        }
        if (fb.fbo) glDeleteFramebuffers(1, &fb.fbo);
        return {};
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    RFramebufferHandle h{ NextId(m_NextID) };
    m_Framebuffers[h] = fb;
    return h;
}

void GLBackend::DestroyFramebuffer(RFramebufferHandle handle)
{
    if (auto it = m_Framebuffers.find(handle); it != m_Framebuffers.end()) {
        auto &fb = it->second;

        if (fb.colorAttachment) {
            if (fb.samples == 1 && !fb.bColorIsTexture) {
                glDeleteRenderbuffers(1, &fb.colorAttachment);
            } else {
                glDeleteTextures(1, &fb.colorAttachment);
            }
        }

        if (fb.colorAttachmentMS) {
            if (fb.samples > 1 && fb.bColorIsTexture) glDeleteTextures(1, &fb.colorAttachmentMS);
            else glDeleteRenderbuffers(1, &fb.colorAttachmentMS);
        }

        if (fb.depthStencil) {
            if (fb.bDepthIsTexture && fb.samples == 1) glDeleteTextures(1, &fb.depthStencil);
            else glDeleteRenderbuffers(1, &fb.depthStencil);
        }

        if (fb.fbo) glDeleteFramebuffers(1, &fb.fbo);
        m_Framebuffers.erase(it);
    }
}

void GLBackend::BindFramebuffer(RFramebufferHandle handle)
{
    auto it = m_Framebuffers.find(handle);
    glBindFramebuffer(GL_FRAMEBUFFER, it != m_Framebuffers.end() ? it->second.fbo : 0);
}

void GLBackend::UnbindFramebuffer()
{
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLBackend::ResolveFramebuffer(RFramebufferHandle src, RFramebufferHandle dst,
                                   EResolveMask mask, EResolveFilter filter)
{
    auto itS = m_Framebuffers.find(src);
    auto itD = m_Framebuffers.find(dst);
    if (itS == m_Framebuffers.end() || itD == m_Framebuffers.end()) return;

    const auto& A = itS->second;
    const auto& B = itD->second;

    glBindFramebuffer(GL_READ_FRAMEBUFFER, A.fbo);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, B.fbo);

    glBlitFramebuffer(
        0, 0, A.width, A.height,
        0, 0, B.width, B.height,
        ToGLMask(mask),
        ToGLFilter(filter)
    );

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GLBackend::SetUniformInt(RShaderHandle sh, const char* name, int v) {
    GLuint p = GetProgramFromHandle(m_Shaders, sh); if (!p) return;
    glUseProgram(p);
    GLint loc = GetUniformLoc(p, name); if (loc < 0) return;
    glUniform1i(loc, v);
}

void GLBackend::SetUniformFloat(RShaderHandle sh, const char* name, float v) {
    GLuint p = GetProgramFromHandle(m_Shaders, sh); if (!p) return;
    glUseProgram(p);
    GLint loc = GetUniformLoc(p, name); if (loc < 0) return;
    glUniform1f(loc, v);
}

void GLBackend::SetUniformVec2(RShaderHandle sh, const char* name, const float* v2) {
    if (!v2) return;
    GLuint p = GetProgramFromHandle(m_Shaders, sh); if (!p) return;
    glUseProgram(p);
    GLint loc = GetUniformLoc(p, name); if (loc < 0) return;
    glUniform2fv(loc, 1, v2);
}

void GLBackend::SetUniformVec3(RShaderHandle sh, const char* name, const float* v3) {
    if (!v3) return;
    GLuint p = GetProgramFromHandle(m_Shaders, sh); if (!p) return;
    glUseProgram(p);
    GLint loc = GetUniformLoc(p, name); if (loc < 0) return;
    glUniform3fv(loc, 1, v3);
}

void GLBackend::SetUniformVec4(RShaderHandle sh, const char* name, const float* v4) {
    if (!v4) return;
    GLuint p = GetProgramFromHandle(m_Shaders, sh); if (!p) return;
    glUseProgram(p);
    GLint loc = GetUniformLoc(p, name); if (loc < 0) return;
    glUniform4fv(loc, 1, v4);
}

void GLBackend::SetUniformMat4(RShaderHandle sh, const char* name, const float* mat4) {
    if (!mat4) return;
    GLuint p = GetProgramFromHandle(m_Shaders, sh); if (!p) return;
    glUseProgram(p);
    GLint loc = GetUniformLoc(p, name); if (loc < 0) return;
    glUniformMatrix4fv(loc, 1, GL_FALSE, mat4);
}

void GLBackend::LinkUniformBlock(RShaderHandle sh, const char* blockName, uint32_t bindingPoint) {
    GLuint p = GetProgramFromHandle(m_Shaders, sh); if (!p || !blockName) return;
    GLuint idx = glGetUniformBlockIndex(p, blockName);
    if (idx != GL_INVALID_INDEX) {
        glUniformBlockBinding(p, idx, bindingPoint);
    }
}

void GLBackend::SubmitMesh(RMeshHandle mesh, RShaderHandle shader, const FMatrix4 &transform)
{
    // Lookup mesh
    auto mit = m_Meshes.find(mesh);
    if (mit == m_Meshes.end()) return;
    const FGLMesh& m = mit->second;
    if (m.indexCount == 0 || m.vao == 0) return;

    // Lookup shader
    auto sit = m_Shaders.find(shader);
    if (sit == m_Shaders.end() || sit->second.program == 0) return;
    GLuint prog = sit->second.program;

    // Bind and set model matrix
    glUseProgram(prog);
    if (GLint loc = glGetUniformLocation(prog, "u_Model"); loc >= 0) {
        glUniformMatrix4fv(loc, 1, GL_FALSE, transform.GetValue());
    }

    // Draw
    glBindVertexArray(m.vao);
    if (m.ebo) {
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m.indexCount), GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m.indexCount));
    }
    glBindVertexArray(0);
}