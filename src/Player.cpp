#include "Player.h"

#include <glm/gtx/norm.hpp>

#include "InputHandler.h"

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
            playerState.g_PlayerPosition += movement * playerState.g_PlayerMoveSpeed * delta_time;
            playerState.g_PlayerPosition.w = 1.0f;
        }
    }
} // namespace Player
