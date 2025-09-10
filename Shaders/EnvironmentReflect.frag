#version 330 core
out vec4 FragColor;

in vec3 Normal;
in vec3 Position;

uniform vec3 cameraPos;
uniform samplerCube skybox;

void main()
{
    // Incident vector: from surface point toward the camera
    vec3 I = normalize(Position - cameraPos);

    // Reflection vector: reflect I around the surface normal
    vec3 R = reflect(I, normalize(Normal));

    // Look up that reflection vector in the cubemap
    FragColor = vec4(texture(skybox, R).rgb, 1.0);
}
