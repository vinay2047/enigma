#version 330 core

out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

#define MAX_LIGHTS 8

struct PointLight {
    vec3  position;
    vec3  color;
    float intensity;
    float radius;
};

uniform sampler2D textureDiffuse;
uniform bool      useTexture;
uniform vec3      objectColor;

uniform vec3       viewPos;
uniform PointLight lights[MAX_LIGHTS];
uniform int        numLights;
uniform float      ambientStrength;
uniform vec3       ambientColor;

// Fog
uniform float fogDensity;
uniform vec3  fogColor;

void main()
{
    vec3 baseColor = useTexture ? texture(textureDiffuse, TexCoord).rgb : objectColor;
    vec3 norm      = normalize(Normal);

    // Ambient
    vec3 result = ambientStrength * ambientColor * baseColor;

    for (int i = 0; i < numLights && i < MAX_LIGHTS; i++) {
        vec3  diff     = lights[i].position - FragPos;
        float dist     = length(diff);
        vec3  lightDir = normalize(diff);

        // Attenuation
        float atten = lights[i].intensity /
                      (1.0 + 0.22 * dist + 0.20 * dist * dist);
        atten *= max(0.0, 1.0 - dist / lights[i].radius);

        // Diffuse
        float dif = max(dot(norm, lightDir), 0.0);
        result += dif * lights[i].color * baseColor * atten;

        // Specular (Blinn-Phong)
        vec3  viewDir    = normalize(viewPos - FragPos);
        vec3  halfDir    = normalize(lightDir + viewDir);
        float spec       = pow(max(dot(norm, halfDir), 0.0), 64.0);
        result += spec * lights[i].color * 0.25 * atten;
    }

    // Exponential fog — very light, nearly disabled
    float fogFactor = exp(-fogDensity * 0.1 * length(FragPos - viewPos));
    fogFactor       = clamp(fogFactor, 0.6, 1.0);
    result          = mix(fogColor, result, fogFactor);

    // Minimum brightness floor so nothing is pitch black
    result = max(result, baseColor * 0.35);

    FragColor = vec4(result, 1.0);
}
