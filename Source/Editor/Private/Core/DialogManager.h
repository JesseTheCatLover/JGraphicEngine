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
    TDialog* OpenDialog()
    {
        TDialog* instance = FindDialogInstance<TDialog>();
        if (!instance)
        {
            auto ptr = MakeUnique<TDialog>();
            instance = ptr.get();
            instance->OnCreate(m_Host, m_Runtime);
            m_Dialogs.emplace_back(std::move(ptr));

            // First open
            instance->OnOpen(m_Host, m_Runtime);
            return instance;
        }

        // Is Instance already open?
        const bool bIsOpen = instance->IsOpen();

        if (!bIsOpen)
        {
            // Re-opening a closed dialog
            instance->OnOpen(m_Host, m_Runtime);
        }
        else
        {
            // Dialog is already open: just focus it
            instance->OnRequestFocus(m_Host, m_Runtime);
        }
    }

    void DrawDialogs();

private:
    EditorHost& m_Host;
    EditorRuntime& m_Runtime;
    std::vector<TUniquePtr<IEditorDialog>> m_Dialogs;

    template<typename TDialog>
    TDialog* FindDialogInstance()
    {
        for (auto& dlg : m_Dialogs)
            if (auto* casted = dynamic_cast<TDialog*>(dlg.get()))
                return casted;
        return nullptr;
    }
};