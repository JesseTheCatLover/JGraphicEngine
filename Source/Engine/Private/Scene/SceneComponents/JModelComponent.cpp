// Copyright 2025 JesseTheCatLover

#include "Scene/SceneComponents/JModelComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"

#include "Rendering/RRenderRoute.h"
#include "Rendering/RRenderQueue.h"
#include "Resources/JResourceManager.h"
#include "Resources/GpuResources/JModelResource.h"

void JModelComponent::SetModel(const std::string& modelKey)
{
    m_ModelKey = modelKey;
    m_Model = JResourceManager::Get().Load<JModelResource>(modelKey, modelKey);
}

void JModelComponent::EmitToRoute(RRenderRoute& route) const
{
    if (!IsVisible()) return;

    auto model = m_Model.lock();
    if (!model) return;

    const auto& subs = model->GetSubmeshes();
    if (subs.empty()) return;

    const FMatrix4 world = GetWorldTransform().ToMatrix();

    for (const auto& sm : subs)
    {
        if (!sm.mesh.IsValid()) continue;
        RDrawCommand cmd{};
        cmd.state.mesh     = sm.mesh;
        cmd.state.shader   = GetShaderHandle();   // set externally (e.g., default lit)
        cmd.state.material = sm.material;
        cmd.transform      = world;

        const uint16_t depthBucket = 0; // will be filled by renderer depth bucketer
        cmd.packet = RRenderQueue::MakeSortKey(GetRenderLayer(), cmd.state.shader.id, cmd.state.material.id, depthBucket);

        route.Submit(cmd);
    }
}

void JModelComponent::SerializeProperties(JsonWriter& writer) const
{
    Super::SerializeProperties(writer);
    writer.Write("model_key", m_ModelKey);
}

void JModelComponent::DeserializeProperties(const JsonReader& reader)
{
    Super::DeserializeProperties(reader);
    m_ModelKey = reader.Read("model_key", std::string{});
    if (!m_ModelKey.empty())
        m_Model = JResourceManager::Get().Load<JModelResource>(m_ModelKey, m_ModelKey);
}
