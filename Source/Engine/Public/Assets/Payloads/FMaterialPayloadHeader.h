//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

struct FMaterialPayloadHeader
{
    uint32_t version;   // e.g. 1
    uint32_t flags;     // bitmask for which slots/fields are used

    uint32_t baseColorTexturePathLength; // bytes, NOT including a null terminator
    uint32_t normalTexturePathLength;
    uint32_t metallicTexturePathLength;
    uint32_t roughnessTexturePathLength;
    uint32_t metalRoughnessTexturePathLength;
    uint32_t occlusionTexturePathLength;
    uint32_t emissiveTexturePathLength;
};

// Flags
enum : uint32_t
{
    MATERIAL_FLAG_HAS_BASE_COLOR_TEXTURE   = 1u << 0,
    MATERIAL_FLAG_HAS_NORMAL_TEXTURE       = 1u << 1,
    MATERIAL_FLAG_HAS_METALLIC_TEXTURE     = 1u << 2,
    MATERIAL_FLAG_HAS_ROUGHNESS_TEXTURE    = 1u << 3,
    MATERIAL_FLAG_HAS_METAL_ROUGHNESS_MAP  = 1u << 4,
    MATERIAL_FLAG_HAS_OCCLUSION_TEXTURE    = 1u << 5,
    MATERIAL_FLAG_HAS_EMISSIVE_TEXTURE     = 1u << 6,
};

struct FMaterialParams
{
    float baseColorFactor[4];   // RGBA
    float metallicFactor;       // default 0.0f
    float roughnessFactor;      // default 1.0f
    float emissiveFactor[3];    // default 0.0f
    float emissiveIntensity;    // default 0.0f

    float normalScale;          // default 1.0f
    float occlusionStrength;    // default 1.0f

    // Tiling (Repeat) values for U and V
    float uvTiling[2];          // default {1.0f, 1.0f}

    // Optionally:
    // float clearCoatFactor;
    // float clearCoatRoughness;
};
