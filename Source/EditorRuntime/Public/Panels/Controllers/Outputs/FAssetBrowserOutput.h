// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Panels/Controllers/Documents/FAssetBrowserDocument.h"

struct FAssetBrowserOutput
{
    bool bValid = false; // becomes true once built at least once
    FAssetBrowserDocument document; // snapshot for UI rendering
};
