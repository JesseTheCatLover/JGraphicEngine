// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/JsonReader.h"
#include <fstream>
#include <iostream>

bool JsonReader::LoadFromFile(const std::string& filePath)
{
    std::ifstream in(filePath);
    if (!in.is_open()) return false;

    try {
        in >> m_Data;
    } catch (const std::exception& e) {
        std::cerr << "[JsonReader]: Failed to parse JSON: " << e.what() << std::endl;
        return false;
    }

    return true;
}
glm::vec2 JsonReader::ReadVec2(const std::string& key, const glm::vec2& defaultVal) const
{
    auto arr = ReadArray<float, 2>(key, { defaultVal.x, defaultVal.y });
    return glm::vec2(arr[0], arr[1]);
}

glm::vec3 JsonReader::ReadVec3(const std::string& key, const glm::vec3& defaultVal) const
{
    auto arr = ReadArray<float, 3>(key, { defaultVal.x, defaultVal.y, defaultVal.z });
    return glm::vec3(arr[0], arr[1], arr[2]);
}

glm::vec4 JsonReader::ReadVec4(const std::string& key, const glm::vec4& defaultVal) const
{
    auto arr = ReadArray<float, 4>(key, { defaultVal.x, defaultVal.y, defaultVal.z, defaultVal.w });
    return glm::vec4(arr[0], arr[1], arr[2], arr[3]);
}

FVector2 JsonReader::ReadVector2(const std::string& key, const FVector2& defaultVal) const
{
    if (!m_Data.contains(key) || !m_Data[key].is_array() || m_Data[key].size() < 2)
        return defaultVal;

    return FVector2(
        m_Data[key][0].get<float>(),
        m_Data[key][1].get<float>()
    );
}

FVector3 JsonReader::ReadVector3(const std::string& key, const FVector3& defaultVal) const
{
    if (!m_Data.contains(key) || !m_Data[key].is_array() || m_Data[key].size() < 3)
        return defaultVal;

    return FVector3(
        m_Data[key][0].get<float>(),
        m_Data[key][1].get<float>(),
        m_Data[key][2].get<float>()
    );
}

FVector4 JsonReader::ReadVector4(const std::string& key, const FVector4& defaultVal) const
{
    if (!m_Data.contains(key) || !m_Data[key].is_array() || m_Data[key].size() < 4)
        return defaultVal;

    return FVector4(
        m_Data[key][0].get<float>(),
        m_Data[key][1].get<float>(),
        m_Data[key][2].get<float>(),
        m_Data[key][3].get<float>()
    );
}

FQuat JsonReader::ReadQuat(const std::string& key, const FQuat& defaultVal) const
{
    if (!m_Data.contains(key) || !m_Data[key].is_array() || m_Data[key].size() < 4)
        return defaultVal;

    return FQuat(
        m_Data[key][0].get<float>(),
        m_Data[key][1].get<float>(),
        m_Data[key][2].get<float>(),
        m_Data[key][3].get<float>()
    );
}

FTransform JsonReader::ReadTransform(const std::string& key, const FTransform& defaultVal) const
{
    if (!m_Data.contains(key) || !m_Data[key].is_object())
        return defaultVal;

    const auto& obj = m_Data[key];

    FTransform result;
    result.position() = ReadVector3(obj.contains("position") ? "position" : "", defaultVal.position());
    result.rotation() = ReadQuat(obj.contains("rotation") ? "rotation" : "", defaultVal.rotation());
    result.scale() = ReadVector3(obj.contains("scale") ? "scale" : "", defaultVal.scale());

    return result;
}

JsonReader JsonReader::GetObject(const std::string& Key) const
{
    if (!m_Data.contains(Key)) return JsonReader();
    return JsonReader(m_Data[Key]);
}

std::vector<JsonReader> JsonReader::GetArray(const std::string& Key) const
{
    std::vector<JsonReader> results;
    if (!m_Data.contains(Key) || !m_Data[Key].is_array()) return results;

    for (const auto& item : m_Data[Key])
        results.emplace_back(item);

    return results;
}