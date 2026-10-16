// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

class JEngine;

class IRuntimeLayer
{
public:
    virtual ~IRuntimeLayer() = default;

    virtual bool OnInitialize(JEngine& engine) = 0;
    virtual void OnTick(float deltaTime) = 0;
    virtual void OnRenderOverlay(float deltaTime) = 0;
    virtual void OnShutdown() = 0;
};