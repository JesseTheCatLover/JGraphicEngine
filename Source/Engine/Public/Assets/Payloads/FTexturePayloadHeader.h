//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>

struct FTexturePayloadHeader
{
    uint32_t version = 1;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t channels = 0;
    uint8_t  bSRGB = 0;
    uint8_t  reserved[3]{};
    uint64_t pixelDataSize = 0;
};
