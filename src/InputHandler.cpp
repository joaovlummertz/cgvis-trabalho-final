#include "InputHandler.h"
#include "Player.h"
namespace InputHandler
{
    InputState inputState;

    void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        // Not tracking button states for now
    }

    void CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
    {
        // Store current mouse position
        inputState.g_MouseX = xpos;
        inputState.g_MouseY = ypos;
    }

    void KeyCallback(GLFWwindow *window, int key, int scancode, int action, int mod)
    {
        bool pressed = action != GLFW_RELEASE;

        if (key == GLFW_KEY_W)
            inputState.g_MoveForwardPressed = pressed;

        if (key == GLFW_KEY_S)
            inputState.g_MoveBackwardPressed = pressed;

        if (key == GLFW_KEY_A)
            inputState.g_MoveLeftPressed = pressed;

        if (key == GLFW_KEY_D)
            inputState.g_MoveRightPressed = pressed;

        if (key == GLFW_KEY_SPACE)
            inputState.g_MoveUpPressed = pressed;

        if (key == GLFW_KEY_LEFT_SHIFT)
            inputState.g_MoveDownPressed = pressed;

        if (key == GLFW_KEY_E)
            inputState.g_interact = action == GLFW_PRESS;

        // Se o usuário pressionar a tecla ESC, fechamos a janela.
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);

        // Toggle between first-person and third-person cameras
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            inputState.g_UseFirstPersonCamera = !inputState.g_UseFirstPersonCamera;
        }

        if (key == GLFW_KEY_H && action == GLFW_PRESS)
        {
            inputState.g_ShowHitbox = !inputState.g_ShowHitbox;
        }
        if (key == GLFW_KEY_P && action == GLFW_PRESS)
        {
            inputState.g_UseNoclip = !inputState.g_UseNoclip;
            printf("noClip activated\n");
        }
    }

    void Init(GLFWwindow *window)
    {
        std::cout << "Initializing inputStateHandler";
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        std::cout << "Finished initializing inputStateHandler";
    };
};
