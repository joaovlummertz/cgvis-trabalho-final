//     Universidade Federal do Rio Grande do Sul
//             Instituto de Informática
//       Departamento de Informática Aplicada
//
//    INF01047 Computação Gráfica e Visualização I
//               Prof. Eduardo Gastal
//
//     -==============================================-
//
//                    MEIA-VIDA 3
//    Elias Furtado Helfer e João Vitor Leffa Lummertz
//
//     -==============================================-

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <tiny_obj_loader.h>
#include <stb_image.h>

#include "utils.h"
#include "matrices.h"
#include "SceneData.h"
#include "Player.h"
#include "PlayerHitbox.h"
#include "PlayerHitbox.h"
#include "Camera.h"
#include "Renderer.h"
#include "Window.h"

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

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                        filename);
                throw std::runtime_error("Objeto sem nome.");
            }
        }
    }
};

void BuildTrianglesAndAddToVirtualScene(ObjModel *, const std::string &basepath, Renderer *renderer, bool build_collision = false, float collision_scale = 1.0f);

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

    // Instantiate and clear out the monolith!
    Renderer gameRenderer;
    gameRenderer.Initialize();

    // Asset Loading
    ObjModel playermodel("../../assets/OBJ/gordon.obj");
    BuildTrianglesAndAddToVirtualScene(&playermodel, "../../assets/SMD/", &gameRenderer);

    ObjModel mapmodel("../../assets/OBJ/maps/fullmap.obj");
    BuildTrianglesAndAddToVirtualScene(&mapmodel, "../../assets/textures/", &gameRenderer, true, 0.02f);

    ObjModel trammodel("../../assets/OBJ/tram.obj");
    BuildTrianglesAndAddToVirtualScene(&trammodel, "../../assets/textures/", &gameRenderer);

    ObjModel tramdoormodel("../../assets/OBJ/tramDoor.obj");
    BuildTrianglesAndAddToVirtualScene(&tramdoormodel, "../../assets/textures/", &gameRenderer);

    InitPlayerHitbox();
    Camera camera;

    while (!glfwWindowShouldClose(WindowManager.window))
    {
        static float previous_time = (float)glfwGetTime();
        float current_time = (float)glfwGetTime();
        float delta_time = current_time - previous_time;
        previous_time = current_time;

        // Systems Updates
        Player::UpdatePlayer(delta_time, InputHandler::inputState.g_UseNoclip);

        double mouseX = InputHandler::inputState.g_MouseX;
        double mouseY = InputHandler::inputState.g_MouseY;
        double dx = mouseX - g_LastMouseX;
        double dy = mouseY - g_LastMouseY;
        g_LastMouseX = mouseX;
        g_LastMouseY = mouseY;

        camera.Update(InputHandler::inputState, Player::playerState, dx, dy);

        // --- RENDER PASS ---
        gameRenderer.ClearColor(0.9f, 0.9f, 1.0f, 1.0f);

        glm::mat4 view = Matrix_Camera_View(camera.camera_position_c, camera.camera_view_vector, camera.camera_up_vector);
        float field_of_view = 3.141592f / 3.0f;
        glm::mat4 projection = Matrix_Perspective(field_of_view, g_ScreenRatio, -0.1f, -100000.0f);

        gameRenderer.BeginFrame(view, projection);

        // Draw Player
        if (!InputHandler::inputState.g_UseFirstPersonCamera)
        {
            glm::mat4 model = Matrix_Translate(Player::playerState.g_PlayerPosition.x, Player::playerState.g_PlayerPosition.y, Player::playerState.g_PlayerPosition.z) * Matrix_Rotate_Y(Player::playerState.g_PlayerYaw) * Matrix_Scale(0.02f, 0.02f, 0.02f);
            gameRenderer.SetModelMatrix(model);
            gameRenderer.DrawVirtualObject("Gordon_Hi", g_VirtualScene);
        }

        // Draw Map environment structures
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

void BuildTrianglesAndAddToVirtualScene(ObjModel *model, const std::string &basepath, Renderer *renderer, bool build_collision, float collision_scale)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float> model_coefficients;
    std::vector<float> normal_coefficients;
    std::vector<float> texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval, maxval, maxval);
        glm::vec3 bbox_max = glm::vec3(minval, minval, minval);

        GLuint tex_id = 0;
        int mat_id = model->shapes[shape].mesh.material_ids[0];
        if (mat_id >= 0 && mat_id < (int)model->materials.size())
        {
            const std::string &texname = model->materials[mat_id].diffuse_texname;

            if (!texname.empty())
                tex_id = renderer->LoadTextureImage((basepath + texname).c_str());
        }

        std::vector<glm::vec3> shape_vertices;
        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3 * triangle + vertex];

                indices.push_back(first_index + 3 * triangle + vertex);

                const float vx = model->attrib.vertices[3 * idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3 * idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3 * idx.vertex_index + 2];
                // printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back(vx);   // X
                model_coefficients.push_back(vy);   // Y
                model_coefficients.push_back(vz);   // Z
                model_coefficients.push_back(1.0f); // W

                shape_vertices.push_back(glm::vec3(vx, vy, vz));

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                if (idx.normal_index != -1)
                {
                    const float nx = model->attrib.normals[3 * idx.normal_index + 0];
                    const float ny = model->attrib.normals[3 * idx.normal_index + 1];
                    const float nz = model->attrib.normals[3 * idx.normal_index + 2];
                    normal_coefficients.push_back(nx);   // X
                    normal_coefficients.push_back(ny);   // Y
                    normal_coefficients.push_back(nz);   // Z
                    normal_coefficients.push_back(0.0f); // W
                }

                if (idx.texcoord_index != -1)
                {
                    const float u = model->attrib.texcoords[2 * idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2 * idx.texcoord_index + 1];
                    texture_coefficients.push_back(u);
                    texture_coefficients.push_back(v);
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.texture_id = tex_id;
        theobject.name = model->shapes[shape].name;
        theobject.first_index = first_index;                  // Primeiro índice
        theobject.num_indices = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;              // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        g_VirtualScene[model->shapes[shape].name] = theobject;

        if (build_collision)
        {
            CollisionObject collision_object;
            collision_object.name = model->shapes[shape].name;
            collision_object.bbox_min = bbox_min * collision_scale;
            collision_object.bbox_max = bbox_max * collision_scale;
            collision_object.vertices.reserve(shape_vertices.size());
            for (const glm::vec3 &vertex : shape_vertices)
            {
                collision_object.vertices.push_back(vertex * collision_scale);
            }

            g_CollisionScene.push_back(collision_object);
        }
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0;            // "(location = 0)" em "shader_vertex.glsl"
    GLint number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if (!normal_coefficients.empty())
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if (!texture_coefficients.empty())
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2;             // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}