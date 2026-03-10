//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <typeindex>
#include <memory>

#include "IEditorService.h"

class TEditorServiceContainer
{
public:
    template<typename T, typename... Args>
    T& Register(Args&&... args)
    {
        auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
        T& ref = *ptr;
        m_Map.emplace(std::type_index(typeid(T)), std::move(ptr));
        return ref;
    }

    template<typename T>
    T& Get()
    {
        return *static_cast<T*>(m_Map.at(std::type_index(typeid(T))).get());
    }

    void Tick(float dt)
    {
        for (auto& [_, svc] : m_Map)
            svc->Tick(dt);
    }

    void RegisterShellCommand(ShellCommandService& shell)
    {
        for (auto& [_, svc] : m_Map)
            svc->RegisterShellCommands(shell);
    }

private:
    std::unordered_map<std::type_index, std::unique_ptr<IEditorService>> m_Map;
};
