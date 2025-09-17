#version 330 core

layout (location = 0) in vec2 aPos;    // Vertex positions (-1 to 1)
layout (location = 1) in vec2 aTexCoords;    // Texture coordinates (0 to 1)

out vec2 TexCoords;

void main()
{
    TexCoords = aTexCoords;           // Pass to fragment shader
    gl_Position = vec4(aPos, 0.0, 1.0); // Fullscreen quad in NDC
}
