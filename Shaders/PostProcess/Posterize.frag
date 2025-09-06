#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;
uniform int levels = 5; // e.g., 5

void main() {
    vec3 color = texture(screenTexture, TexCoords).rgb;
    color = floor(color * float(levels)) / float(levels);
    FragColor = vec4(color, 1.0);
}
