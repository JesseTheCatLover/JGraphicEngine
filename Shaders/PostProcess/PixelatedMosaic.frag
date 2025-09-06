#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform float pixels = 400.0; // Lower = bigger blocks

void main()
{
    vec2 uv = TexCoords;
    uv = floor(uv * pixels) / pixels; // Snap UV to grid
    FragColor = texture(screenTexture, uv);
}
