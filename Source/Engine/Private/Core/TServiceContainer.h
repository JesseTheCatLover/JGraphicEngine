//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <any>
#include <memory>
#include <typeindex>
#include <unordered_map>

class TServiceContainer
{
private:
    friend class JEngine; // only JEngine has access to TServiceContainer

    std::unordered_map<std::type_index, std::shared_ptr<void>> m_Services;

    template<typename T, typename... Args>
    void RegisterService(Args&&... args)
    {
        m_Services[typeid(T)] = std::make_shared<T>(std::forward<Args>(args)...);
    }

    template<typename T>
    T* GetService()
    {
        auto it = m_Services.find(typeid(T));
        if (it != m_Services.end())
            return static_cast<T*>(it->second.get());
        return nullptr;
    }

    template<typename T, typename... Args>
    T* GetOrCreateService(Args&&... args)
    {
        auto* svc = GetService<T>();
        if (!svc)
        {
            RegisterService<T>(std::forward<Args>(args)...);
            svc = GetService<T>();
        }
        return svc;
    }
};
