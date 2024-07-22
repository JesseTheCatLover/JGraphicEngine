// Copyright (c) 2024. JesseTheCatLover. All Rights Reserved.

#include "JShader.h"
#include <fstream>
#include <sstream>
#include <iostream>

JShader::JShader(const std::string &vertexPath, const std::string &fragmentPath)
{
    std::string vertexCode = LoadShaderSource(vertexPath);
    std::string fragmentCode = LoadShaderSource(fragmentPath);

    GLuint vertexShader = CompileShader(vertexCode, GL_VERTEX_SHADER);
    GLuint fragmentShader = CompileShader(fragmentCode, GL_FRAGMENT_SHADER);

    program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);

    GLint success;
    GLchar infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(program, 512, NULL, infoLog);
        std::cerr << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void JShader::Use()
{
    glUseProgram(program);
}

void JShader::SetBool(const std::string& name, bool value) const
{
    glUniform1i(glGetUniformLocation(program, name.c_str()), (int)value);
}

void JShader::SetInt(const std::string& name, int value) const
{
    glUniform1i(glGetUniformLocation(program, name.c_str()), value);
}

void JShader::SetFloat(const std::string& name, float value) const
{
    glUniform1f(glGetUniformLocation(program, name.c_str()), value);
}

void JShader::SetMat4(const std::string &name, const glm::mat4 &mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(program, name.c_str()), 1, GL_FALSE, &mat[0][0]);
}

std::string JShader::LoadShaderSource(const std::string& path)
{
    std::ifstream file;
    std::stringstream stream;
    std::string source;

    file.open("E:\\C++ Projects\\OpenGLProject\\OpenGLProject\\Shaders\\" + path);
    if (file.is_open()) {
        stream << file.rdbuf();
        source = stream.str();
        file.close();
    } else
    {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << path << std::endl;
    }

    return source;
}

GLuint JShader::CompileShader(const std::string& source, GLenum type)
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
        std::cerr << "ERROR::SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }

    return shader;
}

JShader::~JShader() {
    glDeleteProgram(program);
}
