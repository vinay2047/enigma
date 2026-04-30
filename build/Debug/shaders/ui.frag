#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D uiTexture;
uniform bool      useTexture;
uniform vec4      color;
void main() {
    if (useTexture)
        FragColor = texture(uiTexture, TexCoord) * color;
    else
        FragColor = color;
}
