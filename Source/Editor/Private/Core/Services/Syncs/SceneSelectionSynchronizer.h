//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstddef>

#include "Core/IEditorService.h"

class EditorRuntime;
class EditorHost;

class SceneSelectionSynchronizer : public IEditorService
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    std::size_t m_ListenerID = 0;

public:
    explicit SceneSelectionSynchronizer(EditorHost& host, EditorRuntime& runtime);
    ~SceneSelectionSynchronizer();

    void Bind(EditorHost& host);
    void Unbind(EditorHost& host);
    void SyncNow(EditorHost& host);
};
