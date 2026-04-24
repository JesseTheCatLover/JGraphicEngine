//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "StaticMeshResource.h"
#include <cstring>

#include "Assets/Payloads/FStaticMeshPayloadHeader.h"
#include "Rendering/IRenderDevice.h"

// ---------------------------------------------------------------
// Constructor
// ---------------------------------------------------------------
StaticMeshResource::StaticMeshResource(FDesc desc, AssetRegistrySubsystem* registry)
    : GpuResource(registry)
    , m_Desc(std::move(desc))
    , m_Registry(registry)
{
}

// ---------------------------------------------------------------
// GPU Lifecycle
// ---------------------------------------------------------------
bool StaticMeshResource::OnCreateGpuResources()
{
    if (!m_CpuReady)
        LoadCPU();

    if (!m_CpuReady)
        return false;

    UploadGPU();
    // ReleaseCPU();
    return true;
}

void StaticMeshResource::OnDestroyGpuResources()
{
    if (IRenderDevice* dev = GetDevice())
    {
        for (auto& sm : m_SubmeshesGPU)
        {
            if (sm.mesh.IsValid())
                dev->DestroyMesh(sm.mesh);
        }
    }

    m_SubmeshesGPU.clear();
}

// ---------------------------------------------------------------
// CPU Loading
// ---------------------------------------------------------------
void StaticMeshResource::LoadCPU()
{
    FAssetHeader header{};
    std::vector<uint8_t> payload;

    // Load binary
    if (!LoadAssetBinary(m_Desc.assetID, header, payload))
        return;

    if (payload.size() < sizeof(FStaticMeshPayloadHeader))
        return;

    const uint8_t* data = payload.data();
    size_t offset = 0;

    auto ensure = [&](uint64_t needed) -> bool
    {
        return offset + needed <= payload.size();
    };

    auto readRaw = [&](void* dst, size_t size)
    {
        std::memcpy(dst, data + offset, size);
        offset += size;
    };

    // -----------------------------------------------------------
    // 1) Read main header
    // -----------------------------------------------------------
    FStaticMeshPayloadHeader smHeader{};
    readRaw(&smHeader, sizeof(FStaticMeshPayloadHeader));

    m_MeshCPU.VertexCount  = smHeader.vertexCount;
    m_MeshCPU.IndexCount   = smHeader.indexCount;
    m_MeshCPU.VertexStride = smHeader.vertexStride;

    m_MeshCPU.bHasNormals  = smHeader.bHasNormals  != 0;
    m_MeshCPU.bHasTangents = smHeader.bHasTangents != 0;
    m_MeshCPU.bHasUVs      = smHeader.bHasUVs      != 0;

    const uint64_t vtxBytes   = smHeader.vertexBufferSize;
    const uint64_t idxBytes   = smHeader.indexBufferSize;
    const uint64_t subBytes   = smHeader.subMeshTableSize;
    const uint32_t slotCount  = smHeader.materialSlotCount;

    // -----------------------------------------------------------
    // 2) Vertex buffer
    // -----------------------------------------------------------
    if (!ensure(vtxBytes)) return;

    m_MeshCPU.VertexBuffer.resize((size_t)vtxBytes);
    readRaw(m_MeshCPU.VertexBuffer.data(), (size_t)vtxBytes);

    // -----------------------------------------------------------
    // 3) Index buffer
    // -----------------------------------------------------------
    if (!ensure(idxBytes)) return;

    m_MeshCPU.IndexBuffer.resize((size_t)idxBytes);
    readRaw(m_MeshCPU.IndexBuffer.data(), (size_t)idxBytes);

    // -----------------------------------------------------------
    // 4) Submesh table
    // -----------------------------------------------------------
    const size_t expectedSubBytes =
        sizeof(FModelSubMesh) * smHeader.subMeshCount;

    if (subBytes != expectedSubBytes)
        return;

    if (!ensure(subBytes))
        return;

    // 1) Read disk submeshes (FModelSubMesh)
    std::vector<FModelSubMesh> rawSubs(smHeader.subMeshCount);
    readRaw(rawSubs.data(), subBytes);

    // 2) Convert → FSubmeshCPU
    m_MeshCPU.Submeshes.resize(smHeader.subMeshCount);

    for (size_t i = 0; i < rawSubs.size(); ++i)
    {
        m_MeshCPU.Submeshes[i].firstIndex        = rawSubs[i].firstIndex;
        m_MeshCPU.Submeshes[i].indexCount        = rawSubs[i].indexCount;
        m_MeshCPU.Submeshes[i].materialSlotIndex = rawSubs[i].materialIndex;
    }

    // -----------------------------------------------------------
    // 5) Material slots (header + string blob)
    // -----------------------------------------------------------
    m_MeshCPU.MaterialSlots.clear();
    m_MeshCPU.MaterialSlots.reserve(slotCount);

    for (uint32_t i = 0; i < slotCount; ++i)
    {
        // Read slot header
        if (!ensure(sizeof(FMaterialSlot)))
            return;

        FMaterialSlot slotHeader{};
        readRaw(&slotHeader, sizeof(FMaterialSlot));

        const uint32_t nameLen = slotHeader.nameLength;
        const uint32_t idLen = slotHeader.materialAssetIDLength;

        // Read name
        if (!ensure(nameLen))
            return;

        std::string name;
        name.resize(nameLen);
        if (nameLen > 0)
            readRaw(name.data(), nameLen);

        else
            readRaw(nullptr, 0);

        // Read material asset ID
        if (!ensure(idLen))
            return;

        std::string matID;
        matID.resize(idLen);
        if (idLen > 0)
            readRaw(matID.data(), idLen);

        else
            readRaw(nullptr, 0);

        // Store
        FMaterialSlotCPU cpuSlot{};
        cpuSlot.name = std::move(name);
        cpuSlot.materialAssetID = std::move(matID);

        m_MeshCPU.MaterialSlots.emplace_back(std::move(cpuSlot));
    }

    m_CpuReady = true;
}

// ---------------------------------------------------------------
// GPU Uploading
// ---------------------------------------------------------------
void StaticMeshResource::UploadGPU()
{
    IRenderDevice* dev = GetDevice();
    if (!dev) return;

    m_SubmeshesGPU.clear();

    // empty mesh?
    if (m_MeshCPU.VertexBuffer.empty() || m_MeshCPU.IndexBuffer.empty())
        return;

    RMesh meshDesc{};

    // -----------------------------------------------------------
    // 1) Convert CPU raw buffers → GPU typed buffers
    // -----------------------------------------------------------
    {
        const size_t floatCount = m_MeshCPU.VertexBuffer.size() / sizeof(float);

        meshDesc.vertices.resize(floatCount);
        std::memcpy(
            meshDesc.vertices.data(),
            m_MeshCPU.VertexBuffer.data(),
            m_MeshCPU.VertexBuffer.size()
        );
    }

    {
        const size_t idxCount = m_MeshCPU.IndexBuffer.size() / sizeof(uint32_t);

        meshDesc.indices.resize(idxCount);
        std::memcpy(
            meshDesc.indices.data(),
            m_MeshCPU.IndexBuffer.data(),
            m_MeshCPU.IndexBuffer.size()
        );
    }

    meshDesc.vertexStride = m_MeshCPU.VertexStride;
    meshDesc.bHasNormals  = m_MeshCPU.bHasNormals;
    meshDesc.bHasTangents = m_MeshCPU.bHasTangents;
    meshDesc.bHasUVs      = m_MeshCPU.bHasUVs;

    // -----------------------------------------------------------
    // 2) Create shared GPU mesh buffer
    // -----------------------------------------------------------
    RMeshHandle handle = dev->CreateMesh(meshDesc);

    // -----------------------------------------------------------
    // 3) Build GPU submeshes referencing same mesh buffer
    // -----------------------------------------------------------
    m_SubmeshesGPU.reserve(m_MeshCPU.Submeshes.size());

    for (const auto& smCPU : m_MeshCPU.Submeshes)
    {
        FSubmeshGPU gpuSM{};
        gpuSM.mesh = handle;
        gpuSM.firstIndex = smCPU.firstIndex;
        gpuSM.indexCount = smCPU.indexCount;
        gpuSM.materialSlotIndex = smCPU.materialSlotIndex;

        m_SubmeshesGPU.push_back(gpuSM);
    }
}

// ---------------------------------------------------------------
// CPU Cleanup
// ---------------------------------------------------------------
void StaticMeshResource::ReleaseCPU()
{
    m_MeshCPU  = FMeshCPU{};   // reset
    m_CpuReady = false;
}
