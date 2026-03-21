// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetImportSubsystem.h"

#include <algorithm>
#include <cctype>

#include "Assets/Importers/IAssetImporter.h"
#include "Assets/Importers/StaticMeshImporter.h"
#include "Assets/Importers/TextureImporter.h"
#include "Utilities/UFileSystem.h"
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

void AssetImportSubsystem::RegisterEssentialImporters()
{
    RegisterImporter(MakeUnique<TextureImporter>());
    RegisterImporter(MakeUnique<StaticMeshImporter>());
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

void AssetImportSubsystem::Shutdown()
{
    ClearImporters();
}

bool AssetImportSubsystem::Import(const FAssetImportRequest& request,
                                  const VirtualPathMounter& pathMounter,
                                  FAssetImportResult& outResult) const
{
    outResult = {};

    if (!UFileSystem::FileExists(request.sourceFilePath))
    {
        outResult.errors.push_back("Source file does not exist: " + request.sourceFilePath);
        return false;
    }

    if (request.sourceFilePath.empty())
    {
        outResult.errors.emplace_back("Source file path is empty.");
        return false;
    }

    if (request.destinationVirtualFolder.empty())
    {
        outResult.errors.emplace_back("Destination virtual folder is empty.");
        return false;
    }

    std::string extension = UPath::GetExtension(request.sourceFilePath);

    if (extension.empty())
    {
        outResult.errors.emplace_back("Source file has no extension.");
        return false;
    }

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

        const std::vector<std::string> supported = importer->GetSupportedSourceExtensions();
        for (const std::string& ext : supported)
        {
            if (ToLowerCopy(ext) == normalizedExtension)
                return importer.get();
        }
    }

    return nullptr;
}