#pragma once

#include <map>
#include <vector>
#include <string>
#include "SceneData.h"
#include "Renderer.h"

class ModelLoader
{
public:
    // Loads an OBJ file, builds OpenGL graphics structures, and adds them to your scene collections
    static void LoadAndAddToScene(
        const char *model_path,
        const char *texture_basepath,
        Renderer &renderer,
        std::map<std::string, SceneObject> &virtualScene,
        std::vector<CollisionObject> &collisionScene,
        bool build_collision = false,
        float collision_scale = 1.0f);
};