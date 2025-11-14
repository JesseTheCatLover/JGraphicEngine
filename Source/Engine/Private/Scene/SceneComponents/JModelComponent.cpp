// Copyright 2025 JesseTheCatLover

#include "Scene/SceneComponents/JModelComponent.h"

#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Resources/JResourceManager.h"
#include "Resources/GpuResources/JModelResource.h"
#include "Rendering/RCommandQueue.h"

void JModelComponent::SetModel(const std::string& modelKey)
{
    m_ModelKey = modelKey;
    m_Model = JResourceManager::Get().Load<JModelResource>(modelKey, modelKey);
}

void JModelComponent::GatherProxies(IRenderSubmission& submission,
                                    const FRenderContext& ctx) const
{
    if (!IsVisible())
        return;

    auto model = m_Model.lock();
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

void JModelComponent::SerializeProperties(JsonWriter& writer) const
{
    Super::SerializeProperties(writer);
    writer.Write("model_key", m_ModelKey);
    writer.Write("shader_id", m_Shader.id);
}

void JModelComponent::DeserializeProperties(const JsonReader& reader)
{
    Super::DeserializeProperties(reader);
    m_ModelKey = reader.Read("model_key", std::string{});
    if (!m_ModelKey.empty())
        m_Model = JResourceManager::Get().Load<JModelResource>(m_ModelKey, m_ModelKey);

    const Rint shaderId = reader.Read("shader_id", Rint{0});
    m_Shader = shaderId != 0 ? RShaderHandle{ shaderId } : RShaderHandle::Invalid();
}
