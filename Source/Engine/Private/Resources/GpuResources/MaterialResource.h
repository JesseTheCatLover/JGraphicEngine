//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>

#include "Rendering/FSurfaceDesc.h"
#include "Resources/GpuResource.h"
#include "Rendering/RHandles.h"

class ResourceSubsystem;
class AssetRegistrySubsystem;

/**
 * @class MaterialResource
 * @brief GPU-backed material resource whose GPU handle is managed by RendererSubsystem.
 *
 * This class bridges high-level material asset data (textures, parameters)
 * with the RendererSubsystem's material creation system that uses FSurfaceDesc.
 */
class MaterialResource final : public GpuResource
{
public:

    /** Descriptor passed during creation, typically from the asset manager. */
    struct FDesc
    {
        std::string assetID;   // e.g., GUID or path within the AssetRegistry
    };

private:
    FDesc                   m_Desc;
    AssetRegistrySubsystem* m_Registry  = nullptr;
    ResourceSubsystem*      m_Resources = nullptr;

    // CPU-side data
    FSurfaceDesc m_Surface{};
    bool         m_CpuReady = false;

    // GPU-side handle managed by RendererSubsystem
    RMaterialHandle m_Handle{};

public:
    explicit MaterialResource(FDesc desc, ResourceSubsystem* resources,
        AssetRegistrySubsystem* registry);

    bool OnCreateGpuResources() override;
    void OnDestroyGpuResources() override;

    /** Exposes the live GPU material handle used by the renderer. */
    [[nodiscard]] RMaterialHandle GetHandle() const { return m_Handle; }

    /** Accessor for CPU-side surface descriptor. */
    [[nodiscard]] const FSurfaceDesc& GetSurfaceDesc() const { return m_Surface; }

private:
    // Loading and staging steps
    void LoadCPU();
    void UploadGPU(IRenderDevice* device);
    void ReleaseCPU();

    RTextureHandle GetTextureHandleForAssetID(const std::string& assetID);
};
