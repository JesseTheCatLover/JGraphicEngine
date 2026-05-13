//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "Core/Memory/SmartPointers.h"
#include "../../Private/Core/TServiceContainer.h" // TODO: future build system should handle this better.

template<typename T>
TSharedPtr<T> JEngine::GetService()
{
    return m_Services->GetService<T>();
}

template<typename T>
void JEngine::RegisterFactory(std::function<TSharedPtr<T>()> factory)
{
    m_Services->RegisterFactory<T>(std::move(factory));
}