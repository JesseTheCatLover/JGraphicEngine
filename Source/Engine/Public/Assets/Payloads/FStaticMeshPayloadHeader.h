// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.
#pragma once
#include <cstdint>

struct FStaticMeshPayloadHeader
{
    uint32_t version = 1;

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;
    uint32_t subMeshCount = 0;
    uint32_t materialSlotCount = 0;

    uint32_t vertexStride = 0;

    uint8_t  bHasNormals = 0;
    uint8_t  bHasTangents = 0;
    uint8_t  bHasUVs = 0;
    uint8_t  reserved = 0;

    uint64_t vertexBufferSize = 0;
    uint64_t indexBufferSize  = 0;
    uint64_t subMeshTableSize = 0;
};

struct FModelSubMesh
{
    uint32_t firstIndex = 0;
    uint32_t indexCount = 0;
    uint32_t materialIndex = 0;
};

struct FMaterialSlot
{
    uint32_t nameLength = 0;
    uint32_t baseColorTextureLength = 0;
};