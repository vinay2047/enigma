#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
in float Alpha;
uniform vec3 particleColor;
void main() {
    float dist = length(TexCoord - vec2(0.5));
    float a    = smoothstep(0.5, 0.1, dist) * Alpha;
    FragColor  = vec4(particleColor, a);
}
