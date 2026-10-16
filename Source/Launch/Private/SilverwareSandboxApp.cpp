// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#include "SilverwareSandboxApp.h"

#include <iostream>

#include "Core/JEngine.h"
#include "Rendering/IPlatformWindow.h"

bool SilverwareSandboxApp::OnInitialize(JEngine& engine)
{
    IPlatformWindow* window = engine.GetPrimaryWindow();

    if (!window)
    {
        std::cerr << "[SilverwareSandbox]: No primary window available.\n";
        return false;
    }

    window->SetTitle("Silverware Sandbox");

    std::cout << "[SilverwareSandbox]: Initialized.\n";
    return true;
}

void SilverwareSandboxApp::OnTick(float deltaTime)
{
    (void)deltaTime;
}

void SilverwareSandboxApp::OnRenderOverlay(float deltaTime)
{
    (void)deltaTime;
}

void SilverwareSandboxApp::OnShutdown()
{
    std::cout << "[SilverwareSandbox]: Shutdown.\n";
}