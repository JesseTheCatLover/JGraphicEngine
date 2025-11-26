//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "ActionAxisConfig.h"
#include "IInputMappingStyle.h"

class ActionAxisStyle : public IInputMappingStyle
{
public:
    explicit ActionAxisStyle(const FActionAxisMap& configMap);

    void BuildChannels(std::vector<FInputChannelDesc>& outChannels) override;
    void UpdateChannels(float deltaTime, const std::vector<FInputDeviceState>& devices, std::vector<float>& channelData) override;

    [[nodiscard]] FActionStateBool GetBoolState (InputChannelHandle handle) const override;
    [[nodiscard]] FActionStateAxis1D GetAxis1DState(InputChannelHandle handle) const override;
    [[nodiscard]] FActionStateAxis2D GetAxis2DState(InputChannelHandle handle) const override;

private:
    FActionAxisMap m_ConfigMap;

    // per-channel state, index == handle (0..N-1)
    std::vector<FActionStateBool>   m_BoolStates;
    std::vector<FActionStateAxis1D> m_Axis1DStates;
    std::vector<FActionStateAxis2D> m_Axis2DStates;

    // Helper to get current value from a binding
    float GetBindingValue(const FInputBinding& binding, const std::vector<FInputDeviceState>& devices) const;
};
