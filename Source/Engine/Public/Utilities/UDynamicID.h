// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include <cstdint>

/**
 * @class UDynamicID
 * @brief Lightweight utility for allocating and recycling opaque integer identifiers.
 *
 * UDynamicID provides a simple dynamic ID allocation strategy based on a monotonically
 * increasing counter combined with a free-list of released IDs. This makes it
 * suitable for systems that require stable handles during an object's lifetime,
 * but do not require strict generation tracking or safety checks.
 *
 * Typical usage:
 *  - Call Allocate() when creating a new object requiring an ID.
 *  - Store the returned ID alongside the object (e.g., in a map or array).
 *  - Call Free(id) when the object is destroyed to allow ID reuse.
 *
 * The class is intentionally minimal:
 *  - It does not enforce uniqueness beyond its own allocation logic.
 *  - It does not validate IDs passed to Free() or guard against double-free.
 *  - It is not thread-safe; external synchronization is required in
 *    multi-threaded contexts.
 *
 * Recommended use cases include editor tools, transient runtime objects,
 * or internal handles where ID misuse can be controlled by higher-level code.
 */
class UDynamicID
{
public:
    using IDType = u_int32_t; ///< Underlying integral ID type.

    static constexpr IDType InvalidID = 0; ///< Sentinel value representing an invalid or uninitialized ID.

    /**
     * @brief Allocates and returns a new ID.
     *
     * If there are IDs available in the free-list (from previous Free() calls),
     * one of those IDs is returned (LIFO order). If the free-list is empty,
     * the internal counter is incremented and a new ID is generated.
     *
     * @return A valid ID suitable for use as an opaque handle.
     */
    IDType Allocate()
    {
        if (!m_FreeIDs.empty())
        {
            IDType id = m_FreeIDs.back();
            m_FreeIDs.pop_back();
            return id;
        }

        return ++m_NextID;
    }

    /**
     * @brief Releases an ID back to the allocator for potential reuse.
     *
     * After Free(id) is called, the ID may be returned again by a future
     * Allocate() invocation. No validation is performed:
     *  - Passing an ID that was never allocated is undefined behavior.
     *  - Passing the same ID multiple times is undefined behavior.
     *
     * It is the caller's responsibility to ensure that an ID is no longer in use
     * before returning it to the allocator.
     *
     * @param id The ID to return to the free-list.
     */
    void Free(IDType id)
    {
        m_FreeIDs.push_back(id);
    }

private:
    IDType m_NextID = 0;           ///< Last issued ID value; incremented when no free IDs are available.
    std::vector<IDType> m_FreeIDs; ///< Free-list of released IDs available for reuse (LIFO).
};
