//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string_view>

#include "Assets/FAssetHeader.h"
#include "FAssetImportRequest.h"
#include "FAssetImportResult.h"

class VirtualPathMounter;

class IAssetImporter
{
public:
    virtual ~IAssetImporter() = default;

    virtual const char* GetImporterName() const = 0;
    virtual bool CanImportExtension(std::string_view extension) const = 0;
    virtual EAssetType GetOutputAssetType() const = 0;

    virtual bool Import(const FAssetImportRequest& request,
                        const VirtualPathMounter& pathMounter,
                        FAssetImportResult& outResult) const = 0;
};