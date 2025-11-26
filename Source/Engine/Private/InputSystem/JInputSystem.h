//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include "IInputBackend.h"
#include "Core/Memory/SmartPointers.h"
#include "InputSystem/FActionStates.h"
#include "InputSystem/MappingStyle/IInputMappingStyle.h"

class JInputSystem
{
    friend class JEngine;
    friend class InputManager;

private:
    JInputSystem();
    ~JInputSystem();

    IInputBackend* m_Backend = nullptr;
    TUniquePtr<IInputMappingStyle> m_MappingStyle;

    std::vector<FRawInputEvent>  m_Events;

    std::vector<FInputDeviceState> m_DevicesState;
    std::vector<FInputDeviceState> m_PrevDevicesState;

    std::vector<FInputChannelDesc> m_Channels;
    std::vector<float> m_ChannelData; // Packed

    std::unordered_map<std::string, InputChannelHandle> m_NameToHandle;

    // Key arrays for quick check
    std::vector<uint8_t> m_KeyCurrent;
    std::vector<uint8_t> m_KeyPrevious;

    void Initialize(IInputBackend* backend);
    void Shutdown();

    void SetMappingStyle(TUniquePtr<IInputMappingStyle> style);

    void Tick(float deltaTime);

    // ---- Engine-level low-level ----
    bool IsKeyDown(int keyCode) const;
    bool WasKeyPressed(int keyCode) const;
    bool WasKeyReleased(int keyCode) const;

    // ---- Channel queries (used by InputManager) ----
    FActionStateBool GetBoolChannel (InputChannelHandle handle) const;
    FActionStateAxis1D GetAxis1DChannel(InputChannelHandle handle) const;
    FActionStateAxis2D GetAxis2DChannel(InputChannelHandle handle) const;

    InputChannelHandle FindChannelIdByName(const std::string& name) const;

    // Helpers
    void ProcessEvents(); // RawInputEvent -> m_devices & key arrays
    void RebuildChannels(); // ask style -> build m_channels + nameToId
};
