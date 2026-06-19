#include "PlayerHitbox.h"

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <string>

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
    const float g_PlayerHitboxHeight = 1.5f;

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
    const float half_width = g_PlayerHitboxRadius;
    const float half_depth = g_PlayerHitboxRadius;

    const GLfloat positions[] = {
        -half_width,
        0.0f,
        -half_depth,
        1.0f,
        half_width,
        0.0f,
        -half_depth,
        1.0f,
        half_width,
        0.0f,
        half_depth,
        1.0f,
        -half_width,
        0.0f,
        half_depth,
        1.0f,
        -half_width,
        g_PlayerHitboxHeight,
        -half_depth,
        1.0f,
        half_width,
        g_PlayerHitboxHeight,
        -half_depth,
        1.0f,
        half_width,
        g_PlayerHitboxHeight,
        half_depth,
        1.0f,
        -half_width,
        g_PlayerHitboxHeight,
        half_depth,
        1.0f,
    };

    const GLuint indices[] = {
        0, 1, 1, 2, 2, 3, 3, 0,
        4, 5, 5, 6, 6, 7, 7, 4,
        0, 4, 1, 5, 2, 6, 3, 7};

    LoadDebugShadersFromFiles();

    glGenVertexArrays(1, &g_PlayerHitboxVAO);
    glBindVertexArray(g_PlayerHitboxVAO);

    glGenBuffers(1, &g_PlayerHitboxVBOPositions);
    glBindBuffer(GL_ARRAY_BUFFER, g_PlayerHitboxVBOPositions);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &g_PlayerHitboxEBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_PlayerHitboxEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

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
    glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_CULL_FACE);
    glBindVertexArray(0);

    glUseProgram(0);
}
