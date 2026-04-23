#pragma once
#include <glm/glm.hpp>

struct AABB {
    glm::vec3 min{0.f};
    glm::vec3 max{0.f};

    bool intersects(const AABB& o) const {
        return (min.x <= o.max.x && max.x >= o.min.x) &&
               (min.y <= o.max.y && max.y >= o.min.y) &&
               (min.z <= o.max.z && max.z >= o.min.z);
    }
    bool containsPoint(const glm::vec3& p) const {
        return p.x >= min.x && p.x <= max.x &&
               p.y >= min.y && p.y <= max.y &&
               p.z >= min.z && p.z <= max.z;
    }
    // Expand by half-extents
    static AABB fromCenter(const glm::vec3& c, const glm::vec3& half) {
        return {c - half, c + half};
    }
};

// Ray-AABB intersection; returns t (distance) or -1 if no hit
float rayAABB(const glm::vec3& origin, const glm::vec3& dir,
              const AABB& box);

// Slide-based collision response: returns corrected position
glm::vec3 resolveCollision(glm::vec3 newPos, const glm::vec3& half,
                            const AABB* walls, int count);
