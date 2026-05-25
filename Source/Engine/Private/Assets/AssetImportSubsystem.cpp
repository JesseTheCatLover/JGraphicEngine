// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "AssetImportSubsystem.h"

#include <algorithm>
#include <cctype>

#include "Assets/Importers/IAssetImporter.h"
#include "Assets/Importers/ModelImporter.h"
#include "Assets/Importers/TextureImporter.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"

namespace
{
    // Helper to lower-case strings safely
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
    RegisterImporter(MakeUnique<ModelImporter>());
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
    // Reset output
    outResult = {};

    // 1. Memory/String validations first
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

    // 2. Disk IO validations second (Prevents crashing OS functions with empty paths)
    if (!UFileSystem::FileExists(request.sourceFilePath))
    {
        outResult.errors.push_back("Source file does not exist: " + request.sourceFilePath);
        return false;
    }

    // 3. Extract and normalize extension
    std::string extension = UPath::GetExtension(request.sourceFilePath);
    if (extension.empty())
    {
        outResult.errors.emplace_back("Source file has no extension.");
        return false;
    }

    extension = ToLowerCopy(extension);

    // 4. Find importer and delegate
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

    const std::string normalizedRequestExt = ToLowerCopy(extension);

    for (const TUniquePtr<IAssetImporter>& importer : m_Importers)
    {
        if (!importer)
            continue;

        // Note: GetSupportedSourceExtensions() returns by value.
        // We bind to const auto& to extend lifetime without unnecessary copies
        const auto& supportedExtensions = importer->GetSupportedSourceExtensions();

        for (const std::string& ext : supportedExtensions)
        {
            if (ToLowerCopy(ext) == normalizedRequestExt)
            {
                return importer.get();
            }
        }
    }

    return nullptr;
}
