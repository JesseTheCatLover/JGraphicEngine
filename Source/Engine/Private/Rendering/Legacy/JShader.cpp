// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#include "JShader.h"

#include <fstream>
#include <sstream>
#include <iostream>

JShader::JShader(const string &VertexPath, const string &FragmentPath, const char *GeometryPath)
{
    m_Program = glCreateProgram();

    // Vertex
    string VertexCode = LoadShaderSource(VertexPath + ".vert");
    GLuint VertexShader = CompileShader(VertexCode, GL_VERTEX_SHADER);
    glAttachShader(m_Program, VertexShader);

    // Fragment
    string FragmentCode = LoadShaderSource(FragmentPath + ".frag");
    GLuint FragmentShader = CompileShader(FragmentCode, GL_FRAGMENT_SHADER);
    glAttachShader(m_Program, FragmentShader);

    // Optional Geometry
    GLuint GeometryShader = 0;
    if (GeometryPath)
    {
        string GeometryCode = LoadShaderSource(string(GeometryPath) + ".geom");
        GeometryShader = CompileShader(GeometryCode, GL_GEOMETRY_SHADER);
        glAttachShader(m_Program, GeometryShader);
    }

    // Link once
    glLinkProgram(m_Program);
    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(m_Program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_Program, 512, NULL, infoLog);
        cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << endl;
    }

    // Cleanup
    glDeleteShader(VertexShader);
    glDeleteShader(FragmentShader);
    if (GeometryShader) glDeleteShader(GeometryShader);
}


void JShader::Use()
{
    glUseProgram(m_Program);
}

void JShader::SetBool(const string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(m_Program, name.c_str()), (int)value);
}

void JShader::SetInt(const string& name, int value) const
{
    glUniform1i(glGetUniformLocation(m_Program, name.c_str()), value);
}

void JShader::SetFloat(const string& name, float value) const
{
    glUniform1f(glGetUniformLocation(m_Program, name.c_str()), value);
}

void JShader::SetVec2(const string &name, glm::vec2 value) const
{
    glUniform2fv(glGetUniformLocation(m_Program, name.c_str()), 1, &value[0]);
}

void JShader::SetVec2(const string &name, float x, float y) const
{
    glUniform2f(glGetUniformLocation(m_Program, name.c_str()), x, y);
}

void JShader::SetVec3(const string &name, glm::vec3 value) const
{
    glUniform3fv(glGetUniformLocation(m_Program, name.c_str()), 1, &value[0]);
}

void JShader::SetVec3(const string &name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(m_Program, name.c_str()), x, y, z);
}

void JShader::SetVec4(const string &name, glm::vec4 value) const
{
    glUniform4fv(glGetUniformLocation(m_Program, name.c_str()), 1, &value[0]);
}

void JShader::SetVec4(const string &name, float x, float y, float z, float w) const
{
    glUniform4f(glGetUniformLocation(m_Program, name.c_str()), x, y, z, w);
}

void JShader::SetMat2(const string &name, const glm::mat2 &mat) const
{
    glUniformMatrix2fv(glGetUniformLocation(m_Program, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void JShader::SetMat3(const string &name, const glm::mat3 &mat) const
{
    glUniformMatrix3fv(glGetUniformLocation(m_Program, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void JShader::SetMat4(const string &name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(m_Program, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

void JShader::LinkUniformBlock(const std::string &blockName, GLuint bindingPoint) const
{
    GLuint blockIndex = glGetUniformBlockIndex(m_Program, blockName.c_str());
    if (blockIndex != GL_INVALID_INDEX)
    {
        glUniformBlockBinding(m_Program, blockIndex, bindingPoint);
    }
    else
    {
        std::cerr << "[JShader] Warning: UBO block '" << blockName << "' not found in shader.\n";
    }
}

string JShader::LoadShaderSource(const string& path)
{
    ifstream file;
    stringstream stream;
    string source;

    file.open(string(ENGINE_DIRECTORY) + "/Assets/Shaders/" + path);
    if (file.is_open()) {
        stream << file.rdbuf();
        source = stream.str();
        file.close();
    } else
    {
        cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << path << endl;
    }

    return source;
}

GLuint JShader::CompileShader(const string& source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    const char* src = source.c_str();
    glShaderSource(shader, 1, &src, NULL);
    glCompileShader(shader);

    GLint success;
    GLchar infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << endl;
    }

    return shader;
}

JShader::~JShader() {
    glDeleteProgram(m_Program);
}
