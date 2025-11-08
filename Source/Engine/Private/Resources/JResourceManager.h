// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <string>
#include <typeindex>
#include <type_traits>
#include <cstdint>
#include <vector>

#include "Core/JCoreObject.h"

class IRenderDevice;

/**
 * @struct FGpuLifecycle
 * @brief Optional GPU lifecycle hooks that a resource class may implement.
 *
 * Allows resources to react to GPU initialization and teardown events.
 * The manager automatically detects and invokes these methods if they exist:
 *   - void CreateGpuResources(IRenderDevice* device)
 *   - void DestroyGpuResources(IRenderDevice* device)
 *
 * These methods are optional. If not defined on a type, no call is made.
 * This mechanism ensures GPU-backed resources (e.g., textures, meshes, shaders)
 * can manage their GPU memory automatically.
 */
struct FGpuLifecycle
{
    /** @brief Attempt to call CreateGpuResources() if defined on the resource. */
    template <class T>
    static auto TryOnCreateGpuResources(T& obj, IRenderDevice* dev, int)
        -> decltype(obj.CreateGpuResources(dev), void())
    {
        if (dev) obj.CreateGpuResources(dev);
    }

    /** @brief No-op fallback if CreateGpuResources() does not exist. */
    template <class T>
    static void TryOnCreateGpuResources(T&, IRenderDevice*, ...) {}

    /** @brief Attempt to call DestroyGpuResources() if defined on the resource. */
    template <class T>
    static auto TryOnDestroyGpuResources(T& obj, IRenderDevice* dev, int)
        -> decltype(obj.DestroyGpuResources(dev), void())
    {
        if (dev) obj.DestroyGpuResources(dev);
    }

    /** @brief No-op fallback if DestroyGpuResources() does not exist. */
    template <class T>
    static void TryOnDestroyGpuResources(T&, IRenderDevice*, ...) {}
};

/**
 * @class JResourceManager
 * @brief Centralized manager for loading, caching, and managing resource lifetimes.
 *
 * This manager handles both CPU and GPU-backed resources derived from JCoreObject.
 * It supports:
 *   - Safe shared ownership via std::shared_ptr.
 *   - Fast lookup by key (e.g., file path or name) or unique object ID.
 *   - Automatic GPU resource setup/teardown using IRenderDevice.
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
 * - Automatic GPU integration:
 *     * Prefers constructor with (IRenderDevice*, Args...).
 *     * Falls back to (Args...) + optional SetRenderDevice(IRenderDevice*).
 *     * Calls CreateGpuResources() after construction.
 * - Normalized keys ensure cross-platform consistent lookups.
 */
class JResourceManager
{
public:
    /** @brief Shared pointer to the base resource type. */
    using BasePtr = std::shared_ptr<JCoreObject>;

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
     *  2. Otherwise constructs T(Args...) and calls SetRenderDevice(IRenderDevice*) if defined.
     *  3. Calls CreateGpuResources(IRenderDevice*) if defined.
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
        auto created = CreateInstance<T>(
            std::integral_constant<bool,
                std::is_constructible<T, IRenderDevice*, Args...>::value>{},
            std::forward<Args>(args)...
        );

        const uint64_t id = created->GetID();

        // Post-create GPU hook
        FGpuLifecycle::TryOnCreateGpuResources(*created, m_Device, 0);

        // Add to cache
        {
            std::unique_lock wlock(m_Mutex);
            m_ByID[id] = created;
            m_ByKey[norm] = Entry{ created, std::type_index(typeid(T)) };
        }

        return created;
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
     * If the cache holds the last reference, DestroyGpuResources() will be called.
     * @param key Resource key.
     * @return True if removed.
     */
    bool Unload(const std::string& key);

    /**
     * @brief Unloads all resources with only the cache reference remaining.
     * Calls DestroyGpuResources() before freeing.
     * @return Number of resources removed.
     */
    size_t UnloadUnused();

    /**
     * @brief Clears all cached resources from memory.
     * Calls DestroyGpuResources() on each before releasing.
     */
    void UnloadAll();

private:
    JResourceManager() = default;
    ~JResourceManager() = default;

    //======================================================================
    // Internal Structures
    //======================================================================

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

    //======================================================================
    // Helpers
    //======================================================================

    /**
     * @brief Preferred path: construct with IRenderDevice* if supported.
     */
    template<class T, class... Args>
    std::shared_ptr<T> CreateInstance(std::true_type /* has (IRenderDevice*, Args...) */, Args&&... args)
    {
        return std::make_shared<T>(m_Device, std::forward<Args>(args)...);
    }

    /**
     * @brief Fallback: construct normally, then set render device if supported.
     */
    template<class T, class... Args>
    std::shared_ptr<T> CreateInstance(std::false_type /* fallback */, Args&&... args)
    {
        auto ptr = std::make_shared<T>(std::forward<Args>(args)...);

        if constexpr (requires(T& t, IRenderDevice* d) { t.SetRenderDevice(d); })
            ptr->SetRenderDevice(m_Device);

        return ptr;
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
