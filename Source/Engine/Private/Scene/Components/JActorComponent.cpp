//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/JActorComponent.h"
#include "Core/Serialization/JsonWriter.h"

void JActorComponent::Initialize()
{
}

void JActorComponent::OnAttachment()
{
}

void JActorComponent::Serialize(JsonWriter &Writer) const
{
    Writer.BeginObject();
    Writer.Write("type", GetClassTypeName());
    Writer.Write("id", GetID());

    SerializeProperties(Writer);

    Writer.EndObject();
}

void JActorComponent::Deserialize(const JsonReader &Reader)
{
    // ID and type are handled by engine-level reflection/factory.
    DeserializeProperties(Reader);
}

void JActorComponent::Tick(float DeltaTime)
{
}
