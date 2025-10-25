//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <vector>

using Rint = uint32_t;

enum class ETexType { Tex2D, CubeMap };
enum class ETexFormat { RGBA8, RGB8, SRGB8_A8, SRGB8, R8, RG16F, RGBA16F /* etc. */ };

enum class ETexWrap { Repeat, ClampEdge };
enum class ETexFilter { Linear, LinearMipmapLinear, Nearest };

struct FVertexAttribute
{
    Rint location;
    Rint offset; // in bytes
    Rint size;   // components (e.g., 3 for vec3)
    Rint type;
    bool normalized = false;
};

struct RMesh
{
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    Rint vertexStride;
    std::vector<FVertexAttribute> attributes;
    Rint gpuVertexBuffer;
    Rint gpuIndexBuffer;
    bool bHasNormals = false;
    bool bHasUVs = false;
    bool bHasTangents = false;
    bool bHasBones = false;
};

struct RTexture
{
    ETexType   type = ETexType::Tex2D;
    ETexFormat format = ETexFormat::RGBA8;
    int        width = 0, height = 0, channels = 4;
    bool       bGenerateMipmaps = true;
    bool       bSRGB = false;             // convenience hint
    bool       bClampToEdge = false;
    bool       bUseAnisotropy = true;
    int        bMaxLevel = 0;             // 0 = let backend decide
    // For 2D:
    const void* data = nullptr;
    // For cubemap: six face pointers (posx, negx, posy, negy, posz, negz)
    const void* faces[6] = {nullptr,nullptr,nullptr,nullptr,nullptr,nullptr};

    ETexWrap wrapS = ETexWrap::Repeat, wrapT = ETexWrap::Repeat, wrapR = ETexWrap::Repeat;
    ETexFilter minFilter = ETexFilter::LinearMipmapLinear;
    ETexFilter magFilter = ETexFilter::Linear;
};

struct RShader
{
    std::string vertexSource;
    std::string fragmentSource;
    std::string geometrySource;
};

struct RFramebuffer
{
    int width = 0, height = 0;
    int samples = 1;                 // >1 => MSAA
    bool colorAsTexture = true;      // false => renderbuffer for color (MSAA recommended)
    bool depthAsTexture = false;     // true if you need to sample depth
};

struct RMaterial
{
};