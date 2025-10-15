// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
using Rint = uint32_t;

struct RMeshHandle
{
    Rint id = 0;
    bool IsValid() const { return id != 0; }
    static RMeshHandle Invalid() { return {0}; }
};
struct RTextureHandle
{
    Rint id = 0;
    bool IsValid() const { return id != 0; }
    static RTextureHandle Invalid() { return {0}; }
};
struct RShaderHandle
{
    Rint id = 0;
    bool IsValid() const { return id != 0; }
    static RShaderHandle Invalid() { return {0}; }
};
struct RFramebufferHandle
{
    Rint id = 0;
    bool IsValid() const { return id != 0; }
    static RFramebufferHandle Invalid() { return {0}; }
};
