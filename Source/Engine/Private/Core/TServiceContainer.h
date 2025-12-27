//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>

class TEditorServiceContainer
{
private:
    friend class JEngine; // only JEngine can call anything here

    // Register how to make a service
    template<typename T>
    void RegisterFactory(std::function<std::shared_ptr<T>()> factory) {
        m_Factories[typeid(T)] = [factory]() { return factory(); };
    }

    // Get an existing service, or create one if needed
    template<typename T>
    std::shared_ptr<T> GetService() {
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

    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;
    std::unordered_map<std::type_index, std::function<std::shared_ptr<void>()>> m_Factories;
};
