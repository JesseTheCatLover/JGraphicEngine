// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JModelResource.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include <iostream>

JModelResource::JModelResource(const std::string& InPath)
    : Path(InPath)
{
    LoadModelFromFile(Path);
}

void JModelResource::LoadModelFromFile(const std::string& InPath)
{
    Model = std::make_shared<JModel>(InPath);

    if (!Model)
    {
        std::cerr << "[JModelResource]: Failed to load model: " << InPath << std::endl;
    }
}

void JModelResource::Serialize(JsonWriter& Writer) const
{
    // Only store metadata for now
    Writer.Write("Type", GetClassTypeName());
    Writer.Write("ID", GetID());
    Writer.Write("Path", Path);
}

void JModelResource::Deserialize(const JsonReader& Reader)
{
    Path = Reader.Read<std::string>("Path", "");
    if (!Path.empty())
        LoadModelFromFile(Path);
}
