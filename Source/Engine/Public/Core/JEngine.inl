//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include "../../Private/Core/TServiceContainer.h" // TODO: future build system should handle this better.

template<typename T>
std::shared_ptr<T> JEngine::GetService()
{
    return m_Services->GetService<T>();
}

template<typename T>
void JEngine::RegisterFactory(std::function<std::shared_ptr<T>()> factory)
{
    m_Services->RegisterFactory<T>(std::move(factory));
}