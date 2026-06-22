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
#include "TramIntro.h"
#include "Entity.h"
#include "Interaction.h"

std::map<std::string, SceneObject> g_VirtualScene;
std::vector<CollisionObject> g_CollisionScene;
std::vector<Entity *> g_WorldEntities;

double g_LastMouseX = 0.0;
double g_LastMouseY = 0.0;
float g_ScreenRatio = 1.0f;

int main(int argc, char *argv[])
{
    Window WindowManager = Window();
    WindowManager.Init();

    InputHandler::Init(WindowManager.window);
    InputHandler::inputState.g_UseFirstPersonCamera = true;
    glfwGetCursorPos(WindowManager.window, &g_LastMouseX, &g_LastMouseY);

    Renderer gameRenderer;
    gameRenderer.Initialize();

    ModelLoader::LoadAndAddToScene("../../assets/OBJ/gordon.obj", "../../assets/SMD/", gameRenderer, g_VirtualScene, g_CollisionScene);
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/maps/fullmap.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene, true, 0.02f);
    // Equivalent to TramIntro::GetModelMatrix() at the end of the path,
    // expressed without its local-center pivot so rendering and collision
    // hand off to the interactive tram without a visible jump.
    const glm::vec3 tramPosition(35.36484f, -5.054f, -6.749105f);
    const float tramYaw = 1.6048247f;
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/tram.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene, true, 0.02f, tramPosition, tramYaw);
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/tramDoor.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene, true, 0.02f, tramPosition, tramYaw);

    InitPlayerHitbox();
    Camera camera;
    TramIntro::Initialize();

    SlidingDoor *tramDoor = new SlidingDoor("TramDoor", tramPosition, glm::vec3(1.2f, 0.0f, 0.0f), tramYaw);
    g_WorldEntities.push_back(tramDoor);

    SlidingDoor *blastDoor1 = new SlidingDoor(
        "BlastDoor1", glm::vec3(0.0f), glm::vec3(0.0f, -2.6f, 0.0f));
    g_WorldEntities.push_back(blastDoor1);

    SlidingDoor *blastDoor2 = new SlidingDoor(
        "BlastDoor2", glm::vec3(0.0f), glm::vec3(0.0f, -2.6f, 0.0f));
    g_WorldEntities.push_back(blastDoor2);

    while (!glfwWindowShouldClose(WindowManager.window))
    {
        static float previous_time = (float)glfwGetTime();
        float current_time = (float)glfwGetTime();
        float delta_time = current_time - previous_time;
        previous_time = current_time;

        double mouseX = InputHandler::inputState.g_MouseX;
        double mouseY = InputHandler::inputState.g_MouseY;
        double dx = mouseX - g_LastMouseX;
        double dy = mouseY - g_LastMouseY;
        g_LastMouseX = mouseX;
        g_LastMouseY = mouseY;

        if (TramIntro::IsActive())
        {
            TramIntro::Update(delta_time);
        }
        else
        {
            Player::UpdatePlayer(delta_time, InputHandler::inputState.g_UseNoclip);
        }

        camera.Update(InputHandler::inputState, Player::playerState, dx, dy);

        if (TramIntro::IsActive())
            InputHandler::inputState.g_interact = false;

        if (!TramIntro::IsActive() && InputHandler::inputState.g_interact)
        {
            InputHandler::inputState.g_interact = false;

            glm::vec3 rayOrigin = glm::vec3(camera.camera_position_c);
            glm::vec3 rayDir = glm::normalize(glm::vec3(camera.camera_view_vector));

            printf("\n raycasting \n");

            // Raycast check against collision objects bound to our entities using prefix matching
            SlidingDoor *closestDoor = nullptr;
            float closestT = std::numeric_limits<float>::max();

            for (auto &entity : g_WorldEntities)
            {
                float entityClosestT;
                if (entity->IntersectsRay(rayOrigin, rayDir, entityClosestT) &&
                    entityClosestT < closestT)
                {
                    closestT = entityClosestT;
                    closestDoor = dynamic_cast<SlidingDoor *>(entity);
                }
            }

            // If we found the closest door structure, interact with it!
            if (closestDoor != nullptr)
            {
                printf("\nDoor group found and interacted!\n");
                closestDoor->Toggle();
            }
        }

        // --- UPDATE RUNTIME ENTITIES ---
        for (auto &entity : g_WorldEntities)
        {
            entity->Update(delta_time);
        }

        // --- Rendering ---
        gameRenderer.ClearColor(0.9f, 0.9f, 1.0f, 1.0f);

        glm::mat4 view = Matrix_Camera_View(camera.camera_position_c, camera.camera_view_vector, camera.camera_up_vector);

        float field_of_view = 3.141592f / 3.0f;
        glm::mat4 projection = Matrix_Perspective(field_of_view, g_ScreenRatio, -0.1f, -500.0f);

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

        const bool tramIntroActive = TramIntro::IsActive();
        glm::mat4 tramModel;
        if (tramIntroActive)
        {
            tramModel = TramIntro::GetModelMatrix();
        }
        else
        {
            tramModel = Matrix_Translate(tramPosition.x, tramPosition.y, tramPosition.z) *
                        Matrix_Rotate_Y(tramYaw) *
                        Matrix_Scale(0.02f, 0.02f, 0.02f);
        }

        gameRenderer.SetModelMatrix(tramModel);
        gameRenderer.DrawVirtualObject("Tram", g_VirtualScene);

        // During the intro the door is rigidly attached to the animated tram.
        // At the endpoint its SlidingDoor entity takes over rendering and physics.
        if (tramIntroActive)
            gameRenderer.DrawVirtualObject("TramDoor", g_VirtualScene);

        for (auto &entity : g_WorldEntities)
        {
            if (tramIntroActive && entity == tramDoor)
                continue;
            entity->Draw(gameRenderer, g_VirtualScene);
        }

        // Debug hitboxes
        DrawPlayerHitbox(view, projection);

        glfwSwapBuffers(WindowManager.window);
        glfwPollEvents();
    }

    for (auto entity : g_WorldEntities)
        delete entity;

    CleanupPlayerHitbox();
    ModelLoader::Cleanup();
    gameRenderer.Shutdown();
    glfwTerminate();
    return 0;
}
