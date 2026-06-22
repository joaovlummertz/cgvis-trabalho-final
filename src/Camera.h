#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "InputHandler.h"
#include "Player.h"

class Camera
{
public:
    glm::vec4 camera_position_c;
    glm::vec4 camera_lookat_l;
    glm::vec4 camera_view_vector;
    glm::vec4 camera_up_vector = glm::vec4(0.0f, 1.0f, 0.0f, 0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
    float eye_height = 1.0f;
    float g_CameraTheta = 0.0f;
    float g_CameraPhi = 0.0f;

    void Update(InputState input, PlayerState playerState, float dx, float dy);

private:
    void HandleFirstPersonCamera(InputState input, PlayerState playerState, float dx, float dy);

    void HandleThirdPersonCamera(InputState input, PlayerState playerState, float dx, float dy);
};
