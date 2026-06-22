#ifndef PLAYER_H
#define PLAYER_H

#include <glm/vec4.hpp>
#include <glm/vec3.hpp>

struct PlayerState
{
    glm::vec4 g_PlayerPosition = glm::vec4(31.247f, -4.820f, -6.341f, 0.0f);
    float g_PlayerMoveSpeed = 6.5f;
    float g_PlayerYaw = 0.0f;
    float g_PlayerPitch = 0.0f;
    float g_PlayerVerticalVelocity = 0.0f;
    bool g_PlayerIsGrounded = false;
    glm::vec3 g_PlayerGroundNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    int g_PlayerHealth = 5;
    float g_DamageFlashTimer = 0.0f;
};
namespace Player
{

    extern PlayerState playerState;

    void UpdatePlayer(float delta_time, bool noClip);
    glm::vec4 GetForwardVector();
} // namespace Player

#endif
