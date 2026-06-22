#ifndef PLAYER_H
#define PLAYER_H

#include <glm/vec4.hpp>

struct PlayerState
{
    glm::vec4 g_PlayerPosition = glm::vec4(31.247f, -4.820f, -6.341f, 0.0f);
    float g_PlayerMoveSpeed = 8.0f;
    float g_PlayerYaw = 0.0f;
    float g_PlayerPitch = 0.0f;
    float g_PlayerVerticalVelocity = 0.0f;
    bool g_PlayerIsGrounded = false;
};
namespace Player
{

    extern PlayerState playerState;

    void UpdatePlayer(float delta_time, bool noClip);
    glm::vec4 GetForwardVector();
} // namespace Player

#endif
