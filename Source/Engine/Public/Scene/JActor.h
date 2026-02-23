// Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/JCoreObject.h"
#include "SceneComponents/JSceneComponent.h"
#include <memory>
#include <vector>
#include <string>
#include <type_traits>
#include "Core/Memory/SmartPointers.h"
#include "JActor.generated.h"

class JCameraComponent;
struct FRenderContext;
class IRenderSubmission;
class JModelComponent;
class JShader;
class JActorComponent;
class JScene;

/**
 * @class JActor
 * @brief Base class for all objects that can exist in a scene.
 *
 * JActor represents any entity that can be placed into a scene.
 * Each actor owns a root scene component defining its transform
 * and may contain additional logic or scene components for behavior.
 */
JCLASS()
class JActor : public JCoreObject
{
    GENERATED_BODY()

    friend class JScene;
    friend class SceneManager;

private:
    JPROPERTY(HiddenInInspector)
    size_t m_VectorIndex; ///< internal index for O(1) removal from scene

    JSceneComponent* m_RootComponent = nullptr; ///< Root of the scene component hierarchy
    std::vector<JSceneComponent*> m_SceneComponents; ///< Scene components attached to this actor
    std::vector<JActorComponent*> m_ActorComponents; ///< Actor components attached to this actor

    // ---- Runtime ownership (OWNING) ----
    // Default subobjects are owned by JCoreObject (m_DefaultSubobjectsOwned).
    // Runtime-added components must be owned here.
    std::vector<TUniquePtr<JActorComponent>> m_RuntimeComponentsOwned;

    JPROPERTY(HiddenToInspector, EditAnywhere, Scriptable)
    bool m_bIsVisible = true;

    bool m_bPendingDestroy = false;

    JActor* m_ParentActor = nullptr;
    std::vector<JActor*> m_ChildActors;

private:
    /**
    * @brief Initializes and assigns the root scene component for this actor.
    *
    * Every actor must have a root scene component to define its transform
    * hierarchy. This function is responsible for creating or assigning
    * the root component and ensuring it is registered in the actor's
    * internal scene component list.
    *
    * @note This should only be called once during actor construction.
    *       Runtime modifications of the root component are not recommended.
    *
    * After calling this function:
    *  - m_RootComponent will be non-null.
    *  - Newly added scene components without a specified parent
    *    will attach to this root by default.
    */
    void SetupRootComponent();

    // Central registration path (used by default + runtime + load)
    void RegisterComponent(JActorComponent* comp, JSceneComponent* attachParent = nullptr);

    void FlushDestroyedComponents();

    /**
     * @brief Only called internally from JScene, and completely destroys the actor and its components.
     */
    virtual void ExecuteDestroy();

    [[nodiscard]] std::vector<JSceneComponent*> ListSceneComponentsRaw() const
    {
        std::vector<JSceneComponent*> result;
        result.reserve(m_SceneComponents.size());
        for (const auto& comp : m_SceneComponents)
            if (comp)
                result.push_back(comp);
        return result;
    }

    [[nodiscard]] std::vector<JActorComponent*> ListActorComponentsRaw() const
    {
        std::vector<JActorComponent*> result;
        result.reserve(m_ActorComponents.size());
        for (const auto& comp : m_ActorComponents)
            if (comp)
                result.push_back(comp);
        return result;
    }

    // For load: restore parent relationship and register
    void TakeComponentOwnershipFromLoad(JActorComponent* comp, JSceneComponent* explicitAttachParent = nullptr);

    JSceneComponent* ResolveAttachParent(JSceneComponent* sc, JSceneComponent* explicitParent) const;
    void RemoveRuntimeOwnedComponent(JActorComponent* ptr);

public:
    JActor();
    virtual ~JActor() = default;

    // -------------------- Playtime API --------------------

    /**
     * @brief Called when the actor is first created or spawned.
     * Override for setup logic that happens before BeginPlay().
     */
    virtual void Initialize();

    /**
     * @brief Called when the actor starts "playing".
     * Override to implement runtime behavior that happens when the scene begins.
     */
    virtual void BeginPlay();

    /**
     * @brief Called every frame while the actor is active.
     * Override to implement per-frame logic.
     * @param deltaTime Time since last frame in seconds.
     */
    virtual void Tick(float deltaTime);

    /**
     * @brief Called when the actor is being removed from the scene.
     * Override for cleanup or finalization logic.
     */
    virtual void EndPlay();

    // -------------------- Lifecycle API --------------------

    /**
     * @brief Marks this actor to be destroyed for the next frame.
     * @return False, if already requested; True if requested for the first time.
     */
    virtual bool DestroyActor();

    /**
     * @return True if listed in pending destroy
     */
    [[nodiscard]] bool IsPendingDestroy() const { return m_bPendingDestroy; }

    // -------------------- Root Transform API --------------------

    [[nodiscard]] bool IsRootActor() const
    {
        return m_ParentActor == nullptr;
    }

    [[nodiscard]] JSceneComponent* GetRootComponent() const { return m_RootComponent; }
    void SetRootComponent(JSceneComponent* root) { m_RootComponent = root; }

    /** Attach this actor under another actor (parental actor hierarchy). */
    bool AttachToActor(JActor* newParent, bool bKeepWorldTransform = true);

    /** Detach from parent actor and become a root actor again. */
    void DetachFromParentActor(bool bKeepWorldTransform = true);

    // -------------------- Transform API (world space) --------------------

    /** World location of the actor’s root. */
    [[nodiscard]] FVector3 GetActorLocation() const
    {
        return m_RootComponent ? m_RootComponent->GetWorldPosition() : FVector3(0.f);
    }

    /** Set world location of the actor. */
    void SetActorLocation(const FVector3& worldLocation)
    {
        if (m_RootComponent)
            m_RootComponent->SetWorldPosition(worldLocation);
    }

    /** Set world location of the actor. */
    void SetActorLocation(float x, float y, float z)
    {
        SetActorLocation(FVector3(x, y, z));
    }

    /** Add a world‐space offset to the actor. */
    void AddActorWorldOffset(const FVector3& delta)
    {
        if (!m_RootComponent) return;
        SetActorLocation(GetActorLocation() + delta);
    }

    /** Add a world-space offset to the actor. */
    void AddActorWorldOffset(float x, float y, float z)
    {
        AddActorWorldOffset(FVector3(x, y, z));
    }

    /** World rotation (as FRotator) of the actor. */
    [[nodiscard]] FRotator GetActorRotation() const
    {
        return m_RootComponent ? m_RootComponent->GetWorldRotationAsRotator() : FRotator{};
    }

    /** Set world rotation (FRotator) of the actor. */
    void SetActorRotation(const FRotator& worldRot)
    {
        if (m_RootComponent)
            m_RootComponent->SetWorldRotation(worldRot);
    }

    /** Set world rotation (FQuat) of the actor. */
    void SetActorRotation(const FQuat& worldRot)
    {
        if (m_RootComponent)
            m_RootComponent->SetWorldRotation(worldRot);
    }

    /** Set world rotation from pitch/yaw/roll (degrees). */
    void SetActorRotation(float pitch, float yaw, float roll)
    {
        SetActorRotation(FRotator(pitch, yaw, roll));
    }

    /** World rotation (as quaternion) of the actor. */
    [[nodiscard]] FQuat GetActorQuat() const
    {
        return m_RootComponent ? m_RootComponent->GetWorldRotationAsQuat() : FQuat();
    }

    /** Set world rotation (quaternion) of the actor. */
    void SetActorQuat(const FQuat& worldQuat)
    {
        if (m_RootComponent)
            m_RootComponent->SetWorldRotation(worldQuat);
    }

    /** Add a world‐space rotation (FRotator). */
    void AddActorWorldRotation(const FRotator& deltaRot)
    {
        SetActorRotation(GetActorRotation() + deltaRot);
    }

    /** Add world rotation delta from pitch/yaw/roll (degrees). */
    void AddActorWorldRotation(float pitch, float yaw, float roll)
    {
        AddActorWorldRotation(FRotator(pitch, yaw, roll));
    }

    /** World scale of the actor (from root component). */
    [[nodiscard]] FVector3 GetActorScale() const
    {
        return m_RootComponent ? m_RootComponent->GetWorldTransform().GetScale()
                               : FVector3(1.f);
    }

    /** Set world scale of the actor. */
    void SetActorScale(const FVector3& worldScale)
    {
        if (m_RootComponent)
            m_RootComponent->SetWorldScale(worldScale);
    }

    /** Full world transform of the actor. */
    [[nodiscard]] FTransform GetActorTransform() const
    {
        return m_RootComponent ? m_RootComponent->GetWorldTransform()
                               : FTransform();
    }

    /** Set full world transform of the actor. */
    void SetActorTransform(const FTransform& worldTransform)
    {
        if (m_RootComponent)
            m_RootComponent->SetWorldTransform(worldTransform);
    }

    // -------------------- Transform API (relative/local space) --------------------

    /** Local (relative) location of the actor’s root component. */
    [[nodiscard]] FVector3 GetActorRelativeLocation() const
    {
        return m_RootComponent ? m_RootComponent->GetLocalPosition() : FVector3(0.f);
    }

    /** Set local (relative) location of the actor’s root component. */
    void SetActorRelativeLocation(const FVector3& relLocation)
    {
        if (m_RootComponent)
            m_RootComponent->SetLocalPosition(relLocation);
    }

    /** Set local (relative) location of the actor’s root component. */
    void SetActorRelativeLocation(float x, float y, float z)
    {
        SetActorRelativeLocation(FVector3(x, y, z));
    }

    /** Add a local‐space offset to the actor. */
    void AddActorLocalOffset(const FVector3& delta)
    {
        if (!m_RootComponent) return;
        SetActorRelativeLocation(GetActorRelativeLocation() + delta);
    }

    /** Add a local-space offset to the actor. */
    void AddActorLocalOffset(float x, float y, float z)
    {
        AddActorLocalOffset(FVector3(x, y, z));
    }

    /** Local rotation (as FRotator) of the actor. */
    [[nodiscard]] FRotator GetActorRelativeRotation() const
    {
        return m_RootComponent
            ? m_RootComponent->GetLocalRotationAsRotator() : FRotator{};
    }

    /** Set local rotation from pitch/yaw/roll (degrees). */
    void SetActorRelativeRotation(float pitch, float yaw, float roll)
    {
        SetActorRelativeRotation(FRotator(pitch, yaw, roll));
    }

    /** Set local rotation (as FRotator) of the actor. */
    void SetActorRelativeRotation(const FRotator& relRot)
    {
        if (m_RootComponent)
            m_RootComponent->SetLocalRotation(relRot.ToQuat());
    }

    /** Local rotation (as quaternion) of the actor. */
    [[nodiscard]] FQuat GetActorRelativeQuat() const
    {
        return m_RootComponent ? m_RootComponent->GetLocalRotationAsQuat() : FQuat();
    }

    /** Set local rotation (quaternion) of the actor. */
    void SetActorRelativeQuat(const FQuat& worldQuat)
    {
        if (m_RootComponent)
            m_RootComponent->SetLocalRotation(worldQuat);
    }

    /** Add a local‐space rotation (FRotator). */
    void AddActorRelativeRotation(const FRotator& deltaRot)
    {
        SetActorRelativeRotation(GetActorRelativeRotation() + deltaRot);
    }

    /** Add local rotation delta from pitch/yaw/roll (degrees). */
    void AddActorRelativeRotation(float pitch, float yaw, float roll)
    {
        AddActorRelativeRotation(FRotator(pitch, yaw, roll));
    }

    /** Local scale of the actor (from root component). */
    [[nodiscard]] FVector3 GetActorRelativeScale() const
    {
        return m_RootComponent ? m_RootComponent->GetLocalScale() : FVector3(1.f);
    }

    /** Set local scale of the actor (from root component). */
    void SetActorRelativeScale(const FVector3& relScale)
    {
        if (m_RootComponent)
            m_RootComponent->SetLocalScale(relScale);
    }

    /** Full local transform of the actor. */
    [[nodiscard]] FTransform GetActorRelativeTransform() const
    {
        return m_RootComponent ? m_RootComponent->GetLocalTransform()
                               : FTransform();
    }

    /** Set full local transform of the actor. */
    void SetActorRelativeTransform(const FTransform& relTransform)
    {
        if (m_RootComponent)
            m_RootComponent->SetLocalTransform(relTransform);
    }

    // -------------------- Component API --------------------

    /**
    * @brief Create and register a default component for this actor.
    *
    * Called only during actor construction, to define the actor's
    * default component hierarchy before the game begins.
    *
    * Components created through this function are automatically added to the
    * correct internal list:
    *  - If @p T derives from JSceneComponent, it is attached to the root
    *    scene component.
    *  - Otherwise, it is added as an actor component.
    *
    * @tparam T Component type to create (must derive from JActorComponent)
    * @param name Component name for identification or debugging
    * @return Raw pointer to the created component (Non-owning)
    *
    * @note This is intended for defining an actor's built-in components in the constructor,
    * not for spawning or adding components dynamically at runtime.
    */
    template<typename T>
    T* CreateDefaultComponent(const std::string& name)
    {
        static_assert(std::is_base_of_v<JActorComponent, T>, "T must derive from JActorComponent");

        T* comp = CreateDefaultSubobject<T>(name);

        RegisterComponent(comp);
        return comp;
    }

    /**
    * @brief Dynamically create and add a component to this actor at runtime.
    *
    * Unlike CreateDefaultComponent, this is intended for runtime additions.
    * Ownership is retained by the actor, and the component is automatically
    * registered in the proper internal list.
    *
    * @tparam T Component type (must derive from JActorComponent)
    * @tparam Args Constructor argument types
    * @param args Arguments forwarded to T's constructor
    * @param attachParent Optional parent for scene components (defaults to root scene component)
    * @return Raw pointer to the created component (Non-owning)
    */
    template<typename T, typename... Args>
    T* AddRuntimeComponent(const std::string& name, JSceneComponent* attachParent = nullptr, Args&&... args)
    {
        static_assert(std::is_base_of_v<JActorComponent, T>, "T must derive from JActorComponent");

        // Build Init (TLS is nice but not required for runtime add; still we can push it)
        FObjectInitializer init{};
        init.Scene = GetOwningScene();
        init.Owner = this;
        init.Name  = name;
        init.bIsCDO = false;

        // Construct directly using Init if ctor exists; else fallback
        TUniquePtr<T> owned;

        if constexpr (std::is_constructible_v<T, const FObjectInitializer&, Args...>)
            owned = MakeUnique<T>(init, std::forward<Args>(args)...);
        else if constexpr (std::is_constructible_v<T, Args...>)
            owned = MakeUnique<T>(std::forward<Args>(args)...);
        else
            static_assert(sizeof(T) == 0, "AddRuntimeComponent: no compatible ctor");

        T* ptr = owned.get();

        // Ensure base fields are correct even if fallback ctor was used
        ptr->SetOwnerActor(this);

        // Ownership + view registration
        m_RuntimeComponentsOwned.emplace_back(std::move(owned));
        RegisterComponent(ptr, attachParent);
        return ptr;
    }

    /**
    * @brief Retrieve the first component of type T attached to this actor.
    *
    * Searches both actor components and scene components for the first component
    * of the specified type. Always returns nullptr if not found.
    *
    * @tparam T Component type to search for
    * @return Pointer to the first matching component, or nullptr if none found
    */
    template<typename T>
    T* GetComponent() const
    {
        for (auto* c : m_ActorComponents)
            if (auto* casted = dynamic_cast<T*>(c)) return casted;

        for (auto* c : m_SceneComponents)
            if (auto* casted = dynamic_cast<T*>(c)) return casted;

        return nullptr;
    }

    [[nodiscard]] const std::vector<JActorComponent*>& GetActorComponents() const { return m_ActorComponents; }
    [[nodiscard]] const std::vector<JSceneComponent*>& GetSceneComponents() const { return m_SceneComponents; }

    // -------------------- Rendering --------------------

    void GatherRenderables(IRenderSubmission& submission, const FRenderContext& ctx) const;

    JCameraComponent* GetCameraComponent();

    // -------------------- Getter/Setter --------------------

    [[nodiscard]] size_t GetVectorIndex() const { return m_VectorIndex; }
    void SetVectorIndex(size_t index) { m_VectorIndex = index; }

    [[nodiscard]] bool IsVisible() const { return m_bIsVisible; }
    void SetVisible(bool bIsVisible) { m_bIsVisible = bIsVisible; }

    [[nodiscard]] std::string GetActorName() const { return GetObjectName(); }
    void SetActorName(const std::string& n) { SetObjectName(n); }

    [[nodiscard]] JActor* GetParentActor() const { return m_ParentActor; }
    [[nodiscard]] const std::vector<JActor*>& GetChildActors() const { return m_ChildActors; }
};