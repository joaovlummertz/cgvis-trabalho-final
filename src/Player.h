#ifndef PLAYER_H
#define PLAYER_H

#include <glm/vec4.hpp>

struct PlayerState
{
    glm::vec4 g_PlayerPosition = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
    float g_PlayerMoveSpeed = 15.0f;
    float g_PlayerYaw = 0.0f;
    float g_PlayerPitch = 0.0f;
};
namespace Player
{

    extern PlayerState playerState;

    void UpdatePlayer(float delta_time, bool noClip);
    glm::vec4 GetForwardVector();
} // namespace Player

#endif
