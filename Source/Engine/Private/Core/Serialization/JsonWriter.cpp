//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/JsonWriter.h"
#include <fstream>


void JsonWriter::BeginObject()
{
    m_Data = nlohmann::json::object();
}

void JsonWriter::WriteVec3(const std::string& Key, const glm::vec3& Vec)
{
    m_Data[Key] = { Vec.x, Vec.y, Vec.z };
}

bool JsonWriter::SaveToFile(const std::string& FilePath) const
{
    std::ofstream out(FilePath);
    if (!out.is_open()) return false;

    out << m_Data.dump(4); // pretty print with 4 spaces
    return true;
}
