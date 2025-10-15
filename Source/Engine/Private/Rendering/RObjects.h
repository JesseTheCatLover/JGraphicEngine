//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstdint>
#include <vector>

using Rint = uint32_t;

struct RMesh
{
    std::vector<float> vertices;
    std::vector<uint32_t> indices;
    Rint gpuVertexBuffer = 0;
    Rint gpuIndexBuffer = 0;
};

struct RTexture
{
    Rint width = 0;
    Rint height = 0;
    Rint format = 0;
    void* gpuPtr = nullptr;
};

struct RShader
{

};

struct RFramebuffer
{

};

struct RMaterial
{
};