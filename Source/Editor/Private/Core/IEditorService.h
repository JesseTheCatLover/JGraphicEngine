//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once
class ShellCommandService;

class IEditorService
{
public:
    virtual ~IEditorService() = default;
    virtual void Tick(float deltaTime) {}
    virtual void RegisterShellCommands(ShellCommandService& shell){}
};
