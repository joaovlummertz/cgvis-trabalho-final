#ifndef PLAYER_HITBOX_H
#define PLAYER_HITBOX_H

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

extern glm::vec4 g_PlayerPosition;

void InitPlayerHitbox();
void DrawPlayerHitbox(glm::mat4 view, glm::mat4 projection);

#endif
