//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/JComponent.h"

void JComponent::Initialize()
{
}

void JComponent::OnAttachment()
{
    Initialize();
}

void JComponent::Serialize(JsonWriter &Writer) const
{
    Writer.StartObject();
    Writer.Write("type", GetClassTypeName());
    Writer.Write("id", GetID());

    SerializeProperties(Writer);

    Writer.EndObject();
}

void JComponent::Deserialize(const JsonReader &Reader)
{
    // ID and type are handled by engine-level reflection/factory.
    DeserializeProperties(Reader);
}

void JComponent::Tick(float DeltaTime)
{
}
