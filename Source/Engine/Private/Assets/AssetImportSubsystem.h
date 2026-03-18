// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <string>

#include "Assets/FAssetImportRequest.h"
#include "../../Public/Assets/FAssetImportResult.h"
#include "Core/Memory/SmartPointers.h"

class IAssetImporter;
class VirtualPathMounter;

class AssetImportSubsystem
{
public:
    AssetImportSubsystem() = default;
    ~AssetImportSubsystem() = default;

    void RegisterImporter(TUniquePtr<IAssetImporter> importer);
    void ClearImporters();

    bool Import(const FAssetImportRequest& request,
                const VirtualPathMounter& pathMounter,
                FAssetImportResult& outResult) const;

private:
    const IAssetImporter* FindImporterForExtension(const std::string& extension) const;

private:
    std::vector<TUniquePtr<IAssetImporter>> m_Importers;
};