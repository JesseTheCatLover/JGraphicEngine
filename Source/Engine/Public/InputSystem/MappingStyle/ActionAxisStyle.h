//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>
#include "ActionAxisConfig.h"
#include "IInputMappingStyle.h"

class ActionAxisStyle : public IInputMappingStyle
{
public:
    ActionAxisStyle(const FActionAxisMap& configMap);

    void BuildChannels(std::vector<FInputChannelDesc>& outChannels) override;
    void UpdateChannels(float dt, const std::vector<FInputDeviceState>& devices, std::vector<float>& channelData) override;

private:
    FActionAxisMap m_ConfigMap;
};
