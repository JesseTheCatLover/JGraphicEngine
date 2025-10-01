//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/JsonReader.h"
#include <fstream>

bool JsonReader::LoadFromFile(const std::string& FilePath)
{
    std::ifstream in(FilePath);
    if (!in.is_open()) return false;

    in >> m_Data;
    return true;
}

glm::vec3 JsonReader::ReadVec3(const std::string& Key, const glm::vec3& Default)
{
    if (!m_Data.contains(Key)) return Default;
    auto arr = m_Data[Key];
    return glm::vec3(arr.value(0, Default.x),
                     arr.value(1, Default.y),
                     arr.value(2, Default.z));
}