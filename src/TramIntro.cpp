#include "TramIntro.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "InputHandler.h"
#include "Player.h"

namespace TramIntro
{
namespace
{
    constexpr float kTramScale = 0.02f;
    constexpr float kTramIntroDuration = 24.0f;
    constexpr float kTramHeadingOffset = 3.141592f / 2.0f;
    const glm::vec3 kTramPassengerOffset = glm::vec3(0.0f, -0.55f, 0.0f);

    struct State
    {
        bool active = true;
        bool previous_first_person_camera = false;
        float elapsed = 0.0f;
        float current_t = 0.0f;
        std::vector<glm::vec3> path_points;
        glm::vec3 tram_local_center = glm::vec3(0.0f);
        glm::vec3 current_position = glm::vec3(0.0f);
        glm::vec3 current_tangent = glm::vec3(0.0f, 0.0f, 1.0f);
    };

    State g_State;

    glm::vec3 CubicBezierPoint(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, float t)
    {
        float u = 1.0f - t;
        float uu = u * u;
        float tt = t * t;

        return (uu * u) * p0 + (3.0f * uu * t) * p1 + (3.0f * u * tt) * p2 + (tt * t) * p3;
    }

    glm::vec3 CubicBezierTangent(const glm::vec3 &p0, const glm::vec3 &p1, const glm::vec3 &p2, const glm::vec3 &p3, float t)
    {
        float u = 1.0f - t;

        return (3.0f * u * u) * (p1 - p0) + (6.0f * u * t) * (p2 - p1) + (3.0f * t * t) * (p3 - p2);
    }

    float Length(const glm::vec3 &v)
    {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }

    glm::vec3 NormalizeOrFallback(const glm::vec3 &v, const glm::vec3 &fallback)
    {
        float len = Length(v);
        if (len < 1e-6f)
            return fallback;
        return v / len;
    }

    void EvaluateTramPath(float t, glm::vec3 &position, glm::vec3 &tangent)
    {
        const std::vector<glm::vec3> &points = g_State.path_points;
        if (points.empty())
        {
            position = glm::vec3(0.0f);
            tangent = glm::vec3(0.0f, 0.0f, 1.0f);
            return;
        }

        if ((points.size() - 1) % 3 != 0)
        {
            position = points.front();
            tangent = glm::vec3(0.0f, 0.0f, 1.0f);
            return;
        }

        int segment_count = (int)((points.size() - 1) / 3);
        float clamped_t = std::max(0.0f, std::min(1.0f, t));
        float scaled_t = clamped_t * (float)segment_count;
        int segment_index = (int)std::floor(scaled_t);
        if (segment_index >= segment_count)
            segment_index = segment_count - 1;
        float local_t = scaled_t - (float)segment_index;
        int base = segment_index * 3;

        const glm::vec3 &p0 = points[base + 0];
        const glm::vec3 &p1 = points[base + 1];
        const glm::vec3 &p2 = points[base + 2];
        const glm::vec3 &p3 = points[base + 3];

        position = CubicBezierPoint(p0, p1, p2, p3, local_t);
        tangent = CubicBezierTangent(p0, p1, p2, p3, local_t);
    }

    glm::mat4 BuildModelMatrix(const glm::vec3 &tram_position, const glm::vec3 &tram_tangent)
    {
        glm::vec3 tangent = NormalizeOrFallback(tram_tangent, glm::vec3(0.0f, 0.0f, 1.0f));
        float yaw = std::atan2(tangent.x, tangent.z) + kTramHeadingOffset;

        return glm::translate(glm::mat4(1.0f), tram_position) *
               glm::rotate(glm::mat4(1.0f), yaw, glm::vec3(0.0f, 1.0f, 0.0f)) *
               glm::scale(glm::mat4(1.0f), glm::vec3(kTramScale, kTramScale, kTramScale)) *
               glm::translate(glm::mat4(1.0f), -g_State.tram_local_center);
    }
} // namespace

void Initialize()
{
    g_State.tram_local_center = glm::vec3(-432.0f, 58.5f, 24.0f);

    g_State.path_points = {
        glm::vec3(50.408f, -3.604f, -63.140f),
        glm::vec3(49.213f, -4.148f, -63.086f),
        glm::vec3(42.376f, -4.148f, -61.222f),
        glm::vec3(34.892f, -4.148f, -53.009f),
        glm::vec3(34.960f, -3.604f, -52.646f),
        glm::vec3(34.907f, -3.604f, -22.550f),
        glm::vec3(35.455f, -3.604f, -6.452f),
    };

    g_State.elapsed = 0.0f;
    g_State.current_t = 0.0f;
    g_State.current_position = g_State.path_points.front();
    g_State.current_tangent = glm::vec3(0.0f, 0.0f, 1.0f);
    g_State.previous_first_person_camera = InputHandler::inputState.g_UseFirstPersonCamera;

    EvaluateTramPath(0.0f, g_State.current_position, g_State.current_tangent);
    glm::vec3 initial_forward = NormalizeOrFallback(g_State.current_tangent, glm::vec3(0.0f, 0.0f, 1.0f));
    Player::playerState.g_PlayerPosition = glm::vec4(g_State.path_points.front(), 1.0f);
    Player::playerState.g_PlayerYaw = std::atan2(initial_forward.x, initial_forward.z);
    Player::playerState.g_PlayerPitch = 0.0f;

    g_State.active = true;
}

void Update(float delta_time)
{
    if (!g_State.active)
        return;

    g_State.elapsed += delta_time;
    float t = g_State.elapsed / kTramIntroDuration;
    if (t > 1.0f)
        t = 1.0f;
    g_State.current_t = t;

    EvaluateTramPath(t, g_State.current_position, g_State.current_tangent);
    Player::playerState.g_PlayerPosition = glm::vec4(g_State.current_position + kTramPassengerOffset, 1.0f);
    InputHandler::inputState.g_UseFirstPersonCamera = true;

    if (g_State.elapsed >= kTramIntroDuration)
    {
        g_State.active = false;
        InputHandler::inputState.g_UseFirstPersonCamera = g_State.previous_first_person_camera;
    }
}

bool IsActive()
{
    return g_State.active;
}

glm::mat4 GetModelMatrix()
{
    return BuildModelMatrix(g_State.current_position, g_State.current_tangent);
}
} // namespace TramIntro
