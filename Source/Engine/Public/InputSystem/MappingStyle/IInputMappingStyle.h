//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <vector>

#include "../FInputDeviceState.h"
#include "../InputChannels.h"

class IInputMappingStyle
{
public:
    virtual ~IInputMappingStyle() = default;

    // Called when style is installed to describe the exposed channels
    virtual void BuildChannels(std::vector<FInputChannelDesc>& outChannels) = 0;

    // Called every frame after devices are updated
    virtual void UpdateChannels(float dt, const std::vector<FInputDeviceState>& devices,
        std::vector<float>& channelData /* raw storage managed by JInputSystem */) = 0;
};
