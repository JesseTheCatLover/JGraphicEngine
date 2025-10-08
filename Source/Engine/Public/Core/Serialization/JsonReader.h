// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <vector>
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"
#include "Core/Math/FMath.h"
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
    JsonReader(const nlohmann::json& node) : m_Data(node) {}

    /** Load JSON from file */
    bool LoadFromFile(const std::string& filePath);

    /** Read a value by key with default fallback */
    template<typename T>
    T Read(const std::string& Key, const T& DefaultValue = T()) const
    {
        if (m_Data.contains(Key)) return m_Data[Key].get<T>();
        return DefaultValue;
    }

    /** @brief Read a glm::vec2 from JSON. */
    [[nodiscard]] glm::vec2 ReadVec2(const std::string& key, const glm::vec2& defaultVal = glm::vec2(0)) const;

    /** @brief Read a glm::vec3 from JSON. */
    [[nodiscard]] glm::vec3 ReadVec3(const std::string& key, const glm::vec3& defaultVal = glm::vec3(0)) const;

    /** @brief Read a glm::vec4 from JSON. */
    [[nodiscard]] glm::vec4 ReadVec4(const std::string& key, const glm::vec4& defaultVal = glm::vec4(0)) const;

    /** Read an FVector2 from JSON */
    [[nodiscard]] FVector2 ReadVector2(const std::string& key, const FVector2& defaultVal) const;

    /** Read an FVector3 from JSON */
    [[nodiscard]] FVector3 ReadVector3(const std::string& key, const FVector3& defaultVal) const;

    /** Read an FVector4 from JSON */
    [[nodiscard]] FVector4 ReadVector4(const std::string& key, const FVector4& defaultVal) const;

    /** Read an FQuat from JSON */
    [[nodiscard]] FQuat ReadQuat(const std::string& key, const FQuat& defaultVal = FQuat(0, 0, 0, 1)) const;

    /** Read an FTransform from JSON */
    [[nodiscard]] FTransform ReadTransform(const std::string& key, const FTransform& defaultVal = FTransform()) const;

    /** Get a nested object by key */
    [[nodiscard]] JsonReader GetObject(const std::string& key) const;

    /** Get an array of nested objects by key */
    [[nodiscard]] std::vector<JsonReader> GetArray(const std::string& key) const;

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
