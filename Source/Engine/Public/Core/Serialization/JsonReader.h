// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include "glm/vec3.hpp"
#include <nlohmann/json.hpp>

/**
 * @class JsonReader
 * @brief Lightweight wrapper around nlohmann::json for deserialization.
 *
 * Provides a simple API to load engine objects from JSON files.
 */
class JsonReader
{
public:
    JsonReader() = default;

    /** @brief Load JSON from file. */
    bool LoadFromFile(const std::string& FilePath);

    /** @brief Get a value by key with default fallback. */
    template<typename T>
    T Read(const std::string& Key, const T& DefaultValue = T()) const
    {
        if (m_Data.contains(Key)) return m_Data[Key].get<T>();
        return DefaultValue;
    }

    glm::vec3 ReadVec3(const std::string& Key, const glm::vec3& Default = glm::vec3(0));

    /** @brief Access raw JSON reference (advanced usage). */
    const nlohmann::json& GetData() const { return m_Data; }

private:
    nlohmann::json m_Data;
};
