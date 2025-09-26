#pragma once
#include <memory>
#include <unordered_map>
#include <string>
#include "Scene/JScene.h"

/**
 * @class JSceneManager
 * @brief Global interface for creating, managing, and accessing scenes.
 *
 * JSceneManager owns all JScene instances and provides a single point of access
 * for gameplay and editor systems. All scene operations (spawning actors, removing actors,
 * updating actors, querying actors) go through the active scene managed by this class.
 *
 * This class also exposes editor-friendly callbacks for actor addition/removal events.
 */
class JSceneManager
{
private:
    std::unordered_map<std::string, std::unique_ptr<JScene>> m_Scenes; ///< Name → Scene lookup
    JScene* m_ActiveScene = nullptr; ///< Currently active scene

public:
    /** @brief Default constructor. */
    JSceneManager() = default;

    /**
     * @brief Create a new scene with a given name.
     * @param name Name of the scene.
     * @return Pointer to the newly created scene.
     */
    JScene* CreateScene(const std::string& name);

    /**
     * @brief Sets the active scene by name.
     * @param name Name of the scene to activate.
     */
    void SetActiveScene(const std::string& name);

    /** @brief Returns a pointer to the currently active scene. */
    JScene* GetActiveScene() const { return m_ActiveScene; }

    // -------------------- Actor Operations --------------------

    /**
     * @brief Spawn a new actor in the active scene.
     * @tparam T Actor type (must derive from JActor)
     * @tparam Args Constructor argument types
     * @param args Arguments forwarded to T’s constructor
     * @return Pointer to the newly spawned actor, or nullptr if no active scene
     */
    template<typename T, typename... Args>
    T* SpawnActor(Args &&... args)
    {
        if (!m_ActiveScene) return nullptr;
        T* actor = m_ActiveScene->SpawnActor<T>(std::forward<Args>(args)...);
        if (actor && OnActorAdded) OnActorAdded(actor);
        return actor;
    }

    /**
     * @brief Find an actor by ID in the active scene.
     * @param id Unique actor ID
     * @return Pointer to actor, or nullptr if not found or no active scene
     */
    JActor* FindActorByID(unsigned int id);

    /**
     * @brief Find an actor of type T by ID in the active scene.
     * @tparam T Actor type (must derive from JActor)
     * @param id Unique actor ID
     * @return Pointer to actor of type T, or nullptr if not found or wrong type
     */
    template<typename T>
    T* FindActorByID(unsigned int id)
    {
        if (!m_ActiveScene) return nullptr;
        return m_ActiveScene->FindActorByID<T>(id);
    }

    /**
     * @brief Find the first actor of type T in the active scene.
     * @tparam T Actor type (must derive from JActor)
     * @return Pointer to first matching actor, or nullptr if none found
     */
    template<typename T>
    T* FindActorOfType()
    {
        if(!m_ActiveScene) return nullptr;
        return m_ActiveScene->FindActorOfType<T>();
    }

    /**
     * @brief Find all actors of type T in the active scene.
     * @tparam T Actor type (must derive from JActor)
     * @return Vector of pointers to all matching actors, empty if none found
     */
    template<typename T>
    std::vector<T*> FindActorsOfType()
    {
        if (!m_ActiveScene) return {};
        return m_ActiveScene->FindActorsOfType<T>();
    }

    /**
     * @brief Remove an actor by pointer in the active scene.
     * @param actorPtr Pointer to the actor to remove
     * @return true if removed successfully, false otherwise
     */
    bool RemoveActor(JActor* actorPtr);

    /**
     * @brief Remove an actor by ID in the active scene.
     * @param id Unique actor ID
     * @return true if removed successfully, false otherwise
     */
    bool RemoveActor(unsigned int id);

    /**
     * @brief Update all actors in the active scene.
     * @param deltaTime Time since last frame
     */
    void Update(float deltaTime);

    // -------------------- Editor Callbacks --------------------
    /** Callback invoked whenever an actor is added in the active scene. */
    std::function<void(JActor*)> OnActorAdded;

    /** Callback invoked whenever an actor is removed from the active scene. */
    std::function<void(JActor*)> OnActorRemoved;
};