#include "Camera.h"
#include <cmath>

void Camera::Update(InputState input, PlayerState playerState, float dx, float dy)
{

    if (input.g_UseFirstPersonCamera)
    {
        HandleFirstPersonCamera(input, playerState, dx, dy);
    }
    else
    {
        HandleThirdPersonCamera(input, playerState, dx, dy);
    }
}

void Camera::HandleFirstPersonCamera(InputState input, PlayerState playerState, float dx, float dy)
{
    Player::playerState.g_PlayerYaw -= 0.01f * dx;
    Player::playerState.g_PlayerPitch -= 0.01f * dy;

    float pitchmax = 3.14f / 2.0f - 0.01f;
    float pitchmin = -pitchmax;

    if (Player::playerState.g_PlayerPitch > pitchmax)
        Player::playerState.g_PlayerPitch = pitchmax;
    if (Player::playerState.g_PlayerPitch < pitchmin)
        Player::playerState.g_PlayerPitch = pitchmin;

    glm::vec4 forward = glm::vec4(
        cosf(Player::playerState.g_PlayerPitch) * sinf(Player::playerState.g_PlayerYaw),
        sinf(Player::playerState.g_PlayerPitch),
        cosf(Player::playerState.g_PlayerPitch) * cosf(Player::playerState.g_PlayerYaw),
        0.0f);

    camera_position_c = Player::playerState.g_PlayerPosition + glm::vec4(0.0f, eye_height, 0.0f, 0.0f);
    camera_view_vector = forward;
    camera_lookat_l = camera_position_c + camera_view_vector;
}

void Camera::HandleThirdPersonCamera(InputState input, PlayerState playerState, float dx, float dy)
{
    g_CameraTheta -= 0.01f * dx;
    g_CameraPhi += 0.01f * dy;
    Player::playerState.g_PlayerYaw = g_CameraTheta + 3.14f; // Player faces opposite to camera in third-person

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2,
    // mas usamos um pequeno afastamento para evitar singularidade quando a
    // câmera ficar exatamente alinhada com o eixo Y global.
    float phimax = 3.141592f / 2.0f - 0.01f;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;
    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;

    const float r = 3.5f;
    float y = r * sin(g_CameraPhi);
    float z = r * cos(g_CameraPhi) * cos(g_CameraTheta);
    float x = r * cos(g_CameraPhi) * sin(g_CameraTheta);

    // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
    // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
    camera_position_c = glm::vec4(x, y, z, 0.0f) + playerState.g_PlayerPosition;        // Ponto "c", centro da câmera
    camera_lookat_l = playerState.g_PlayerPosition + glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
    camera_view_vector = camera_lookat_l - camera_position_c;                           // Vetor "view", sentido para onde

}
