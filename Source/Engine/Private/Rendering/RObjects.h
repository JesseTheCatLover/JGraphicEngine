//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

using Rint = uint32_t;

struct RMesh
{
    Rint vertexBuffer = 0;
    Rint indexBuffer = 0;
    Rint vertexCount = 0;
    Rint indexCount = 0;
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