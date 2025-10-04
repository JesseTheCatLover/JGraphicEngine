//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/SceneComponents/JSceneComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include <algorithm>

void JSceneComponent::AttachToComponent(JSceneComponent* parent)
{
    if (m_Parent)
        Detach();

    m_Parent = parent;
    if (m_Parent)
        m_Parent->m_Children.push_back(this);
}

void JSceneComponent::Detach()
{
    if (m_Parent)
    {
        auto& siblings = m_Parent->m_Children;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }

    // Reparent to actor root if exists
    if (GetOwner() && GetOwner()->GetRootComponent())
    {
        AttachToComponent(GetOwner()->GetRootComponent());
    }
    else
    {
        m_Parent = nullptr; // truly detached
    }
}

glm::mat4 JSceneComponent::GetWorldTransform() const
{
    if (m_Parent)
        return m_Parent->GetWorldTransform() * GetLocalTransform();
    return GetLocalTransform();
}

void JSceneComponent::OnAttachment()
{
    JTransformComponent::OnAttachment();

    // Owner is guaranteed to be set here
    if (GetOwner())
    {
        // e.g., add this component to the actor's scene graph
        GetOwner()->AddSceneComponent(this);
    }
}

void JSceneComponent::SerializeProperties(JsonWriter& Writer) const
{
    // Serialize local transform
    JTransformComponent::SerializeProperties(Writer);

    // Serialize hierarchy
    Writer.Write("children_count", static_cast<int>(m_Children.size()));
    for (size_t i = 0; i < m_Children.size(); ++i)
    {
        JsonWriter childWriter;
        m_Children[i]->Serialize(childWriter);
        Writer.Write("child_" + std::to_string(i), childWriter.GetData());
    }

    // Serialize parent by ID if exists
    Writer.Write("parent_id", m_Parent ? m_Parent->GetID() : 0);
}

void JSceneComponent::DeserializeProperties(const JsonReader& Reader)
{
    // Deserialize local transform
    JTransformComponent::DeserializeProperties(Reader);

    // Now deserialize hierarchy
    int childCount = Reader.Read("children_count", 0);
    for (int i = 0; i < childCount; ++i)
    {
        std::string key = "child_" + std::to_string(i);
        if (Reader.GetData().contains(key))
        {
            JsonReader childReader;
            childReader.GetData() = Reader.GetData()[key];
            // TODO: create child component and attach
        }
    }

    uint64_t parentID = Reader.Read("parent_id", 0);
}
