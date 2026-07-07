//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once

class ShellCommandService;

class IPanelSubsystem
{
public:
    virtual ~IPanelSubsystem() = default;
    virtual void Tick(float deltaTime) = 0;
    virtual void RegisterShellCommands(ShellCommandService& shell){}
};