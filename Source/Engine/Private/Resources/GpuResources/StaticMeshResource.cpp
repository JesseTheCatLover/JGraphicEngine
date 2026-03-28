//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "StaticMeshResource.h"
#include <cstring>

#include "Assets/Payloads/FStaticMeshPayloadHeader.h"
#include "Rendering/IRenderDevice.h"

StaticMeshResource::StaticMeshResource(FDesc desc, AssetRegistrySubsystem* registry)
    : GpuResource(registry)
    , m_Desc(std::move(desc))
    , m_Registry(registry)
{
}

bool StaticMeshResource::OnCreateGpuResources()
{
    if (!m_CpuReady)
        LoadCPU();

    if (!m_CpuReady)
        return false;

    UploadGPU();
    ReleaseCPU();

    return true;
}

void StaticMeshResource::OnDestroyGpuResources()
{
    IRenderDevice* dev = GetDevice();
    if (!dev) return;

    for (auto& sm : m_SubmeshesGPU)
    {
        if (sm.mesh.IsValid())
            dev->DestroyMesh(sm.mesh);
    }

    m_SubmeshesGPU.clear();
}

void StaticMeshResource::LoadCPU()
{
    FAssetHeader header{};
    std::vector<uint8_t> payload;

    if (!LoadAssetBinary(m_Desc.assetID, header, payload))
        return;

    if (payload.size() < sizeof(FStaticMeshPayloadHeader))
        return;

    const uint8_t* base   = payload.data();
    size_t         offset = 0;

    // 1) Read static mesh payload header
    const auto* smHeader = reinterpret_cast<const FStaticMeshPayloadHeader*>(base + offset);
    offset += sizeof(FStaticMeshPayloadHeader);

    // Fill CPU meta info
    m_MeshCPU.VertexCount  = smHeader->vertexCount;
    m_MeshCPU.IndexCount   = smHeader->indexCount;
    m_MeshCPU.VertexStride = smHeader->vertexStride;

    m_MeshCPU.bHasNormals  = (smHeader->bHasNormals  != 0);
    m_MeshCPU.bHasTangents = (smHeader->bHasTangents != 0);
    m_MeshCPU.bHasUVs      = (smHeader->bHasUVs      != 0);

    const uint64_t vertexBufferSize = smHeader->vertexBufferSize;
    const uint64_t indexBufferSize  = smHeader->indexBufferSize;
    const uint64_t subMeshTableSize = smHeader->subMeshTableSize;

    auto ensureAvailable = [&](uint64_t needed) -> bool
    {
        return offset + needed <= payload.size();
    };

    // 2) Copy vertex buffer
    if (!ensureAvailable(vertexBufferSize))
        return;

    m_MeshCPU.VertexBuffer.resize(static_cast<size_t>(vertexBufferSize));
    std::memcpy(
        m_MeshCPU.VertexBuffer.data(),
        base + offset,
        static_cast<size_t>(vertexBufferSize)
    );
    offset += static_cast<size_t>(vertexBufferSize);

    // 3) Copy index buffer
    if (!ensureAvailable(indexBufferSize))
        return;

    m_MeshCPU.IndexBuffer.resize(static_cast<size_t>(indexBufferSize));
    std::memcpy(
        m_MeshCPU.IndexBuffer.data(),
        base + offset,
        static_cast<size_t>(indexBufferSize)
    );
    offset += static_cast<size_t>(indexBufferSize);

    // 4) Read submesh table
    if (!ensureAvailable(subMeshTableSize))
        return;

    const size_t subMeshCount = smHeader->subMeshCount;
    const size_t expectedSubMeshSize = sizeof(FModelSubMesh) * subMeshCount;
    if (subMeshTableSize != expectedSubMeshSize)
    {
        // format mismatch, bail out for now
        return;
    }

    const auto* subMeshSrc =
        reinterpret_cast<const FModelSubMesh*>(base + offset);

    m_MeshCPU.Submeshes.resize(subMeshCount);
    for (size_t i = 0; i < subMeshCount; ++i)
    {
        const auto& src = subMeshSrc[i];
        auto&       dst = m_MeshCPU.Submeshes[i];

        dst.firstIndex        = src.firstIndex;
        dst.indexCount        = src.indexCount;
        dst.materialSlotIndex = src.materialIndex;
    }
    offset += static_cast<size_t>(subMeshTableSize);

    // 5) Read material slots
    const uint32_t materialSlotCount = smHeader->materialSlotCount;
    m_MeshCPU.MaterialSlots.clear();
    m_MeshCPU.MaterialSlots.reserve(materialSlotCount);

    for (uint32_t i = 0; i < materialSlotCount; ++i)
    {
        // 5.1) Header
        if (!ensureAvailable(sizeof(FMaterialSlot)))
            return;

        const auto* slotHeader =
            reinterpret_cast<const FMaterialSlot*>(base + offset);
        offset += sizeof(FMaterialSlot);

        const uint32_t nameLen = slotHeader->nameLength;
        const uint32_t assetIDLen = slotHeader->materialAssetIDLength;

        // 5.2) Name
        if (!ensureAvailable(nameLen))
            return;

        std::string name;
        if (nameLen > 0)
        {
            name.assign(
                reinterpret_cast<const char*>(base + offset),
                nameLen
            );
        }
        offset += nameLen;

        // 5.3) Material Asset ID
        if (!ensureAvailable(assetIDLen))
            return;

        std::string matID;
        if (assetIDLen > 0)
        {
            matID.assign(
                reinterpret_cast<const char*>(base + offset),
                assetIDLen
            );
        }
        offset += assetIDLen;

        // Store CPU structure
        FMaterialSlotCPU slotCPU{};
        slotCPU.name           = std::move(name);
        slotCPU.materialAssetID = std::move(matID);

        m_MeshCPU.MaterialSlots.push_back(std::move(slotCPU));
    }

    // Optional: you can assert offset == payload.size() if the asset format guarantees no extra trailing data

    m_CpuReady = true;
}

void StaticMeshResource::UploadGPU()
{
    IRenderDevice* dev = GetDevice();
    if (!dev) return;

    m_SubmeshesGPU.clear();
    m_SubmeshesGPU.reserve(m_MeshCPU.Submeshes.size());

    if (m_MeshCPU.VertexBuffer.empty() || m_MeshCPU.IndexBuffer.empty())
        return;

    // Build a single RMesh description for the shared buffers.
    // m_MeshCPU.VertexBuffer / IndexBuffer are raw bytes; RMesh expects typed vectors.

    RMesh meshDesc{};

    // 1) Vertices: convert byte buffer into float vector
    {
        const size_t floatCount = m_MeshCPU.VertexBuffer.size() / sizeof(float);
        meshDesc.vertices.resize(floatCount);

        std::memcpy(
            meshDesc.vertices.data(),
            m_MeshCPU.VertexBuffer.data(),
            m_MeshCPU.VertexBuffer.size()
        );
    }

    // 2) Indices: assuming 32-bit indices
    {
        const size_t indexCount = m_MeshCPU.IndexBuffer.size() / sizeof(uint32_t);
        meshDesc.indices.resize(indexCount);

        std::memcpy(
            meshDesc.indices.data(),
            m_MeshCPU.IndexBuffer.data(),
            m_MeshCPU.IndexBuffer.size()
        );
    }

    // 3) Basic metadata
    meshDesc.vertexStride = static_cast<Rint>(m_MeshCPU.VertexStride);
    meshDesc.bHasNormals  = m_MeshCPU.bHasNormals;
    meshDesc.bHasTangents = m_MeshCPU.bHasTangents;
    meshDesc.bHasUVs      = m_MeshCPU.bHasUVs;

    // 4) Create the GPU mesh
    RMeshHandle sharedMeshHandle = dev->CreateMesh(meshDesc);

    // 5) Create GPU submesh entries that reference this mesh and a material slot index
    for (const auto& smCPU : m_MeshCPU.Submeshes)
    {
        FSubmeshGPU smGPU{};
        smGPU.mesh              = sharedMeshHandle;
        smGPU.firstIndex        = smCPU.firstIndex;
        smGPU.indexCount        = smCPU.indexCount;
        smGPU.materialSlotIndex = smCPU.materialSlotIndex;

        m_SubmeshesGPU.push_back(smGPU);
    }
}

void StaticMeshResource::ReleaseCPU()
{
    m_MeshCPU = FMeshCPU{}; // reset to default
    m_CpuReady = false;
}
