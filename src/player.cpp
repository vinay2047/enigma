#include "player.h"
#include <glm/gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

glm::mat4 Player::getViewMatrix() const {
    return glm::lookAt(eyePos(), eyePos() + front, up);
}

void Player::processMouse(float dx, float dy)
{
    // Clamp delta to prevent huge jumps (common in WSL)
    dx = std::max(-50.f, std::min(50.f, dx));
    dy = std::max(-50.f, std::min(50.f, dy));

    yaw   += dx * sensitivity;
    pitch -= dy * sensitivity;
    constrainPitch();

    glm::vec3 f;
    f.x = std::cos(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    f.y = std::sin(glm::radians(pitch));
    f.z = std::sin(glm::radians(yaw)) * std::cos(glm::radians(pitch));
    front = glm::normalize(f);
    right = glm::normalize(glm::cross(front, glm::vec3(0.f, 1.f, 0.f)));
    up    = glm::normalize(glm::cross(right, front));
}

void Player::processKeyboard(bool w, bool s, bool a, bool d,
                              bool jump, bool crouch, float dt)
{
    glm::vec3 flatFront = glm::normalize(glm::vec3(front.x, 0.f, front.z));
    float spd = speed * (crouching ? 0.5f : 1.0f);

    if (w) position += flatFront * spd * dt;
    if (s) position -= flatFront * spd * dt;
    if (a) position -= right    * spd * dt;
    if (d) position += right    * spd * dt;

    // Crouch toggle
    crouching = crouch;

    // Jump
    if (jump && !jumping) {
        jumping = true;
        jumpVel = 4.5f;
    }
}

void Player::update(float dt)
{
    if (jumping) {
        jumpVel  += gravity * dt;
        position.y += jumpVel * dt;

        float floor = 0.f;  // ground level
        if (position.y <= floor) {
            position.y = floor;
            jumping    = false;
            jumpVel    = 0.f;
        }
    }
}

void Player::constrainPitch() {
    pitch = std::max(-89.f, std::min(89.f, pitch));
}
