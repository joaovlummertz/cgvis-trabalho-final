
#pragma once

#define GLFW_INCLUDE_NONE
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cstdio>

extern float g_ScreenRatio;

class Window
{
public:
    GLFWwindow *window;

    void Init();
};

void ErrorCallback(int error, const char *description);

void FramebufferSizeCallback(GLFWwindow *window, int width, int height);
