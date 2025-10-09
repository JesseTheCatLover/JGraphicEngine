// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <stack>
#include <nlohmann/json.hpp>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "Core/Math/FMath.h"

/**
 * @class JsonWriter
 * @brief Lightweight wrapper around nlohmann::json for serialization.
 *
 * Provides both simple embedding and streaming-style nested JSON support.
 */
class JsonWriter
{
public:
    JsonWriter() = default;

    // ----------------- Streaming API -----------------

    /** @brief Begin writing a nested JSON object (optional key for embedding). */
    void BeginObject(const std::string& key = "");

    /** @brief End the most recently begun JSON object. */
    void EndObject();

    /** @brief Begin a JSON array under a key. */
    void BeginArray(const std::string& key);

    /** @brief End the most recently begun JSON array. */
    void EndArray();

    /** @brief Add a key/value pair to the current object. */
    template<typename T>
    void Write(const std::string& key, const T& value)
    {
        if (m_Stack.empty())
            m_Data[key] = value;
        else
            (*m_Stack.top())[key] = value;
    }

    // ----------------- glm types -----------------
    /** @brief Write a glm::vec2 to JSON. */
    void WriteVec2(const std::string& key, const glm::vec2& vec);

    /** @brief Write a glm::vec3 to JSON. */
    void WriteVec3(const std::string& key, const glm::vec3& vec);

    /** @brief Write a glm::vec4 to JSON. */
    void WriteVec4(const std::string& key, const glm::vec4& vec);

    // ----------------- FMath types -----------------
    void WriteVect2(const std::string& key, const FVector2& vec);
    void WriteVect3(const std::string& key, const FVector3& vec);
    void WriteVect4(const std::string& key, const FVector4& vec);
    void WriteRotator(const std::string& key, const FRotator& rotator);
    void WriteQuat(const std::string& key, const FQuat& quat);
    void WriteTransform(const std::string& key, const FTransform& transform);

    // ----------------- Embedding API -----------------

    /** @brief Embed a complete JSON object under a key. */
    void WriteObject(const std::string& key, const nlohmann::json& object);

    /** @brief Append a JSON object to an array (creates array if missing). */
    void WriteObjectToArray(const std::string& key, const nlohmann::json& object);

    /** @brief Get raw JSON reference (advanced usage). */
    nlohmann::json& GetData() { return m_Data; }

    /** @brief Write JSON to file. */
    bool SaveToFile(const std::string& filePath) const;

private:
    nlohmann::json m_Data;

    // Stack of currently active objects/arrays for streaming API
    std::stack<nlohmann::json*> m_Stack;
};
