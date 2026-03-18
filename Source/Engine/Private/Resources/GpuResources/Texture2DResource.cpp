//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Texture2DResource.h"

#include <iostream>
#include <fstream>

#include "Assets/AssetRegistrySubsystem.h"
#include "Rendering/IRenderDevice.h"

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
    m_Pixels.clear();
    m_W = 0;
    m_H = 0;

    if (!m_AssetRegistry)
    {
        std::cerr << "[Texture2DResource]: AssetRegistrySubsystem null\n";
        return;
    }

    const FAssetRecord* record =
        m_AssetRegistry->FindByAssetID(m_Desc.assetId);

    if (!record)
    {
        std::cerr << "[Texture2DResource]: Asset not found: "
                  << m_Desc.assetId << "\n";
        return;
    }

    const std::string& path = record->physicalPath;
    std::ifstream file(path, std::ios::binary);
    if (!file)
    {
        std::cerr << "[Texture2DResource]: Failed to open .jasset file: " << path << "\n";
        return;
    }

    struct FTexHeader
    {
        uint32_t magic;
        uint32_t width;
        uint32_t height;
        uint32_t format; // RGBA8, etc.
    } header{};

    file.read(reinterpret_cast<char*>(&header), sizeof(header));

    constexpr uint32_t Magic = 0x4A415354; // 'JAST'

    if (header.magic != Magic)
    {
        std::cerr << "[Texture2DResource]: Invalid .jasset magic header in " << path << "\n";
        return;
    }

    m_W = static_cast<int>(header.width);
    m_H = static_cast<int>(header.height);

    const size_t dataSize = static_cast<size_t>(m_W) * static_cast<size_t>(m_H) * 4;
    m_Pixels.resize(dataSize);

    file.read(reinterpret_cast<char*>(m_Pixels.data()), dataSize);
    if (!file)
    {
        std::cerr << "[Texture2DResource]: Failed reading pixel payload\n";
        m_Pixels.clear();
        return;
    }

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
