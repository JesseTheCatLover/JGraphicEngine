//  Copyright 2025-2026 JesseTheCatLover. All Rights Reserved.

#pragma once

#include <vector>
#include "Core/Memory/SmartPointers.h"
#include "UI/IEditorDialog.h"

class EditorHost;
class EditorRuntime;

class DialogManager
{
public:
    DialogManager(EditorHost& host, EditorRuntime& runtime)
        : m_Host(host)
        , m_Runtime(runtime)
    {}

    template<typename TDialog>
    TDialog* OpenDialog();

    void DrawDialogs();

private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    std::vector<TUniquePtr<IEditorDialog>> m_Dialogs;

    template<typename TDialog>
    TDialog* FindDialogInstance();
};