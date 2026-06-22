#pragma once

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <algorithm>
#include <cmath>

namespace Interaction
{
    // Core Slab Method for Ray vs Axis-Aligned Bounding Box (AABB)
    inline bool RayIntersectsAABB(glm::vec3 rayOrigin, glm::vec3 rayDir, glm::vec3 boxMin, glm::vec3 boxMax, float &tResult)
    {
        float tMin = 0.0f; // Prevent interacting with things behind the player
        float tMax = 8.0f; // Player interaction reach distance (e.g., 8 units)

        for (int i = 0; i < 3; ++i)
        {
            if (std::abs(rayDir[i]) < 1e-6f)
            {
                // Ray is parallel to this axis slab; check if origin is outside bounds
                if (rayOrigin[i] < boxMin[i] || rayOrigin[i] > boxMax[i])
                    return false;
            }
            else
            {
                float ood = 1.0f / rayDir[i];
                float t1 = (boxMin[i] - rayOrigin[i]) * ood;
                float t2 = (boxMax[i] - rayOrigin[i]) * ood;

                if (t1 > t2)
                    std::swap(t1, t2);

                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);

                if (tMin > tMax)
                    return false;
            }
        }

        tResult = tMin;
        return true;
    }
}
