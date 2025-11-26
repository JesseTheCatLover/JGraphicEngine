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
#include "Core/Memory/SmartPointers.h"

class IRenderDevice;

/**
 * @class JResourceSystem
 * @brief Centralized internal system for loading, caching, and managing runtime resources keyed by asset UUID.
 *
 * JResourceSystem guarantees that each asset UUID maps to exactly one runtime
 * resource instance (e.g., mesh data, textures, audio buffers). The system owns
 * a single TSharedPtr for each resource and hands out additional TSharedPtr
 * references to engine systems and components.
 *
 * Used only by engine core and components, not exposed to gameplay scripts.
 * If a resource implements IGpuResource, the system also drives
 * CreateGpuResources / DestroyGpuResources based on its lifetime.
 *
 * A resource is considered unused when its TSharedPtr use count is equal to 1
 * (only stored in the system) and may be reclaimed by UnloadUnused().
 */
class JResourceSystem
{
    friend class JEngine;

public:
    using JAssetID = std::string;

    ~JResourceSystem() = default;

private:
    JResourceSystem() = default;

    /** @brief Shared pointer to the base resource type. */
    using BasePtr = TSharedPtr<JCoreObject>;

    /** @brief Internal cache entry. */
    struct Entry
    {
        BasePtr ptr;
        std::type_index type{ typeid(void) };
    };

    mutable std::shared_mutex m_Mutex;                  ///< Thread-safe read/write access.
    std::unordered_map<JAssetID, Entry> m_ByAsset;      ///< Cache by asset UUID.
    IRenderDevice* m_Device = nullptr;                  ///< GPU device pointer.

public:

    // Disable copy/move
    JResourceSystem(const JResourceSystem&) = delete;
    JResourceSystem& operator=(const JResourceSystem&) = delete;
    JResourceSystem(JResourceSystem&&) = delete;
    JResourceSystem& operator=(JResourceSystem&&) = delete;

    void Shutdown();

    //======================================================================
    // Render Device
    //======================================================================

    void SetRenderDevice(IRenderDevice* device) { m_Device = device; }
    [[nodiscard]] IRenderDevice* GetRenderDevice() const { return m_Device; }

    //======================================================================
    // Load / Creation
    //======================================================================

    /**
     * @brief Loads a resource by asset UUID or creates it if not already cached.
     *
     * Construction order:
     *  1. If available, prefers constructor: T(IRenderDevice*, Args...).
     *  2. Otherwise, constructs T(Args...), then if it implements IGpuResource:
     *       - SetRenderDevice(IRenderDevice*)
     *       - CreateGpuResources(IRenderDevice*)
     *
     * @tparam T Resource type (must derive from JCoreObject).
     * @tparam Args Constructor argument types.
     * @param assetId Unique asset UUID string.
     * @param args Forwarded constructor arguments (typically asset-specific info).
     */
    template<class T, class... Args>
    TSharedPtr<T> Load(const JAssetID& assetId, Args&&... args)
    {
        static_assert(std::is_base_of<JCoreObject, T>::value, "T must derive from JCoreObject");

        // Fast path read
        {
            std::shared_lock rlock(m_Mutex);
            if (auto it = m_ByAsset.find(assetId); it != m_ByAsset.end())
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
                std::cerr << "[JResourceSystem]: Failed to create gpu resource, RenderDevice is null\n";
            }
        }

        {
            std::unique_lock wlock(m_Mutex);
            m_ByAsset[assetId] = Entry{ createdResource, std::type_index(typeid(T)) };
        }

        return createdResource;
    }

    //======================================================================
    // Get / Query
    //======================================================================

    /**
     * @brief Retrieves an untyped resource by UUID key.
     * @param key UUID Identifier key.
     * @return Shared pointer or nullptr if not found.
     */
    [[nodiscard]] TSharedPtr<JCoreObject> Get(const JAssetID& assetId) const;

    /**
     * @brief Retrieves a typed resource by UUID key.
     * @param key UUID Identifier key.
     * @return Shared pointer or nullptr if not found.
     */
    template<class T>
    [[nodiscard]] TSharedPtr<T> GetAs(const JAssetID& assetId) const
    {
        return std::dynamic_pointer_cast<T>(Get(assetId));
    }

    /**
     * @brief Checks if a resource exists in the cache.
     * @param key UUID Identifier key.
     * @return True if present.
     */
    [[nodiscard]] bool Has(const JAssetID& assetId) const;

    //======================================================================
    // Unload / Garbage Collection
    //======================================================================

    /**
      * @brief Removes a specific resource from the cache by asset UUID.
      *
      * If the cache holds the last reference and the resource implements IGpuResource,
      * DestroyGpuResources() will be called before releasing it.
      */
    bool Unload(const JAssetID& assetId);

    /**
     * @brief Unloads all unused resources with only the using cache reference remaining.
     * @return Number of resources removed.
     */
    size_t UnloadUnused();

    /**
     * @brief Clears all cached resources from memory.
     */
    void UnloadAll();

    // Debug helpers
    void DebugDump() const;

private:
    //======================================================================
    // Helpers
    //======================================================================

    template<class T, class... Args>
    TSharedPtr<T> CreateInstance(std::true_type /* has (IRenderDevice*, Args...) */, Args&&... args)
    {
        auto p = MakeShared<T>(m_Device, std::forward<Args>(args)...);

        if (auto* gpu = dynamic_cast<IGpuResource*>(p.get()))
        {
            gpu->SetRenderDevice(m_Device);
        }
        return p;
    }

    template<class T, class... Args>
    TSharedPtr<T> CreateInstance(std::false_type /* fallback */, Args&&... args)
    {
        auto p = MakeShared<T>(std::forward<Args>(args)...);

        if (auto* gpu = dynamic_cast<IGpuResource*>(p.get()))
        {
            gpu->SetRenderDevice(m_Device);
        }
        return p;
    }
};
