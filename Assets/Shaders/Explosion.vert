#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoords;

out VS_OUT
{
    vec2 TexCoords;
} vs_out;

layout (std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
};
uniform mat4 u_Model;

void main()
{
    vs_out.TexCoords = aTexCoords;
    gl_Position = u_Projection * u_View * u_Model * vec4(aPos, 1.0);
}