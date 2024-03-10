#ifndef SHADERBEAMS_SHADER_BUFFERS_H
#define SHADERBEAMS_SHADER_BUFFERS_H

#include <SFML/Graphics/Shader.hpp>
#include <GL/glew.h>
#include <array>


#define GLSL_USE_DOUBLE_PRECISION 1

#if GLSL_USE_DOUBLE_PRECISION
#define GLSL_float double
#define OpenGLDataType GL_DOUBLE
#else
#define GLSL_float float
#define OpenGLDataType GL_FLOAT
#endif


struct GLSL_Basis {
    GLSL_float t[2];
    GLSL_float n[2];
};

struct GLSL_SolutionFull {
    GLSL_float x, y;
    GLSL_float M;
    GLSL_float T;
    GLSL_Basis tn;
    GLSL_float Fx, Fy;
};

struct GLSL_SolutionBase {
    GLSL_float u, w;
    GLSL_float M;
    GLSL_float T;
    GLSL_Basis tn;
};

struct GLSL_SolutionCorr {
    GLSL_float u, w;
    GLSL_float M;
    GLSL_float T;
    GLSL_float N, Q;
    GLSL_float Pt, Pn;
};

struct GLSL_Element {
    GLSL_SolutionFull full;
    GLSL_SolutionBase base;
    GLSL_SolutionCorr corr;
};

struct GLSL_UniformParams {
    int corr_selector;
    GLSL_float EI;
    GLSL_float initial_angle;
    GLSL_float total_weight;
    GLSL_float total_length;
    GLSL_float gap;
    int elements_count;
};

#define UP_ARRAY_SIZE 7
#define GLSL_PACK_UP(arr, up) GLSL_float arr[UP_ARRAY_SIZE] { (GLSL_float)up.corr_selector, up.EI, up.initial_angle, up.total_weight, up.total_length, up.gap, (GLSL_float)up.elements_count }


struct VBO_vertex {
    [[maybe_unused]] GLSL_float s;
    [[maybe_unused]] GLSL_float element;
};

class ShaderBuffers {
public:
    ShaderBuffers();

    void re_alloc(size_t new_elements_count, size_t new_segments_count);

    GLSL_Element* get_buffer_ptr();

    void draw(GLSL_UniformParams up, GLSL_float zoom = 1.0f, std::array<GLSL_float, 2> look_at = {0.0, 0.0}, bool dashed = false);

    void free();

    ~ShaderBuffers() { free(); }

private:
    void internal_re_alloc_VBO(size_t new_elements_count, size_t new_segments_count);

    void internal_re_alloc_SSBO(size_t new_elements_count);

    void internal_ensure_free_VBO();

    void internal_ensure_free_SSBO();

    sf::Shader shader;

    bool vbo_allocated = false;
    GLuint vbo_index = NULL;
    GLsizei vbo_vertices_count = NULL;

    bool ssbo_allocated = false;
    GLuint ssbo_index = NULL;
    GLSL_Element* ssbo_mapped_ptr = nullptr;

    size_t elements_count = NULL, segments_count = NULL;
};


#endif //SHADERBEAMS_SHADER_BUFFERS_H
