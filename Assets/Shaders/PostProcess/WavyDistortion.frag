#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float time;

void main()
{
    float wave = sin(TexCoords.y * 10.0 + time * 2.0) * 0.02;
    vec2 uv = vec2(TexCoords.x + wave, TexCoords.y);
    FragColor = texture(screenTexture, uv);
}
