//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>

#include "Resources/GpuResource.h"
#include "Rendering/RHandles.h"
#include "Rendering/RObjects.h"

class AssetRegistrySubsystem;

class StaticMeshResource : public GpuResource
{
public:

    struct FSubmeshGPU
    {
        RMeshHandle mesh{};
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        uint32_t materialSlotIndex = 0; // index into CPU material slots
    };

    struct FDesc
    {
        std::string assetID;
    };

public:
    explicit StaticMeshResource(FDesc desc, AssetRegistrySubsystem* registry);

    bool OnCreateGpuResources() override;
    void OnDestroyGpuResources() override;

    [[nodiscard]] const std::vector<FSubmeshGPU>& GetSubmeshes() const { return m_SubmeshesGPU; }
    [[nodiscard]] const std::string& GetAssetID() const { return m_Desc.assetID; }

    // Expose material slot info for higher-level systems (MaterialResource, renderer, etc.)
    struct FMaterialSlotCPU
    {
        std::string name;
        std::string materialAssetID;
    };

    struct FSubmeshCPU
    {
        uint32_t firstIndex        = 0;
        uint32_t indexCount        = 0;
        uint32_t materialSlotIndex = 0;
    };

    [[nodiscard]] const std::vector<FMaterialSlotCPU>& GetMaterialSlotsCPU() const { return m_MeshCPU.MaterialSlots; }
    [[nodiscard]] const std::vector<FSubmeshCPU>& GetSubmeshesCPU() const { return m_MeshCPU.Submeshes;   }

private:
    struct FMeshCPU
    {
        uint32_t VertexCount  = 0;
        uint32_t IndexCount   = 0;
        uint32_t VertexStride = 0;

        bool bHasNormals  = false;
        bool bHasTangents = false;
        bool bHasUVs      = false;

        // Raw shared buffers as in the payload
        std::vector<uint8_t> VertexBuffer;
        std::vector<uint8_t> IndexBuffer;

        std::vector<FSubmeshCPU>      Submeshes;
        std::vector<FMaterialSlotCPU> MaterialSlots;
    };

    void LoadCPU();
    void UploadGPU();
    void ReleaseCPU();

private:
    FDesc m_Desc;

    AssetRegistrySubsystem* m_Registry = nullptr;

    // CPU staging
    FMeshCPU m_MeshCPU;
    bool     m_CpuReady = false;

    // GPU objects
    std::vector<FSubmeshGPU> m_SubmeshesGPU;
};
