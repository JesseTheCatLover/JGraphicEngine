//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "FObjectInitializer.h"

struct FObjectInitTLS
{
    static const FObjectInitializer* Get();
    static void Push(const FObjectInitializer& init);
    static void Pop();

    // RAII helper
    struct FScope
    {
        explicit FScope(const FObjectInitializer& init) { Push(init); }
        ~FScope() { Pop(); }

        FScope(const FScope&) = delete;
        FScope& operator=(const FScope&) = delete;
    };
};