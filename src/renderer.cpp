#include "renderer.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cstring>

// ── Box mesh (unit cube centered at origin) ────────────────────────────────
Mesh Renderer::buildBox() {
    // 24 vertices (4 per face × 6 faces), pos+normal+uv
    float v[] = {
        // Back face (-Z)
        -0.5f,-0.5f,-0.5f, 0,0,-1, 0,0,  0.5f,-0.5f,-0.5f, 0,0,-1, 1,0,
         0.5f, 0.5f,-0.5f, 0,0,-1, 1,1, -0.5f, 0.5f,-0.5f, 0,0,-1, 0,1,
        // Front face (+Z)
        -0.5f,-0.5f, 0.5f, 0,0,1, 0,0,  0.5f,-0.5f, 0.5f, 0,0,1, 1,0,
         0.5f, 0.5f, 0.5f, 0,0,1, 1,1, -0.5f, 0.5f, 0.5f, 0,0,1, 0,1,
        // Left face (-X)
        -0.5f, 0.5f, 0.5f,-1,0,0, 1,0, -0.5f, 0.5f,-0.5f,-1,0,0, 1,1,
        -0.5f,-0.5f,-0.5f,-1,0,0, 0,1, -0.5f,-0.5f, 0.5f,-1,0,0, 0,0,
        // Right face (+X)
         0.5f, 0.5f, 0.5f, 1,0,0, 1,0,  0.5f, 0.5f,-0.5f, 1,0,0, 1,1,
         0.5f,-0.5f,-0.5f, 1,0,0, 0,1,  0.5f,-0.5f, 0.5f, 1,0,0, 0,0,
        // Bottom (-Y)
        -0.5f,-0.5f,-0.5f, 0,-1,0, 0,1,  0.5f,-0.5f,-0.5f, 0,-1,0, 1,1,
         0.5f,-0.5f, 0.5f, 0,-1,0, 1,0, -0.5f,-0.5f, 0.5f, 0,-1,0, 0,0,
        // Top (+Y)
        -0.5f, 0.5f,-0.5f, 0,1,0, 0,1,  0.5f, 0.5f,-0.5f, 0,1,0, 1,1,
         0.5f, 0.5f, 0.5f, 0,1,0, 1,0, -0.5f, 0.5f, 0.5f, 0,1,0, 0,0,
    };
    unsigned int idx[] = {
        0,1,2, 2,3,0,   4,5,6, 6,7,4,
        8,9,10,10,11,8, 12,13,14,14,15,12,
        16,17,18,18,19,16, 20,21,22,22,23,20
    };
    Mesh m;
    glGenVertexArrays(1,&m.vao);
    glGenBuffers(1,&m.vbo);
    glGenBuffers(1,&m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
    int stride = 8*sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,stride,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,stride,(void*)(3*sizeof(float)));
    glEnableVertexAttribArray(2); glVertexAttribPointer(2,2,GL_FLOAT,GL_FALSE,stride,(void*)(6*sizeof(float)));
    glBindVertexArray(0);
    m.indexCount = 36;
    return m;
}

Mesh Renderer::buildQuad2D() {
    float v[] = {
        0.f,0.f, 0.f,0.f,  1.f,0.f, 1.f,0.f,
        1.f,1.f, 1.f,1.f,  0.f,1.f, 0.f,1.f
    };
    unsigned int idx[] = {0,1,2, 2,3,0};
    Mesh m;
    glGenVertexArrays(1,&m.vao);
    glGenBuffers(1,&m.vbo);
    glGenBuffers(1,&m.ebo);
    glBindVertexArray(m.vao);
    glBindBuffer(GL_ARRAY_BUFFER,m.vbo);
    glBufferData(GL_ARRAY_BUFFER,sizeof(v),v,GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,m.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,sizeof(idx),idx,GL_STATIC_DRAW);
    int stride=4*sizeof(float);
    glEnableVertexAttribArray(0); glVertexAttribPointer(0,2,GL_FLOAT,GL_FALSE,stride,(void*)0);
    glEnableVertexAttribArray(1); glVertexAttribPointer(1,2,GL_FLOAT,GL_FALSE,stride,(void*)(2*sizeof(float)));
    glBindVertexArray(0);
    m.indexCount=6;
    return m;
}

bool Renderer::init(int w, int h) {
    screenW=(float)w; screenH=(float)h;
    mainShader     = Shader("shaders/main.vert",    "shaders/main.frag");
    uiShader       = Shader("shaders/ui.vert",      "shaders/ui.frag");
    particleShader = Shader("shaders/particle.vert","shaders/particle.frag");
    boxMesh  = buildBox();
    quadMesh = buildQuad2D();
    return mainShader.ID && uiShader.ID;
}

void Renderer::shutdown() {
    glDeleteVertexArrays(1,&boxMesh.vao);  glDeleteBuffers(1,&boxMesh.vbo);  glDeleteBuffers(1,&boxMesh.ebo);
    glDeleteVertexArrays(1,&quadMesh.vao); glDeleteBuffers(1,&quadMesh.vbo); glDeleteBuffers(1,&quadMesh.ebo);
}

void Renderer::renderObject(const GameObject& obj, const Shader& sh) {
    if (!obj.visible) return;

    glm::mat4 model = glm::mat4(1.f);
    model = glm::translate(model, obj.transform.position);

    // Door hinge rotation (around Y axis at hinge edge)
    if (obj.isDoor && obj.curAngle != 0.f) {
        glm::vec3 hinge = obj.transform.position;
        // Pick hinge edge based on door orientation
        if (obj.transform.scale.x > obj.transform.scale.z) {
            // Door spans X axis (e.g. north/south wall) — hinge on left X edge
            hinge.x -= obj.transform.scale.x * 0.5f;
        } else {
            // Door spans Z axis (e.g. east/west wall) — hinge on near Z edge
            hinge.z -= obj.transform.scale.z * 0.5f;
        }
        model = glm::mat4(1.f);
        model = glm::translate(model, hinge);
        model = glm::rotate(model, glm::radians(obj.curAngle), obj.doorAxis);
        model = glm::translate(model, -hinge + obj.transform.position);
    }

    // General Y rotation
    if (obj.transform.rotation.y != 0.f && !obj.isDoor)
        model = glm::rotate(model, glm::radians(obj.transform.rotation.y), {0.f,1.f,0.f});

    model = glm::scale(model, obj.transform.scale);
    sh.setMat4("model", model);

    // Material
    sh.setBool("useTexture", obj.material.useTexture);
    sh.setVec3("objectColor", obj.material.color);
    sh.setFloat("shininess", obj.material.shininess);
    if (obj.material.useTexture) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, obj.material.diffuseTex);
        sh.setInt("textureDiffuse", 0);
    }

    glBindVertexArray(boxMesh.vao);
    glDrawElements(GL_TRIANGLES, boxMesh.indexCount, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::renderRoom(const Room& room, const glm::mat4& view,
                           const glm::mat4& proj, const glm::vec3& eyePos)
{
    glEnable(GL_DEPTH_TEST);
    // No backface culling — player is INSIDE the room boxes,
    // so interior faces must be visible from both sides.
    glDisable(GL_CULL_FACE);

    mainShader.use();
    mainShader.setMat4("view",       view);
    mainShader.setMat4("projection", proj);
    mainShader.setVec3("viewPos",    eyePos);
    mainShader.setFloat("ambientStrength", room.ambientStrength);
    mainShader.setVec3("ambientColor",     room.ambientColor);
    mainShader.setFloat("fogDensity",      room.fogDensity);
    mainShader.setVec3("fogColor",         room.fogColor);

    room.lighting.uploadToShader(mainShader.ID);

    for (const auto& obj : room.objects)
        renderObject(obj, mainShader);
}

void Renderer::drawRect(float x, float y, float w, float h,
                         glm::vec4 color, const glm::mat4& proj)
{
    uiShader.use();
    uiShader.setBool("useTexture", false);
    uiShader.setVec4("color", color);

    glm::mat4 model = glm::mat4(1.f);
    model = glm::translate(model, {x, y, 0.f});
    model = glm::scale(model, {w, h, 1.f});
    // Combine into projection
    uiShader.setMat4("projection", proj * model);

    glBindVertexArray(quadMesh.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

void Renderer::drawTexturedRect(float x, float y, float w, float h,
                                 GLuint tex, glm::vec4 tint, const glm::mat4& proj)
{
    uiShader.use();
    uiShader.setBool("useTexture", true);
    uiShader.setVec4("color", tint);
    uiShader.setInt("uiTexture", 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex);

    glm::mat4 model = glm::translate(glm::mat4(1.f),{x,y,0.f});
    model = glm::scale(model,{w,h,1.f});
    uiShader.setMat4("projection", proj * model);

    glBindVertexArray(quadMesh.vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
