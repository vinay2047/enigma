#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "room.h"
#include "shader.h"
#include "particle.h"

// Vertex layout: pos(3) + normal(3) + uv(2)
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

struct Mesh {
    GLuint vao {0}, vbo {0}, ebo {0};
    int    indexCount {0};
};

class Renderer {
public:
    Shader mainShader;
    Shader uiShader;
    Shader particleShader;

    bool init(int screenW, int screenH);
    void shutdown();

    void renderRoom   (const Room& room, const glm::mat4& view,
                       const glm::mat4& proj, const glm::vec3& eyePos);
    void renderObject (const GameObject& obj, const Shader& sh);
    void renderUI     (int screenW, int screenH);

    // Mesh factory
    static Mesh buildBox();
    static Mesh buildQuad2D();

    // 2D rect draw (used by UI)
    void drawRect(float x, float y, float w, float h,
                  glm::vec4 color, const glm::mat4& proj);
    void drawTexturedRect(float x,float y,float w,float h,
                          GLuint tex, glm::vec4 tint,
                          const glm::mat4& proj);

    Mesh  boxMesh;
    Mesh  quadMesh;
    float screenW {1280.f}, screenH {720.f};
};
