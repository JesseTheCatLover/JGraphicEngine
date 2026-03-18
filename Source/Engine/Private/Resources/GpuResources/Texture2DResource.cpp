//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Texture2DResource.h"

#include <iostream>
#include <fstream>

#include "Assets/AssetFile.h"
#include "Assets/AssetRegistrySubsystem.h"
#include "Assets/Payloads/FTexturePayloadHeader.h"
#include "Rendering/IRenderDevice.h"
#include "Utilities/UFileSystem.h"

Texture2DResource::Texture2DResource(FDesc desc, AssetRegistrySubsystem* assetRegistry)
    : m_Desc(std::move(desc)),
    m_AssetRegistry(assetRegistry)
{
}

void Texture2DResource::OnCreateGpuResources()
{
    if (!m_CpuReady)
        LoadCPU();

    if (!m_CpuReady)
        return;

    UploadGPU();
    ReleaseCPU();
}

void Texture2DResource::OnDestroyGpuResources()
{
    IRenderDevice* dev = GetDevice();
    if (!dev)
        return;

    if (m_Texture.IsValid())
        dev->DestroyTexture(m_Texture);

    m_Texture = {};
}

void Texture2DResource::LoadCPU()
{
    m_CpuReady = false;
    m_Pixels.clear();

    // ------------------------------------------------------------
    // Resolve asset record from registry
    // ------------------------------------------------------------
    if (!m_AssetRegistry)
    {
        std::cerr << "[Texture2DResource]: Asset registry is null" << std::endl;
        return;
    }

    const FAssetRecord* record = m_AssetRegistry->FindByAssetID(m_Desc.assetId);
    if (!record)
    {
        std::cerr << "[Texture2DResource]: Asset not found in registry. ID: " << m_Desc.assetId << std::endl;
        return;
    }

    const std::string& assetPath = record->physicalPath;
    if (assetPath.empty())
    {
        std::cerr << "[Texture2DResource]: Asset record has empty physical path. ID: "
                  << m_Desc.assetId << std::endl;
        return;
    }

    if (!UFileSystem::FileExists(assetPath))
    {
        std::cerr << "[Texture2DResource]: Asset file does not exist: "
                  << assetPath << std::endl;
        return;
    }

    // ------------------------------------------------------------
    // Read asset container
    // ------------------------------------------------------------
    FAssetHeader header{};
    std::vector<uint8_t> payload;

    if (!AssetFile::ReadBinaryAsset(assetPath, header, payload))
    {
        std::cerr << "[Texture2DResource]: Failed to read binary asset: " << assetPath << std::endl;
        return;
    }

    if (payload.size() < sizeof(FTexturePayloadHeader))
    {
        std::cerr << "[Texture2DResource]: Texture payload too small: " << assetPath << std::endl;
        return;
    }

    // ------------------------------------------------------------
    // Parse texture payload
    // ------------------------------------------------------------
    const FTexturePayloadHeader* texHeader =
        reinterpret_cast<const FTexturePayloadHeader*>(payload.data());

    const size_t expectedSize =
        sizeof(FTexturePayloadHeader) +
        static_cast<size_t>(texHeader->pixelDataSize);

    if (payload.size() < expectedSize)
    {
        std::cerr << "[Texture2DResource]: Texture payload truncated: " << assetPath << std::endl;
        return;
    }

    m_W = static_cast<int>(texHeader->width);
    m_H = static_cast<int>(texHeader->height);

    const unsigned char* pixelData =
        payload.data() + sizeof(FTexturePayloadHeader);

    m_Pixels.assign(pixelData, pixelData + texHeader->pixelDataSize);
    m_CpuReady = true;
}

void Texture2DResource::UploadGPU()
{
    IRenderDevice* dev = GetDevice();
    if (!dev)
    {
        std::cerr << "[Texture2DResource]: UploadGPU failed: device is null.\n";
        return;
    }

    if (m_W <= 0 || m_H <= 0 || m_Pixels.empty())
    {
        std::cerr << "[Texture2DResource]: UploadGPU failed: invalid CPU staging.\n";
        return;
    }

    RTexture ro{};
    ro.type = ETexType::Tex2D;
    ro.width = m_W;
    ro.height = m_H;
    ro.channels = 4;
    ro.data = m_Pixels.data();
    ro.format = m_Desc.bSRGB ? ETexFormat::SRGB8_A8 : ETexFormat::RGBA8;
    ro.bGenerateMipmaps = m_Desc.bGenerateMipmaps;

    ro.wrapS = m_Desc.wrapS;
    ro.wrapT = m_Desc.wrapT;
    ro.minFilter = m_Desc.minFilter;
    ro.magFilter = m_Desc.magFilter;

    ro.bSRGB = m_Desc.bSRGB;

    if (m_Texture.IsValid())
        dev->DestroyTexture(m_Texture);

    m_Texture = dev->CreateTexture(ro);

    if (!m_Texture.IsValid())
        std::cerr << "[Texture2DResource]: CreateTexture failed for asset: "
                  << m_Desc.assetId << "\n";
}

void Texture2DResource::ReleaseCPU()
{
    m_Pixels.clear();
    m_Pixels.shrink_to_fit();
    m_W = 0;
    m_H = 0;
    m_CpuReady = false;
}
