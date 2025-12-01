//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "EngineEditor.h"
#include "Core/JEngine.h"

EngineEditor::EngineEditor():
m_Context(JEngine::Get().GetEngineContext()),
m_SceneManager(*JEngine::Get().GetSceneManager()),
m_Renderer(*JEngine::Get().GetRenderer()),
m_SceneAPI(m_Context, m_SceneManager),
m_ViewportAPI(m_Context, m_Renderer)
{
}
