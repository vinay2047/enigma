#pragma once
#include <glm/glm.hpp>

class Player {
public:
    glm::vec3 position    {0.f, 1.7f, 0.f};
    float     yaw         {-90.f};
    float     pitch       {0.f};
    float     speed       {4.0f};
    float     sensitivity {0.008f};
    float     height      {1.7f};
    float     crouchHeight{0.9f};
    bool      crouching   {false};
    bool      jumping     {false};
    float     jumpVel     {0.f};
    float     gravity     {-9.8f};

    // Derived
    glm::vec3 front {0.f, 0.f, -1.f};
    glm::vec3 right {1.f, 0.f,  0.f};
    glm::vec3 up    {0.f, 1.f,  0.f};

    glm::mat4 getViewMatrix() const;
    glm::vec3 eyePos() const { return position + glm::vec3(0.f, crouching ? crouchHeight : height, 0.f); }

    void processKeyboard(bool w, bool s, bool a, bool d,
                         bool jump, bool crouch, float dt);
    void processMouse(float dx, float dy);
    void update(float dt);          // gravity, jump arc
    void constrainPitch();

    // Player AABB (for collision)
    struct { float w=0.4f, h=1.8f; } extent;
};
