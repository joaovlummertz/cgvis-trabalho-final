#include <iostream>
#include <limits>
#include <glfw3.h>

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

class InputStateHandler
{
public:
    InputState inputState;

    InputStateHandler(GLFWwindow *window)
    {
        std::cout << "Initializing inputStateHandler";
        glfwSetKeyCallback(window, KeyCallback);
        glfwSetMouseButtonCallback(window, MouseButtonCallback);
        glfwSetCursorPosCallback(window, CursorPosCallback);
        glfwSetScrollCallback(window, ScrollCallback);
    }

private:
    float g_CameraTheta = 0.0f;    // Ângulo no plano ZX em relação ao eixo Z
    float g_CameraPhi = 0.0f;      // Ângulo em relação ao eixo Y
    float g_CameraDistance = 3.5f; // Distância da câmera para a origem
    double g_LastCursorPosX, g_LastCursorPosY;
    bool g_UseFirstPersonCamera = false;

    void ScrollCallback(GLFWwindow *window, double xoffset, double yoffset)
    {
        // Atualizamos a distância da câmera para a origem utilizando a
        // movimentação da "rodinha", simulando um ZOOM.
        g_CameraDistance -= 0.1f * yoffset;

        const float verysmallnumber = std::numeric_limits<float>::epsilon();
        if (g_CameraDistance < verysmallnumber)
            g_CameraDistance = verysmallnumber;
    }

    void MouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
    {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
            inputState.g_LeftMouseButtonPressed = true;
        }
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
        {
            inputState.g_LeftMouseButtonPressed = false;
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        {
            glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
            inputState.g_RightMouseButtonPressed = true;
        }
        if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
        {
            inputState.g_RightMouseButtonPressed = false;
        }
    }

    void CursorPosCallback(GLFWwindow *window, double xpos, double ypos)
    {
        // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
        // pressionado, computamos quanto que o mouse se movimento desde o último
        // instante de tempo, e usamos esta movimentação para atualizar os
        // parâmetros que definem a posição da câmera dentro da cena virtual.
        // Assim, temos que o usuário consegue controlar a câmera.

        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        if (!g_UseFirstPersonCamera)
        {

            // Atualizamos parâmetros da câmera com os deslocamentos
            g_CameraTheta -= 0.01f * dx;
            g_CameraPhi += 0.01f * dy;

            // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
            float phimax = 3.141592f / 2;
            float phimin = -phimax;

            if (g_CameraPhi > phimax)
                g_CameraPhi = phimax;

            if (g_CameraPhi < phimin)
                g_CameraPhi = phimin;
        }
        else
        {
            g_PlayerYaw -= 0.01f * dx;
            g_PlayerPitch -= 0.01f * dy;

            float pitchmax = 3.14f / 2.0f - 0.01f;
            float pitchmin = -pitchmax;

            if (g_PlayerPitch > pitchmax)
                g_PlayerPitch = pitchmax;

            if (g_PlayerPitch < pitchmin)
                g_PlayerPitch = pitchmin;
        }

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;

        if (inputState.g_RightMouseButtonPressed)
        {
            // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
            float dx = xpos - g_LastCursorPosX;
            float dy = ypos - g_LastCursorPosY;

            // Atualizamos as variáveis globais para armazenar a posição atual do
            // cursor como sendo a última posição conhecida do cursor.
            g_LastCursorPosX = xpos;
            g_LastCursorPosY = ypos;
        }
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

        // Se o usuário pressionar a tecla ESC, fechamos a janela.
        if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
            glfwSetWindowShouldClose(window, GL_TRUE);

        // O código abaixo implementa a seguinte lógica:
        //   Se apertar tecla X       então g_AngleX += delta;
        //   Se apertar tecla shift+X então g_AngleX -= delta;
        //   Se apertar tecla Y       então g_AngleY += delta;
        //   Se apertar tecla shift+Y então g_AngleY -= delta;
        //   Se apertar tecla Z       então g_AngleZ += delta;
        //   Se apertar tecla shift+Z então g_AngleZ -= delta;

        float delta = 3.141592 / 16; // 22.5 graus, em radianos.

        // Se o usuário apertar a tecla C, alternamos entre a câmera padrão e a câmera em primeira pessoa.
        if (key == GLFW_KEY_C && action == GLFW_PRESS)
        {
            g_UseFirstPersonCamera = !g_UseFirstPersonCamera;
        }

        // // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
        // if (key == GLFW_KEY_R && action == GLFW_PRESS)
        // {
        //     LoadShadersFromFiles();
        //     fprintf(stdout, "Shaders recarregados!\n");
        //     fflush(stdout);
        // }
    }
};