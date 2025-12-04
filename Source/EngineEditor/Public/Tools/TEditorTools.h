//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include "Core/Memory/SmartPointers.h"
#include "Utilities/UDynamicID.h"

/**
 * @class TEditorTools
 * @brief Type-safe registry and lifetime manager for editor tool instances.
 *
 * TEditorTools associates each tool instance with an opaque ID allocated by UDynamicID
 * and manages their lifetime via unique ownership semantics. It provides a simple
 * handle-based interface for creating, destroying, and iterating over tools of a
 * given type.
 *
 * Each created tool:
 *  - Is dynamically allocated and owned by the registry.
 *  - Is identified externally by a UDynamicID::IDType handle.
 *  - Is automatically destroyed when removed from the registry or when the registry
 *    itself is destroyed.
 *
 * The allocator and registry are intentionally minimal:
 *  - No generation tracking is performed; handles are recycled by UDynamicID.
 *  - Destroy() fails silently (returns false) if an unknown ID is passed.
 *  - Get() returns nullptr for unknown IDs; no assertions or logging are performed.
 *  - The class is not thread-safe; external synchronization is required in
 *    multi-threaded contexts.
 *
 * Typical usage:
 *  - Call Create() (with or without arguments) to construct a new tool instance.
 *  - Store and pass the returned ID as an opaque handle in higher-level systems.
 *  - Call Destroy(id) when the tool is no longer needed to release its resources
 *    and return the ID to the allocator.
 *  - Use Get(id) to access a specific tool, or ForEach(...) to operate on all tools.
 *
 * Recommended use cases include editor subsystems that need to manage multiple
 * instances of tool objects (e.g., selection tools, gizmos, inspectors) where
 * tools are referenced via stable, opaque identifiers rather than raw pointers.
 *
 * @tparam TTool The concrete tool type managed by this registry. Must be
 *         constructible with the arguments passed to Create() and compatible
 *         with TUniquePtr<TTool>.
 */
template<typename TTool>
class TEditorTools
{
private:
    UDynamicID m_Allocator; ///< Allocator responsible for issuing and recycling tool IDs.
    std::unordered_map<UDynamicID::IDType, TUniquePtr<TTool>> m_Tools; ///< Mapping from tool IDs to owned tool instances.

public:
    /**
     * @brief Creates a new tool instance using its default constructor.
     *
     * A new ID is allocated from the internal UDynamicID allocator and associated
     * with a freshly constructed TTool instance. The registry takes ownership of
     * the tool and will destroy it when Destroy() is called or when the registry
     * is destructed.
     *
     * @return A valid ID that uniquely identifies the created tool within this registry.
     */
    UDynamicID::IDType Create()
    {
        UDynamicID::IDType id = m_Allocator.Allocate();
        m_Tools[id] = MakeUnique<TTool>();
        return id;
    }

    /**
     * @brief Creates a new tool instance with forwarded constructor arguments.
     *
     * This overload allows construction of TTool with arbitrary parameters.
     * The arguments are perfectly forwarded to TTool's constructor.
     *
     * Example:
     * @code
     * TEditorTools<FooTool> Tools;
     * auto id = Tools.Create(42, "Name"); // Constructs FooTool(42, "Name")
     * @endcode
     *
     * @tparam Args Parameter pack type for the constructor arguments.
     * @param args Arguments forwarded to TTool's constructor.
     * @return A valid ID that uniquely identifies the created tool within this registry.
     */
    template<typename... Args>
    UDynamicID::IDType Create(Args&&... args)
    {
        UDynamicID::IDType id = m_Allocator.Allocate();
        m_Tools[id] = MakeUnique<TTool>(std::forward<Args>(args)...);
        return id;
    }

    /**
     * @brief Destroys a tool associated with the given ID and recycles its handle.
     *
     * If the ID is present in the registry, the corresponding tool instance is
     * destroyed, its entry is removed from the internal map, and the ID is
     * returned to the UDynamicID allocator for potential reuse.
     *
     * If the ID does not exist in the registry, this function performs no action
     * and returns false.
     *
     * @param id The ID of the tool to destroy.
     * @return true if a tool with the given ID existed and was destroyed;
     *         false if no matching tool was found.
     */
    bool Destroy(UDynamicID::IDType id)
    {
        auto erased = m_Tools.erase(id);
        if (erased > 0)
        {
            m_Allocator.Free(id);
            return true;
        }
        return false;
    }

    /**
     * @brief Retrieves a mutable pointer to the tool associated with the given ID.
     *
     * This function returns a raw pointer to the managed tool instance, or nullptr
     * if no tool is associated with the specified ID. The caller must not store
     * the returned pointer beyond the lifetime of the registry or after the tool
     * is destroyed.
     *
     * @param id The ID of the tool to retrieve.
     * @return A pointer to the tool instance, or nullptr if the ID is not found.
     */
    TTool* Get(UDynamicID::IDType id)
    {
        auto it = m_Tools.find(id);
        return it != m_Tools.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief Retrieves a const pointer to the tool associated with the given ID.
     *
     * This const overload allows read-only access to tool instances. It returns
     * a pointer to the managed tool or nullptr if the ID is not present.
     *
     * @param id The ID of the tool to retrieve.
     * @return A const pointer to the tool instance, or nullptr if the ID is not found.
     */
    const TTool* Get(UDynamicID::IDType id) const
    {
        auto it = m_Tools.find(id);
        return it != m_Tools.end() ? it->second.get() : nullptr;
    }

    /**
     * @brief Applies a callable to each tool in the registry.
     *
     * Iterates over all tools and invokes the provided callable with the ID and
     * the tool instance as arguments. The callable must be invocable with the
     * signature:
     *
     * @code
     * void Fn(UDynamicID::IDType id, TTool& tool);
     * @endcode
     *
     * The iteration order is unspecified and corresponds to the underlying
     * std::unordered_map.
     *
     * @tparam Fn Callable type.
     * @param fn A callable object or lambda to apply to each tool.
     */
    template<typename Fn>
    void ForEach(Fn&& fn)
    {
        for (auto& [id, tool] : m_Tools)
            fn(id, *tool);
    }

    /**
     * @brief Applies a callable to each tool in the registry (const overload).
     *
     * This overload allows iteration over the registry from a const context.
     * The callable must accept a const reference to the tool:
     *
     * @code
     * void Fn(UDynamicID::IDType id, const TTool& tool);
     * @endcode
     *
     * The iteration order is unspecified and corresponds to the underlying
     * std::unordered_map.
     *
     * @tparam Fn Callable type.
     * @param fn A callable object or lambda to apply to each tool.
     */
    template<typename Fn>
    void ForEach(Fn&& fn) const
    {
        for (auto& [id, tool] : m_Tools)
            fn(id, *tool);
    }
};
