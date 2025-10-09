//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/JActorComponent.h"
#include "Core/Serialization/JsonWriter.h"

void JActorComponent::Initialize()
{
}

void JActorComponent::OnAttachment()
{
}

void JActorComponent::Serialize(JsonWriter &writer) const
{
    writer.BeginObject();
    writer.Write("type", GetClassTypeName());
    writer.Write("id", GetID());

    SerializeProperties(writer);

    writer.EndObject();
}

void JActorComponent::Deserialize(const JsonReader &reader)
{
    // ID and type are handled by engine-level reflection/factory.
    DeserializeProperties(reader);
}

void JActorComponent::Tick(float deltaTime)
{
}
