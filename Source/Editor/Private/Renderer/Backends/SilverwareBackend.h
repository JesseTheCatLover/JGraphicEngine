// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Core/Memory/SmartPointers.h"
#include "Silverware/Backend/IRenderingContext.h"
#include "Silverware/Backends/RmlUi/RmlUiRuntime.h"
#include "Silverware/Core/Resources/LayeredResourceProvider.h"

class EditorRuntime;
class IPlatformWindow;

class SilverwareBackend
{
public:
    SilverwareBackend() = default;
    ~SilverwareBackend();

    SilverwareBackend(const SilverwareBackend&) = delete;
    SilverwareBackend& operator=(const SilverwareBackend&) = delete;

private:
    TUniquePtr<Silverware::IRenderingContext> m_RenderingContext;
    TUniquePtr<Silverware::FLayeredResourceProvider> m_ResourceProvider;
    TUniquePtr<Silverware::Backends::RmlUi::RmlUiRuntime> m_RmlUiRuntime;

public:
    bool Initialize(EditorRuntime& editorRuntime, IPlatformWindow& window);
    void Shutdown();

    bool Synchronize(Silverware::CoreWidget& root, Silverware::EBackendSyncMode mode);

    void Render(IPlatformWindow& window);

    bool IsInitialized() const { return m_RmlUiRuntime != nullptr; }

private:
    bool MountResources(EditorRuntime& editorRuntime);
    bool InitializeRuntime(IPlatformWindow& window);
};