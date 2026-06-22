#pragma once

#include <map>
#include <vector>
#include <string>
#include <glm/vec3.hpp>
#include "SceneData.h"
#include "Renderer.h"

class ModelLoader
{
public:
    static void Cleanup();

    // Loads an OBJ file, builds OpenGL graphics structures, and adds them to your scene collections
    static void LoadAndAddToScene(
        const char *model_path,
        const char *texture_basepath,
        Renderer &renderer,
        std::map<std::string, SceneObject> &virtualScene,
        std::vector<CollisionObject> &collisionScene,
        bool build_collision = false,
        float collision_scale = 1.0f,
        const glm::vec3 &collision_offset = glm::vec3(0.0f),
        float collision_yaw = 0.0f);
};
