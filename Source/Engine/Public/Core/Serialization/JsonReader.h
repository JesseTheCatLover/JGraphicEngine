// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"
#include <nlohmann/json.hpp>

/**
 * @class JsonReader
 * @brief Lightweight wrapper around nlohmann::json for deserialization.
 *
 * Supports reading nested objects, arrays, and engine-specific types like vectors ecs.
 */
class JsonReader
{
public:
    /** Default constructor */
    JsonReader() = default;

    /** Construct from existing json node (for nested objects) */
    JsonReader(const nlohmann::json& Node) : m_Data(Node) {}

    /** Load JSON from file */
    bool LoadFromFile(const std::string& FilePath);

    /** Read a value by key with default fallback */
    template<typename T>
    T Read(const std::string& Key, const T& DefaultValue = T()) const
    {
        if (m_Data.contains(Key)) return m_Data[Key].get<T>();
        return DefaultValue;
    }


    /** @brief Read a glm::vec2 from JSON. */
    glm::vec2 ReadVec2(const std::string& Key, const glm::vec2& Default = glm::vec2(0)) const;

    /** @brief Read a glm::vec3 from JSON. */
    glm::vec3 ReadVec3(const std::string& Key, const glm::vec3& Default = glm::vec3(0)) const;

    /** @brief Read a glm::vec4 from JSON. */
    glm::vec4 ReadVec4(const std::string& Key, const glm::vec4& Default = glm::vec4(0)) const;

    /** Get a nested object by key */
    JsonReader GetObject(const std::string& Key) const;

    /** Get an array of nested objects by key */
    std::vector<JsonReader> GetArray(const std::string& Key) const;

    /** Access the underlying JSON node (advanced) */
    const nlohmann::json& GetData() const { return m_Data; }

private:
    nlohmann::json m_Data;

    /** Helper: safely read an array of size N into a std::array or glm type */
    template<typename T, size_t N>
    std::array<T, N> ReadArray(const std::string& Key, const std::array<T, N>& Default) const
    {
        std::array<T, N> result = Default;

        // If the key doesn't exist or isn't an array, return default
        if (!m_Data.contains(Key) || !m_Data.at(Key).is_array())
            return result;

        // Read each element safely with default fallback
        const auto& arr = m_Data.at(Key);
        for (size_t i = 0; i < N && i < arr.size(); ++i)
        {
            try
            {
                result[i] = arr.at(i).get<T>();
            }
            catch (...)
            {
                // leave default value if conversion fails
            }
        }

        return result;
    }
};
