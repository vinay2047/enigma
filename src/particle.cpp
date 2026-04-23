#include "particle.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cstdlib>
#include <cmath>

const float ParticleSystem::s_quad[] = {
    // x      y     u    v
    -0.5f, -0.5f,  0.f, 0.f,
     0.5f, -0.5f,  1.f, 0.f,
     0.5f,  0.5f,  1.f, 1.f,
    -0.5f, -0.5f,  0.f, 0.f,
     0.5f,  0.5f,  1.f, 1.f,
    -0.5f,  0.5f,  0.f, 1.f,
};

void ParticleSystem::init() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(s_quad), s_quad, GL_STATIC_DRAW);
    // pos xy
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float),(void*)0);
    // uv
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4*sizeof(float),(void*)(2*sizeof(float)));
    glBindVertexArray(0);
}

void ParticleSystem::emit(const glm::vec3& origin, int count,
                           glm::vec3 color, float speed, float life)
{
    for (int i = 0; i < count; i++) {
        if ((int)particles.size() >= MAX_PARTICLES) break;
        Particle p;
        p.position = origin + glm::vec3(
            ((rand()%200)/100.f - 1.f),
            ((rand()%200)/100.f - 1.f) * 0.3f,
            ((rand()%200)/100.f - 1.f));
        p.velocity = glm::vec3(
            ((rand()%200)/100.f-1.f)*speed,
            ((rand()%100)/100.f)*speed*0.5f,
            ((rand()%200)/100.f-1.f)*speed);
        p.color   = color;
        p.maxLife = life + ((rand()%100)/100.f)*life*0.5f;
        p.life    = p.maxLife;
        p.size    = 0.03f + ((rand()%30)/1000.f);
        particles.push_back(p);
    }
}

void ParticleSystem::update(float dt) {
    for (auto& p : particles) {
        p.life     -= dt;
        p.position += p.velocity * dt;
        p.velocity.y += 0.02f * dt;  // slow drift up
        p.alpha     = p.life / p.maxLife;
    }
    particles.erase(std::remove_if(particles.begin(), particles.end(),
        [](const Particle& p){ return p.life <= 0.f; }), particles.end());
}

void ParticleSystem::render(unsigned int shaderID,
                             const glm::mat4& view, const glm::mat4& proj)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    glUseProgram(shaderID);
    glUniformMatrix4fv(glGetUniformLocation(shaderID,"view"),
                       1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderID,"projection"),
                       1, GL_FALSE, glm::value_ptr(proj));

    glBindVertexArray(vao);
    for (const auto& p : particles) {
        glUniform3fv(glGetUniformLocation(shaderID,"particlePos"),
                     1, glm::value_ptr(p.position));
        glUniform1f (glGetUniformLocation(shaderID,"size"),   p.size);
        glUniform1f (glGetUniformLocation(shaderID,"Alpha"),  p.alpha);
        glUniform3fv(glGetUniformLocation(shaderID,"particleColor"),
                     1, glm::value_ptr(p.color));
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void ParticleSystem::shutdown() {
    glDeleteVertexArrays(1, &vao);
    glDeleteBuffers(1, &vbo);
}
