#version 330 core
layout (location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    TexCoords = aPos;

    // Remove translation from view (so skybox stays centered around camera)
    vec4 pos = projection * view * vec4(aPos, 1.0);

    // Trick: force depth to 1.0 by making z=w
    gl_Position = pos.xyww;
}
