#ifndef SHADERBEAMS_SHADER_BUFFERS_H
#define SHADERBEAMS_SHADER_BUFFERS_H

#include <SFML/Graphics/Shader.hpp>
#include <GL/glew.h>
#include "Solver.h"


struct VBO_vertex {
    float s;
    float element;
};

class ShaderBuffers {
public:
    ShaderBuffers(const char* vs_path, const char* fs_path);

    void re_alloc(size_t new_elements_count, size_t new_segments_count);

    ElementParams* get_buffer_ptr();

    void draw();

    void free();

    ~ShaderBuffers() { free(); }

private:
    sf::Shader shader;
    bool allocated = false;

    GLuint vbo_index = NULL;
    GLsizei vbo_vertices_count = NULL;

    GLuint ssbo_index = NULL;
    ElementParams* ssbo_mapped_ptr = nullptr;
};


#endif //SHADERBEAMS_SHADER_BUFFERS_H
