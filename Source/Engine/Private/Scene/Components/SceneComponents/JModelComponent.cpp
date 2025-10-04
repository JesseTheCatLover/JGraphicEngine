//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/SceneComponents/JModelComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Rendering/JShader.h"
#include "Resources/JResourceManager.h"
#include "Resources/JModelResource.h"

void JModelComponent::SetModel(const std::string& InPath)
{
    m_ModelPath = InPath;

    m_ModelResource = JResourceManager::Get().Load<JModelResource>(InPath, InPath);
}

void JModelComponent::Draw(JShader& Shader) const
{
    if (auto modelRes = m_ModelResource.lock())
    {
        auto model = modelRes->GetModel();
        if (model)
        {
            // Get world transform from the scene graph
            glm::mat4 worldTransform = GetWorldTransform();

            // Send it to the shader (as the "model matrix")
            Shader.Use();
            Shader.SetMat4("u_Model", worldTransform);

            // Now draw the model
            model->Draw(Shader);
        }
    }
}

void JModelComponent::SerializeProperties(JsonWriter& Writer) const
{
    Super::SerializeProperties(Writer);

    // Save the path/key of the model
    Writer.Write("model_path", m_ModelPath);
}

void JModelComponent::DeserializeProperties(const JsonReader& Reader)
{
    Super::DeserializeProperties(Reader);

    m_ModelPath = Reader.Read("model_path", std::string{});

    // Lazy load: you can load the model now or defer it until Draw()
    if (!m_ModelPath.empty())
    {
        m_ModelResource = JResourceManager::Get().Load<JModelResource>(m_ModelPath, m_ModelPath);
    }
}
