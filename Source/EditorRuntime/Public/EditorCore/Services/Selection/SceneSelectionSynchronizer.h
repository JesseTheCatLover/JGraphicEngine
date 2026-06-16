//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <cstddef>

#include "EditorCore/IEditorService.h"
#include "Core/Delegates/TDelegateSubscription.h"
#include "Core/Delegates/TMulticastDelegate.h"

class EditorRuntime;
class EditorHost;

class SceneSelectionSynchronizer : public IEditorService
{
private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;

    // Auto-unsubscribes in destructor
    TDelegateSubscription<TMulticastDelegate<>> m_SelectionChangedSub;

public:
    explicit SceneSelectionSynchronizer(EditorHost& host, EditorRuntime& runtime);
    ~SceneSelectionSynchronizer() override = default;

    void Bind();
    void Unbind();
    void SyncNow();
};
