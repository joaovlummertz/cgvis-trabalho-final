#include "Player.h"

#include <cmath>
#include <cstdio>
#include <glm/gtx/norm.hpp>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <glad/glad.h>

#include "SceneData.h"
#include "InputHandler.h"

namespace Player
{
    glm::vec4 ResolveCollisions(glm::vec4 position, bool noClip, bool *hit_floor = nullptr, bool *hit_ceiling = nullptr, glm::vec3 *ground_normal = nullptr);

    namespace
    {
        constexpr float kGravity = 12.0f;
        constexpr float kJumpSpeed = 9.0f;
        constexpr float kMaxFallSpeed = 25.0f;
        constexpr float kStepSize = 0.05f;
        constexpr float kGroundSnapDistance = 0.08f;
        constexpr float kCollisionSkin = 0.002f;
        constexpr float kSlopeLimitY = 0.55f;

        glm::vec4 ProjectOntoPlane(glm::vec4 v, glm::vec3 normal)
        {
            glm::vec3 vv = glm::vec3(v);
            glm::vec3 projected = vv - glm::dot(vv, normal) * normal;
            return glm::vec4(projected, v.w);
        }

        glm::vec4 ResolveMovement(glm::vec4 start_position,
                                  glm::vec4 displacement,
                                  bool noClip,
                                  bool *hit_floor = nullptr,
                                  bool *hit_ceiling = nullptr,
                                  glm::vec3 *ground_normal = nullptr)
        {
            if (hit_floor != nullptr)
                *hit_floor = false;
            if (hit_ceiling != nullptr)
                *hit_ceiling = false;
            if (ground_normal != nullptr)
                *ground_normal = glm::vec3(0.0f, -1.0f, 0.0f);

            float dist = glm::length(displacement);
            int num_steps = std::max(1, (int)std::ceil(dist / kStepSize));
            glm::vec4 step_displacement = displacement / (float)num_steps;
            glm::vec4 current_pos = start_position;

            for (int i = 0; i < num_steps; ++i)
            {
                glm::vec4 next_pos = current_pos + step_displacement;
                glm::vec3 before = glm::vec3(current_pos);
                glm::vec3 after = glm::vec3(next_pos);

                current_pos = ResolveCollisions(next_pos, noClip, hit_floor, hit_ceiling, ground_normal);

                if (hit_floor != nullptr || hit_ceiling != nullptr)
                {
                    glm::vec3 resolved = glm::vec3(current_pos);
                    if (hit_floor != nullptr && after.y < before.y && resolved.y >= after.y)
                        *hit_floor = true;
                    if (hit_ceiling != nullptr && after.y > before.y && resolved.y <= after.y)
                        *hit_ceiling = true;

                    // Vertical movement is handled in its own pass. Once the
                    // floor or ceiling blocks that pass, consuming the
                    // remaining substeps only repeats the same penetration and
                    // correction, which appears as contact jitter.
                    if ((step_displacement.y < 0.0f && hit_floor != nullptr && *hit_floor) ||
                        (step_displacement.y > 0.0f && hit_ceiling != nullptr && *hit_ceiling))
                    {
                        break;
                    }
                }
            }

            return current_pos;
        }
    }

    PlayerState playerState;

    glm::vec4 GetForwardVector()
    {
        return glm::vec4(
            cosf(playerState.g_PlayerPitch) * sinf(playerState.g_PlayerYaw),
            sinf(playerState.g_PlayerPitch),
            cosf(playerState.g_PlayerPitch) * cosf(playerState.g_PlayerYaw),
            0.0f);
    }

    // Robust triangle-point closest point algorithm (Ericson, Real-Time Collision Detection)
    glm::vec3 ClosestPointTriangle(glm::vec3 p, glm::vec3 a, glm::vec3 b, glm::vec3 c)
    {
        // Check if P in vertex region outside A
        glm::vec3 ab = b - a;
        glm::vec3 ac = c - a;
        glm::vec3 ap = p - a;
        float d1 = glm::dot(ab, ap);
        float d2 = glm::dot(ac, ap);
        if (d1 <= 0.0f && d2 <= 0.0f)
            return a; // barycentric coordinates (1,0,0)

        // Check if P in vertex region outside B
        glm::vec3 bp = p - b;
        float d3 = glm::dot(ab, bp);
        float d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3)
            return b; // barycentric coordinates (0,1,0)

        // Check if P in edge region of AB, if so return projection of P onto AB
        float vc = d1 * d4 - d3 * d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f)
        {
            float v = d1 / (d1 - d3);
            return a + v * ab; // barycentric coordinates (1-v, v, 0)
        }

        // Check if P in vertex region outside C
        glm::vec3 cp = p - c;
        float d5 = glm::dot(ab, cp);
        float d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6)
            return c; // barycentric coordinates (0,0,1)

        // Check if P in edge region of AC, if so return projection of P onto AC
        float vb = d5 * d2 - d1 * d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f)
        {
            float w = d2 / (d2 - d6);
            return a + w * ac; // barycentric coordinates (1-w, 0, w)
        }

        // Check if P in edge region of BC, if so return projection of P onto BC
        float va = d3 * d6 - d5 * d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f)
        {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + w * (c - b); // barycentric coordinates (0, 1-w, w)
        }

        // P inside face region. Compute Q through its barycentric coordinates (u, v, w)
        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        return a + ab * v + ac * w; // = u*a + v*b + w*c, u = 1.0f - v - w
    }

    glm::vec4 ResolveCollisions(glm::vec4 position,
                                bool noClip,
                                bool *hit_floor,
                                bool *hit_ceiling,
                                glm::vec3 *ground_normal)
    {
        float r = 0.25f;
        // Approximating a capsule height of 1.5f with three overlapping spheres
        float sphere_heights[3] = {0.25f, 0.75f, 1.25f};

        glm::vec3 pos = glm::vec3(position);

        // Run solver up to 4 iterations to handle multi-surface corners/wedges
        for (int iter = 0; iter < 4; ++iter)
        {
            bool collided = false;

            if (noClip)
            {
                return glm::vec4(pos, 1.0f);
            }

            for (float h_offset : sphere_heights)
            {
                glm::vec3 sphere_center = pos + glm::vec3(0.0f, h_offset, 0.0f);

                // Sphere AABB
                glm::vec3 A_min = sphere_center - glm::vec3(r);
                glm::vec3 A_max = sphere_center + glm::vec3(r);

                for (const auto &obj : g_CollisionScene)
                {
                    glm::vec3 B_min = obj.bbox_min;
                    glm::vec3 B_max = obj.bbox_max;

                    // Broad phase check (AABB overlap)
                    bool overlap = (A_min.x <= B_max.x) && (A_max.x >= B_min.x) &&
                                   (A_min.y <= B_max.y) && (A_max.y >= B_min.y) &&
                                   (A_min.z <= B_max.z) && (A_max.z >= B_min.z);
                    if (!overlap)
                        continue;

                    // Narrow phase (triangle overlap)
                    for (size_t i = 0; i < obj.vertices.size(); i += 3)
                    {
                        glm::vec3 a = obj.vertices[i];
                        glm::vec3 b = obj.vertices[i + 1];
                        glm::vec3 c = obj.vertices[i + 2];

                        glm::vec3 q = ClosestPointTriangle(sphere_center, a, b, c);
                        float dist2 = glm::length2(sphere_center - q);

                        const float contact_radius = r + kCollisionSkin;
                        if (dist2 < contact_radius * contact_radius)
                        {
                            float dist = std::sqrt(dist2);

                            glm::vec3 normal;
                            if (dist > 1e-6f)
                            {
                                normal = (sphere_center - q) / dist;
                            }
                            else
                            {
                                normal = glm::normalize(glm::cross(b - a, c - a));
                            }

                            const float penetration = r - dist;
                            if (penetration > 0.0f)
                            {
                                collided = true;
                                pos += normal * penetration;
                            }

                            // Contacts inside the small skin count as grounded
                            // without repeatedly pushing the player away from
                            // an already stable floor.
                            if (hit_floor != nullptr && normal.y > 0.5f)
                                *hit_floor = true;
                            if (hit_ceiling != nullptr && normal.y < -0.5f)
                                *hit_ceiling = true;
                            if (ground_normal != nullptr && normal.y > kSlopeLimitY && normal.y > ground_normal->y)
                                *ground_normal = normal;

                            if (penetration > 0.0f)
                            {
                                // Update tracking variables immediately for next tests
                                sphere_center = pos + glm::vec3(0.0f, h_offset, 0.0f);
                                A_min = sphere_center - glm::vec3(r);
                                A_max = sphere_center + glm::vec3(r);
                            }
                        }
                    }
                }
            }

            if (!collided)
                break;
        }

        return glm::vec4(pos, 1.0f);
    }

    void UpdatePlayer(float delta_time, bool noClip)
    {
        glm::vec4 forward;
        glm::vec4 right;
        glm::vec4 up;

        forward = glm::vec4(sinf(playerState.g_PlayerYaw), 0.0f, cosf(playerState.g_PlayerYaw), 0.0f);
        right = glm::vec4(-forward.z, 0.0f, forward.x, 0.0f);
        up = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);

        glm::vec4 movement = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        if (InputHandler::inputState.g_MoveForwardPressed)
            movement += forward;
        if (InputHandler::inputState.g_MoveBackwardPressed)
            movement -= forward;
        if (InputHandler::inputState.g_MoveLeftPressed)
            movement -= right;
        if (InputHandler::inputState.g_MoveRightPressed)
            movement += right;

        glm::vec4 current_pos = playerState.g_PlayerPosition;

        if (noClip)
        {
            playerState.g_PlayerVerticalVelocity = 0.0f;
            playerState.g_PlayerIsGrounded = false;

            if (InputHandler::inputState.g_MoveUpPressed)
                movement += up;
            if (InputHandler::inputState.g_MoveDownPressed)
                movement -= up;

            if (glm::length2(movement) > 0.0f)
            {
                movement = movement / glm::length(movement);
                glm::vec4 displacement = movement * playerState.g_PlayerMoveSpeed * delta_time;
                current_pos = ResolveMovement(current_pos, displacement, noClip);
            }
        }
        else
        {
            bool can_use_ground = playerState.g_PlayerIsGrounded && playerState.g_PlayerGroundNormal.y >= kSlopeLimitY;

            if (InputHandler::inputState.g_JumpRequested && can_use_ground)
            {
                playerState.g_PlayerVerticalVelocity = kJumpSpeed;
                playerState.g_PlayerIsGrounded = false;
                can_use_ground = false;
            }

            if (glm::length2(movement) > 0.0f)
                movement = movement / glm::length(movement);

            glm::vec4 horizontal_displacement = movement * playerState.g_PlayerMoveSpeed * delta_time;
            if (can_use_ground)
                horizontal_displacement = ProjectOntoPlane(horizontal_displacement, playerState.g_PlayerGroundNormal);

            current_pos = ResolveMovement(current_pos, horizontal_displacement, noClip);

            bool hit_floor = false;
            bool hit_ceiling = false;
            glm::vec3 contact_ground_normal(0.0f, 1.0f, 0.0f);

            if (can_use_ground)
            {
                current_pos = ResolveMovement(
                    current_pos,
                    glm::vec4(0.0f, -kGroundSnapDistance, 0.0f, 0.0f),
                    noClip,
                    &hit_floor,
                    &hit_ceiling,
                    &contact_ground_normal);

                if (hit_floor && contact_ground_normal.y >= kSlopeLimitY)
                {
                    playerState.g_PlayerIsGrounded = true;
                    playerState.g_PlayerGroundNormal = contact_ground_normal;
                    playerState.g_PlayerVerticalVelocity = 0.0f;
                }
                else
                {
                    playerState.g_PlayerIsGrounded = false;
                    playerState.g_PlayerGroundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }

            if (!playerState.g_PlayerIsGrounded)
            {
                playerState.g_PlayerVerticalVelocity -= kGravity * delta_time;
                if (playerState.g_PlayerVerticalVelocity < -kMaxFallSpeed)
                    playerState.g_PlayerVerticalVelocity = -kMaxFallSpeed;

                glm::vec4 vertical_displacement = glm::vec4(0.0f, playerState.g_PlayerVerticalVelocity * delta_time, 0.0f, 0.0f);
                current_pos = ResolveMovement(current_pos, vertical_displacement, noClip, &hit_floor, &hit_ceiling, &contact_ground_normal);

                if (hit_floor && playerState.g_PlayerVerticalVelocity <= 0.0f && contact_ground_normal.y >= kSlopeLimitY)
                {
                    playerState.g_PlayerVerticalVelocity = 0.0f;
                    playerState.g_PlayerIsGrounded = true;
                    playerState.g_PlayerGroundNormal = contact_ground_normal;
                }
                else if (hit_ceiling && playerState.g_PlayerVerticalVelocity > 0.0f)
                {
                    playerState.g_PlayerVerticalVelocity = 0.0f;
                }
                else if (!playerState.g_PlayerIsGrounded)
                {
                    playerState.g_PlayerGroundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
            }
        }

        playerState.g_PlayerPosition = current_pos;
        playerState.g_PlayerPosition.w = 1.0f;
        InputHandler::inputState.g_JumpRequested = false;
        // printf("Player position: x=%.3f y=%.3f z=%.3f\n",
        //        playerState.g_PlayerPosition.x,
        //        playerState.g_PlayerPosition.y,
        //        playerState.g_PlayerPosition.z);
    }
} // namespace Player
