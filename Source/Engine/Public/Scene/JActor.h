// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/JCoreObject.h"
#include "SceneComponents/JSceneComponent.h"
#include <memory>
#include <vector>
#include <string>
#include <type_traits>
#include "Core/Memory/SmartPointers.h"

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
class JActor : public JCoreObject
{
    DECLARE_JOBJECT(JActor)

    friend class JScene;
    friend class SceneManager;

private:
    std::string m_Name; ///< Actor name
    size_t m_VectorIndex; ///< internal index for O(1) removal from scene
    TSharedPtr<JSceneComponent> m_RootComponent; ///< Root of the scene component hierarchy
    std::vector<TSharedPtr<JSceneComponent>> m_SceneComponents; ///< Scene components attached to this actor
    std::vector<TSharedPtr<JActorComponent>> m_ActorComponents; ///< Actor components attached to this actor

    bool m_bPendingDestroy = false;
    JScene* m_OwningScene = nullptr; // Set by JScene when adding actor

    void FlushDestroyedComponents();

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

    /**
     * @brief Only called internally from JScene, and completely destroys the actor and its components.
     */
    virtual void ExecuteDestroy();

    [[nodiscard]] std::vector<JSceneComponent*> GetSceneComponentsRaw() const
    {
        std::vector<JSceneComponent*> result;
        result.reserve(m_SceneComponents.size());
        for (const auto& comp : m_SceneComponents)
            if (comp)
                result.push_back(comp.get());
        return result;
    }

    [[nodiscard]] std::vector<JActorComponent*> GetActorComponentsRaw() const
    {
        std::vector<JActorComponent*> result;
        result.reserve(m_ActorComponents.size());
        for (const auto& comp : m_ActorComponents)
            if (comp)
                result.push_back(comp.get());
        return result;
    }

    // Attach an already-constructed logic component (from load) to this actor
    void AttachActorComponentFromLoad(JActorComponent* comp)
    {
        if (!comp) return;

        comp->SetOwnerActor(this);

        // Take ownership via TSharedPtr
        TSharedPtr<JActorComponent> ptr(comp);
        m_ActorComponents.push_back(std::move(ptr));
    }

    // Attach an already-constructed scene component (from load) to this actor
    void AttachSceneComponentFromLoad(JSceneComponent* comp, JSceneComponent* parent = nullptr)
    {
        if (!comp) return;

        comp->SetOwnerActor(this);

        // Wrap raw pointer
        TSharedPtr<JSceneComponent> ptr(comp);

        // Restore parent relationship (for now: parent or root)
        if (parent)
            comp->AttachToComponent(parent);
        else if (m_RootComponent)
            comp->AttachToComponent(m_RootComponent.get());

        m_SceneComponents.push_back(std::move(ptr));
    }

public:
    JActor();
    virtual ~JActor() = default;

    // -------------------- Lifecycle --------------------

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

    /**
     * @brief Marks this actor to be destroyed for the next frame.
     * @return False, if already requested; True if requested for the first time.
     */
    virtual bool DestroyActor();

    /**
     * @return True if listed in pending destroy
     */
    [[nodiscard]] bool IsPendingDestroy() const { return m_bPendingDestroy; }

    // -------------------- Actor API --------------------

    bool IsRootActor() const
    {
        // For now: every actor is treated as root.
        // TODO: Later, change this when you implement parent/child actors.
        return true;
    }

    [[nodiscard]] FVector3 GetActorPosition() const
    {
        return m_RootComponent ? m_RootComponent->GetWorldPosition() : FVector3(0);
    }

    void SetActorPosition(const FVector3& pos)
    {
        if (m_RootComponent) m_RootComponent->SetWorldPosition(pos);
    }

    void SetActorPosition(float x, float y, float z)
    {
        SetActorPosition(FVector3(x, y, z));
    }

    [[nodiscard]] FRotator GetActorRotation() const
    {
        return m_RootComponent ? m_RootComponent->GetWorldRotationAsRotator() : FRotator{};
    }


    void SetActorRotation(const FRotator& rotator)
    {
        if (m_RootComponent) m_RootComponent->SetWorldRotation(rotator.ToQuat());
    }

    // -------------------- Component API --------------------

    /**
    * @brief Create and register a default component for this actor.
    *
    * Called only during actor construction, to define the actor's
    * default component hierarchy before the game begins.
    *
    * Components created through this function are owned by the actor
    * and automatically added to the correct internal list:
    *  - If @p T derives from JSceneComponent, it is attached to the root
    *    scene component.
    *  - Otherwise, it is added as an actor component.
    *
    * @tparam T Component type to create (must derive from JActorComponent)
    * @tparam Args Constructor argument types
    * @param name Component name for identification or debugging
    * @param args Arguments forwarded to T's constructor
    * @return Raw pointer to the created component (Non-owning)
    *
    * @note This is intended for defining an actor's built-in components in the constructor,
    * not for spawning or adding components dynamically at runtime.
    */
    template<typename T, typename... Args>
    T* CreateDefaultComponent(const std::string& name, Args&&... args)
    {
        static_assert(std::is_base_of_v<JActorComponent, T>, "T must derive from JActorComponent");

        // Create the component
        auto component = MakeShared<T>(std::forward<Args>(args)...);
        component->SetOwnerActor(this);
        component->SetName(name);

        // Handle special case for JSceneComponent
        if constexpr (std::is_base_of_v<JSceneComponent, T>)
        {
            auto sceneComp = std::static_pointer_cast<JSceneComponent>(component);
            m_SceneComponents.push_back(sceneComp);
        }
        else
        {
            m_ActorComponents.push_back(component);
        }

        return component.get();
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
    * @param parent Optional parent for scene components (defaults to root scene component)
    * @return Raw pointer to the created component (Non-owning)
    */
    template<typename T, typename... Args>
    T* AddRuntimeComponent(Args&&... args, JSceneComponent* parent = nullptr)
    {
        static_assert(std::is_base_of_v<JActorComponent, T>, "T must derive from JActorComponent");

        // Create the component
        auto component = MakeShared<T>(std::forward<Args>(args)...);
        component->SetOwnerActor(this);

        if constexpr (std::is_base_of_v<JSceneComponent, T>)
        {
            auto sceneComp = std::static_pointer_cast<JSceneComponent>(component);

            // Attach to specified parent or root by default
            if (parent)
                sceneComp->AttachToComponent(parent);
            else
                sceneComp->AttachToComponent(m_RootComponent.get());

            m_SceneComponents.push_back(sceneComp);
        }
        else
        {
            m_ActorComponents.push_back(component);
        }

        return component.get();
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
        // Search actor components
        for (auto& comp : m_ActorComponents)
            if (auto casted = dynamic_cast<T*>(comp.get()))
                return casted;

        // Search scene components
        for (auto& comp : m_SceneComponents)
            if (auto casted = dynamic_cast<T*>(comp.get()))
                return casted;

        return nullptr;
    }

    // -------------------- Root & Transform --------------------

    [[nodiscard]] JSceneComponent* GetRootComponent() const { return m_RootComponent.get(); }
    void SetRootComponent(TSharedPtr<JSceneComponent> root) { m_RootComponent = std::move(root); }

    // -------------------- Rendering --------------------

    void GatherRenderables(IRenderSubmission& submission, const FRenderContext& ctx) const; // TODO: This is temporarily here

    JCameraComponent* GetCameraComponent();

    // -------------------- Getter/Setter --------------------

    [[nodiscard]] std::string GetName() const { return m_Name; }
    void SetName(const std::string& name) { m_Name = name; }

    [[nodiscard]] size_t GetVectorIndex() const { return m_VectorIndex; }
    void SetVectorIndex(size_t index) { m_VectorIndex = index; }

    [[nodiscard]] JScene* GetOwningScene() const { return m_OwningScene; }
};