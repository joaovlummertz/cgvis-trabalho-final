#ifndef SCENE_DATA_H
#define SCENE_DATA_H

#include <cstddef>
#include <map>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <glm/vec3.hpp>

struct SceneObject
{
    std::string name;
    size_t first_index;
    size_t num_indices;
    GLenum rendering_mode;
    GLuint vertex_array_object_id;
    GLuint texture_id;
    bool emissive;
};

struct CollisionObject
{
    std::string name;
    glm::vec3 bbox_min;
    glm::vec3 bbox_max;
    std::vector<glm::vec3> vertices;
};

extern std::map<std::string, SceneObject> g_VirtualScene;
extern std::vector<CollisionObject> g_CollisionScene;
extern std::vector<glm::vec3> g_LightPoints;

#endif
