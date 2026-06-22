#include "ModelLoader.h"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <limits>
#include <algorithm>
#include <cassert>
#include <cmath>
#include <tiny_obj_loader.h>
#include <glad/glad.h>
#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

namespace
{
    std::vector<GLuint> g_ModelVertexArrays;
    std::vector<GLuint> g_ModelBuffers;
}

// Internal helper structure hidden inside the source file
struct ObjModel
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;

    ObjModel(const char *filename, const char *basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i + 1);
                basepath = dirname.c_str();
            }
        }

        std::string warn, err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr, "Erro: Objeto sem nome dentro do arquivo '%s'.\n", filename);
                throw std::runtime_error("Objeto sem nome.");
            }
        }
    }
};

void ModelLoader::Cleanup()
{
    if (!g_ModelBuffers.empty())
    {
        glDeleteBuffers(static_cast<GLsizei>(g_ModelBuffers.size()), g_ModelBuffers.data());
        g_ModelBuffers.clear();
    }

    if (!g_ModelVertexArrays.empty())
    {
        glDeleteVertexArrays(static_cast<GLsizei>(g_ModelVertexArrays.size()), g_ModelVertexArrays.data());
        g_ModelVertexArrays.clear();
    }
}

void ModelLoader::LoadAndAddToScene(
    const char *model_path,
    const char *texture_basepath,
    Renderer &renderer,
    std::map<std::string, SceneObject> &virtualScene,
    std::vector<CollisionObject> &collisionScene,
    bool build_collision,
    float collision_scale,
    const glm::vec3 &collision_offset,
    float collision_yaw)
{
    // Parse the file using our internal helper
    ObjModel model(model_path);

    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    g_ModelVertexArrays.push_back(vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float> model_coefficients;
    std::vector<float> normal_coefficients;
    std::vector<float> texture_coefficients;

    for (size_t shape = 0; shape < model.shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model.shapes[shape].mesh.num_face_vertices.size();
        if (num_triangles == 0)
            continue;

        const float minval = std::numeric_limits<float>::lowest();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval, maxval, maxval);
        glm::vec3 bbox_max = glm::vec3(minval, minval, minval);

        GLuint tex_id = 0;
        int mat_id = -1;
        if (!model.shapes[shape].mesh.material_ids.empty())
            mat_id = model.shapes[shape].mesh.material_ids[0];

        if (mat_id >= 0 && mat_id < (int)model.materials.size())
        {
            const std::string &texname = model.materials[mat_id].diffuse_texname;
            if (!texname.empty())
            {
                // Call texture loading encapsulated inside the renderer instance
                tex_id = renderer.LoadTextureImage((std::string(texture_basepath) + texname).c_str());
            }
        }

        std::vector<glm::vec3> shape_vertices;
        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model.shapes[shape].mesh.num_face_vertices[triangle] == 3);

            const size_t triangle_offset = 3 * triangle;
            if (triangle_offset + 2 >= model.shapes[shape].mesh.indices.size())
            {
                fprintf(stderr, "Aviso: triangulo incompleto ignorado em '%s' (%s).\n",
                        model_path, model.shapes[shape].name.c_str());
                continue;
            }

            tinyobj::index_t triangle_indices[3];
            glm::vec3 triangle_positions[3];
            bool valid_triangle = true;
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                triangle_indices[vertex] = model.shapes[shape].mesh.indices[triangle_offset + vertex];
                const int vertex_index = triangle_indices[vertex].vertex_index;
                if (vertex_index < 0 ||
                    static_cast<size_t>(3 * vertex_index + 2) >= model.attrib.vertices.size())
                {
                    valid_triangle = false;
                    break;
                }

                triangle_positions[vertex] = glm::vec3(
                    model.attrib.vertices[3 * vertex_index + 0],
                    model.attrib.vertices[3 * vertex_index + 1],
                    model.attrib.vertices[3 * vertex_index + 2]);
            }

            if (!valid_triangle)
            {
                fprintf(stderr, "Aviso: indice de vertice invalido ignorado em '%s' (%s).\n",
                        model_path, model.shapes[shape].name.c_str());
                continue;
            }

            glm::vec3 face_normal = glm::cross(
                triangle_positions[1] - triangle_positions[0],
                triangle_positions[2] - triangle_positions[0]);
            if (glm::dot(face_normal, face_normal) > 0.0f)
                face_normal = glm::normalize(face_normal);
            else
                face_normal = glm::vec3(0.0f, 1.0f, 0.0f);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = triangle_indices[vertex];
                indices.push_back(static_cast<GLuint>(model_coefficients.size() / 4));

                const float vx = triangle_positions[vertex].x;
                const float vy = triangle_positions[vertex].y;
                const float vz = triangle_positions[vertex].z;

                model_coefficients.push_back(vx);
                model_coefficients.push_back(vy);
                model_coefficients.push_back(vz);
                model_coefficients.push_back(1.0f);

                shape_vertices.push_back(glm::vec3(vx, vy, vz));

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if (idx.normal_index >= 0 &&
                    static_cast<size_t>(3 * idx.normal_index + 2) < model.attrib.normals.size())
                {
                    const float nx = model.attrib.normals[3 * idx.normal_index + 0];
                    const float ny = model.attrib.normals[3 * idx.normal_index + 1];
                    const float nz = model.attrib.normals[3 * idx.normal_index + 2];
                    normal_coefficients.push_back(nx);
                    normal_coefficients.push_back(ny);
                    normal_coefficients.push_back(nz);
                }
                else
                {
                    normal_coefficients.push_back(face_normal.x);
                    normal_coefficients.push_back(face_normal.y);
                    normal_coefficients.push_back(face_normal.z);
                }
                normal_coefficients.push_back(0.0f);

                if (idx.texcoord_index >= 0 &&
                    static_cast<size_t>(2 * idx.texcoord_index + 1) < model.attrib.texcoords.size())
                {
                    const float u = model.attrib.texcoords[2 * idx.texcoord_index + 0];
                    const float v = model.attrib.texcoords[2 * idx.texcoord_index + 1];
                    texture_coefficients.push_back(u);
                    texture_coefficients.push_back(v);
                }
                else
                {
                    texture_coefficients.push_back(0.0f);
                    texture_coefficients.push_back(0.0f);
                }
            }
        }

        if (indices.size() == first_index)
            continue;

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.texture_id = tex_id;
        theobject.name = model.shapes[shape].name;
        theobject.first_index = first_index;
        theobject.num_indices = last_index - first_index + 1;
        theobject.rendering_mode = GL_TRIANGLES;
        theobject.vertex_array_object_id = vertex_array_object_id;

        virtualScene[model.shapes[shape].name] = theobject;

        if (build_collision)
        {
            CollisionObject collision_object;
            collision_object.name = model.shapes[shape].name;
            collision_object.bbox_min = glm::vec3(std::numeric_limits<float>::max());
            collision_object.bbox_max = glm::vec3(std::numeric_limits<float>::lowest());
            collision_object.vertices.reserve(shape_vertices.size());

            const float cos_yaw = std::cos(collision_yaw);
            const float sin_yaw = std::sin(collision_yaw);
            for (const glm::vec3 &vertex : shape_vertices)
            {
                const glm::vec3 scaled = vertex * collision_scale;
                glm::vec3 transformed(
                    cos_yaw * scaled.x + sin_yaw * scaled.z,
                    scaled.y,
                    -sin_yaw * scaled.x + cos_yaw * scaled.z);
                transformed += collision_offset;

                collision_object.vertices.push_back(transformed);
                collision_object.bbox_min = glm::min(collision_object.bbox_min, transformed);
                collision_object.bbox_max = glm::max(collision_object.bbox_max, transformed);
            }
            collisionScene.push_back(collision_object);
        }
    }

    // VBO Generation Buffers
    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    g_ModelBuffers.push_back(VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), model_coefficients.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    GLuint VBO_normal_coefficients_id;
    glGenBuffers(1, &VBO_normal_coefficients_id);
    g_ModelBuffers.push_back(VBO_normal_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), normal_coefficients.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(1);

    GLuint VBO_texture_coefficients_id;
    glGenBuffers(1, &VBO_texture_coefficients_id);
    g_ModelBuffers.push_back(VBO_texture_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), texture_coefficients.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(2);

    GLuint indices_id;
    glGenBuffers(1, &indices_id);
    g_ModelBuffers.push_back(indices_id);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}
