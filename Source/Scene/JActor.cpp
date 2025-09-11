// Copyright 2025 JesseTheCatLover. All Rights Reserved.

#include "JActor.h"

#include "../Rendering/JModel.h"
#include "../Rendering/JShader.h"
#include "glm/ext/matrix_transform.hpp"


glm::mat4 JActor::GetModelMatrix() const
{
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, Position);
    model = glm::rotate(model, glm::radians(Rotation.x), glm::vec3(1,0,0));
    model = glm::rotate(model, glm::radians(Rotation.y), glm::vec3(0,1,0));
    model = glm::rotate(model, glm::radians(Rotation.z), glm::vec3(0,0,1));
    model = glm::scale(model, Scale);
    return model;
}

void JActor::Draw(JShader &shader) const
{
    shader.Use();
    shader.SetMat4("model", GetModelMatrix());
    Model->Draw(shader);
}

void JActor::DrawConfig(JShader& shader, JShader &outlineShader) const
{
    if (Config.bDrawOutline)
    {
        glEnable(GL_DEPTH_TEST);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE); // set stencil to 1 where fragments are drawn
        glStencilFunc(GL_ALWAYS, 1, 0xFF);
        glStencilMask(0xFF);
    }
    if (Config.bBackCulling)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_BACK);
    }
    Draw(shader);
    glDisable(GL_CULL_FACE);

    if (Config.bDrawOutline)
    {
        glEnable(GL_CULL_FACE);
        glCullFace(GL_FRONT); // render only backfaces
        glEnable(GL_DEPTH_TEST);

        outlineShader.Use();
        outlineShader.SetFloat("outlineThickness", Config.OutlineThickness); // adjust thickness
        Draw(outlineShader);

        // Draw normal model again to cover inner faces
        glCullFace(GL_BACK); // render frontfaces
        Draw(shader);

        // Reset
        glDisable(GL_CULL_FACE);
        glStencilMask(0xFF);
        glStencilFunc(GL_ALWAYS, 0, 0xFF);
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    }
}