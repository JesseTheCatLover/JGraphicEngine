//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>
#include <nlohmann/json.hpp>

class JActor;

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
class JScene
{
    friend class SceneManager;

private:
    std::string m_Name;  ///< Name of the scene (e.g. "Lake", "Level1").
    std::vector<std::unique_ptr<JActor>> m_Actors; ///< Storage of all actors in the scene.
    unsigned int m_NextActorID; ///< Auto-incremented ID counter for uniquely identifying actors.
    std::unordered_map<unsigned int, JActor*> m_ActorsByID; ///< Fast lookup map from ID → actor.

    mutable nlohmann::json m_CachedJson; ///< Cached serialization of the scene.
    mutable bool m_bIsDirty = true; ///< track if cache needs rebuilding

    /**
    * @brief Serializes the scene into a JSON object.
    *
    * Converts the entire scene, including all actors and their properties,
    * into a `nlohmann::json` object suitable for saving to disk or transmitting.
    * This includes the scene name, next actor ID, actor count, and all actor details.
    *
    * @return A JSON object representing the scene.
    */
    nlohmann::json Serialize() const; // TODO: Define a reboost serialization system later

    /**
    * @brief Deserializes a JSON object into the scene.
    *
    * Reconstructs the scene and all its actors from the given JSON data.
     * Existing actors in the scene will be cleared, and actor lookup tables
    * will be rebuilt. Actor properties such as ID, name, position, and rotation
    * will be restored from the JSON.
    *
    * @param data JSON object containing serialized scene information.
    */
    void Deserialize(const nlohmann::json &data);

    /**
     * @brief Internal helper that registers an actor into the scene’s storage.
     * @param actor The actor instance to add (ownership is transferred).
     */
    void AddActorToList(std::unique_ptr<JActor> actor);

    /**
     * @brief Construct a new JScene with the given name.
     * @param name The name of the scene.
     */
    JScene(const std::string& name);

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

        // Create actor of type T with forwarded constructor arguments
        auto actor = std::make_unique<T>(std::forward<Args>(args)...);
        actor->id = m_NextActorID++; // assign unique ID

        T *ptr = actor.get();
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
     * @brief Updates all actors in the scene.
     * @param deltaTime The time (in seconds) since the last frame.
     */
    void UpdateActors(float deltaTime);

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

        return static_cast<T *>(it->second); // assume you know the type
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
            if (T *casted = dynamic_cast<T *>(actor.get()))
                return casted; // return first match
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

        std::vector<T *> results;
        for (auto &actor: m_Actors)
        {
            if (T *casted = dynamic_cast<T *>(actor.get()))
                results.push_back(casted);
        }
        return results;
    }
};
