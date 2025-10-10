//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include "JActor.h"

#include "Core/JCoreObject.h"

/**
 * @class JScene
 * @brief A container and manager for all actors in a scene.
 *
 * JScene is responsible for owning, updating, and managing actors.
 * Actors are stored internally as unique pointers for lifetime management,
 * while a fast lookup table (ID → pointer) is used for quick access.
 *
 * Scenes also provide events for actor creation and removal, allowing
 * editor tools or gameplay systems to react dynamically.
 *
 * @note This class is read-only. To modify a scene, use SceneManager.h exclusively.
 */
class JScene : public JCoreObject
{
    DECLARE_JOBJECT(JScene)

    friend class SceneManager;

private:
    std::string m_Name;  ///< Name of the scene (e.g. "Lake", "Level1").
    std::vector<std::unique_ptr<JActor>> m_Actors; ///< Storage of all actors in the scene.
    std::unordered_map<uint64_t, JActor*> m_ActorsByID; ///< Fast lookup map from ID → actor.

    mutable nlohmann::json m_CachedJson; ///< Cached serialization of the scene.
    mutable bool m_bIsDirty = true; ///< track if cache needs rebuilding

    /**
     * @brief Construct a new JScene with the given name.
     * @param name The name of the scene.
     */
    explicit JScene(const std::string& name);

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
    virtual void Destroy();

    /**
     * @brief Internal helper that registers an actor into the scene’s storage.
     * @param actor The actor instance to add (ownership is transferred).
     */
    void AddActorToList(std::unique_ptr<JActor> actor);

    /** @brief Rename the scene. */
    void SetName(const std::string& name);

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
    T* SpawnActor(Args &&... args)
    {
        static_assert(std::is_base_of<JActor, T>::value, "T must derive from JActor");

        // Construct actor — ID is automatically assigned in JCoreObject
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);

        T* ptr = actor.get();
        AddActorToList(std::move(actor));
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
     * @param id The unique ID of the actor to remove.
     * @return true if removed successfully, false otherwise.
     */
    bool RemoveActor(unsigned int id);

public:

    /** @return The scene’s name. */
    inline const std::string& GetName() const { return m_Name;}

    /**
     * @brief Finds an actor by its unique ID.
     * @param id The unique ID of the actor.
     * @return Pointer to the actor, or nullptr if not found.
     */
    JActor* FindActorByID(unsigned int id);

    /**
     * @brief Finds an actor of type T by ID and casts automatically.
     * @tparam T Must be derived from JActor.
     * @param id The unique ID of the actor.
     * @return Pointer to the actor of type T, or nullptr if not found or wrong type.
     */
    template<typename T>
    T* FindActorByID(unsigned int id)
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

    /**
     * @brief Serializes the scene and all contained actors to JSON.
     *
     * This method writes the scene's name, actor list, and any scene-level metadata
     * into the provided JsonWriter. Each actor’s own Serialize() method is called
     * recursively, ensuring full hierarchical serialization.
     *
     * The output JSON typically includes:
     *  - Scene name
     *  - List of actors (with class type, ID, and component data)
     *  - Optional scene metadata
     *
     * @param writer Reference to the JsonWriter used for structured output.
     * @note This function does not write to disk directly. The SceneManager or Resource system handles file I/O.
     */
    void Serialize(class JsonWriter& writer) const override;

    /**
     * @brief Deserializes the scene and reconstructs all actors from JSON.
     *
     * This method reads scene data produced by Serialize(), recreating actors
     * and restoring their properties, IDs, and components. Any existing actors
     * in the scene are cleared before loading new data.
     *
     * Expected JSON structure:
     *  {
     *      "Name": "ExampleScene",
     *      "Actors": [
     *          { "Type": "JActor", "ID": 1, "Components": [...] },
     *          ...
     *      ]
     *  }
     *
     * @param reader Reference to the JsonReader providing parsed scene data.
     * @note Called automatically by SceneManager during scene loading.
     */
    void Deserialize(const class JsonReader& reader) override;
};
