//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <string>
#include "Scene/JScene.h"

struct FSceneSaveInfo;
struct FSceneLoadResult;

/**
 * @struct FSceneMeta
 * @brief Metadata container for a scene file.
 *
 * SceneMeta stores lightweight information about a scene without
 * fully loading it into memory. Useful for editor lists, previews,
 * or file management systems.
 */
struct FSceneMeta
{
    /** @brief Name of the scene. */
    std::string name;

    /** @brief Number of actors currently in the scene. */
    int actorCount = 0;

    /** @brief Path or identifier for a thumbnail image representing the scene. */
    std::string thumbnail;

    /** @brief Timestamp of the last modification of the scene file (human-readable). */
    std::string lastModified;
};

/**
 * @class SceneManager
 * @brief Global interface for accessing and managing the active scene.
 *
 * SceneManager provides a single point of access to the currently active Scene
 * for gameplay and editor systems. All scene operations—such as spawning actors,
 * removing actors, updating actors, and querying actors—must go through this manager.
 *
 * The class also handles loading and saving scenes, and provides scene metadata
 * that can be fed into editor tools or other systems for inspection or manipulation.
 *
 * Editor-friendly callbacks for actor addition and removal events are exposed,
 * allowing tools and systems to react dynamically.
 */

class SceneManager
{
    friend class JEngine;

private:
    SceneManager() = default;

    std::unique_ptr<JScene> m_ActiveScene; ///< Currently active scene

    /**
     * @brief Passes Tick to all actors in the active scene.
     * @param deltaTime Time since last frame
     */
    void Tick(float deltaTime);

    void BuildSaveInfoFromScene(const JScene *scene, FSceneSaveInfo &outInfo) const;

    void ApplyLoadedResultToScene(const FSceneLoadResult &loadResult, JScene &scene);

public:
    ~SceneManager() = default;

    // Non-copyable / non-movable
    SceneManager(const SceneManager &) = delete;

    SceneManager &operator=(const SceneManager &) = delete;

    SceneManager(SceneManager &&) = delete;

    SceneManager &operator=(SceneManager &&) = delete;

    /** @brief Returns a pointer to the currently active scene. */
    [[nodiscard]] JScene *GetActiveScene() const { return m_ActiveScene.get(); }

    // -------------------- Actor Runtime API --------------------

    /**
     * @brief Spawn a new actor in the active scene.
     * @tparam T Actor type (must derive from JActor)
     * @param name Optional actor name.
     * @tparam Args Constructor argument types
     * @param args Arguments forwarded to T’s constructor
     * @return Pointer to the newly spawned actor, or nullptr if no active scene
     */
    template<typename T, typename... Args>
    T* SpawnActor(std::string name = {}, Args&&... args)
    {
        if (!m_ActiveScene) return nullptr;

        // Forward name first, then args
        T* actor = m_ActiveScene->SpawnActor<T>(std::move(name), std::forward<Args>(args)...);

        if (actor && OnActorAdded)
            OnActorAdded(actor);

        return actor;
    }

    /**
     * @brief Gathers all actors in the scene.
     * @return List of raw JActor pointers.
     */
    [[nodiscard]] std::vector<JActor *> ListAllActors() const
    {
        if (!m_ActiveScene) return {};

        return m_ActiveScene->ListAllActors();
    }

    /**
     * @brief Find an actor by ID in the active scene.
     * @param id Unique runtime actor ID
     * @return Pointer to actor, or nullptr if not found or no active scene
     */
    [[nodiscard]] JActor *FindActorByID(uint64_t id) const;

    /**
     * @brief Find an actor of type T by ID in the active scene.
     * @tparam T Actor type (must derive from JActor)
     * @param id Unique runtime actor ID
     * @return Pointer to actor of type T, or nullptr if not found or wrong type
     */
    template<typename T>
    T *FindActorByID(uint64_t id)
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
    T *FindActorOfType()
    {
        if (!m_ActiveScene) return nullptr;
        return m_ActiveScene->FindActorOfType<T>();
    }

    /**
     * @brief Find all actors of type T in the active scene.
     * @tparam T Actor type (must derive from JActor)
     * @return Vector of pointers to all matching actors, empty if none found
     */
    template<typename T>
    std::vector<T *> FindActorsOfType()
    {
        if (!m_ActiveScene) return {};
        return m_ActiveScene->FindActorsOfType<T>();
    }

    /**
     * @brief Request destroy on an actor by pointer in the active scene.
     * @param actorPtr Pointer to the actor to remove
     * @return False, if already requested; True if requested for the first time.
     */
    bool DestroyActor(JActor *actorPtr);

    /**
     * @brief Request destroy on an actor by ID in the active scene.
     * @param id Unique runtime actor ID
     * @return False, if already requested; True if requested for the first time.
     */
    bool DestroyActor(uint64_t id);

    /**
     * @brief Immediately destroys an actor by runtime ID. Editor preferred. Not suitable for general gameplay.
     * @param id Unique runtime actor ID
     * @return False, if already requested; True if requested for the first time.
     */
    bool ImmediateDestroyActor(uint64_t id);

    // -------------------- Scene Runtime API --------------------

    /**
     * @brief Renames a scene in memory.
     *
     * Updates the internal name of the scene object and calls renaming event, but does not rename the
     * corresponding file on disk. Writing to file must be handled separately.
     *
     * @param scene    Scene to rename.
     * @param newName  New name to assign to the scene.
     * @return true if renamed successfully, false otherwise.
     */

    bool RenameScene(JScene *scene, const std::string &newName);

    // -------------------- File API --------------------

    /**
     * @brief Creates a new scene file on disk.
     *
     * Initializes an empty scene with the given name and writes it to the
     * specified file path. If a file already exists at the location,
     * this function will only overwrite it if @p bOverwrite is true.
     *
     * @param name       Name of the scene (used for the internal JScene object).
     * @param filename   Scene file name (without extension, relative to scene directory).
     * @param bOverwrite Whether to overwrite an existing file (default: false).
     * @return true if the scene file was successfully created and saved, false otherwise.
     */
    bool CreateSceneFile(const std::string &name, const std::string &filename, bool bOverwrite = false) const;

    /**
    * @brief Loads a scene from disk and makes it the active scene.
    *
    * The function deserializes the scene data from JSON format, restores
    * all actors and properties, and sets the loaded scene as the active scene.
    *
    * @param filename Scene file name (without extension, relative to scene directory).
    * @return Pointer to the loaded scene, or nullptr if loading failed.
    */
    JScene *LoadSceneFile(const std::string &filename);

    /**
     * @brief Saves a scene to disk in JSON format.
     *
     * Serializes and writes MetaData on the provided scene object and writes it to the specified file.
     *
     * @param scene     Pointer to the scene to save.
     * @param filename  Scene file name (without extension, relative to scene directory).
     * @return true if the scene was successfully saved, false otherwise.
     */
    bool SaveSceneFile(const JScene *scene, const std::string &filename) const;


    /**
     * @brief Reads metadata from a scene file without fully loading the scene.
     *
     * Extracts information such as scene name, actor count, thumbnail path,
     * and last modified timestamp from the file’s MetaData.
     *
     * @param filename Full path to the scene file (.jscene).
     * @return SceneMeta struct containing metadata. Fields may be empty if file could not be opened.
     */
    static FSceneMeta ReadSceneMeta(const std::string &filename);

    /**
     * @brief Lists metadata for all scenes in a directory.
     *
     * Iterates over all `.jscene` files in the given directory and extracts
     * their metadata using ReadSceneMeta().
     *
     * @param directory Path to the directory containing scene files.
     * @return Vector of SceneMeta structures for each valid scene file.
     */
    static std::vector<FSceneMeta> ListScenesMeta(const std::string &directory);


    /**
     * @brief Lists all available scene files in a directory.
     *
     * Finds all files with the `.jscene` extension in the specified directory
     * and returns their filenames (without path).
     *
     * @param directory Path to the directory containing scene files.
     * @return Vector of scene file names (e.g. "Lake.jscene, MainMenu.jscene, ...").
     */
    static std::vector<std::string> ListAvailableSceneFiles(const std::string &directory);

    // -------------------- Callbacks --------------------
    /** Callback invoked whenever a scene is loaded. */
    std::function<void(JScene *)> OnSceneLoaded;

    /** Callback invoked whenever a scene is saved. */
    std::function<void(const JScene *)> OnSceneSaved;

    /** Callback invoked whenever a scene is renamed */
    std::function<void(const JScene *, std::string newName)> OnSceneRenamed;

    /** Callback invoked whenever an actor is added in the active scene. */
    std::function<void(JActor *)> OnActorAdded;

    /** Callback invoked whenever removing an actor is attempted. */
    std::function<void(JActor *)> OnActorRemoving;

    /** Callback invoked whenever an actor is removed from the active scene. */
    std::function<void(unsigned int ID)> OnActorRemoved;
};
