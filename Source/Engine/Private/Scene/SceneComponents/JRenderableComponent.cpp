#include "Scene/SceneComponents/JRenderableComponent.h"

#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"

void JRenderableComponent::SerializeCustom(JsonWriter& writer) const
{
    Super::SerializeCustom(writer);
    writer.Write("visible", m_Visible);
    writer.Write("render_layer", static_cast<uint8_t>(m_RenderLayer));
}

void JRenderableComponent::Deserialize(const JsonReader& reader)
{
    Super::Deserialize(reader);
    m_Visible = reader.Read("visible", true);
    m_RenderLayer = static_cast<ERenderLayer>(
        reader.Read("render_layer", static_cast<uint8_t>(ERenderLayer::Opaque)));
}
