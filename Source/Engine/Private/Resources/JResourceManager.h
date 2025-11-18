// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <string>
#include <typeindex>
#include <type_traits>
#include <cstdint>
#include <iostream>
#include <ostream>
#include <vector>

#include "IGpuResource.h"
#include "Core/JCoreObject.h"

class IRenderDevice;

/**
 * @class JResourceManager
 * @brief Centralized manager for loading, caching, and managing resource lifetimes.
 *
 * This manager handles both CPU and GPU-backed resources derived from JCoreObject.
 * It supports:
 *   - Safe shared ownership via std::shared_ptr.
 *   - Fast lookup by key (e.g., file path or name) or unique object ID.
 *   - Automatic GPU resource setup/teardown using IRenderDevice for types that implement IGpuResource.
 *
 *
 * ### Usage
 * Typical engine usage pattern:
 * @code
 * auto& rm = JResourceManager::Get();
 * rm.SetRenderDevice(&Renderer);
 * auto model = rm.Load<JModelResource>("Models/Tree.fbx");
 * @endcode
 *
 * ### Design Notes
 * - Thread-safe: uses std::shared_mutex for concurrent read/write access.
 * - CPU-only resources:
 *     * Constructed and cached normally; no device interaction.
 * - GPU resources (types implementing IGpuResource):
 *     * Preferred constructor: T(IRenderDevice*, Args...).
 *     * Fallback: T(Args...), then SetRenderDevice(IRenderDevice*).
 *     * After construction, CreateGpuResources(IRenderDevice*) is called.
 *     * On Unload / UnloadUnused / UnloadAll, if the cache holds the last reference,
 *       DestroyGpuResources(IRenderDevice*) is called before release.
 * - Normalized keys ensure cross-platform consistent lookups.
 */
class JResourceManager
{
private:
    JResourceManager() = default;
    ~JResourceManager() = default;

    //======================================================================
    // Internal Structures
    //======================================================================

    /** @brief Shared pointer to the base resource type. */
    using BasePtr = std::shared_ptr<JCoreObject>;

    /** @brief Internal cache entry. */
    struct Entry
    {
        BasePtr ptr;
        std::type_index type{ typeid(void) };
    };

    mutable std::shared_mutex m_Mutex;              ///< Thread-safe read/write access.
    std::unordered_map<std::string, Entry> m_ByKey; ///< Cache by normalized key.
    std::unordered_map<uint64_t, BasePtr> m_ByID;   ///< Cache by object ID.
    IRenderDevice* m_Device = nullptr;              ///< GPU device pointer.

public:
    /** @brief Global singleton accessor. */
    static JResourceManager& Get()
    {
        static JResourceManager instance;
        return instance;
    }

    // Disable copy/move
    JResourceManager(const JResourceManager&) = delete;
    JResourceManager& operator=(const JResourceManager&) = delete;
    JResourceManager(JResourceManager&&) = delete;
    JResourceManager& operator=(JResourceManager&&) = delete;

    void Shutdown();

    //======================================================================
    // Render Device
    //======================================================================

    /**
     * @brief Assigns the active render device.
     * This device is passed to resources during creation.
     */
    void SetRenderDevice(IRenderDevice* device) { m_Device = device; }

    /**
     * @brief Returns the currently assigned render device.
     * @return Pointer to IRenderDevice (may be nullptr).
     */
    [[nodiscard]] IRenderDevice* GetRenderDevice() const { return m_Device; }

    //======================================================================
    // Load / Creation
    //======================================================================

     /**
     * @brief Loads a resource by key or creates it if not already cached.
     *
     * Construction order:
     *  1. If available, prefers constructor: T(IRenderDevice*, Args...).
     *  2. Otherwise constructs T(Args...), then if it implements IGpuResource:
     *       - SetRenderDevice(IRenderDevice*)
     *       - CreateGpuResources(IRenderDevice*)
     *
     * @tparam T Resource type (must derive from JCoreObject).
     * @tparam Args Constructor argument types.
     * @param key Unique string key (e.g., path, asset name).
     * @param args Forwarded constructor arguments.
     * @return Shared pointer to the cached or newly created resource.
     */
    template<class T, class... Args>
    std::shared_ptr<T> Load(const std::string& key, Args&&... args)
    {
        static_assert(std::is_base_of<JCoreObject, T>::value, "T must derive from JCoreObject");
        const std::string norm = NormalizeKey(key);

        // Fast path read
        {
            std::shared_lock rlock(m_Mutex);
            if (auto it = m_ByKey.find(norm); it != m_ByKey.end())
                return std::dynamic_pointer_cast<T>(it->second.ptr);
        }

        // Create new instance
        auto createdResource = CreateInstance<T>(
            std::integral_constant<bool, std::is_constructible<T, IRenderDevice*, Args...>::value>{},
            std::forward<Args>(args)...
        );

        // If it's a GPU resource, finalize wiring and create its GPU cache.
        if constexpr (std::is_base_of_v<IGpuResource, T>)
        {
            if (m_Device)
            {
                createdResource->SetRenderDevice(m_Device);
                createdResource->CreateGpuResources(m_Device);
            }
            else
            {
                std::cerr << "[JResourceManager]: Failed to create gpu resource, RenderDevice is null\n";
            }
        }

        const uint64_t id = createdResource->GetRuntimeID();

        // Add to cache
        {
            std::unique_lock wlock(m_Mutex);
            m_ByID[id] = createdResource;
            m_ByKey[norm] = Entry{ createdResource, std::type_index(typeid(T)) };
        }

        return createdResource;
    }

    //======================================================================
    // Get / Query
    //======================================================================

    /**
     * @brief Retrieves an untyped resource by string key.
     * @param key Identifier key.
     * @return Shared pointer or nullptr if not found.
     */
    [[nodiscard]] std::shared_ptr<JCoreObject> Get(const std::string& key) const;

    /**
     * @brief Retrieves an untyped resource by unique object ID.
     * @param id Unique identifier.
     * @return Shared pointer or nullptr if not found.
     */
    [[nodiscard]] std::shared_ptr<JCoreObject> GetByID(uint64_t id) const;

    /**
     * @brief Retrieves a typed resource by key.
     * @tparam T Desired type.
     * @param key Identifier key.
     * @return Shared pointer of type T or nullptr if not found or type mismatch.
     */
    template<class T>
    [[nodiscard]] std::shared_ptr<T> GetAs(const std::string& key) const
    {
        return std::dynamic_pointer_cast<T>(Get(key));
    }

    /**
     * @brief Retrieves a typed resource by unique object ID.
     * @tparam T Desired type.
     * @param id Object ID.
     * @return Shared pointer of type T or nullptr if not found or type mismatch.
     */
    template<class T>
    [[nodiscard]] std::shared_ptr<T> GetByIDAs(uint64_t id) const
    {
        return std::dynamic_pointer_cast<T>(GetByID(id));
    }

    /**
     * @brief Checks if a resource exists in the cache.
     * @param key Identifier key.
     * @return True if present.
     */
    [[nodiscard]] bool Has(const std::string& key) const;

    //======================================================================
    // Unload / Garbage Collection
    //======================================================================

    /**
      * @brief Removes a specific resource from the cache.
      *
      * If the cache holds the last reference and the resource implements IGpuResource,
      * DestroyGpuResources() will be called before releasing it.
      *
      * @param key Resource key.
      * @return True if removed.
      */
    bool Unload(const std::string& key);

    /**
     * @brief Unloads all resources with only the cache reference remaining.
     * Calls DestroyGpuResources() on IGpuResource types before freeing.
     * @return Number of resources removed.
     */
    size_t UnloadUnused();

    /**
     * @brief Clears all cached resources from memory.
     * Calls DestroyGpuResources() on IGpuResource types before releasing.
     */
    void UnloadAll();

private:
    //======================================================================
    // Helpers
    //======================================================================

    /**
     * @brief Preferred path: construct with IRenderDevice* if supported.
     */
    template<class T, class... Args>
    std::shared_ptr<T> CreateInstance(std::true_type /* has (IRenderDevice*, Args...) */, Args&&... args)
    {
        auto p = std::make_shared<T>(m_Device, std::forward<Args>(args)...);

        // If it is a GPU resource, ensure CreateGpuResources is called after construction.
        if (auto* gpu = dynamic_cast<IGpuResource*>(p.get()))
        {
            // constructor already received device, but call again to be explicit & safe
            gpu->SetRenderDevice(m_Device);
            // CreateGpuResources is called by caller (Load) to keep the policy in one place.
        }
        return p;
    }

    /**
     * @brief Fallback: construct normally; if it implements IGpuResource, wire device later.
     */
    template<class T, class... Args>
    std::shared_ptr<T> CreateInstance(std::false_type /* fallback */, Args&&... args)
    {
        auto p = std::make_shared<T>(std::forward<Args>(args)...);

        if (auto* gpu = dynamic_cast<IGpuResource*>(p.get()))
        {
            gpu->SetRenderDevice(m_Device);
            // CreateGpuResources is called by caller (Load) to keep the policy in one place.
        }
        return p;
    }

    /**
     * @brief Normalizes a resource key for consistent lookups.
     *
     * Converts all backslashes to forward slashes and lowercases all letters.
     * Example:
     *   "Assets\\Models\\Tree.FBX" → "assets/models/tree.fbx"
     *
     * @param s Input key string.
     * @return Normalized key string.
     */
    static std::string NormalizeKey(std::string s);
};