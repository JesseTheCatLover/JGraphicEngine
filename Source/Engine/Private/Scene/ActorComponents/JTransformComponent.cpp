//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/ActorComponents/JTransformComponent.h"

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"

void JTransformComponent::Serialize(JsonWriter &writer) const
{
    writer.BeginObject("transform");
    writer.Write("position", m_LocalTransform.GetPosition());
    writer.Write("rotation", m_LocalTransform.GetRotation());
    writer.Write("scale", m_LocalTransform.GetScale());
    writer.EndObject();
}

void JTransformComponent::Deserialize(const JsonReader &reader)
{
    auto transformReader = reader.GetObject("transform");

    m_LocalTransform.SetPosition(transformReader.Read("position", FVector3{0.0f}));
    m_LocalTransform.SetRotation(transformReader.Read("rotation", FQuat{1.0f, 0.0f, 0.0f, 0.0f}));
    m_LocalTransform.SetScale(transformReader.Read("scale", FVector3{1.0f}));
}
