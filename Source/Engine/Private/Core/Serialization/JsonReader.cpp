// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/JsonReader.h"
#include <fstream>
#include <iostream>

bool JsonReader::LoadFromFile(const std::string& FilePath)
{
    std::ifstream in(FilePath);
    if (!in.is_open()) return false;

    try {
        in >> m_Data;
    } catch (const std::exception& e) {
        std::cerr << "[JsonReader]: Failed to parse JSON: " << e.what() << std::endl;
        return false;
    }

    return true;
}

glm::vec2 JsonReader::ReadVec2(const std::string& Key, const glm::vec2& Default) const
{
    auto arr = ReadArray<float, 2>(Key, { Default.x, Default.y });
    return glm::vec2(arr[0], arr[1]);
}

glm::vec3 JsonReader::ReadVec3(const std::string& Key, const glm::vec3& Default) const
{
    auto arr = ReadArray<float, 3>(Key, { Default.x, Default.y, Default.z });
    return glm::vec3(arr[0], arr[1], arr[2]);
}

glm::vec4 JsonReader::ReadVec4(const std::string& Key, const glm::vec4& Default) const
{
    auto arr = ReadArray<float, 4>(Key, { Default.x, Default.y, Default.z, Default.w });
    return glm::vec4(arr[0], arr[1], arr[2], arr[3]);
}

JsonReader JsonReader::GetObject(const std::string& Key) const
{
    if (m_Data.contains(Key) && m_Data[Key].is_object())
        return JsonReader(m_Data[Key]);
    return JsonReader(nlohmann::json::object());
}

std::vector<JsonReader> JsonReader::GetArray(const std::string& Key) const
{
    std::vector<JsonReader> result;
    if (m_Data.contains(Key) && m_Data[Key].is_array())
    {
        for (auto& elem : m_Data[Key])
            result.emplace_back(elem);
    }
    return result;
}
