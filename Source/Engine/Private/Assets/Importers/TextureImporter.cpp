//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Assets/Importers/TextureImporter.h"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstring>
#include <vector>

#include <stb/stb_image.h>

#include "Assets/AssetFile.h"
#include "Assets/FAssetHeader.h"
#include "Core/Project/VirtualPathMounter.h"
#include "Utilities/UFileSystem.h"
#include "Utilities/UPath.h"
#include "Utilities/UUUID.h"

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

    static bool IsSupportedImportRoot(const std::string& virtualPath)
    {
        return virtualPath.rfind("/Project", 0) == 0
            || virtualPath.rfind("/Engine", 0) == 0;
    }

    static std::string MakeDestinationVirtualPath(const std::string& destinationVirtualFolder,
                                                  const std::string& sourceFilePath)
    {
        const std::string sourceStem = UPath::GetFileName(sourceFilePath, /*bIncludeExtension*/false);
        return UPath::Join(destinationVirtualFolder, sourceStem + ".jasset");
    }

    static std::vector<uint8_t> BuildTexturePayloadRGBA8(int width,
                                                         int height,
                                                         int channels,
                                                         bool bSRGB,
                                                         const unsigned char* pixelData,
                                                         size_t pixelByteCount)
    {
        struct FTexturePayloadHeader
        {
            uint32_t version = 1;
            uint32_t width = 0;
            uint32_t height = 0;
            uint32_t channels = 0;
            uint8_t  bSRGB = 0;
            uint8_t  reserved[3] = { 0, 0, 0 };
            uint64_t pixelDataSize = 0;
        };

        FTexturePayloadHeader header{};
        header.width = static_cast<uint32_t>(width);
        header.height = static_cast<uint32_t>(height);
        header.channels = static_cast<uint32_t>(channels);
        header.bSRGB = bSRGB ? 1 : 0;
        header.pixelDataSize = static_cast<uint64_t>(pixelByteCount);

        std::vector<uint8_t> bytes(sizeof(FTexturePayloadHeader) + pixelByteCount);
        std::memcpy(bytes.data(), &header, sizeof(FTexturePayloadHeader));

        if (pixelByteCount > 0)
        {
            std::memcpy(bytes.data() + sizeof(FTexturePayloadHeader), pixelData, pixelByteCount);
        }

        return bytes;
    }
}

std::vector<std::string> TextureImporter::GetSupportedSourceExtensions() const
{
    return { "png", "jpg", "jpeg" };
}

bool TextureImporter::Import(const FAssetImportRequest& request,
                             const VirtualPathMounter& pathMounter,
                             FAssetImportResult& outResult) const
{
    outResult = {};

    // ------------------------------------------------------------
    // Validate source
    // ------------------------------------------------------------
    if (request.sourceFilePath.empty())
    {
        outResult.errors.push_back("Source file path is empty.");
        return false;
    }

    if (!UFileSystem::FileExists(request.sourceFilePath))
    {
        outResult.errors.push_back("Source file does not exist: " + request.sourceFilePath);
        return false;
    }

    // ------------------------------------------------------------
    // Validate destination folder
    // ------------------------------------------------------------
    if (request.destinationVirtualFolder.empty())
    {
        outResult.errors.push_back("Destination virtual folder is empty.");
        return false;
    }

    if (!IsSupportedImportRoot(request.destinationVirtualFolder))
    {
        outResult.errors.push_back(
            "Destination virtual folder must be under a supported mounted root such as /Project or /Engine."
        );
        return false;
    }

    const std::string sourceExt = ToLowerCopy(UPath::GetExtension(request.sourceFilePath));
    const auto supported = GetSupportedSourceExtensions();

    bool bSupported = false;
    for (const std::string& ext : supported)
    {
        if (ToLowerCopy(ext) == sourceExt)
        {
            bSupported = true;
            break;
        }
    }

    if (!bSupported)
    {
        outResult.errors.push_back("Unsupported source texture extension: " + sourceExt);
        return false;
    }

    // ------------------------------------------------------------
    // Build destination paths
    // ------------------------------------------------------------
    const std::string destinationVirtualPath =
        MakeDestinationVirtualPath(request.destinationVirtualFolder, request.sourceFilePath);

    std::string destinationPhysicalPath;
    if (!pathMounter.ResolveVirtualToPhysical(destinationVirtualPath, destinationPhysicalPath))
    {
        outResult.errors.push_back(
            "Failed to resolve destination virtual path to a physical path: " + destinationVirtualPath
        );
        return false;
    }

    if (UFileSystem::FileExists(destinationPhysicalPath) && !request.bOverwrite)
    {
        outResult.errors.push_back(
            "Destination asset already exists and overwrite is disabled: " + destinationVirtualPath
        );
        return false;
    }

    if (!UFileSystem::CreateDirectory(UPath::GetParent(destinationPhysicalPath)))
    {
        outResult.errors.push_back("Failed to create destination folder: " + UPath::GetParent(destinationPhysicalPath));
        return false;
    }

    // ------------------------------------------------------------
    // Load source image
    // ------------------------------------------------------------
    stbi_set_flip_vertically_on_load(1);

    int width = 0;
    int height = 0;
    int sourceChannels = 0;

    unsigned char* pixels = stbi_load(
        request.sourceFilePath.c_str(),
        &width,
        &height,
        &sourceChannels,
        4 // force RGBA8
    );

    if (!pixels)
    {
        outResult.errors.push_back("Failed to decode source texture image: " + request.sourceFilePath);
        return false;
    }

    if (width <= 0 || height <= 0)
    {
        stbi_image_free(pixels);
        outResult.errors.push_back("Decoded texture has invalid dimensions.");
        return false;
    }

    const size_t pixelByteCount =
        static_cast<size_t>(width) *
        static_cast<size_t>(height) * 4u;

    const bool bSRGB = true; // good default for imported color textures in v1

    std::vector<uint8_t> payloadBytes =
        BuildTexturePayloadRGBA8(width, height, 4, bSRGB, pixels, pixelByteCount);

    stbi_image_free(pixels);
    pixels = nullptr;

    // ------------------------------------------------------------
    // Build asset header
    // ------------------------------------------------------------
    FAssetHeader header{};
    header.assetID = UUUID::GenerateUUID();
    header.assetName = UPath::GetFileName(request.sourceFilePath, /*bIncludeExtension*/false);
    header.assetType = EAssetType::Texture2D;
    header.encoding = EAssetEncoding::Binary;
    header.containerVersion = FAssetHeader::CurrentContainerVersion;
    header.payloadVersion = 1;
    header.sourcePath = UPath::Normalize(request.sourceFilePath); // optional metadata only
    header.importerName = GetImporterName();
    header.dependencyAssetIDs.clear();

    // ------------------------------------------------------------
    // Write .jasset
    // ------------------------------------------------------------
    if (!AssetFile::WriteBinaryAsset(destinationPhysicalPath, header, payloadBytes))
    {
        outResult.errors.push_back("Failed to write imported texture asset file: " + destinationPhysicalPath);
        return false;
    }

    // ------------------------------------------------------------
    // Fill result
    // ------------------------------------------------------------
    FImportedAssetInfo created{};
    created.assetID = header.assetID;
    created.assetType = EAssetType::Texture2D;
    created.virtualPath = destinationVirtualPath;
    created.physicalPath = destinationPhysicalPath;

    outResult.createdAssets.push_back(std::move(created));
    outResult.bSuccess = true;
    return true;
}