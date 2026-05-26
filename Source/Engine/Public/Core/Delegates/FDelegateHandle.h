//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <cstddef>

struct FDelegateHandle
{
    std::size_t id = 0;

    constexpr bool IsValid() const { return id != 0; }
    constexpr void Reset() { id = 0; }

    friend constexpr bool operator==(FDelegateHandle a, FDelegateHandle b) { return a.id == b.id; }
    friend constexpr bool operator!=(FDelegateHandle a, FDelegateHandle b) { return a.id != b.id; }
};
