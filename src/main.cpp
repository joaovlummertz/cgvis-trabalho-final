#include <map>
#include <vector>
#include <cmath>
#include <algorithm>
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

static const glm::vec3 g_GroundCrowbarPosition(-48.027f, -5.760f, 16.319f);
static bool g_GroundCrowbarAvailable = true;

static bool IsPlayerSteppingOnGroundCrowbar()
{
    const glm::vec3 playerPosition(Player::playerState.g_PlayerPosition);
    const float dx = playerPosition.x - g_GroundCrowbarPosition.x;
    const float dz = playerPosition.z - g_GroundCrowbarPosition.z;
    const float pickupRadius = 0.65f;
    const float maxVerticalDistance = 1.5f;

    return dx * dx + dz * dz <= pickupRadius * pickupRadius &&
           std::fabs(playerPosition.y - g_GroundCrowbarPosition.y) <= maxVerticalDistance;
}

static glm::mat4 GetCrowbarModelMatrixFirstPerson(float swing)
{
    // Camera-space coordinates keep the crowbar fixed to the viewport:
    // +X is right, +Y is up, and -Z is forward.
    // The OBJ's geometry is offset far from its local origin, so center it
    // before scaling and rotating it into the HUD position.
    const glm::vec3 crowbarCenter(-10.7806295f, 33.118441f, 8.964881f);

    return Matrix_Translate(0.20f, -0.12f, -0.45f) *
           Matrix_Rotate_X(-1.2f * swing) *
           Matrix_Rotate_Z(0.4f * swing) *
           Matrix_Rotate_Z(0.12f) *
           Matrix_Rotate_X(1.25f) *
           Matrix_Rotate_Y(3.141592f) *
           Matrix_Scale(0.02f, 0.02f, 0.02f) *
           Matrix_Translate(-crowbarCenter.x, -crowbarCenter.y, -crowbarCenter.z);
}

static glm::mat4 GetCrowbarModelMatrixThirdPerson(const PlayerState &playerState, float swing)
{
    glm::mat4 model = Matrix_Translate(playerState.g_PlayerPosition.x, playerState.g_PlayerPosition.y, playerState.g_PlayerPosition.z) *
                      Matrix_Rotate_Y(playerState.g_PlayerYaw) *
                      Matrix_Translate(0.35f, 0.84f, 0.24f) *
                      Matrix_Rotate_X(-1.2f * swing) *
                      Matrix_Rotate_Z(0.4f * swing) *
                      Matrix_Rotate_Z(1.95f) *
                      Matrix_Rotate_X(0.95f) *
                      Matrix_Scale(0.02f, 0.02f, 0.02f);
    return model;
}

static glm::mat4 GetGroundCrowbarModelMatrix()
{
    const glm::vec3 crowbarCenter(-10.7806295f, 33.118441f, 8.964881f);
    // The crowbar already lies along the ground. Roll it around its long
    // (local Z) axis so the hooked end faces sideways instead of upward.
    const float halfHeightWhenRolled = 0.0164f;

    return Matrix_Translate(g_GroundCrowbarPosition.x,
                            g_GroundCrowbarPosition.y + halfHeightWhenRolled,
                            g_GroundCrowbarPosition.z) *
           Matrix_Rotate_Y(0.6f) *
           Matrix_Rotate_Z(3.141592f / 2.0f) *
           Matrix_Scale(0.02f, 0.02f, 0.02f) *
           Matrix_Translate(-crowbarCenter.x, -crowbarCenter.y, -crowbarCenter.z);
}

int main(int argc, char *argv[])
{
    Window WindowManager = Window();
    WindowManager.Init();

    InputHandler::Init(WindowManager.window);
    InputHandler::inputState.g_UseFirstPersonCamera = true;
    glfwGetCursorPos(WindowManager.window, &g_LastMouseX, &g_LastMouseY);

    Renderer gameRenderer;
    gameRenderer.Initialize();

    glfwPollEvents();
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/gordon.obj", "../../assets/SMD/", gameRenderer, g_VirtualScene, g_CollisionScene);
    glfwPollEvents();
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/crowbar.obj", "../../assets/SMD/", gameRenderer, g_VirtualScene, g_CollisionScene);
    glfwPollEvents();
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/maps/fullmap.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene, true, 0.02f);
    // Equivalent to TramIntro::GetModelMatrix() at the end of the path,
    // expressed without its local-center pivot so rendering and collision
    // hand off to the interactive tram without a visible jump.
    const glm::vec3 tramPosition(34.36484f, -5.054f, -6.749105f);
    const float tramYaw = 1.6048247f;
    glfwPollEvents();
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/tram.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene, true, 0.02f, tramPosition, tramYaw);
    glfwPollEvents();
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/tramDoor.obj", "../../assets/textures/", gameRenderer, g_VirtualScene, g_CollisionScene, true, 0.02f, tramPosition, tramYaw);
    ModelLoader::LoadAndAddToScene("../../assets/OBJ/zombie.obj", "../../assets/SMD/", gameRenderer, g_VirtualScene, g_CollisionScene, true, 0.02f, glm::vec3(-59.820f, -5.760f, -0.224f));

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

    SlidingDoor *glassDoor1 = new SlidingDoor(
        "GlassDoor1", glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.7f));
    g_WorldEntities.push_back(glassDoor1);

    SlidingDoor *glassDoor2 = new SlidingDoor(
        "GlassDoor2", glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.7f));
    g_WorldEntities.push_back(glassDoor2);

    SlidingDoor *babtechDoor = new SlidingDoor(
        "BabtechDoor", glm::vec3(0.0f), glm::vec3(0.0f, -2.2f, 0.0f));
    g_WorldEntities.push_back(babtechDoor);

    SlidingDoor *largeDoor = new SlidingDoor(
        "LargeDoor", glm::vec3(0.0f), glm::vec3(0.0f, -3.2f, 0.0f));
    g_WorldEntities.push_back(largeDoor);

    ChasingZombie *finalBoss = new ChasingZombie(
        "zombie", glm::vec3(-59.820f, -5.760f, -0.224f));
    g_WorldEntities.push_back(finalBoss);

    const float crowbarAttackDuration = 0.3f;
    float crowbarAttackElapsed = crowbarAttackDuration;
    bool crowbarHitRegistered = false;
    bool leftMouseWasDown = false;

    while (!glfwWindowShouldClose(WindowManager.window))
    {
        static float previous_time = (float)glfwGetTime();
        float current_time = (float)glfwGetTime();
        float delta_time = current_time - previous_time;
        previous_time = current_time;

        const bool leftMouseDown = glfwGetMouseButton(WindowManager.window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        if (leftMouseDown && !leftMouseWasDown && InputHandler::inputState.g_EquippedCrowbar)
        {
            crowbarAttackElapsed = 0.0f;
            crowbarHitRegistered = false;
        }
        leftMouseWasDown = leftMouseDown;

        if (crowbarAttackElapsed < crowbarAttackDuration)
        {
            crowbarAttackElapsed += delta_time;
            if (crowbarAttackElapsed > crowbarAttackDuration)
                crowbarAttackElapsed = crowbarAttackDuration;
        }

        const float crowbarAttackProgress = crowbarAttackElapsed / crowbarAttackDuration;
        const float crowbarSwing = std::sin(crowbarAttackProgress * 3.141592f);

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

            if (g_GroundCrowbarAvailable && IsPlayerSteppingOnGroundCrowbar())
            {
                g_GroundCrowbarAvailable = false;
                InputHandler::inputState.g_HasCrowbar = true;
                InputHandler::inputState.g_EquippedCrowbar = true;
                printf("Crowbar picked up\n");
            }
        }

        camera.Update(InputHandler::inputState, Player::playerState, dx, dy);

        if (TramIntro::IsActive())
            InputHandler::inputState.g_interact = false;

        if (!TramIntro::IsActive() && InputHandler::inputState.g_interact)
        {
            InputHandler::inputState.g_interact = false;

            glm::vec3 rayOrigin = glm::vec3(camera.camera_position_c);
            glm::vec3 rayDir = glm::normalize(glm::vec3(camera.camera_view_vector));

            const CollisionObject *closestObject = nullptr;
            float closestObjectT = std::numeric_limits<float>::max();

            for (const auto &object : g_CollisionScene)
            {
                float objectT;
                if (Interaction::RayIntersectsAABB(
                        rayOrigin, rayDir, object.bbox_min, object.bbox_max, objectT) &&
                    objectT < closestObjectT)
                {
                    closestObjectT = objectT;
                    closestObject = &object;
                }
            }

            if (closestObject != nullptr)
            {
                const glm::vec3 hitPosition = rayOrigin + rayDir * closestObjectT;
                printf("Ray hit coordinates: x=%.3f y=%.3f z=%.3f (distance: %.2f)\n",
                       hitPosition.x, hitPosition.y, hitPosition.z, closestObjectT);
            }
            else
            {
                printf("Ray hit: nothing\n");
            }

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

        // Crowbar hit detection at the peak of the swing (progress == 0.5)
        if (crowbarAttackProgress >= 0.5f && !crowbarHitRegistered && finalBoss != nullptr)
        {
            crowbarHitRegistered = true;
            const glm::vec3 playerPos(Player::playerState.g_PlayerPosition);
            const float dx = finalBoss->position.x - playerPos.x;
            const float dz = finalBoss->position.z - playerPos.z;
            const float distSq = dx * dx + dz * dz;
            if (distSq < 3.0f * 3.0f && distSq > 0.00001f)
            {
                const float invDist = 1.0f / std::sqrt(distSq);
                const float nx = dx * invDist;
                const float nz = dz * invDist;
                const float yaw = Player::playerState.g_PlayerYaw;
                const float dot = nx * std::sin(yaw) + nz * std::cos(yaw);
                if (dot > 0.3f)
                    finalBoss->Hit();
            }
        }

        // --- UPDATE RUNTIME ENTITIES ---
        for (auto &entity : g_WorldEntities)
        {
            entity->Update(delta_time);
        }

        // Remove entities that died this frame
        g_WorldEntities.erase(
            std::remove_if(g_WorldEntities.begin(), g_WorldEntities.end(),
                [&finalBoss](Entity *e) {
                    if (e->IsDead())
                    {
                        if (e == finalBoss)
                            finalBoss = nullptr;
                        delete e;
                        return true;
                    }
                    return false;
                }),
            g_WorldEntities.end());

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

        if (g_GroundCrowbarAvailable)
        {
            gameRenderer.SetModelMatrix(GetGroundCrowbarModelMatrix());
            gameRenderer.DrawVirtualObject("crowbar", g_VirtualScene);
        }

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

        // Debug hitboxes belong to the world pass.
        DrawPlayerHitbox(view, projection);

        if (InputHandler::inputState.g_HasCrowbar && InputHandler::inputState.g_EquippedCrowbar)
        {
            glm::mat4 crowbarModel;
            if (InputHandler::inputState.g_UseFirstPersonCamera)
            {
                // Render the first-person weapon as a final camera-space pass.
                // Clearing only depth keeps it visible in front of world geometry
                // while preserving depth between the crowbar's own triangles.
                glClear(GL_DEPTH_BUFFER_BIT);
                gameRenderer.BeginFrame(glm::mat4(1.0f), projection);
                crowbarModel = GetCrowbarModelMatrixFirstPerson(crowbarSwing);
            }
            else
                crowbarModel = GetCrowbarModelMatrixThirdPerson(Player::playerState, crowbarSwing);

            gameRenderer.SetModelMatrix(crowbarModel);
            gameRenderer.DrawVirtualObject("crowbar", g_VirtualScene);
        }

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
