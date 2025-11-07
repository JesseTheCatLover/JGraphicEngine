// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/Scene/JRenderableComponent.h"

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"
#include "Rendering/RRenderRoute.h"

void JRenderableComponent::EmitToRoute(RRenderRoute &route) const
{
    if (!CanRender())
        return;

    // Build one draw command so the renderer computes it later.
    RDrawCommand cmd{};
    cmd.state.mesh = m_Mesh;
    cmd.state.shader = m_Shader;
    cmd.state.material = m_Material;

    // World transform from the scene graph
    cmd.transform = GetWorldTransform().ToMatrix();

    // Pack sort key (layer + state buckets, depth left as 0)
    const uint16_t depthBucket = 0; // 0 => ComputeDepthBucketsFor() will fill it
    cmd.packet = RRenderQueue::MakeSortKey(m_RenderLayer, m_Shader.id, m_Material.id, depthBucket);

    route.Submit(cmd);
}

void JRenderableComponent::SerializeProperties(JsonWriter &writer) const
{
    Super::SerializeProperties(writer);
    writer.Write("visible", m_Visible);
    writer.Write("render_layer", static_cast<uint8_t>(m_RenderLayer));
}

void JRenderableComponent::DeserializeProperties(const JsonReader &reader)
{
    Super::DeserializeProperties(reader);
    m_Visible = reader.Read("visible", true);
    m_RenderLayer = static_cast<ERenderLayer>(reader.Read("render_layer", static_cast<uint8_t>(ERenderLayer::Opaque)));
}
