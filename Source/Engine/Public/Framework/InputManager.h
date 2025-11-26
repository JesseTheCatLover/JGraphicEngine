//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <unordered_map>

#include "Core/Math/FVector2.h"
#include "InputSystem/InputChannels.h"

class JInputSystem;

/**
 * @class InputManager
 * @brief Gameplay/editor-facing interface for InputSystem.
 */
class InputManager
{
    friend class JEngine;

public:
    ~InputManager() = default;

    // Non-copyable / non-movable
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;
    InputManager(InputManager&&) = delete;
    InputManager& operator=(InputManager&&) = delete;

private:
    explicit InputManager() = default;

    bool Initialize(JInputSystem* system);

    void Tick(float deltaTime);

    /// Non-owning pointer to the engine input subsystem.
    JInputSystem* m_InputSystem = nullptr;

    /// Cache: name -> channel handle.
    std::unordered_map<std::string, InputChannelHandle> m_Cache;

    /**
     * @brief Returns (and caches) the channel handle for the given name.
     * @note Returns InvalidInputChannel if the name cannot be resolved.
     */
    [[nodiscard]] InputChannelHandle GetChannelHandle(const std::string& name);

public:
    [[nodiscard]] bool GetActionDown (const std::string& name);
    [[nodiscard]] bool GetActionUp (const std::string& name);
    [[nodiscard]] bool GetActionHeld (const std::string& name);

    [[nodiscard]] float GetAxis (const std::string& name);
    [[nodiscard]] FVector2 GetAxis2D(const std::string& name);
};
