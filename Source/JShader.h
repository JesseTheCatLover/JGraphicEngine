// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#pragma once

#include <string>
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

using namespace std;

class JShader
{
public:
    JShader(const string &VertexPath, const string &FragmentPath, const char* GeometryPath = nullptr);
    void Use();
    void SetBool(const string &name, bool value) const;
    void SetInt(const string &name, int value) const;
    void SetFloat(const string &name, float value) const;
    void SetVec2(const string &name, glm::vec2 value) const;
    void SetVec2(const string &name, float x, float y) const;
    void SetVec3(const string &name, glm::vec3 value) const;
    void SetVec3(const string &name, float x, float y, float z) const;
    void SetVec4(const string &name, glm::vec4 value) const;
    void SetVec4(const string &name, float x, float y, float z, float w) const;
    void SetMat2(const string &name, const glm::mat2 &mat) const;
    void SetMat3(const string &name, const glm::mat3 &mat) const;
    void SetMat4(const string &name, const glm::mat4 &mat) const;
    GLuint GetProgram() const { return program; }
    ~JShader();

private:
    GLuint program;
    string LoadShaderSource(const string &path);
    GLuint CompileShader(const string &source, GLenum type);
};
