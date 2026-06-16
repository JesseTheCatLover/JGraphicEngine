//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class IPanelSubsystem
{
public:
    virtual ~IPanelSubsystem() = default;
    virtual void Tick(float deltaTime) = 0;
};