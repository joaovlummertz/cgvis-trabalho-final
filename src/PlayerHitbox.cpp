#include "PlayerHitbox.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>

#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "InputHandler.h"

GLuint LoadShader_Vertex(const char *filename);
GLuint LoadShader_Fragment(const char *filename);
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

namespace
{
    GLuint g_DebugGpuProgramID = 0;
    GLint g_debug_model_uniform = -1;
    GLint g_debug_view_uniform = -1;
    GLint g_debug_projection_uniform = -1;
    GLuint g_PlayerHitboxVAO = 0;
    GLuint g_PlayerHitboxVBOPositions = 0;
    GLuint g_PlayerHitboxEBO = 0;

    const float g_PlayerHitboxRadius = 0.25f;

    void LoadDebugShadersFromFiles()
    {
        GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_debug_vertex.glsl");
        GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_debug_fragment.glsl");

        if (g_DebugGpuProgramID != 0)
            glDeleteProgram(g_DebugGpuProgramID);

        g_DebugGpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

        g_debug_model_uniform = glGetUniformLocation(g_DebugGpuProgramID, "model");
        g_debug_view_uniform = glGetUniformLocation(g_DebugGpuProgramID, "view");
        g_debug_projection_uniform = glGetUniformLocation(g_DebugGpuProgramID, "projection");
    }
} // namespace

void InitPlayerHitbox()
{
    std::vector<GLfloat> positions;
    std::vector<GLuint> indices;
    float r = g_PlayerHitboxRadius;
    float sphere_heights[3] = {0.25f, 0.75f, 1.25f};

    int points_per_circle = 16;
    GLuint vertex_offset = 0;

    for (float h_offset : sphere_heights)
    {
        glm::vec3 center = glm::vec3(0.0f, h_offset, 0.0f);

        // 1. XY Circle (front-facing)
        GLuint xy_start = vertex_offset;
        for (int i = 0; i < points_per_circle; ++i)
        {
            float theta = 2.0f * 3.14159265f * (float)i / (float)points_per_circle;
            positions.push_back(center.x + r * cosf(theta));
            positions.push_back(center.y + r * sinf(theta));
            positions.push_back(center.z);
            positions.push_back(1.0f);
        }
        for (int i = 0; i < points_per_circle; ++i)
        {
            indices.push_back(xy_start + i);
            indices.push_back(xy_start + (i + 1) % points_per_circle);
        }
        vertex_offset += points_per_circle;

        // 2. YZ Circle (side-facing)
        GLuint yz_start = vertex_offset;
        for (int i = 0; i < points_per_circle; ++i)
        {
            float theta = 2.0f * 3.14159265f * (float)i / (float)points_per_circle;
            positions.push_back(center.x);
            positions.push_back(center.y + r * cosf(theta));
            positions.push_back(center.z + r * sinf(theta));
            positions.push_back(1.0f);
        }
        for (int i = 0; i < points_per_circle; ++i)
        {
            indices.push_back(yz_start + i);
            indices.push_back(yz_start + (i + 1) % points_per_circle);
        }
        vertex_offset += points_per_circle;

        // 3. ZX Circle (top-facing)
        GLuint zx_start = vertex_offset;
        for (int i = 0; i < points_per_circle; ++i)
        {
            float theta = 2.0f * 3.14159265f * (float)i / (float)points_per_circle;
            positions.push_back(center.x + r * cosf(theta));
            positions.push_back(center.y);
            positions.push_back(center.z + r * sinf(theta));
            positions.push_back(1.0f);
        }
        for (int i = 0; i < points_per_circle; ++i)
        {
            indices.push_back(zx_start + i);
            indices.push_back(zx_start + (i + 1) % points_per_circle);
        }
        vertex_offset += points_per_circle;
    }

    LoadDebugShadersFromFiles();

    glGenVertexArrays(1, &g_PlayerHitboxVAO);
    glBindVertexArray(g_PlayerHitboxVAO);

    glGenBuffers(1, &g_PlayerHitboxVBOPositions);
    glBindBuffer(GL_ARRAY_BUFFER, g_PlayerHitboxVBOPositions);
    glBufferData(GL_ARRAY_BUFFER, positions.size() * sizeof(GLfloat), positions.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &g_PlayerHitboxEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_PlayerHitboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

    glBindVertexArray(0);
}

void DrawPlayerHitbox(glm::mat4 view, glm::mat4 projection)
{
    if (InputHandler::inputState.g_UseFirstPersonCamera || !InputHandler::inputState.g_ShowHitbox)
        return;

    glm::mat4 model(1.0f);
    model = glm::translate(model, glm::vec3(Player::playerState.g_PlayerPosition.x, Player::playerState.g_PlayerPosition.y, Player::playerState.g_PlayerPosition.z));

    glUseProgram(g_DebugGpuProgramID);
    glUniformMatrix4fv(g_debug_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(g_debug_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(g_debug_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));

    glBindVertexArray(g_PlayerHitboxVAO);
    glDisable(GL_CULL_FACE);
    glLineWidth(2.0f);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    // 3 spheres * 3 circles/sphere * 16 segments/circle * 2 indices/segment = 288 indices
    glDrawElements(GL_LINES, 288, GL_UNSIGNED_INT, 0);

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
    glBindVertexArray(0);

    glUseProgram(0);
}
