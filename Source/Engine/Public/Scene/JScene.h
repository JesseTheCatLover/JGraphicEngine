//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

#include "Core/JCoreObject.h"
#include "JActor.h"
#include "JScene.generated.h"

/**
 * @class JScene
 * @brief A container and manager for all actors in a scene.
 *
 * JScene is responsible for owning, updating, and managing actors.
 * Actors are stored internally as unique pointers for lifetime management,
 * while a fast lookup table (ID -> pointer) is used for quick access.
 *
 * Scenes also provide events for actor creation and removal, allowing
 * editor tools or gameplay systems to react dynamically.
 *
 * @note This class is read-only. To modify a scene, use SceneManager.h exclusively.
 */
JCLASS()
class JScene : public JCoreObject
{
    GENERATED_BODY()

    friend class SceneManager;

private:
    std::vector<std::unique_ptr<JActor>> m_Actors; ///< Storage of all actors in the scene.
    std::unordered_map<uint64_t, JActor*> m_ActorsByID; ///< Fast lookup map from ID -> actor.

    mutable bool m_bIsDirty = true; ///< track if cache needs rebuilding

    /**
     * @brief Called when the scene is first about to load.
     * Override for setup logic that happens before BeginPlay().
     */
    virtual void Initialize();

    /**
     * @brief Called when the scene begins.
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
     * @brief Called when the scene is being removed from the runtime memory.
     * Override for cleanup or finalization logic.
     */
    virtual void EndPlay();

    /**
     * @brief Completely destroys the scene and its actors.
     */
    virtual void DestroyScene();

    /**
     * @brief Private helper, to execute destroy on pending actors.
     */
    void FlushDestroyedActors();

    /**
     * @brief Internal helper that registers an actor into the scene’s storage.
     * @param actor The actor instance to add (ownership is transferred).
     */
    void AddActorToList(std::unique_ptr<JActor> actor);

    // Used by SceneManager to take ownership of a raw actor allocated by SerializationSubsystem (via new)
    void TakeActorOwnershipFromLoad(JActor* actor)
    {
        if (!actor) return;

        std::unique_ptr<JActor> ptr(actor);
        AddActorToList(std::move(ptr));
    }

    /**
     * @brief Spawns a new actor of type T into the scene.
     *
     * The actor is constructed with the provided arguments, assigned a unique ID,
     * and added to the scene. Ownership is managed by the scene.
     *
     * @tparam T Must be derived from JActor.
     * @tparam Args Constructor argument types.
     * @param args Constructor arguments forwarded to T’s constructor.
     * @return Pointer to the newly spawned actor.
     */
    template<typename T, typename... Args>
    T* SpawnActor(std::string name = {}, Args&&... args)
    {
        static_assert(std::is_base_of_v<JActor, T>, "T must derive from JActor");

        // Build initializer for the actor
        FObjectInitializer init{};
        init.Scene = this;
        init.Owner = nullptr;           // scene is "outer"; no owner actor
        init.Name  = std::move(name);
        init.bIsCDO = false;

        // Create via registry so construction rules apply + TLS scope is set
        JCoreObject* raw = RETypeRegistry::Get().CreateInstanceByTypeName(T::StaticREType()->name, init);
        auto* actor = dynamic_cast<T*>(raw);
        assert(actor && "SpawnActor: factory returned wrong type (or type not registered)");

        TUniquePtr<T> owned(actor);
        T* ptr = owned.get();
        AddActorToList(std::move(owned));
        return ptr;
    }

    /**
     * @brief Removes an actor from the scene.
     * @param actorPtr Pointer to the actor to remove.
     * @return true if removed successfully, false otherwise.
     */
    bool RemoveActor(JActor* actorPtr);

    /**
     * @brief Removes an actor from the scene by ID.
     * @param id The unique runtime ID of the actor to remove.
     * @return true if removed successfully, false otherwise.
     */
    bool RemoveActor(uint64_t id);

public:
    explicit JScene() = default;

    void GatherRenderables(IRenderSubmission& submission, const FRenderContext& baseCtx) const;

    JCameraComponent* GetCameraComponent() const;

    /**
     * @brief Gathers all actors in the scene.
     * @return List of raw JActor pointers.
     */
    std::vector<JActor*> ListAllActors() const
    {
        std::vector<JActor*> result;
        result.reserve(m_Actors.size());
        for (const auto& a : m_Actors)
            result.push_back(a.get());
        return result;
    }

    /**
     * @brief Finds an actor by its unique ID.
     * @param id The unique runtime ID of the actor.
     * @return Pointer to the actor, or nullptr if not found.
     */
    JActor* FindActorByID(uint64_t id);

    /**
     * @brief Finds an actor of type T by ID and casts automatically.
     * @tparam T Must be derived from JActor.
     * @param id The unique runtime ID of the actor.
     * @return Pointer to the actor of type T, or nullptr if not found or wrong type.
     */
    template<typename T>
    T* FindActorByID(uint64_t id)
    {
        static_assert(std::is_base_of<JActor, T>::value, "T must derive from JActor");
        auto it = m_ActorsByID.find(id);
        if (it == m_ActorsByID.end()) return nullptr;

        JActor* actor = it->second;
        return (actor->IsA<T>()) ? static_cast<T*>(actor) : nullptr;
    }

    /**
     * @brief Finds the first actor of type T in the scene.
     * @tparam T Must be derived from JActor.
     * @return Pointer to the first matching actor, or nullptr if none found.
     */
    template<typename T>
    T* FindActorOfType()
    {
        static_assert(std::is_base_of<JActor, T>::value, "T must derive from JActor");

        for (auto &actor: m_Actors)
        {
            if (actor->IsA<T>())
                return static_cast<T*>(actor.get()); // return first match
        }
        return nullptr; // no match found
    }

    /**
     * @brief Finds all actors of type T in the scene.
     * @tparam T Must be derived from JActor.
     * @return A vector of pointers to all matching actors.
     */
    template<typename T>
    std::vector<T *> FindActorsOfType()
    {
        static_assert(std::is_base_of<JActor, T>::value, "T must derive from JActor");

        std::vector<T*> results;
        for (auto& actor : m_Actors)
        {
            if (actor->IsA<T>())
                results.push_back(static_cast<T*>(actor.get()));
        }
        return results;
    }
};
