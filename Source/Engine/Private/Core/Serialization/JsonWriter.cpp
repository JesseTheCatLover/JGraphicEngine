// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/JsonWriter.h"
#include <fstream>
#include <iostream>
#include <filesystem>

void JsonWriter::BeginObject(const std::string& key)
{
    if (m_Stack.empty())
    {
        if (key.empty())
        {
            m_Data = JJson::object();
            m_Stack.push(&m_Data);
        }
        else
        {
            m_Data[key] = JJson::object();
            m_Stack.push(&m_Data[key]);
        }
        return;
    }

    JJson& parent = *m_Stack.top();

    if (key.empty())
    {
        // If we're inside an ARRAY, BeginObject() means "append object element"
        if (parent.is_array())
        {
            parent.push_back(JJson::object());
            m_Stack.push(&parent.back());
            return;
        }

        // If we're inside an OBJECT, BeginObject() means "enter current object scope"
        // (no new node created)
        if (parent.is_object())
        {
            m_Stack.push(&parent);
            return;
        }

        // If parent is null or something weird, force it to object.
        parent = JJson::object();
        m_Stack.push(&parent);
        return;
    }

    // Keyed child object
    parent[key] = JJson::object();
    m_Stack.push(&parent[key]);
}

void JsonWriter::EndObject()
{
    if (!m_Stack.empty())
        m_Stack.pop();
}

void JsonWriter::BeginArray(const std::string& key)
{
    if (m_Stack.empty())
    {
        m_Data[key] = JJson::array();
        m_Stack.push(&m_Data[key]);
    }
    else
    {
        JJson& parent = *m_Stack.top();
        parent[key] = JJson::array();
        m_Stack.push(&parent[key]);
    }
}

void JsonWriter::EndArray()
{
    if (!m_Stack.empty())
        m_Stack.pop();
}

void JsonWriter::WriteVec2(const std::string& key, const glm::vec2& vec)
{
    (*m_Stack.top())[key] = { vec.x, vec.y };
}

void JsonWriter::WriteVec3(const std::string& key, const glm::vec3& vec)
{
    (*m_Stack.top())[key] = { vec.x, vec.y, vec.z };
}

void JsonWriter::WriteVec4(const std::string& key, const glm::vec4& vec)
{
    (*m_Stack.top())[key] = { vec.x, vec.y, vec.z, vec.w };
}

void JsonWriter::WriteVect2(const std::string& key, const FVector2& vec)
{
    (*m_Stack.top())[key] = { vec.x, vec.y };
}

void JsonWriter::WriteVect3(const std::string& key, const FVector3& vec)
{
    (*m_Stack.top())[key] = { vec.x, vec.y, vec.z };
}

void JsonWriter::WriteVect4(const std::string& key, const FVector4& vec)
{
    (*m_Stack.top())[key] = { vec.x, vec.y, vec.z, vec.w };
}

void JsonWriter::WriteMatrix4(const std::string &key, const FMatrix4 &matrix)
{
    // Represent matrix as 4x4 nested array: [[r0c0, r0c1, ...], ...]
    JJson matJson = JJson::array();

    for (int row = 0; row < 4; ++row)
    {
        JJson rowJson = JJson::array();
        for (int col = 0; col < 4; ++col)
        {
            rowJson.push_back(matrix.GetMat4()[row][col]);
        }
        matJson.push_back(rowJson);
    }

    Write(key, matJson);
}

void JsonWriter::WriteRotator(const std::string& key, const FRotator& rotator)
{
    (*m_Stack.top())[key] = { rotator.Pitch, rotator.Yaw, rotator.Roll };
}

void JsonWriter::WriteQuat(const std::string& key, const FQuat& quat)
{
    (*m_Stack.top())[key] = JJson::array({
        quat.x(), quat.y(), quat.z(), quat.w()
    });
}

void JsonWriter::WriteTransform(const std::string& key, const FTransform& transform)
{
    const FVector3 pos   = transform.GetPosition();
    const FQuat    rot   = transform.GetRotation();
    const FVector3 scale = transform.GetScale();

    JJson transformObj;
    transformObj["position"] = { pos.x, pos.y, pos.z };
    transformObj["rotation"] = { rot.x(), rot.y(), rot.z(), rot.w() };
    transformObj["scale"]    = { scale.x, scale.y, scale.z };

    (*m_Stack.top())[key] = transformObj;
}

// --------------------- Object and array helpers --------------------

void JsonWriter::WriteObject(const std::string& key, const JJson& object)
{
    if (m_Stack.empty())
        m_Data[key] = object;
    else
        (*m_Stack.top())[key] = object;
}

void JsonWriter::WriteObjectToArray(const std::string& key, const JJson& object)
{
    if (m_Stack.empty())
        return;

    JJson& current = *m_Stack.top();

    if (!current.contains(key) || !current[key].is_array())
        current[key] = JJson::array();

    current[key].push_back(object);
}

bool JsonWriter::SaveToFile(const std::string& filePath) const
{
    std::filesystem::path path(filePath);
    std::filesystem::create_directories(path.parent_path());

    std::ofstream out(filePath);
    if (!out.is_open())
    {
        std::cerr << "[JsonWriter]: Failed to open file: " << filePath
                  << " (directory may not exist or permissions denied)" << std::endl;
        return false;
    }

    out << m_Data.dump(4);
    return true;
}
