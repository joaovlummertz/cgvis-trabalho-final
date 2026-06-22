#include "Renderer.h"
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <vector>
#include <cctype>
#include <deque>
#include <stb_image.h>

namespace
{
    bool MatchesObjectFamily(const std::string &objectName, const std::string &family)
    {
        if (objectName.compare(0, family.size(), family) != 0)
            return false;

        if (objectName.size() == family.size())
            return true;

        const unsigned char suffixStart = static_cast<unsigned char>(objectName[family.size()]);
        return std::isdigit(suffixStart) || suffixStart == '-' || suffixStart == '_';
    }

    bool IsColorKeyTexture(const char *filename)
    {
        const std::string path(filename);
        const std::string::size_type separator = path.find_last_of("/\\");
        const std::string::size_type basename = separator == std::string::npos ? 0 : separator + 1;
        return basename < path.size() && path[basename] == '{';
    }

    void ApplyBlueColorKey(unsigned char *data, int width, int height)
    {
        const int pixelCount = width * height;
        for (int pixel = 0; pixel < pixelCount; ++pixel)
        {
            unsigned char *rgba = data + 4 * pixel;
            if (rgba[0] == 0 && rgba[1] == 0 && rgba[2] == 255)
                rgba[3] = 0;
        }
    }

    // Give fully transparent pixels the RGB value of their nearest visible
    // neighbor. Their alpha stays zero, but filtering and mipmaps no longer
    // mix the old blue color into the visible edge.
    void BleedTransparentPixelColors(unsigned char *data, int width, int height)
    {
        const int pixelCount = width * height;
        std::vector<int> nearestSource(pixelCount, -1);
        std::deque<int> pending;

        for (int pixel = 0; pixel < pixelCount; ++pixel)
        {
            if (data[4 * pixel + 3] != 0)
            {
                nearestSource[pixel] = pixel;
                pending.push_back(pixel);
            }
        }

        const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
        while (!pending.empty())
        {
            const int pixel = pending.front();
            pending.pop_front();
            const int x = pixel % width;
            const int y = pixel / width;

            for (const auto &offset : offsets)
            {
                const int neighborX = x + offset[0];
                const int neighborY = y + offset[1];
                if (neighborX < 0 || neighborX >= width || neighborY < 0 || neighborY >= height)
                    continue;

                const int neighbor = neighborY * width + neighborX;
                if (nearestSource[neighbor] != -1)
                    continue;

                nearestSource[neighbor] = nearestSource[pixel];
                pending.push_back(neighbor);
            }
        }

        for (int pixel = 0; pixel < pixelCount; ++pixel)
        {
            if (data[4 * pixel + 3] != 0 || nearestSource[pixel] == -1)
                continue;

            const unsigned char *source = data + 4 * nearestSource[pixel];
            unsigned char *destination = data + 4 * pixel;
            destination[0] = source[0];
            destination[1] = source[1];
            destination[2] = source[2];
        }
    }
}

Renderer::Renderer() : m_GpuProgramID(0) {}

Renderer::~Renderer()
{
    Shutdown();
}

void Renderer::Shutdown()
{
    if (!m_TextureIDs.empty())
    {
        glDeleteTextures(static_cast<GLsizei>(m_TextureIDs.size()), m_TextureIDs.data());
        m_TextureIDs.clear();
    }

    if (!m_SamplerIDs.empty())
    {
        glDeleteSamplers(static_cast<GLsizei>(m_SamplerIDs.size()), m_SamplerIDs.data());
        m_SamplerIDs.clear();
    }

    if (m_GpuProgramID != 0)
    {
        glDeleteProgram(m_GpuProgramID);
        m_GpuProgramID = 0;
    }
}

void Renderer::Initialize()
{
    // Enable Z-buffer and Backface Culling
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    LoadShaders();
}

void Renderer::ClearColor(float r, float g, float b, float a)
{
    glClearColor(r, g, b, a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::BeginFrame(const glm::mat4 &view, const glm::mat4 &projection)
{
    glUseProgram(m_GpuProgramID);
    glUniformMatrix4fv(m_view_uniform, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(m_projection_uniform, 1, GL_FALSE, glm::value_ptr(projection));
}

void Renderer::SetModelMatrix(const glm::mat4 &model)
{
    glUniformMatrix4fv(m_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
}

void Renderer::SetDamageFlash(float amount)
{
    glUniform1f(m_damage_flash_uniform, amount);
}

void Renderer::DrawVirtualObject(const char *prefix, const std::map<std::string, SceneObject> &virtualScene)
{
    const std::string family(prefix);
    for (const auto &pair : virtualScene)
    {
        if (!MatchesObjectFamily(pair.first, family))
            continue;

        const SceneObject &obj = pair.second;

        glBindVertexArray(obj.vertex_array_object_id);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, obj.texture_id);

        glUniform1i(m_has_texture_uniform, obj.texture_id != 0 ? 1 : 0);

        glDrawElements(obj.rendering_mode, obj.num_indices, GL_UNSIGNED_INT,
                       (void *)(obj.first_index * sizeof(GLuint)));

        glBindVertexArray(0);
    }
}

GLuint Renderer::LoadTextureImage(const char *filename)
{
    stbi_set_flip_vertically_on_load(true);
    int width, height, channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 4);

    if (data == nullptr)
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    if (IsColorKeyTexture(filename))
        ApplyBlueColorKey(data, width, height);
    BleedTransparentPixelColors(data, width, height);

    GLuint texture_id, sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(0, sampler_id);

    stbi_image_free(data);
    m_TextureIDs.push_back(texture_id);
    m_SamplerIDs.push_back(sampler_id);
    return texture_id;
}

void Renderer::LoadShaders()
{
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    if (m_GpuProgramID != 0)
        glDeleteProgram(m_GpuProgramID);

    m_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Collect locations
    m_model_uniform = glGetUniformLocation(m_GpuProgramID, "model");
    m_view_uniform = glGetUniformLocation(m_GpuProgramID, "view");
    m_projection_uniform = glGetUniformLocation(m_GpuProgramID, "projection");
    m_bbox_min_uniform = glGetUniformLocation(m_GpuProgramID, "bbox_min");
    m_bbox_max_uniform = glGetUniformLocation(m_GpuProgramID, "bbox_max");
    m_has_texture_uniform = glGetUniformLocation(m_GpuProgramID, "hasTexture");
    m_light_position_uniform = glGetUniformLocation(m_GpuProgramID, "light_position");
    m_light_color_uniform = glGetUniformLocation(m_GpuProgramID, "light_color");
    m_ambient_color_uniform = glGetUniformLocation(m_GpuProgramID, "ambient_color");
    m_material_ka_uniform = glGetUniformLocation(m_GpuProgramID, "material_ka");
    m_material_kd_uniform = glGetUniformLocation(m_GpuProgramID, "material_kd");
    m_material_ks_uniform = glGetUniformLocation(m_GpuProgramID, "material_ks");
    m_material_shininess_uniform = glGetUniformLocation(m_GpuProgramID, "material_shininess");
    m_damage_flash_uniform = glGetUniformLocation(m_GpuProgramID, "damage_flash");

    // Default lighting settings
    glUseProgram(m_GpuProgramID);
    glUniform1i(glGetUniformLocation(m_GpuProgramID, "TextureImage"), 0);
    glUniform3f(m_light_position_uniform, 20.0f, 30.0f, 25.0f);
    glUniform3f(m_light_color_uniform, 1.0f, 0.98f, 0.92f);
    glUniform3f(m_ambient_color_uniform, 0.18f, 0.18f, 0.22f);
    glUniform3f(m_material_ka_uniform, 0.45f, 0.45f, 0.45f);
    glUniform3f(m_material_kd_uniform, 1.0f, 1.0f, 1.0f);
    glUniform3f(m_material_ks_uniform, 0.35f, 0.35f, 0.35f);
    glUniform1f(m_material_shininess_uniform, 32.0f);
    glUseProgram(0);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint Renderer::LoadShader_Vertex(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint Renderer::LoadShader_Fragment(const char *filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void Renderer::LoadShader(const char *filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try
    {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    }
    catch (std::exception &e)
    {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar *shader_string = str.c_str();
    const GLint shader_string_length = static_cast<GLint>(str.length());

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar *log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if (log_length != 0)
    {
        std::string output;

        if (!compiled_ok)
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete[] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint Renderer::CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if (linked_ok == GL_FALSE)
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar *log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete[] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}
