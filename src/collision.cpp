#include "collision.h"
#include <algorithm>
#include <limits>

float rayAABB(const glm::vec3& origin, const glm::vec3& dir, const AABB& box)
{
    float tmin = -std::numeric_limits<float>::infinity();
    float tmax =  std::numeric_limits<float>::infinity();

    for (int i = 0; i < 3; i++) {
        if (std::abs(dir[i]) < 1e-6f) {
            if (origin[i] < box.min[i] || origin[i] > box.max[i])
                return -1.f;
        } else {
            float t1 = (box.min[i] - origin[i]) / dir[i];
            float t2 = (box.max[i] - origin[i]) / dir[i];
            if (t1 > t2) std::swap(t1, t2);
            tmin = std::max(tmin, t1);
            tmax = std::min(tmax, t2);
        }
    }
    if (tmax < 0.f || tmin > tmax) return -1.f;
    return (tmin < 0.f) ? tmax : tmin;
}

glm::vec3 resolveCollision(glm::vec3 pos, const glm::vec3& half,
                            const AABB* walls, int count)
{
    for (int i = 0; i < count; i++) {
        const AABB& w = walls[i];
        AABB player = AABB::fromCenter(pos, half);
        if (!player.intersects(w)) continue;

        // Find minimum overlap axis and push out
        float ox = std::min(player.max.x - w.min.x, w.max.x - player.min.x);
        float oy = std::min(player.max.y - w.min.y, w.max.y - player.min.y);
        float oz = std::min(player.max.z - w.min.z, w.max.z - player.min.z);

        if (ox <= oy && ox <= oz)
            pos.x += (pos.x < (w.min.x + w.max.x) * 0.5f) ? -ox : ox;
        else if (oz <= oy)
            pos.z += (pos.z < (w.min.z + w.max.z) * 0.5f) ? -oz : oz;
        else
            pos.y += (pos.y < (w.min.y + w.max.y) * 0.5f) ? -oy : oy;
    }
    return pos;
}
