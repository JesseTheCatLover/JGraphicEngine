// Copyright (c) 2025 JesseTheCatLover. All Rights Reserved.

#pragma once
#include <glad/gl.h>
#include <string>
#include <vector>
#include "JTexture.h"
#include "JShader.h"

class JSkybox
{
public:
    JSkybox(const std::string& skyboxDir, const std::string& shaderVert, const std::string& shaderFrag);
    ~JSkybox();

    void Draw(const glm::mat4& view, const glm::mat4& projection);

private:
    GLuint VAO, VBO;
    JTexture cubemap;
    JShader shader;

    void InitCube();
};
