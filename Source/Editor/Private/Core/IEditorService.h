//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
class IEditorService
{
public:
    virtual ~IEditorService() = default;
    virtual void Tick(float deltaTime) {}
};