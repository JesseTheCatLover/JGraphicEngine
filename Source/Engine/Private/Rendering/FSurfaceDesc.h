// Copyright 2025 JesseTheCatLover

#pragma once
#include "Rendering/RHandles.h"

struct FSurfaceParams
{
    float baseColorFactor[4]  = {1,1,1,1}; // if no texture
    float emissiveFactor[3]   = {0,0,0};
    float metallicFactor      = 1.0f;
    float roughnessFactor     = 1.0f;
    float uvTiling[2]         = {1.0f, 1.0f};
};

/**
 * @struct FSurfaceDesc
 * @brief Minimal material surface description required to create an RMaterialHandle.
 *
 * Keep this tiny. We can expand to PBR later (normal, metallicRoughness, AO, emissive).
 */
struct FSurfaceDesc
{
    RTextureHandle baseColor;
    RTextureHandle normal;
    RTextureHandle metallicRoughness;
    RTextureHandle emissive;

    FSurfaceParams params;
};
