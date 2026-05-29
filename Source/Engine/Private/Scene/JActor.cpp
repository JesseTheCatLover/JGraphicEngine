// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#include "Scene/JActor.h"
#include "Scene/SceneComponents/JSceneComponent.h"
#include <algorithm>
#include "Core/JEngine.h"

#include "Scene/SceneComponents/JCameraComponent.h"
#include "Scene/SceneComponents/JRenderableComponent.h"

JActor::JActor() : m_VectorIndex(0)
{
    // Ensure root component exists
    SetupRootComponent();
}

void JActor::SetupRootComponent()
{
    if (m_RootComponent)
        return;

    auto* root = CreateDefaultSubobject<JSceneComponent>("RootComponent");

    // Root pointer must be set BEFORE registration so ResolveAttachParent can avoid self-attach
    m_RootComponent = root;

    RegisterComponent(root, nullptr); // will NOT attach root to anything
}
void JActor::RegisterComponent(JActorComponent* comp, JSceneComponent* attachParent)
{
    assert(comp);

    // Ensure base ownership metadata is consistent
    comp->SetOwnerActor(this);

    auto contains = [](auto* ptr, const auto& vec)
    {
        return std::find(vec.begin(), vec.end(), ptr) != vec.end();
    };

    if (auto* sc = dynamic_cast<JSceneComponent*>(comp))
    {
        // Root rule: first scene component becomes root if not set
        if (!m_RootComponent)
            m_RootComponent = sc;

        if (sc == m_RootComponent)
        {
            sc->ClearPendingAttachParent();
            return; // root never goes into m_SceneComponents and never attaches
        }

        // Resolve parent using: explicit > pending SetupAttachment > root fallback
        JSceneComponent* parent = ResolveAttachParent(sc, attachParent);

        // Finalize attach (or detach if null)
        if (parent && parent != sc)
            sc->AttachToComponent(parent);
        else
            sc->AttachToComponent(nullptr);

        // Ensure pending intent is consumed even if parent ended up null
        sc->ClearPendingAttachParent();

        if (!contains(sc, m_SceneComponents))
            m_SceneComponents.push_back(sc);
    }
    else
    {
        if (!contains(comp, m_ActorComponents))
            m_ActorComponents.push_back(comp);
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

void JActor::TakeComponentOwnershipFromLoad(JActorComponent *comp, JSceneComponent *explicitAttachParent)
{
    if (!comp) return;

    comp->SetOwnerActor(this);

    // Loaded components are runtime-owned (not default subobjects)
    m_RuntimeComponentsOwned.emplace_back(TUniquePtr<JActorComponent>(comp));

    // Register exactly once; for scene components, this also finalizes attachment.
    RegisterComponent(comp, explicitAttachParent);

    // Ensure SetupAttachment doesn't interfere after explicit load wiring
    if (auto* sc = dynamic_cast<JSceneComponent*>(comp))
        sc->ClearPendingAttachParent();
}

JSceneComponent* JActor::ResolveAttachParent(JSceneComponent *sc, JSceneComponent *explicitParent) const
{
    if (!sc) return nullptr;

    // Root must never auto-attach to itself
    if (sc == m_RootComponent)
        return explicitParent; // usually nullptr

    // 1) Explicit parent passed into RegisterComponent wins
    if (explicitParent)
        return explicitParent;

    // 2) SetupAttachment intent (construction-time)
    if (auto* pending = sc->GetPendingAttachParent())
        return pending;

    // 3) Default fallback: attach to root if we have one
    return m_RootComponent;
}

void JActor::RemoveRuntimeOwnedComponent(JActorComponent *ptr)
{
    if (!ptr) return;

    auto it = std::find_if(
        m_RuntimeComponentsOwned.begin(),
        m_RuntimeComponentsOwned.end(),
        [&](const TUniquePtr<JActorComponent>& p) { return p.get() == ptr; }
    );

    if (it != m_RuntimeComponentsOwned.end())
        m_RuntimeComponentsOwned.erase(it); // deletes if runtime-owned
}

bool JActor::AttachToActor(JActor* newParent, bool bKeepWorldTransform)
{
    if (newParent == nullptr)
    {
        DetachFromParentActor(bKeepWorldTransform);
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

    // Cache current world transform only if caller wants preserve-world behavior
    const FTransform worldBefore = (bKeepWorldTransform && m_RootComponent)
        ? m_RootComponent->GetWorldTransform()
        : FTransform();

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
    {
        m_RootComponent->AttachToComponent(m_ParentActor->m_RootComponent);

        if (bKeepWorldTransform)
            m_RootComponent->SetWorldTransform(worldBefore);
        // else: keep serialized local transform as-is (load path)
    }

    return true;
}

void JActor::DetachFromParentActor(bool bKeepWorldTransform)
{
    if (!m_ParentActor)
        return;

    const FTransform worldBefore = (bKeepWorldTransform && m_RootComponent)
        ? m_RootComponent->GetWorldTransform()
        : FTransform();

    auto& siblings = m_ParentActor->m_ChildActors;
    siblings.erase(std::remove(siblings.begin(), siblings.end(), this), siblings.end());

    m_ParentActor = nullptr;

    // Detach root from the parent component; become world/root-space
    if (m_RootComponent)
    {
        m_RootComponent->AttachToComponent(nullptr);

        if (bKeepWorldTransform)
            m_RootComponent->SetWorldTransform(worldBefore);
        // else: keep local as-is (which is already world when detached load path is used intentionally)
    }
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
    m_RootComponent = nullptr;

    // Runtime-owned components get deleted here
    m_RuntimeComponentsOwned.clear();
}

void JActor::FlushDestroyedComponents()
{
    // Actor components
    for (size_t i = 0; i < m_ActorComponents.size(); )
    {
        JActorComponent* comp = m_ActorComponents[i];

        if (!comp || comp->IsPendingDestroy())
        {
            if (comp)
            {
                comp->EndPlay();
                comp->OnDestroy();
            }

            // Remove from view list (swap+pop)
            if (i != m_ActorComponents.size() - 1)
                std::swap(m_ActorComponents[i], m_ActorComponents.back());
            m_ActorComponents.pop_back();

            // Delete if runtime-owned
            RemoveRuntimeOwnedComponent(comp);
            continue;
        }

        ++i;
    }

    // Scene components
    for (size_t i = 0; i < m_SceneComponents.size(); )
    {
        JSceneComponent* comp = m_SceneComponents[i];

        if (!comp || comp->IsPendingDestroy())
        {
            if (comp)
            {
                comp->EndPlay();
                comp->OnDestroy();
                comp->UnlinkFromParent();
            }

            if (i != m_SceneComponents.size() - 1)
                std::swap(m_SceneComponents[i], m_SceneComponents.back());
            m_SceneComponents.pop_back();

            // If a scene component was runtime-owned, delete it here
            RemoveRuntimeOwnedComponent(comp);

            // If root was destroyed, clear root pointer (optional safety)
            if (comp == m_RootComponent)
                m_RootComponent = nullptr;

            continue;
        }

        ++i;
    }
}

void JActor::GatherRenderables(IRenderSubmission &submission, const FRenderContext &ctx) const
{
    // Actor-level culling / conditions can go here later:
    // if (!bIsVisible || IsTooFarFromCamera(...)) return;

    for (auto& comp : m_SceneComponents)
    {
        if (auto* renderable = dynamic_cast<JRenderableComponent*>(comp)) // TODO: Should make a custom cast for future
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
        if (auto* camera = dynamic_cast<JCameraComponent*>(comp)) // TODO: Also a custom RTTI for future
        {
            return camera;
        }
    }
    return nullptr;
}