#pragma once

#include <glad/glad.h>
#include <glm/mat4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <string>
#include <vector>
#include "SceneData.h" // Assuming this holds SceneObject and CollisionObject structs

class Renderer
{
public:
    Renderer();
    ~Renderer();

    // Core Lifecycle
    void Initialize();
    void Shutdown();
    void LoadShaders();
    void ClearColor(float r, float g, float b, float a);

    // Resource Loading
    GLuint LoadTextureImage(const char *filename);

    // Render Passes
    void BeginFrame(const glm::mat4 &view, const glm::mat4 &projection);
    void DrawVirtualObject(const char *prefix, const std::map<std::string, SceneObject> &virtualScene);
    void SetModelMatrix(const glm::mat4 &model);

    // Getters
    GLuint GetGpuProgramID() const { return m_GpuProgramID; }

    GLuint LoadShader_Vertex(const char *filename);
    GLuint LoadShader_Fragment(const char *filename);
    void LoadShader(const char *filename, GLuint shader_id);
    GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id);

private:
    // GPU Program and Uniform State
    GLuint m_GpuProgramID;
    GLint m_model_uniform;
    GLint m_view_uniform;
    GLint m_projection_uniform;
    GLint m_bbox_min_uniform;
    GLint m_bbox_max_uniform;
    GLint m_has_texture_uniform;
    GLint m_light_position_uniform;
    GLint m_light_color_uniform;
    GLint m_ambient_color_uniform;
    GLint m_material_ka_uniform;
    GLint m_material_kd_uniform;
    GLint m_material_ks_uniform;
    GLint m_material_shininess_uniform;
    std::vector<GLuint> m_TextureIDs;
    std::vector<GLuint> m_SamplerIDs;
};
