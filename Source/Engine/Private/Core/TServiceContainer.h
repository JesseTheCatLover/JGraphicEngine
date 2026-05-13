//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>
#include "Core/Memory/SmartPointers.h"

class TServiceContainer
{
private:
    friend class JEngine; // only JEngine can call anything here

    // Register how to make a service
    template<typename T>
    void RegisterFactory(std::function<TSharedPtr<T>()> factory) {
        m_Factories[typeid(T)] = [factory]() { return factory(); };
    }

    // Get an existing service, or create one if needed
    template<typename T>
    TSharedPtr<T> GetService() {
        auto it = m_Services.find(typeid(T));
        if (it != m_Services.end())
            return std::static_pointer_cast<T>(it->second);

        // Create it if we have a factory
        auto fit = m_Factories.find(typeid(T));
        if (fit != m_Factories.end()) {
            auto instance = fit->second();
            m_Services[typeid(T)] = instance;
            return std::static_pointer_cast<T>(instance);
        }

        return nullptr;
    }

    std::unordered_map<std::type_index, TSharedPtr<void>> m_Services;
    std::unordered_map<std::type_index, std::function<TSharedPtr<void>()>> m_Factories;
};
