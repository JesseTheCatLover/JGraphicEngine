// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JModelResource.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include <iostream>

JModelResource::JModelResource(const std::string& inPath)
    : m_Path(inPath)
{
    LoadModelFromFile(m_Path);
}

void JModelResource::LoadModelFromFile(const std::string& inPath)
{
    m_Model = std::make_shared<JModel>(inPath);

    if (!m_Model)
    {
        std::cerr << "[JModelResource]: Failed to load model: " << inPath << std::endl;
    }
}

void JModelResource::Serialize(JsonWriter& writer) const
{
    // Only store metadata for now
    writer.Write("Type", GetClassTypeName());
    writer.Write("ID", GetID());
    writer.Write("Path", m_Path);
}

void JModelResource::Deserialize(const JsonReader& reader)
{
    m_Path = reader.Read<std::string>("Path", "");
    if (!m_Path.empty())
        LoadModelFromFile(m_Path);
}
