#ifndef PLAYER_H
#define PLAYER_H

#include <glm/vec4.hpp>

namespace Player
{
extern glm::vec4 g_PlayerPosition;
extern float g_PlayerMoveSpeed;
extern float g_PlayerYaw;
extern float g_PlayerPitch;

void UpdatePlayer(float delta_time);
glm::vec4 GetForwardVector();
} // namespace Player

#endif
