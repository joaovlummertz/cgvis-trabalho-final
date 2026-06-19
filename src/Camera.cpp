#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

class Camera
{
public:
    glm::vec4 camera_position_c;
    glm::vec4 camera_lookat_l;
    glm::vec4 camera_view_vector;
    glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
    float eye_height = 0.6f;

    void Update(InputState input)
    {
    }

    void SetPosition(glm::vec4 foo)
    {
    }

    void SetOrientation(glm::vec4 bar)
    {
    }

    void HandleFirstPersonCamera()
    {
        InputHandler::g_PlayerYaw = InputHandler::g_CameraTheta + 3.14f;

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().
        float r = InputHandler::g_CameraDistance;
        float y = r * sin(InputHandler::g_CameraPhi);
        float z = r * cos(InputHandler::g_CameraPhi) * cos(InputHandler::g_CameraTheta);
        float x = r * cos(InputHandler::g_CameraPhi) * sin(InputHandler::g_CameraTheta);

        // Abaixo definimos as varáveis que efetivamente definem a câmera virtual.
        // Veja slides 195-227 e 229-234 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        camera_position_c = glm::vec4(x, y, z, 0.0f) + g_PlayerPosition;        // Ponto "c", centro da câmera
        camera_lookat_l = g_PlayerPosition + glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        camera_view_vector = camera_lookat_l - camera_position_c;               // Vetor "view", sentido para onde a câmera está virada
    }

    void HandleThirdPersonCamera()
    {
    }

}

if (!InputHandler::g_UseFirstPersonCamera)
{
}
else
{
    glm::vec4 forward = glm::vec4(
        cosf(InputHandler::g_PlayerPitch) * sinf(InputHandler::g_PlayerYaw),
        sinf(InputHandler::g_PlayerPitch),
        cosf(InputHandler::g_PlayerPitch) * cosf(InputHandler::g_PlayerYaw),
        0.0f);

    camera_position_c = g_PlayerPosition + glm::vec4(0.0f, eye_height, 0.0f, 0.0f);
    camera_view_vector = forward;
    camera_lookat_l = camera_position_c + camera_view_vector;
}