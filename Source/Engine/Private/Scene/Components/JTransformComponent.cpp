// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/JTransformComponent.h"

#include "Core/Serialization/JsonReader.h"
#include "Core/Serialization/JsonWriter.h"

void JTransformComponent::SerializeProperties(JsonWriter& Writer) const
{
    Writer.BeginObject("transform");
    Writer.Write("position", LocalTransform.position());
    Writer.Write("rotation", LocalTransform.rotation());
    Writer.Write("scale", LocalTransform.scale());
    Writer.EndObject();
}

void JTransformComponent::DeserializeProperties(const JsonReader& Reader)
{
    auto transformReader = Reader.GetObject("transform");

    LocalTransform.position() = transformReader.Read("position", FVector3{0.0f});
    LocalTransform.rotation() = transformReader.Read("rotation", FQuat{1.0f, 0.0f, 0.0f, 0.0f});
    LocalTransform.scale() = transformReader.Read("scale", FVector3{1.0f});
}
