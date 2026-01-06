// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <functional>

using Rint = uint32_t;

struct RMeshHandle
{
    Rint id = 0;
    [[nodiscard]] bool IsValid() const { return id != 0; }
    static RMeshHandle Invalid() { return {0}; }
};

struct RTextureHandle
{
    Rint id = 0;
    [[nodiscard]] bool IsValid() const { return id != 0; }
    static RTextureHandle Invalid() { return {0}; }
};

struct RShaderHandle
{
    Rint id = 0;
    [[nodiscard]] bool IsValid() const { return id != 0; }
    static RShaderHandle Invalid() { return {0}; }
};

struct RMaterialHandle
{
    Rint id = 0;
    [[nodiscard]] bool IsValid() const { return id != 0; }
    static RMaterialHandle Invalid() { return {0}; }
};

struct RFramebufferHandle
{
    Rint id = 0;
    [[nodiscard]] bool IsValid() const { return id != 0; }
    static RFramebufferHandle Invalid() { return {0}; }
};

inline bool operator==(RMeshHandle a, RMeshHandle b)             noexcept { return a.id == b.id; }
inline bool operator==(RShaderHandle a, RShaderHandle b)         noexcept { return a.id == b.id; }
inline bool operator==(RTextureHandle a, RTextureHandle b)       noexcept { return a.id == b.id; }
inline bool operator==(RMaterialHandle a, RMaterialHandle b)       noexcept { return a.id == b.id; }
inline bool operator==(RFramebufferHandle a, RFramebufferHandle b) noexcept { return a.id == b.id; }

inline bool operator!=(RMeshHandle a, RMeshHandle b)             noexcept { return !(a == b); }
inline bool operator!=(RShaderHandle a, RShaderHandle b)         noexcept { return !(a == b); }
inline bool operator!=(RTextureHandle a, RTextureHandle b)       noexcept { return !(a == b); }
inline bool operator!=(RFramebufferHandle a, RFramebufferHandle b) noexcept { return !(a == b); }

namespace std
{
    template<> struct hash<RMeshHandle> { size_t operator()(const RMeshHandle& h) const noexcept { return hash<Rint>{}(h.id); } };
    template<> struct hash<RShaderHandle> { size_t operator()(const RShaderHandle& h) const noexcept { return hash<Rint>{}(h.id); } };
    template<> struct hash<RMaterialHandle> { size_t operator()(const RMaterialHandle& h) const noexcept { return hash<Rint>{}(h.id); } };
    template<> struct hash<RTextureHandle>{ size_t operator()(const RTextureHandle& h) const noexcept { return hash<Rint>{}(h.id); } };
    template<> struct hash<RFramebufferHandle>{ size_t operator()(const RFramebufferHandle& h) const noexcept { return hash<Rint>{}(h.id); } };
}