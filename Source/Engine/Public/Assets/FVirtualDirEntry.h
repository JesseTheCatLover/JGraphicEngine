//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstdint>
#include <string>

struct FVirtualDirEntry
{
    enum class EType : uint8_t { Folder, Asset };

    EType type{};
    std::string virtualPath;  // full virtual path
    std::string name;         // leaf name (display name)
};
