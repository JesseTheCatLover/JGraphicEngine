#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

layout (std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
};
uniform mat4 model;
uniform float outlineThickness;

void main()
{
    // Push vertex along its normal
    vec3 pos = aPos + aNormal * outlineThickness;
    gl_Position = u_Projection * u_View * model * vec4(pos, 1.0);
}
