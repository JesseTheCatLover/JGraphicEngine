// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <optional>
#include "Core/JCoreObject.h"

/**
 * @class JResourceManager
 * @brief Centralized manager for loading, storing, and retrieving resources derived from JCoreObject.
 *
 * Supports both string-based keys (e.g., file paths, resource names) and unique IDs.
 * Provides safe access and lifetime management through smart pointers.
 *
 * Designed for ECS systems to reference resources by ID while tools/editor
 * code may reference them by string identifiers.
 */
class JResourceManager
{
public:
    /// Resource pointer type (shared ownership)
    using ResourcePtr = std::shared_ptr<JCoreObject>;

    /**
     * @brief Load a resource by key if it doesn't exist, otherwise return existing.
     *
     * @tparam T Type of the resource to load (must derive from JCoreObject).
     * @param key Unique string identifier (e.g., file path, guid, resource name).
     * @param args Constructor arguments for the resource if it needs to be created.
     * @return Shared pointer to the resource.
     *
     * Example usage:
     * @code auto tree = ResourceManager.Load<JModelResource>(treeGuid, "Models/Tree.fbx");
     */
    template<typename T, typename... Args>
    std::shared_ptr<T> Load(const std::string& key, Args&&... args)
    {
        static_assert(std::is_base_of<JCoreObject, T>::value, "T must derive from JCoreObject");

        // Check if already loaded
        auto existingIt = m_ResourcesByKey.find(key);
        if (existingIt != m_ResourcesByKey.end())
            return std::dynamic_pointer_cast<T>(existingIt->second);

        // Construct new resource
        auto resource = std::make_shared<T>(std::forward<Args>(args)...);
        uint64_t id = resource->GetID();

        m_ResourcesByKey[key] = resource;
        m_ResourcesByID[id] = resource;

        return resource;
    }

    /**
     * @brief Get a resource by its string key.
     * @param key Resource identifier.
     * @return Optional shared pointer. Empty if not found.
     */
    std::optional<ResourcePtr> Get(const std::string& key) const;

    /**
     * @brief Get a resource by its unique ID.
     * @param id JCoreObject ID.
     * @return Optional shared pointer. Empty if not found.
     */
    std::optional<ResourcePtr> GetByID(uint64_t id) const;

    /**
     * @brief Check if a resource exists by key.
     * @param key Resource identifier.
     * @return True if resource exists.
     */
    bool Has(const std::string& key) const;

    /**
     * @brief Unload a resource by its string key.
     * Removes it from both key and ID maps.
     *
     * @param key Resource identifier.
     * @return True if resource was found and removed, false otherwise.
     */
    bool Unload(const std::string& key);

    /**
     * @brief Unload all resources from memory.
     */
    void UnloadAll();

private:
    std::unordered_map<std::string, ResourcePtr> m_ResourcesByKey;
    std::unordered_map<uint64_t, ResourcePtr> m_ResourcesByID;
};
