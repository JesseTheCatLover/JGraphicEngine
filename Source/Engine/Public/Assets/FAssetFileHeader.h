//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <cstring>

#pragma pack(push, 1) // Ensure no padding
struct FAssetFileHeader
{
    char magic[4] = {'J', 'A', 'S', 'T'}; // 'JAST' as a byte array
    uint32_t containerVersion = 1;        // container format version

    uint64_t headerByteSize = 0;          // JSON header length
    uint64_t payloadByteSize = 0;         // payload length
};
#pragma pack(pop)

// Assert the size to catch accidental padding issues
static_assert(sizeof(FAssetFileHeader) == 24, "FAssetFileHeader size mismatch due to padding!");
