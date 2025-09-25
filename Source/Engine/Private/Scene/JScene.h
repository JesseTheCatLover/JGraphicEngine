//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <functional>
#include <vector>
#include <memory>
#include <unordered_map>

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
 */
class JScene
{
private:
    std::string m_Name;  ///< Name of the scene (e.g. "Lake", "Level1").
    std::vector<std::unique_ptr<JActor>> m_Actors; ///< Storage of all actors in the scene.
    unsigned int m_NextActorID; ///< Auto-incremented ID counter for uniquely identifying actors.
    std::unordered_map<unsigned int, JActor*> m_ActorsByID; ///< Fast lookup map from ID → actor.

    /**
     * @brief Internal helper that registers an actor into the scene’s storage.
     * @param actor The actor instance to add (ownership is transferred).
     */
    void AddActorToList(std::unique_ptr<JActor> actor);

public:
    /**
     * @brief Construct a new JScene with the given name.
     * @param name The name of the scene.
     */
    JScene(const std::string& name);

    /** @return The scene’s name. */
    inline const std::string& GetName() const { return m_Name;}

    /** @brief Rename the scene. */
    inline void SetName(const std::string& name) { m_Name = name; }

    /**
     * @brief Updates all actors in the scene.
     * @param deltaTime The time (in seconds) since the last frame.
     */
    void UpdateActors(float deltaTime);

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
    T* SpawnActor(Args&&... args);

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
    T* FindActorByID(unsigned int id);

    /**
     * @brief Finds the first actor of type T in the scene.
     * @tparam T Must be derived from JActor.
     * @return Pointer to the first matching actor, or nullptr if none found.
     */
    template<typename T>
    T* FindActorOfType();

    /**
     * @brief Finds all actors of type T in the scene.
     * @tparam T Must be derived from JActor.
     * @return A vector of pointers to all matching actors.
     */
    template<typename T>
    std::vector<T*> FindActorsOfType();

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

    /** Callback invoked whenever an actor is added to the scene. */
    std::function<void(JActor*)> OnActorAdded;

    /** Callback invoked whenever an actor is removed from the scene. */
    std::function<void(JActor*)> OnActorRemoved;
};
