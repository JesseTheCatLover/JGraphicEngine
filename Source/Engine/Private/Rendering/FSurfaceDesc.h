//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Rendering/RHandles.h"

struct FSurfaceParams
{
    // Base color multiplier
    float baseColorFactor[4]  = {1.0f, 1.0f, 1.0f, 1.0f};

    // Metallic/Roughness scalars
    float metallicFactor      = 1.0f;   // 0 = dielectric, 1 = fully metallic
    float roughnessFactor     = 1.0f;   // 0 = smooth, 1 = rough

    // Emissive color & intensity
    float emissiveFactor[3]   = {0.0f, 0.0f, 0.0f};
    float emissiveIntensity   = 0.0f;

    // Normal & AO scaling
    float normalScale         = 1.0f;   // normal map intensity
    float occlusionStrength   = 1.0f;   // AO weight (0..1)

    // UV tiling
    float uvTiling[2]         = {1.0f, 1.0f};
};


/**
 * @struct FSurfaceDesc
 * @brief Minimal material surface description required to create an RMaterialHandle.
 */
struct FSurfaceDesc
{
    RTextureHandle baseColor;
    RTextureHandle normal;
    RTextureHandle metallicRoughness;
    RTextureHandle occlusion;
    RTextureHandle emissive;

    FSurfaceParams params;
};
