// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/JActor.h"
#include "Scene/SceneComponents/JSceneComponent.h"
#include <algorithm>
#include "Core/JEngine.h"

#include "Scene/SceneComponents/JCameraComponent.h"
#include "Scene/SceneComponents/JModelComponent.h"

JActor::JActor() : m_VectorIndex(0)
{
    // Ensure root component exists
    SetupRootComponent();
}

void JActor::SetupRootComponent()
{
    if (!m_RootComponent)
    {
        m_RootComponent = MakeShared<JSceneComponent>();
        m_RootComponent->SetOwnerActor(this);
        m_RootComponent->SetName("RootComponent");
        m_SceneComponents.push_back(m_RootComponent);
    }
}

void JActor::Initialize()
{
    // Call Initialize on all components
    for (auto& comp : m_ActorComponents)
        comp->Initialize();
    for (auto& comp : m_SceneComponents)
        comp->Initialize();
}

void JActor::BeginPlay()
{
    // Call BeginPlay on all components
    for (auto& comp : m_ActorComponents)
        comp->BeginPlay();
    for (auto& comp : m_SceneComponents)
        comp->BeginPlay();
}

void JActor::Tick(float deltaTime)
{
    // Tick all components that are not pending destroy
    for (auto& comp : m_ActorComponents)
        if (comp && !comp->IsPendingDestroy())
            comp->Tick(deltaTime);

    for (auto& comp : m_SceneComponents)
        if (comp && !comp->IsPendingDestroy())
            comp->Tick(deltaTime);

    // After ticking, clean up destroyed components
    FlushDestroyedComponents();
}

void JActor::EndPlay()
{
    // Call EndPlay on all components
    for (auto& comp : m_ActorComponents)
        comp->EndPlay();
    for (auto& comp : m_SceneComponents)
        comp->EndPlay();
}

bool JActor::DestroyActor()
{
    if (m_bPendingDestroy)
        return false; // already requested

    m_bPendingDestroy = true;

    // Cascade to children: mark them as pending destroy too
    for (JActor* child : m_ChildActors)
    {
        if (child && !child->IsPendingDestroy())
            child->DestroyActor();
    }

    return true;
}

bool JActor::AttachToActor(JActor* newParent)
{
    if (newParent == nullptr)
    {
        DetachFromParentActor();
        return true;
    }

    if (newParent == this)
        return false; // cannot parent to self

    // Prevent simple cycles: walk up the chain from newParent
    for (JActor* p = newParent; p != nullptr; p = p->m_ParentActor)
    {
        if (p == this)
            return false; // would create a cycle
    }

    // Remove from old parent, if any
    if (m_ParentActor)
    {
        auto& siblings = m_ParentActor->m_ChildActors;
        siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());
    }

    m_ParentActor = newParent;
    if (m_ParentActor)
        m_ParentActor->m_ChildActors.push_back(this);

    // Transform parenting: attach this actor's root to parent's root
    if (m_RootComponent && m_ParentActor->m_RootComponent)
        m_RootComponent->AttachToComponent(m_ParentActor->m_RootComponent.get());

    return true;
}

void JActor::DetachFromParentActor()
{
    if (!m_ParentActor)
        return;

    auto& siblings = m_ParentActor->m_ChildActors;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());

    m_ParentActor = nullptr;

    // Detach root from the parent component; become world/root-space
    if (m_RootComponent)
        m_RootComponent->AttachToComponent(nullptr);
}

void JActor::ExecuteDestroy()
{
    DetachFromParentActor();

    // Children are already pending destroy; just clear our list so
    // we don't hold pointers longer than necessary
    m_ChildActors.clear();

    // Existing component cleanup
    for (auto& comp : m_ActorComponents)
    {
        if (!comp) continue;
        comp->EndPlay();
        comp->OnDestroy();
    }

    for (auto& comp : m_SceneComponents)
    {
        if (!comp) continue;

        comp->EndPlay();
        comp->OnDestroy();
    }

    m_ActorComponents.clear();
    m_SceneComponents.clear();
    m_RootComponent.reset();
}

void JActor::FlushDestroyedComponents()
{
    // Actor components
    for (size_t i = 0; i < m_ActorComponents.size(); )
    {
        auto& comp = m_ActorComponents[i];
        if (!comp || comp->IsPendingDestroy())
        {
            // Give it a chance to clean up
            comp->EndPlay();
            comp->OnDestroy();

            // Drop from list (swap + pop)
            if (i != m_ActorComponents.size() - 1)
                std::swap(m_ActorComponents[i], m_ActorComponents.back());

            m_ActorComponents.pop_back();
            // do NOT increment i; we just swapped a new element into this index
        }
        else
        {
            ++i;
        }
    }

    // Scene components
    for (size_t i = 0; i < m_SceneComponents.size(); )
    {
        auto& comp = m_SceneComponents[i];
        if (!comp || comp->IsPendingDestroy())
        {
            // EndPlay for safety
            comp->EndPlay();
            comp->OnDestroy();

            // If it has a parent, detach from it
            if (auto* sceneComp = comp.get())
            {
                sceneComp->UnlinkFromParent();
            }

            // Swap + pop from actor's list
            if (i != m_SceneComponents.size() - 1)
                std::swap(m_SceneComponents[i], m_SceneComponents.back());

            m_SceneComponents.pop_back();
            // DO NOT increment i
        }
        else
        {
            ++i;
        }
    }
}

void JActor::GatherRenderables(IRenderSubmission &submission, const FRenderContext &ctx) const
{
    // Actor-level culling / conditions can go here later:
    // if (!bIsVisible || IsTooFarFromCamera(...)) return;

    for (auto& comp : m_SceneComponents)
    {
        if (auto* renderable = dynamic_cast<JRenderableComponent*>(comp.get())) // TODO: Should make a custom cast for future
        {
            // Let the component turn itself into proxies / draw commands
            renderable->GatherProxies(submission, ctx);
        }
    }
}

JCameraComponent* JActor::GetCameraComponent()
{
    for (auto& comp : m_SceneComponents)
    {
        if (auto* camera = dynamic_cast<JCameraComponent*>(comp.get())) // TODO: Also a custom RTTI for future
        {
            return camera;
        }
    }
    return nullptr;
}

JREFLECT_TYPE(JActor)
{
    JPROPERTY(m_Name);
    JPROPERTY(m_VectorIndex);
    JPROPERTY(m_bIsVisible);
}}