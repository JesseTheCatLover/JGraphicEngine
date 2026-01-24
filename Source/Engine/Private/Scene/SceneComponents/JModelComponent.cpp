// Copyright 2025 JesseTheCatLover

#include "Scene/SceneComponents/JModelComponent.h"

#include "Core/EngineContext.h"
#include "Core/JEngine.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Resources/ResourceSubsystem.h"
#include "Resources/GpuResources/ModelResource.h"
#include "Rendering/RCommandQueue.h"
#include "Scene/JActor.h"

void JModelComponent::SetModel(const std::string& assetID)
{
    m_ModelKey = assetID;
    m_Model = JEngine::Get().GetResourceSubsystem()->Load<ModelResource>(assetID, assetID);
}

void JModelComponent::GatherProxies(IRenderSubmission& submission,
                                    const FRenderContext& ctx) const
{
    if (!IsVisible())
        return;

    auto model = m_Model;
    if (!model)
        return;

    const auto& subs = model->GetSubmeshes();
    if (subs.empty())
        return;

    const FMatrix4 world = GetWorldTransform().ToMatrix();

    // One draw command per submesh
    for (const auto& sm : subs)
    {
        if (!sm.mesh.IsValid())
            continue;

        RDrawCommand cmd{};
        cmd.state.mesh     = sm.mesh;
        cmd.state.shader   = m_Shader;
        cmd.state.material = sm.material;
        cmd.transform      = world;

        cmd.actorID = GetOwnerActor()->GetRuntimeID();

        const bool bSelected = JEngine::Get().GetEngineContext().GetEditorSelectionState().IsSelected(cmd.actorID);
        if (bSelected) // TODO: This selection logic needs to be cleaned and refactored later
        {
            cmd.bWriteCustomDepth = true;
            cmd.customStencil = JEngine::Get().GetEngineContext().GetEditorSelectionState().selectionStencil;
        }
        const uint16_t depthBucket = 0; // filled later by depth bucketer
        cmd.packet = RCommandQueue::MakeSortKey(
            GetRenderLayer(), cmd.state.shader.id, cmd.state.material.id, depthBucket);

        submission.SubmitDrawCommand(cmd);
    }
}

void JModelComponent::AllocateGpuResources()
{
    JRenderableComponent::AllocateGpuResources();

    if (!m_ModelKey.empty())
    {
        SetModel(m_ModelKey);
    }
}

JREFLECT_TYPE(JModelComponent)
{
    JPROPERTY(m_ModelKey);
}}
