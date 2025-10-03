// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Core/Serialization/JsonWriter.h"
#include <fstream>

void JsonWriter::BeginObject(const std::string& Key)
{
    nlohmann::json* newObj = new nlohmann::json(nlohmann::json::object());

    if (m_Stack.empty())
    {
        if (!Key.empty()) m_Data[Key] = *newObj;
        else m_Data = *newObj;
    }
    else
    {
        if (!Key.empty()) (*m_Stack.top())[Key] = *newObj;
        else (*m_Stack.top()).push_back(*newObj); // anonymous object in array
    }

    m_Stack.push(newObj);
}

void JsonWriter::EndObject()
{
    if (m_Stack.empty()) return;
    nlohmann::json* top = m_Stack.top();
    m_Stack.pop();
    delete top; // free temporary pointer
}

void JsonWriter::BeginArray(const std::string& Key)
{
    nlohmann::json* newArr = new nlohmann::json(nlohmann::json::array());
    if (m_Stack.empty()) m_Data[Key] = *newArr;
    else (*m_Stack.top())[Key] = *newArr;

    m_Stack.push(newArr);
}

void JsonWriter::EndArray()
{
    if (m_Stack.empty()) return;
    nlohmann::json* top = m_Stack.top();
    m_Stack.pop();
    delete top;
}

void JsonWriter::WriteVec2(const std::string& Key, const glm::vec2& Vec)
{
    m_Data[Key] = { Vec.x, Vec.y };
}

void JsonWriter::WriteVec3(const std::string& Key, const glm::vec3& Vec)
{
    m_Data[Key] = { Vec.x, Vec.y, Vec.z };
}

void JsonWriter::WriteVec4(const std::string& Key, const glm::vec4& Vec)
{
    m_Data[Key] = { Vec.x, Vec.y, Vec.z, Vec.w };
}

void JsonWriter::WriteObject(const std::string& Key, const nlohmann::json& Object)
{
    if (m_Stack.empty()) m_Data[Key] = Object;
    else (*m_Stack.top())[Key] = Object;
}

void JsonWriter::WriteObjectToArray(const std::string& Key, const nlohmann::json& Object)
{
    if (m_Stack.empty() || !m_Stack.top()->contains(Key) || !(*m_Stack.top())[Key].is_array())
        (*m_Stack.top())[Key] = nlohmann::json::array();

    (*m_Stack.top())[Key].push_back(Object);
}

bool JsonWriter::SaveToFile(const std::string& FilePath) const
{
    std::ofstream out(FilePath);
    if (!out.is_open()) return false;

    out << m_Data.dump(4); // pretty print with 4 spaces
    return true;
}
