// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Core/IRuntimeLayer.h"

class SilverwareSandboxApp final : public IRuntimeLayer
{
public:
    bool OnInitialize(JEngine& engine) override;
    void OnTick(float deltaTime) override;
    void OnRenderOverlay(float deltaTime) override;
    void OnShutdown() override;
};