#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Particle {
    glm::vec3 position {0.f};
    glm::vec3 velocity {0.f};
    glm::vec3 color    {0.7f, 0.7f, 0.8f};
    float     life     {0.f};
    float     maxLife  {3.f};
    float     size     {0.04f};
    float     alpha    {1.f};
};

class ParticleSystem {
public:
    static constexpr int MAX_PARTICLES = 200;
    std::vector<Particle> particles;

    void init();
    void emit(const glm::vec3& origin, int count,
              glm::vec3 color = {0.7f, 0.7f, 0.8f},
              float speed = 0.2f, float life = 4.f);
    void update(float dt);
    void render(unsigned int shaderID,
                const glm::mat4& view, const glm::mat4& proj);
    void shutdown();

private:
    unsigned int vao {0}, vbo {0};
    // billboard quad vertices
    static const float s_quad[];
};
