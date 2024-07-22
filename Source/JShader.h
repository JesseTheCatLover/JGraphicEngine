// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

class JShader
{
public:
    JShader(const std::string &vertexPath, const std::string &fragmentPath);
    void Use();
    void SetBool(const std::string &name, bool value) const;
    void SetInt(const std::string &name, int value) const;
    void SetFloat(const std::string &name, float value) const;
    void SetMat4(const std::string &name, const glm::mat4 &mat) const;
    GLuint GetProgram() const { return program; }
    ~JShader();

private:
    GLuint program;
    std::string LoadShaderSource(const std::string &path);
    GLuint CompileShader(const std::string &source, GLenum type);
};