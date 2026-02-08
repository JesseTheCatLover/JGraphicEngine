//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/SceneComponents/JSceneComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include <algorithm>
#include "Scene/JActor.h"

static bool IsDescendantOf(const JSceneComponent* candidateChild, const JSceneComponent* possibleAncestor)
{
    if (!candidateChild || !possibleAncestor) return false;

    for (auto* p = candidateChild->GetParent(); p != nullptr; p = p->GetParent())
    {
        if (p == possibleAncestor) return true;
    }
    return false;
}

void JSceneComponent::UnlinkFromParent()
{
    if (!m_Parent) return;

    auto& siblings = m_Parent->m_Children;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    m_Parent = nullptr;
}

void JSceneComponent::MarkWorldDirty()
{
    // Already dirty? Don't spam notifications or recurse.
    if (m_WorldDirty)
        return;

    m_WorldDirty = true;

    // Let derived components react to "world transform changed"
    OnWorldTransformChanged();

    // Propagate to children so their world transforms are also considered dirty.
    for (auto* child : m_Children)
    {
        if (child)
            child->MarkWorldDirty();
    }
}

void JSceneComponent::AttachToComponent(JSceneComponent* parent)
{
    if (parent == this)
        return;

    // Prevent cycles: cannot attach under one of our descendants
    if (parent && IsDescendantOf(parent, this))
        return;

    // No-op if same parent
    if (m_Parent == parent)
        return;

    // Detach from current parent
    if (m_Parent)
        UnlinkFromParent();

    // Attach to new parent
    m_Parent = parent;
    if (m_Parent)
    {
        // Avoid duplicates
        auto& kids = m_Parent->m_Children;
        if (std::find(kids.begin(), kids.end(), this) == kids.end())
            kids.push_back(this);
    }

    // Attachment intent is now consumed
    m_PendingAttachParent = nullptr;

    MarkWorldDirty();
}

void JSceneComponent::Detach()
{
    if (m_Parent)
        UnlinkFromParent();

    // Reparent to actor root if exists and isn't this
    if (auto* owner = GetOwnerActor())
    {
        if (auto* root = owner->GetRootComponent())
        {
            if (root != this)
            {
                AttachToComponent(root);
                return;
            }
        }
    }

    // Truly detached
    m_Parent = nullptr;
    MarkWorldDirty();
}

bool JSceneComponent::DestroyComponent()
{
    if (m_bPendingDestroy)
        return false;

    m_bPendingDestroy = true;

    // Mark children too (actor will later flush/remove them from lists)
    for (auto* child : m_Children)
        if (child)
            child->DestroyComponent();

    // Important: detach from parent now to avoid parent keeping a dead pointer
    UnlinkFromParent();
    MarkWorldDirty();

    return true;
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

void JSceneComponent::PostLoad()
{
    JTransformComponent::PostLoad();

    MarkWorldDirty();
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

void JSceneComponent::BeginPlay()
{
    JTransformComponent::BeginPlay();
}

void JSceneComponent::EndPlay()
{
    JTransformComponent::EndPlay();
}

void JSceneComponent::OnDestroy()
{
    JTransformComponent::OnDestroy();
}

void JSceneComponent::Tick(float deltaTime)
{
    JTransformComponent::Tick(deltaTime);
}

void JSceneComponent::Initialize()
{
    JTransformComponent::Initialize();
}