// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <nlohmann/json.hpp>

#include "glm/vec3.hpp"

/**
 * @class JsonWriter
 * @brief Lightweight wrapper around nlohmann::json for serialization.
 *
 * Provides a simple API to serialize engine objects into JSON files.
 */
class JsonWriter
{
public:
    JsonWriter() = default;

    /** @brief Begin writing to an empty JSON object. */
    void BeginObject();

    /** @brief Add a key/value pair (templated). */
    template<typename T>
    void Write(const std::string& Key, const T& Value)
    {
        m_Data[Key] = Value;
    }

    void WriteVec3(const std::string& Key, const glm::vec3& Vec);

    /** @brief Get raw JSON reference (advanced usage). */
    nlohmann::json& GetData() { return m_Data; }

    /** @brief Write JSON to file. */
    bool SaveToFile(const std::string& FilePath) const;

private:
    nlohmann::json m_Data;
};
