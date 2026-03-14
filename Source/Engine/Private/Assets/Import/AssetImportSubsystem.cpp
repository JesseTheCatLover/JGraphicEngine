// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Assets/Import/AssetImportSubsystem.h"

#include <algorithm>
#include <cctype>

#include "Assets/Import/IAssetImporter.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Utilities/UPath.h"

namespace
{
    static std::string ToLowerCopy(std::string s)
    {
        std::transform(s.begin(), s.end(), s.begin(),
            [](unsigned char c)
            {
                return static_cast<char>(std::tolower(c));
            });
        return s;
    }
}

void AssetImportSubsystem::RegisterImporter(TUniquePtr<IAssetImporter> importer)
{
    if (!importer)
        return;

    m_Importers.push_back(std::move(importer));
}

void AssetImportSubsystem::ClearImporters()
{
    m_Importers.clear();
}

bool AssetImportSubsystem::Import(const FAssetImportRequest& request,
                                  const VirtualPathMounter& pathMounter,
                                  FAssetImportResult& outResult) const
{
    outResult = {};

    if (request.sourceFilePath.empty())
    {
        outResult.errors.push_back("Source file path is empty.");
        return false;
    }

    if (request.destinationVirtualFolder.empty())
    {
        outResult.errors.push_back("Destination virtual folder is empty.");
        return false;
    }

    std::string extension = UPath::GetExtension(request.sourceFilePath);
    extension = ToLowerCopy(extension);

    const IAssetImporter* importer = FindImporterForExtension(extension);
    if (!importer)
    {
        outResult.errors.push_back(
            "No registered importer supports extension: " + extension);
        return false;
    }

    const bool bSuccess = importer->Import(request, pathMounter, outResult);
    outResult.bSuccess = bSuccess;
    return bSuccess;
}

const IAssetImporter* AssetImportSubsystem::FindImporterForExtension(const std::string& extension) const
{
    if (extension.empty())
        return nullptr;

    const std::string normalizedExtension = ToLowerCopy(extension);

    for (const TUniquePtr<IAssetImporter>& importer : m_Importers)
    {
        if (!importer)
            continue;

        if (importer->CanImportExtension(normalizedExtension))
            return importer.get();
    }

    return nullptr;
}