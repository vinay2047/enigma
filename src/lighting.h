#pragma once
#include <glm/glm.hpp>
#include <vector>

struct PointLight {
    glm::vec3 position  {0.f};
    glm::vec3 color     {1.f};
    float     intensity {1.f};
    float     radius    {10.f};
    // Flicker
    bool      flickers  {false};
    float     flickerSpeed {8.f};
    float     flickerAmp   {0.3f};
    float     baseIntensity{1.f};
    float     flickerPhase {0.f};
};

class LightingSystem {
public:
    std::vector<PointLight> lights;

    void addLight(const PointLight& l) { lights.push_back(l); }
    void clear()                       { lights.clear(); }
    void update(float dt);             // advance flicker timers

    // Upload up to 8 lights to the bound shader
    void uploadToShader(unsigned int shaderID) const;
};
