//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <memory>

template<typename T>
using TUniquePtr = std::unique_ptr<T>;

template<typename T, typename... Args>
TUniquePtr<T> MakeUnique(Args&&... args)
{
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template<typename T>
using TSharedPtr = std::shared_ptr<T>;

template<typename T, typename... Args>
TSharedPtr<T> MakeShared(Args&&... args)
{
    return std::make_shared<T>(std::forward<Args>(args)...);
}

template<typename T>
using TWeakPtr = std::weak_ptr<T>;
