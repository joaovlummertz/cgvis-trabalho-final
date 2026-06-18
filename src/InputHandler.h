#include <iostream>
#include <limits>
#include <GLFW/glfw3.h>

struct InputState
{
    bool g_LeftMouseButtonPressed = false;
    bool g_RightMouseButtonPressed = false;
    bool g_MoveForwardPressed = false;
    bool g_MoveBackwardPressed = false;
    bool g_MoveLeftPressed = false;
    bool g_MoveRightPressed = false;
    bool g_MoveUpPressed = false;
    bool g_MoveDownPressed = false;
};

namespace InputHandler
{
    extern InputState inputState;

    extern float g_CameraTheta;
    extern float g_CameraPhi;
    extern float g_CameraDistance;
    extern double g_LastCursorPosX;
    extern double g_LastCursorPosY;
    extern bool g_UseFirstPersonCamera;
    extern float g_PlayerYaw;
    extern float g_PlayerPitch;
    extern float g_PlayerMoveSpeed;

    void Init(GLFWwindow *window);
}