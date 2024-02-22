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
    ShaderBuffers();

    void re_alloc(size_t new_elements_count, size_t new_segments_count);

    Element* get_buffer_ptr();

    void draw(UniformParams up, float zoom = 1.0f, sf::Vector2f look_at = sf::Vector2f(0.0, 0.0));

    void free();

    ~ShaderBuffers() { free(); }

private:
    sf::Shader shader;
    bool allocated = false;

    GLuint vbo_index = NULL;
    GLsizei vbo_vertices_count = NULL;

    GLuint ssbo_index = NULL;
    Element* ssbo_mapped_ptr = nullptr;

    size_t elements_count = NULL, segments_count = NULL;
};


#endif //SHADERBEAMS_SHADER_BUFFERS_H
