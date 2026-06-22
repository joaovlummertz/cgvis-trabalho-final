#pragma once

#include <iostream>
#include <limits>
#include <GLFW/glfw3.h>

struct InputState
{
    bool g_MoveForwardPressed = false;
    bool g_MoveBackwardPressed = false;
    bool g_MoveLeftPressed = false;
    bool g_MoveRightPressed = false;
    bool g_MoveUpPressed = false;
    bool g_MoveDownPressed = false;
    bool g_ShowHitbox = false;
    bool g_UseFirstPersonCamera = false;
    bool g_UseNoclip = false;
    bool g_interact = false;
    double g_MouseX = 0.0;
    double g_MouseY = 0.0;
};
namespace InputHandler
{

    extern InputState inputState;
    void Init(GLFWwindow *window);
}
