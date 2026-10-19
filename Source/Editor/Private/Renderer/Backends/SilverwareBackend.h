// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <span>

#include "Core/Memory/SmartPointers.h"
#include "Silverware/Backend/IRenderingContext.h"
#include "Silverware/Backends/RmlUi/RmlUiRuntime.h"
#include "Silverware/Core/Fonts/FontRegistry.h"
#include "Silverware/Core/Resources/LayeredResourceProvider.h"

class EditorRuntime;
class IPlatformWindow;

class SilverwareBackend
{
private:
    TUniquePtr<Silverware::IRenderingContext> m_RenderingContext;
    TUniquePtr<Silverware::FLayeredResourceProvider> m_ResourceProvider;
    TUniquePtr<Silverware::FontRegistry> m_FontRegistry;
    TUniquePtr<Silverware::Backends::RmlUi::RmlUiRuntime> m_RmlUiRuntime;

public:
    SilverwareBackend() = default;
    ~SilverwareBackend();

    SilverwareBackend(const SilverwareBackend&) = delete;
    SilverwareBackend& operator=(const SilverwareBackend&) = delete;

    [[nodiscard]] bool Initialize(EditorRuntime& editorRuntime, IPlatformWindow& window,
        std::span<const Silverware::FResourcePath> registeredFonts);

    [[nodiscard]] bool Initialize(EditorRuntime& editorRuntime, IPlatformWindow& window);

    void Shutdown();

    [[nodiscard]] bool Synchronize(Silverware::CoreWidget& root, Silverware::EBackendSyncMode mode);

    [[nodiscard]] bool TryGetWidgetSize(const Silverware::CoreWidget& widget, float& outWidth, float& outHeight) const;

    void Render(IPlatformWindow& window);
    [[nodiscard]] bool IsInitialized() const noexcept;

private:
    [[nodiscard]] bool MountResources(EditorRuntime& editorRuntime);
    [[nodiscard]] bool InitializeRuntime(IPlatformWindow& window);
};