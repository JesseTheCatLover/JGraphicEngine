//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <memory>
#include <memory>
#include <utility>
#include <type_traits>

/**
 * @brief Exclusive-ownership smart pointer.
 *
 * This pointer type owns its referenced object uniquely. Ownership may be
 * transferred, but never duplicated. Once ownership is moved away, the pointer
 * becomes empty and the managed object is destroyed if no other
 * owners exist.
 *
 * Use this when exactly one system controls the lifetime of an object.
 *
 * @tparam T   Type being managed.
 */
template<typename T>
using TUniquePtr = std::unique_ptr<T>;

/**
 * @brief Constructs an object of type T and returns it wrapped in a TUniquePtr.
 *
 * All provided arguments are forwarded to the constructor of @p T.
 *
 * @tparam T       Type of object to construct.
 * @tparam Args    Arguments forwarded to T's constructor.
 * @param args     Arguments forwarded to T's constructor.
 *
 * @return TUniquePtr<T> Managing the newly constructed object.
 */
template<typename T, typename... Args>
TUniquePtr<T> MakeUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

/**
 * @brief Explicitly transfers unique ownership of an object.
 *
 * This helper wraps a move operation while communicating intent clearly.
 * It is used when passing unique-ownership pointers to APIs that are designed
 * to take ownership from the caller.
 *
 * Example:
 * @code
 * TUniquePtr<Mesh> MeshPtr = MakeUnique<Mesh>(MeshData);
 *
 * // Transfer ownership into the scene. MeshPtr becomes empty after this call.
 * Scene->AddMesh(TakeUniqueOwnership(MeshPtr));
 *
 * // MeshPtr is now null and should not be used.
 * @endcode
 *
 * @tparam T   Type of the object being transferred.
 * @param Obj  Object whose ownership will be moved.
 *
 * @return Rvalue reference enabling ownership transfer.
 */
template<typename T>
constexpr std::remove_reference_t<T>&& TakeUniqueOwnership(T&& Obj) noexcept
{
    return std::move(Obj);
}

/**
 * @brief Reference-counted shared-ownership smart pointer.
 *
 * This pointer type allows multiple systems to access the same object.
 * The managed object is automatically destroyed when the final shared
 * owner releases its reference.
 *
 * Use this for shared resources.
 *
 * @tparam T   Type being managed.
 */
template<typename T>
using TSharedPtr = std::shared_ptr<T>;

/**
 * @brief Constructs an object of type T and returns it wrapped in a TSharedPtr.
 *
 * All provided arguments are forwarded to the constructor of @p T.
 *
 * @tparam T       Type of object to construct.
 * @tparam Args    Arguments forwarded to T's constructor.
 * @param args     Arguments forwarded to T's constructor.
 *
 * @return TSharedPtr<T> Sharing ownership of the newly created object.
 */
template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

/**
 * @brief Non-owning observer pointer referencing a shared-ownership object.
 *
 * This pointer type does not affect the lifetime of the referenced object.
 * It can be converted into a shared pointer by locking it, which yields
 * temporary access if the referenced object is still alive.
 *
 * Use this to break ownership cycles or maintain optional references.
 *
 * @tparam T   Type being observed.
 */
template<typename T>
using TWeakPtr = std::weak_ptr<T>;
