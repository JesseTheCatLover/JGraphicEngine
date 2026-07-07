//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <typeindex>
#include <typeinfo>
#include <cassert>

#include "Tools/ToolService.h"
#include "Core/Memory/SmartPointers.h"
#include "Subsystems/IPanelSubsystem.h"

class EditorHost;
class EditorRuntime;

class TPanelContainer
{
private:
    EditorHost&    m_Host;
    EditorRuntime& m_Runtime;
    ToolService&   m_Tools;

    std::unordered_map<std::type_index, TUniquePtr<IPanelSubsystem>> m_Subsystems;

public:
    TPanelContainer(EditorHost& host, EditorRuntime& runtime, ToolService& tools)
        : m_Host(host)
        , m_Runtime(runtime)
        , m_Tools(tools)
    {
    }

    // ------------------------------------------------------------
    // Register a subsystem (core + plugins)
    // ------------------------------------------------------------
    template<typename TSubsystem, typename... TArgs>
    TSubsystem& RegisterSubsystem(TArgs&&... args)
    {
        static_assert(std::is_base_of_v<IPanelSubsystem, TSubsystem>,
            "Subsystem must derive from IPanelSubsystem");

        std::type_index key = typeid(TSubsystem);

        // Prevent double registration
        assert(!m_Subsystems.contains(key) && "Subsystem already registered!");

        auto ptr = MakeUnique<TSubsystem>(std::forward<TArgs>(args)...);
        TSubsystem& ref = *ptr;
        m_Subsystems.emplace(key, std::move(ptr));
        return ref;
    }

    // ------------------------------------------------------------
    // Get a subsystem (strongly typed)
    // ------------------------------------------------------------
    template<typename TSubsystem>
    TSubsystem& GetSubsystem()
    {
        std::type_index key = typeid(TSubsystem);

        auto it = m_Subsystems.find(key);
        assert(it != m_Subsystems.end() && "Subsystem not registered!");

        return *static_cast<TSubsystem*>(it->second.get());
    }

    template<typename TSubsystem>
    const TSubsystem& GetSubsystem() const
    {
        std::type_index key = typeid(TSubsystem);

        auto it = m_Subsystems.find(key);
        assert(it != m_Subsystems.end() && "Subsystem not registered!");

        return *static_cast<const TSubsystem*>(it->second.get());
    }

    // ------------------------------------------------------------
    // Register shell commands for all active subsystems
    // ------------------------------------------------------------
    void RegisterSubsystemShellCommands(ShellCommandService& shell)
    {
        for (auto& [_, sys] : m_Subsystems)
        {
            if (sys)
            {
                sys->RegisterShellCommands(shell);
            }
        }
    }

    // ------------------------------------------------------------
    // Tick all subsystems
    // ------------------------------------------------------------
    void Tick(float dt)
    {
        for (auto& [_, sys] : m_Subsystems)
            sys->Tick(dt);
    }
};
