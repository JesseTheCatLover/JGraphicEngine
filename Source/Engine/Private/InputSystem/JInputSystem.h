//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <cstdint>

#include "IInputBackend.h"
#include "Core/Memory/SmartPointers.h"
#include "InputSystem/MappingStyles/ActionAxis/ActionAxisStates.h"
#include "InputSystem/InputCallbacks.h"
#include "InputSystem/MappingStyles/IInputMappingStyle.h"

/**
 * @class JInputSystem
 * @brief Engine-internal subsystem responsible for collecting raw input events,
 *        tracking device state, and evaluating logical input channels.
 */
class JInputSystem
{
    friend class JEngine;
    friend class InputManager;

public:
    ~JInputSystem() = default;

    // Non-copyable / non-movable
    JInputSystem(const JInputSystem&) = delete;
    JInputSystem& operator=(const JInputSystem&) = delete;
    JInputSystem(JInputSystem&&) = delete;
    JInputSystem& operator=(JInputSystem&&) = delete;

private:
    JInputSystem();

    void Initialize(IInputBackend* backend);
    void Shutdown();

    void SetMappingStyle(TUniquePtr<IInputMappingStyle> style);

    void Tick(float deltaTime);

    void DispatchCallbacks();

    IInputBackend* m_Backend = nullptr;
    TUniquePtr<IInputMappingStyle> m_MappingStyle;

    std::vector<FRawInputEvent> m_Events;

    std::vector<FInputDeviceState> m_DevicesState;
    std::vector<FInputDeviceState> m_PrevDevicesState;

    std::vector<FInputChannelDesc> m_Channels;
    std::vector<float> m_ChannelData; // Packed
    uint32_t m_ChannelVersion = 0;

    std::unordered_map<std::string, InputChannelHandle> m_NameToHandle;

    // Key arrays for quick check
    std::vector<uint8_t> m_KeyCurrent;
    std::vector<uint8_t> m_KeyPrevious;

    struct FBoolCallbackEntry
    {
        InputCallbackHandle handle{};
        std::string channelName;
        InputChannelHandle channelHandle{ INVALID_CHANNEL_HANDLE };
        EInputEventPhase phase;
        FBoolActionCallback callback{};
    };

    struct FAxis1DCallbackEntry
    {
        InputCallbackHandle handle{};
        std::string channelName;
        InputChannelHandle channelHandle{ INVALID_CHANNEL_HANDLE };
        EInputEventPhase phase;
        FAxis1DActionCallback callback{};
    };

    struct FAxis2DCallbackEntry
    {
        InputCallbackHandle handle{};
        std::string channelName;
        InputChannelHandle channelHandle{ INVALID_CHANNEL_HANDLE };
        EInputEventPhase phase;
        FAxis2DActionCallback callback{};
    };

    std::vector<FBoolCallbackEntry> m_BoolCallbacks;
    std::vector<FAxis1DCallbackEntry> m_Axis1DCallbacks;
    std::vector<FAxis2DCallbackEntry> m_Axis2DCallbacks;

    InputCallbackHandle m_NextCallbackHandle = 1;

    InputCallbackHandle RegisterBoolCallback(
    const std::string& channelName,
    EInputEventPhase phase,
    FBoolActionCallback cb);

    InputCallbackHandle RegisterAxis1DCallback(
    const std::string& channelName,
        EInputEventPhase phase,
        FAxis1DActionCallback cb);

    InputCallbackHandle RegisterAxis2DCallback(
    const std::string& channelName,
        EInputEventPhase phase,
        FAxis2DActionCallback cb);

    void UnregisterCallback(InputCallbackHandle handle);

    // ---- Engine-level / low-level ----
    [[nodiscard]] bool IsKeyDown(uint32_t keyCode) const;
    [[nodiscard]] bool WasKeyPressed(uint32_t keyCode) const;
    [[nodiscard]] bool WasKeyReleased(uint32_t keyCode) const;

    // ---- Channel queries (used by InputManager) ----
    [[nodiscard]] FActionStateBool GetBoolChannel (InputChannelHandle handle) const;
    [[nodiscard]] FActionStateAxis1D GetAxis1DChannel(InputChannelHandle handle) const;
    [[nodiscard]] FActionStateAxis2D GetAxis2DChannel(InputChannelHandle handle) const;

    [[nodiscard]] uint32_t GetChannelVersion() const { return m_ChannelVersion; }
    [[nodiscard]] InputChannelHandle FindChannelIdByName(const std::string& name) const;

    // Helpers
    void ProcessEvents(); // RawInputEvent -> m_devices & key arrays
    void RebuildChannels(); // ask style -> build m_channels + nameToId
};
