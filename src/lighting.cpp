#include "lighting.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <string>
#include <algorithm>

void LightingSystem::update(float dt)
{
    static float t = 0.f;
    t += dt;
    for (auto& l : lights) {
        if (!l.flickers) { l.intensity = l.baseIntensity; continue; }
        float noise = std::sin(t * l.flickerSpeed + l.flickerPhase) * 0.5f
                    + std::sin(t * l.flickerSpeed * 2.3f + l.flickerPhase * 1.7f) * 0.3f;
        l.intensity = std::max(0.05f,
            l.baseIntensity + noise * l.flickerAmp);
    }
}

void LightingSystem::uploadToShader(unsigned int shaderID) const
{
    int n = (int)std::min((int)lights.size(), 8);
    glUniform1i(glGetUniformLocation(shaderID, "numLights"), n);

    for (int i = 0; i < n; i++) {
        std::string b = "lights[" + std::to_string(i) + "].";
        glUniform3fv(glGetUniformLocation(shaderID,(b+"position").c_str()),
                     1, glm::value_ptr(lights[i].position));
        glUniform3fv(glGetUniformLocation(shaderID,(b+"color").c_str()),
                     1, glm::value_ptr(lights[i].color));
        glUniform1f (glGetUniformLocation(shaderID,(b+"intensity").c_str()),
                     lights[i].intensity);
        glUniform1f (glGetUniformLocation(shaderID,(b+"radius").c_str()),
                     lights[i].radius);
    }
}
