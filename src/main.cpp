#include <map>
#include <vector>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>

#include "matrices.h"
#include "SceneData.h"
#include "Player.h"
#include "PlayerHitbox.h"
#include "Camera.h"
#include "InputHandler.h"
#include "Window.h"
#include "Renderer.h"
#include "ModelLoader.h"

std::map<std::string, SceneObject> g_VirtualScene;
std::vector<CollisionObject> g_CollisionScene;

double g_LastMouseX = 0.0;
double g_LastMouseY = 0.0;
float g_ScreenRatio = 1.0f;

int main(int argc, char *argv[])
{
    Window WindowManager = Window();
    WindowManager.Init();

    InputHandler::Init(WindowManager.window);

    Renderer gameRenderer;
    gameRenderer.Initialize();

    ModelLoader::LoadAndAddToScene("../../assets/OBJ/gordon.obj", "../../assets/SMD/", gameRenderer, g_VirtualScene, g_CollisionScene);
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/maps/fullmap.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene, true, 0.02f);
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/tram.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene);
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/tramDoor.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene);

    InitPlayerHitbox();
    Camera camera;

    while (!glfwWindowShouldClose(WindowManager.window))
    {
        static float previous_time = (float)glfwGetTime();
        float current_time = (float)glfwGetTime();
        float delta_time = current_time - previous_time;
        previous_time = current_time;

        // Input & State Management Updates
        Player::UpdatePlayer(delta_time, InputHandler::inputState.g_UseNoclip);

        double mouseX = InputHandler::inputState.g_MouseX;
        double mouseY = InputHandler::inputState.g_MouseY;
        double dx = mouseX - g_LastMouseX;
        double dy = mouseY - g_LastMouseY;
        g_LastMouseX = mouseX;
        g_LastMouseY = mouseY;

        camera.Update(InputHandler::inputState, Player::playerState, dx, dy);

        // --- Rendering ---
        gameRenderer.ClearColor(0.9f, 0.9f, 1.0f, 1.0f);

        glm::mat4 view = Matrix_Camera_View(camera.camera_position_c, camera.camera_view_vector, camera.camera_up_vector);
        float field_of_view = 3.141592f / 3.0f;
        glm::mat4 projection = Matrix_Perspective(field_of_view, g_ScreenRatio, -0.1f, -100000.0f);

        gameRenderer.BeginFrame(view, projection);

        // Draw Player character mesh
        if (!InputHandler::inputState.g_UseFirstPersonCamera)
        {
            glm::mat4 model = Matrix_Translate(Player::playerState.g_PlayerPosition.x, Player::playerState.g_PlayerPosition.y, Player::playerState.g_PlayerPosition.z) * Matrix_Rotate_Y(Player::playerState.g_PlayerYaw) * Matrix_Scale(0.02f, 0.02f, 0.02f);
            gameRenderer.SetModelMatrix(model);
            gameRenderer.DrawVirtualObject("Gordon_Hi", g_VirtualScene);
        }

        // Draw Map Environment meshes
        glm::mat4 mapModel = Matrix_Scale(0.02f, 0.02f, 0.02f) * Matrix_Translate(0.0f, 0.0f, 0.0f);
        gameRenderer.SetModelMatrix(mapModel);

        gameRenderer.DrawVirtualObject("Brush", g_VirtualScene);
        gameRenderer.DrawVirtualObject("Tram", g_VirtualScene);
        gameRenderer.DrawVirtualObject("TramDoor", g_VirtualScene);

        // Debug hitboxes
        DrawPlayerHitbox(view, projection);

        glfwSwapBuffers(WindowManager.window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}