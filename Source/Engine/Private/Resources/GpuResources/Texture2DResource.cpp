//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Texture2DResource.h"

#include <iostream>

#include "Assets/AssetFile.h"
#include "Assets/Payloads/FTexturePayloadHeader.h"
#include "Rendering/IRenderDevice.h"

Texture2DResource::Texture2DResource(FDesc desc, AssetRegistrySubsystem* registry)
    : GpuResource(registry),
      m_Desc(std::move(desc))
{
}

bool Texture2DResource::OnCreateGpuResources()
{
    if (!m_CpuReady)
        LoadCPU();

    if (!m_CpuReady)
        return false;

    if (!UploadGPU())
        return false;

    ReleaseCPU();
    return true;
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

    FAssetHeader header{};
    std::vector<uint8_t> payload;

    if (!LoadAssetBinary(m_Desc.assetID, header, payload))
        return;

    if (payload.size() < sizeof(FTexturePayloadHeader))
    {
        std::cerr << "[Texture2DResource] Payload too small\n";
        return;
    }

    const auto* texHeader =
        reinterpret_cast<const FTexturePayloadHeader*>(payload.data());

    const size_t expected =
        sizeof(FTexturePayloadHeader) + texHeader->pixelDataSize;

    if (payload.size() < expected)
    {
        std::cerr << "[Texture2DResource]: Payload truncated\n";
        return;
    }

    m_W = texHeader->width;
    m_H = texHeader->height;

    const unsigned char* pixelData =
        payload.data() + sizeof(FTexturePayloadHeader);

    m_Pixels.assign(pixelData, pixelData + texHeader->pixelDataSize);

    m_CpuReady = true;
}

bool Texture2DResource::UploadGPU()
{
    IRenderDevice* dev = GetDevice();
    if (!dev)
    {
        std::cerr << "[Texture2DResource]: UploadGPU failed: device null\n";
        return false;
    }

    if (m_W <= 0 || m_H <= 0 || m_Pixels.empty())
    {
        std::cerr << "[Texture2DResource]: UploadGPU failed: invalid CPU data\n";
        return false;
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
    {
        std::cerr << "[Texture2DResource]: Texture creation failed for asset " << m_Desc.assetID << "\n";
        return false;
    }

    return true;
}

void Texture2DResource::ReleaseCPU()
{
    m_Pixels.clear();
    m_Pixels.shrink_to_fit();

    m_W = 0;
    m_H = 0;
    m_CpuReady = false;
}
