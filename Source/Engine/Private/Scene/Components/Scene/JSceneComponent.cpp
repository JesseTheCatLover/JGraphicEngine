//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/JSceneComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include <algorithm>
#include "Scene/JActor.h"

void JSceneComponent::MarkWorldDirty()
{
    if (!m_WorldDirty)
    {
        m_WorldDirty = true;
        for (auto* child : m_Children)
            child->MarkWorldDirty();
    }
}

void JSceneComponent::AttachToComponent(JSceneComponent* parent)
{
    if (parent == this) return; // avoid self-attachment
    if (parent && parent->m_Parent == this) return; // avoid immediate cycle

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
    if (GetOwnerActor() && GetOwnerActor()->GetRootComponent())
    {
        AttachToComponent(GetOwnerActor()->GetRootComponent());
    }
    else
    {
        m_Parent = nullptr; // truly detached
    }
}

FTransform JSceneComponent::GetWorldTransform() const
{
    if (!m_WorldDirty) return m_WorldTransform;

    if (m_Parent)
        m_WorldTransform = m_Parent->GetWorldTransform() * GetLocalTransform();
    else
        m_WorldTransform = GetLocalTransform();

    m_WorldDirty = false; // mark self as clean

    return m_WorldTransform;
}

void JSceneComponent::SetWorldPosition(const FVector3& worldPosition)
{
    if (m_Parent)
    {
        const FTransform parentWorld = m_Parent->GetWorldTransform();
        const FTransform parentInv = parentWorld.Inverse();
        const FVector3 localPos = parentInv.ToMatrix().TransformPoint(worldPosition);
        SetLocalPosition(localPos);
    }
    else
    {
        SetLocalPosition(worldPosition);
    }

    MarkWorldDirty();
}

void JSceneComponent::SetWorldRotation(const FQuat& worldRotation)
{
    if (m_Parent)
    {
        const FQuat parentWorldRot = m_Parent->GetWorldRotationAsQuat();
        const FQuat localRot = parentWorldRot.Inverse() * worldRotation;
        SetLocalRotation(localRot);
    }
    else
    {
        SetLocalRotation(worldRotation);
    }

    MarkWorldDirty();
}

void JSceneComponent::SetWorldRotation(const FRotator &worldRotation)
{
    SetWorldRotation(FQuat::MakeFromRotator(worldRotation));
}

void JSceneComponent::SetWorldScale(const FVector3& worldScale)
{
    if (m_Parent)
    {
        const FVector3 parentWorldScale = m_Parent->GetWorldTransform().GetScale();
        SetLocalScale(worldScale / parentWorldScale);
    }
    else
    {
        SetLocalScale(worldScale);
    }

    MarkWorldDirty();
}

void JSceneComponent::SetWorldTransform(const FTransform& worldTransform)
{
    if (m_Parent)
    {
        const FTransform parentWorld = m_Parent->GetWorldTransform();
        const FTransform parentInv = parentWorld.Inverse();
        const FTransform local = FTransform::MakeFromMatrix((parentInv.ToMatrix() * worldTransform.ToMatrix()));
        SetLocalTransform(local);
    }
    else
    {
        SetLocalTransform(worldTransform);
    }

    MarkWorldDirty();
}

void JSceneComponent::OnAttachment()
{
    JTransformComponent::OnAttachment();
}

void JSceneComponent::SerializeProperties(JsonWriter& writer) const
{
    // Serialize local transform
    JTransformComponent::SerializeProperties(writer);

    // Serialize all children
    for (auto* child : m_Children)
    {
        JsonWriter childWriter;
        child->Serialize(childWriter);
        writer.WriteObjectToArray("children", childWriter.GetData());
    }

    // Serialize parent reference
    writer.Write("parent_id", m_Parent ? m_Parent->GetID() : 0);
}

void JSceneComponent::DeserializeProperties(const JsonReader& reader)
{
    // Deserialize local transform
    JTransformComponent::DeserializeProperties(reader);

    // Deserialize children
    if (reader.GetData().contains("children"))
    {
        const auto& children = reader.GetData()["children"];
        for (const auto& childData : children)
        {
            JsonReader childReader(childData);

            /* auto* child = new JSceneComponent(); // TODO: do it via factory
            child->Deserialize(childReader);
            child->AttachToComponent(this); */
        }
    }

    uint64_t parentID = reader.Read("parent_id", 0);
    // (optional) store for later parent fixup
}
