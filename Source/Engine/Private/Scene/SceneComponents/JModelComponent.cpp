// Copyright 2025 JesseTheCatLover

#include "Scene/SceneComponents/JModelComponent.h"

#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Resources/JResourceManager.h"
#include "Resources/GpuResources/JModelResource.h"
#include "Rendering/RCommandQueue.h"

void JModelComponent::SetModel(const std::string& assetID)
{
    m_ModelKey = assetID;
    m_Model = JResourceManager::Get().Load<JModelResource>(assetID, assetID);
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
        const uint16_t depthBucket = 0; // filled later by depth bucketer
        cmd.packet = RCommandQueue::MakeSortKey(
            GetRenderLayer(), cmd.state.shader.id, cmd.state.material.id, depthBucket);

        submission.SubmitDrawCommand(cmd);
    }
}

void JModelComponent::DeserializeCustom(const class JsonReader &reader)
{
    JRenderableComponent::DeserializeCustom(reader);

    // Then rebuild runtime state from that data
    if (!m_ModelKey.empty())
    {
        m_Model = JResourceManager::Get().Load<JModelResource>(m_ModelKey, m_ModelKey);
    }
}

JREFLECT_TYPE(JModelComponent) {
    JPROPERTY(m_ModelKey);
}}