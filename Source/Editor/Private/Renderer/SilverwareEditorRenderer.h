// Copyright 2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include "Core/Memory/SmartPointers.h"

namespace Silverware
{
    class ContainerWidget;
}

class EditorHost;
class EditorRuntime;
class SilverwareBackend;

class SilverwareEditorRenderer
{
public:
    SilverwareEditorRenderer();
    ~SilverwareEditorRenderer();

    SilverwareEditorRenderer(const SilverwareEditorRenderer&) = delete;
    SilverwareEditorRenderer& operator=(const SilverwareEditorRenderer&) = delete;

private:
    bool BuildEditorLayout();

    EditorHost* m_EditorHost = nullptr;
    EditorRuntime* m_EditorRuntime = nullptr;
    SilverwareBackend* m_Backend = nullptr;

    TUniquePtr<Silverware::ContainerWidget> m_RootWidget;

public:
    bool Initialize(EditorHost& editorHost, EditorRuntime& editorRuntime, SilverwareBackend& backend);

    void Tick(float deltaTime);
    void Shutdown();
};