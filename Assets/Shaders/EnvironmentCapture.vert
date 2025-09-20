#version 330 core
layout (location = 0) in vec3 aPos;      // Vertex position
layout (location = 1) in vec3 aNormal;   // Vertex normal

out vec3 Normal;     // Pass to fragment
out vec3 Position;   // Pass to fragment

layout (std140) uniform CameraData
{
    mat4 u_Projection;
    mat4 u_View;
};
uniform mat4 model;

void main()
{
    // Transform normal into world space
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Transform position into world space
    Position = vec3(model * vec4(aPos, 1.0));

    // Standard MVP transform for rasterization
    gl_Position = u_Projection * u_View * vec4(Position, 1.0);
}
