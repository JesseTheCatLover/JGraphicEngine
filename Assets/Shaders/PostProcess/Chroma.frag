#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float offset = 0.005;

void main()
{
    vec3 col;
    col.r = texture(screenTexture, TexCoords + vec2(offset, 0.0)).r;
    col.g = texture(screenTexture, TexCoords).g;
    col.b = texture(screenTexture, TexCoords - vec2(offset, 0.0)).b;
    FragColor = vec4(col, 1.0);
}
