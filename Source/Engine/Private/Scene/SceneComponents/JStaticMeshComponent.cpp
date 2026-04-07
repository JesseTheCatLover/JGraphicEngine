//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneComponents/JStaticMeshComponent.h"

#include "Core/EngineContext.h"
#include "Core/JEngine.h"
#include "Resources/ResourceSubsystem.h"
#include "Resources/GpuResources/StaticMeshResource.h"
#include "Resources/GpuResources/MaterialResource.h"
#include "Rendering/RCommandQueue.h"
#include "Scene/JActor.h"

void JStaticMeshComponent::SetStaticMesh(const std::string& assetID)
{
    m_ResourceSubsystem = JEngine::Get().GetResourceSubsystem();
    if (!m_ResourceSubsystem)
    {
        std::cerr << "[JStaticMeshComponent]: ResourceSubsystem is null\n";
        m_StaticMesh.reset();
        return;
    }

    m_StaticMeshAssetID = assetID;

    // Load mesh
    m_StaticMesh = m_ResourceSubsystem->Load<StaticMeshResource>(assetID, StaticMeshResource::FDesc{ assetID }
    );

    // Mesh changed -> invalidate cached materials
    m_bMaterialsResolved = false;
    m_ResolvedMaterials.clear();
}

void JStaticMeshComponent::SetMaterialOverride(size_t slotIndex,
                                              const std::string& materialAssetID)
{
    if (m_MaterialOverrideAssetIDs.size() <= slotIndex)
        m_MaterialOverrideAssetIDs.resize(slotIndex + 1);

    m_MaterialOverrideAssetIDs[slotIndex] = materialAssetID;

    // override changed -> force re-resolve
    m_bMaterialsResolved = false;
}

void JStaticMeshComponent::AllocateGpuResources()
{
    JRenderableComponent::AllocateGpuResources();

    if (!m_StaticMeshAssetID.empty())
    {
        SetStaticMesh(m_StaticMeshAssetID);
    }
}

void JStaticMeshComponent::ResolveMaterials() const
{
    if (m_bMaterialsResolved)
        return;

    m_ResolvedMaterials.clear();

    if (!m_StaticMesh)
    {
        m_bMaterialsResolved = true;
        return;
    }

    if (!m_ResourceSubsystem)
    {
        std::cerr << "[JStaticMeshComponent]: ResourceSubsystem null while resolving materials\n";
        m_bMaterialsResolved = true;
        return;
    }

    // Get CPU material slot table
    const auto& slots = m_StaticMesh->GetMaterialSlotsCPU();
    const size_t count = slots.size();

    m_ResolvedMaterials.resize(count, RMaterialHandle{}); // default: invalid

    for (size_t i = 0; i < count; ++i)
    {
        std::string materialAssetID;

        // 1) Per-component override
        if (i < m_MaterialOverrideAssetIDs.size() &&
            !m_MaterialOverrideAssetIDs[i].empty())
        {
            materialAssetID = m_MaterialOverrideAssetIDs[i];
        }
        else
        {
            // 2) Mesh-provided material
            materialAssetID = slots[i].materialAssetID;
        }

        if (materialAssetID.empty())
        {
            // optional: load engine default material here:
            //
            // auto defaultMat = resources->Load<MaterialResource>("Engine/DefaultMaterial");
            // if (defaultMat) m_ResolvedMaterials[i] = defaultMat->GetHandle();
            //
            continue;
        }

        // Load GPU material resource
        auto matRes = m_ResourceSubsystem->Load<MaterialResource>(
            materialAssetID, MaterialResource::FDesc{ materialAssetID }, m_ResourceSubsystem

        );

        if (matRes)
        {
            m_ResolvedMaterials[i] = matRes->GetHandle();
        }
        else
        {
            std::cerr << "[JStaticMeshComponent]: Failed to load material " << materialAssetID << "\n";
        }
    }

    m_bMaterialsResolved = true;
}

void JStaticMeshComponent::GatherProxies(IRenderSubmission& submission,
                                        const FRenderContext& ctx) const
{
    if (!IsVisible())
        return;
    if (!m_StaticMesh)
        return;

    const auto& subsGPU = m_StaticMesh->GetSubmeshes();
    if (subsGPU.empty())
        return;

    ResolveMaterials();

    const FMatrix4 world = GetWorldTransform().ToMatrix();
    const uint64_t actorID = GetOwnerActor()->GetRuntimeID();
    const bool bSelected =
        JEngine::Get().GetEngineContext().GetEditorSelectionState().IsSelected(actorID);

    for (const auto& sm : subsGPU)
    {
        if (!sm.mesh.IsValid())
            continue;

        RDrawCommand cmd{};
        cmd.state.mesh = sm.mesh;

        // Shader override, or renderer fallback
        cmd.state.shader = m_Shader;

        // Correct material per slot
        RMaterialHandle matHandle{};
        if (sm.materialSlotIndex < m_ResolvedMaterials.size())
            matHandle = m_ResolvedMaterials[sm.materialSlotIndex];

        cmd.state.material = matHandle;

        cmd.transform = world;
        cmd.actorID = actorID;

        if (bSelected)
        {
            const auto& selState = JEngine::Get().GetEngineContext().GetEditorSelectionState();
            cmd.bWriteCustomDepth = true;
            cmd.customStencil = selState.selectionStencil;
        }

        const uint16_t depthBucket = 0;

        cmd.packet = RCommandQueue::MakeSortKey(GetRenderLayer(), cmd.state.shader.id,
            cmd.state.material.id, depthBucket);

        submission.SubmitDrawCommand(cmd);
    }
}
