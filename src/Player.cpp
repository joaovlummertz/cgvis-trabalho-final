#include "Player.h"

#include <cmath>
#include <glm/gtx/norm.hpp>
#include <map>
#include <string>
#include <vector>
#include <algorithm>
#include <glad/glad.h>

#include "InputHandler.h"

// Struct identical to the one in main.cpp to resolve types
struct SceneObject
{
    std::string name;              // Nome do objeto
    size_t first_index;            // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t num_indices;            // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum rendering_mode;         // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3 bbox_min;            // Axis-Aligned Bounding Box do objeto
    glm::vec3 bbox_max;
    GLuint texture_id;
    std::vector<glm::vec3> vertices; // Added for collision detection on CPU
};

extern std::map<std::string, SceneObject> g_VirtualScene;

namespace Player
{

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
        if (d1 <= 0.0f && d2 <= 0.0f) return a; // barycentric coordinates (1,0,0)

        // Check if P in vertex region outside B
        glm::vec3 bp = p - b;
        float d3 = glm::dot(ab, bp);
        float d4 = glm::dot(ac, bp);
        if (d3 >= 0.0f && d4 <= d3) return b; // barycentric coordinates (0,1,0)

        // Check if P in edge region of AB, if so return projection of P onto AB
        float vc = d1*d4 - d3*d2;
        if (vc <= 0.0f && d1 >= 0.0f && d3 <= 0.0f) {
            float v = d1 / (d1 - d3);
            return a + v * ab; // barycentric coordinates (1-v, v, 0)
        }

        // Check if P in vertex region outside C
        glm::vec3 cp = p - c;
        float d5 = glm::dot(ab, cp);
        float d6 = glm::dot(ac, cp);
        if (d6 >= 0.0f && d5 <= d6) return c; // barycentric coordinates (0,0,1)

        // Check if P in edge region of AC, if so return projection of P onto AC
        float vb = d5*d2 - d1*d6;
        if (vb <= 0.0f && d2 >= 0.0f && d6 <= 0.0f) {
            float w = d2 / (d2 - d6);
            return a + w * ac; // barycentric coordinates (1-w, 0, w)
        }

        // Check if P in edge region of BC, if so return projection of P onto BC
        float va = d3*d6 - d5*d4;
        if (va <= 0.0f && (d4 - d3) >= 0.0f && (d5 - d6) >= 0.0f) {
            float w = (d4 - d3) / ((d4 - d3) + (d5 - d6));
            return b + w * (c - b); // barycentric coordinates (0, 1-w, w)
        }

        // P inside face region. Compute Q through its barycentric coordinates (u, v, w)
        float denom = 1.0f / (va + vb + vc);
        float v = vb * denom;
        float w = vc * denom;
        return a + ab * v + ac * w; // = u*a + v*b + w*c, u = 1.0f - v - w
    }

    glm::vec4 ResolveCollisions(glm::vec4 position)
    {
        float r = 0.25f;
        // Approximating a capsule height of 1.5f with three overlapping spheres
        float sphere_heights[3] = {0.25f, 0.75f, 1.25f};

        glm::vec3 pos = glm::vec3(position);

        // Run solver up to 4 iterations to handle multi-surface corners/wedges
        for (int iter = 0; iter < 4; ++iter)
        {
            bool collided = false;

            for (float h_offset : sphere_heights)
            {
                glm::vec3 sphere_center = pos + glm::vec3(0.0f, h_offset, 0.0f);

                // Sphere AABB
                glm::vec3 A_min = sphere_center - glm::vec3(r);
                glm::vec3 A_max = sphere_center + glm::vec3(r);

                for (const auto &pair : g_VirtualScene)
                {
                    const std::string &name = pair.first;
                    // Exclude player model shapes from collision
                    if (name.find("Gordon") != std::string::npos)
                        continue;

                    const SceneObject &obj = pair.second;
                    glm::vec3 B_min = obj.bbox_min * 0.02f;
                    glm::vec3 B_max = obj.bbox_max * 0.02f;

                    // Broad phase check (AABB overlap)
                    bool overlap = (A_min.x <= B_max.x) && (A_max.x >= B_min.x) &&
                                   (A_min.y <= B_max.y) && (A_max.y >= B_min.y) &&
                                   (A_min.z <= B_max.z) && (A_max.z >= B_min.z);
                    if (!overlap) continue;

                    // Narrow phase (triangle overlap)
                    for (size_t i = 0; i < obj.vertices.size(); i += 3)
                    {
                        glm::vec3 a = obj.vertices[i] * 0.02f;
                        glm::vec3 b = obj.vertices[i+1] * 0.02f;
                        glm::vec3 c = obj.vertices[i+2] * 0.02f;

                        glm::vec3 q = ClosestPointTriangle(sphere_center, a, b, c);
                        float dist = glm::length(sphere_center - q);

                        if (dist < r)
                        {
                            collided = true;

                            glm::vec3 normal;
                            if (dist > 1e-6f)
                            {
                                normal = (sphere_center - q) / dist;
                            }
                            else
                            {
                                normal = glm::normalize(glm::cross(b - a, c - a));
                            }

                            float penetration = r - dist;
                            pos += normal * penetration;

                            // Update tracking variables immediately for next tests
                            sphere_center = pos + glm::vec3(0.0f, h_offset, 0.0f);
                            A_min = sphere_center - glm::vec3(r);
                            A_max = sphere_center + glm::vec3(r);
                        }
                    }
                }
            }

            if (!collided)
                break;
        }

        return glm::vec4(pos, 1.0f);
    }

    void UpdatePlayer(float delta_time)
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
        if (InputHandler::inputState.g_MoveUpPressed)
            movement += up;
        if (InputHandler::inputState.g_MoveDownPressed)
            movement -= up;

        if (glm::length2(movement) > 0.0f)
        {
            movement = movement / glm::length(movement);
            glm::vec4 displacement = movement * playerState.g_PlayerMoveSpeed * delta_time;

            float dist = glm::length(displacement);
            float step_size = 0.1f;
            int num_steps = std::max(1, (int)std::ceil(dist / step_size));

            glm::vec4 step_displacement = displacement / (float)num_steps;
            glm::vec4 current_pos = playerState.g_PlayerPosition;

            // Run the resolver incrementally along the movement vector to prevent tunneling
            for (int i = 0; i < num_steps; ++i)
            {
                current_pos = ResolveCollisions(current_pos + step_displacement);
            }

            playerState.g_PlayerPosition = current_pos;
            playerState.g_PlayerPosition.w = 1.0f;
        }
    }
} // namespace Player
