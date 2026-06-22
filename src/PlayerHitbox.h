#ifndef PLAYER_HITBOX_H
#define PLAYER_HITBOX_H

#include <glm/mat4x4.hpp>

#include "Player.h"

void InitPlayerHitbox();
void CleanupPlayerHitbox();
void DrawPlayerHitbox(glm::mat4 view, glm::mat4 projection);

#endif
