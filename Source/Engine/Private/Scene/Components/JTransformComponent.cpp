//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

//
// Created by JesseTheCatLover on 4/10/25.
//

#include "Scene/Components/JTransformComponent.h"

void JTransformComponent::SerializeProperties(JsonWriter& Writer) const
{
    Writer.StartObject("transform");
    Writer.Write("position", Position);
    Writer.Write("rotation", Rotation);
    Writer.Write("scale", Scale);
    Writer.EndObject();
}

void JTransformComponent::DeserializeProperties(const JsonReader& Reader)
{
    auto transformReader = Reader.GetChild("transform");
    Position = transformReader.Read("position", glm::vec3{0.0f});
    Rotation = transformReader.Read("rotation", glm::vec3{0.0f});
    Scale    = transformReader.Read("scale", glm::vec3{1.0f});
}