//  Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "Scene/Components/Scene/JModelComponent.h"
#include "Core/Serialization/JsonWriter.h"
#include "Core/Serialization/JsonReader.h"
#include "Rendering/JShader.h"
#include "Resources/JResourceManager.h"
#include "Resources/JModelResource.h"

void JModelComponent::SetModel(const std::string& inPath)
{
    m_ModelPath = inPath;

    m_ModelResource = JResourceManager::Get().Load<JModelResource>(inPath, inPath);
}

void JModelComponent::Draw(JShader& shader) const
{
    if (auto modelRes = m_ModelResource.lock())
    {
        auto model = modelRes->GetModel();
        if (model)
        {
            // Get world transform from the scene graph
            FMatrix worldTransformMat4 = GetWorldTransform().ToMatrix();

            // Send it to the shader (as the "model matrix")
            shader.Use();
            shader.SetMat4("u_Model", worldTransformMat4.Get());

            // Now draw the model
            model->Draw(shader);
        }
    }
}

void JModelComponent::SerializeProperties(JsonWriter& writer) const
{
    Super::SerializeProperties(writer);

    // Save the path/key of the model
    writer.Write("model_path", m_ModelPath);
}

void JModelComponent::DeserializeProperties(const JsonReader& reader)
{
    Super::DeserializeProperties(reader);

    m_ModelPath = reader.Read("model_path", std::string{});

    // Lazy load: you can load the model now or defer it until Draw()
    if (!m_ModelPath.empty())
    {
        m_ModelResource = JResourceManager::Get().Load<JModelResource>(m_ModelPath, m_ModelPath);
    }
}
