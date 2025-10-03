// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <string>
#include <stack>
#include <nlohmann/json.hpp>

#include "glm/vec2.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

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
    void BeginObject(const std::string& Key = "");

    /** @brief End the most recently begun JSON object. */
    void EndObject();

    /** @brief Begin a JSON array under a key. */
    void BeginArray(const std::string& Key);

    /** @brief End the most recently begun JSON array. */
    void EndArray();

    /** @brief Add a key/value pair to the current object. */
    template<typename T>
    void Write(const std::string& Key, const T& Value)
    {
        if (m_Stack.empty())
            m_Data[Key] = Value;
        else
            (*m_Stack.top())[Key] = Value;
    }

    /** @brief Write a glm::vec2 to JSON. */
    void WriteVec2(const std::string& Key, const glm::vec2& Vec);

    /** @brief Write a glm::vec3 to JSON. */
    void WriteVec3(const std::string& Key, const glm::vec3& Vec);

    /** @brief Write a glm::vec4 to JSON. */
    void WriteVec4(const std::string& Key, const glm::vec4& Vec);

    // ----------------- Embedding API -----------------

    /** @brief Embed a complete JSON object under a key. */
    void WriteObject(const std::string& Key, const nlohmann::json& Object);

    /** @brief Append a JSON object to an array (creates array if missing). */
    void WriteObjectToArray(const std::string& Key, const nlohmann::json& Object);

    /** @brief Get raw JSON reference (advanced usage). */
    nlohmann::json& GetData() { return m_Data; }

    /** @brief Write JSON to file. */
    bool SaveToFile(const std::string& FilePath) const;

private:
    nlohmann::json m_Data;

    // Stack of currently active objects/arrays for streaming API
    std::stack<nlohmann::json*> m_Stack;
};
