#include <fstream>
#include <sstream>
#include "shader_buffers.h"

#include <regex>



std::string read_file(const char* path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        fprintf(stderr, "Error reading file '%s'!\n", path);
        abort();
    }
    std::string str((std::istreambuf_iterator<char>(ifs)), (std::istreambuf_iterator<char>()));
    return str;
}

std::string patch_vertex_shader(const std::string& original_code) {
    std::regex re(R"(\/\/ % version (.*)\n)");
    std::string patched_code = std::regex_replace(original_code, re, "#version $1\n");
    return patched_code;
}

ShaderBuffers::ShaderBuffers() {
    const char* vs_path = "shaders\\vertex_shader.vert";
    const char* fs_path = "shaders\\fragment_shader.frag";

    std::string vs_code = read_file(vs_path);
    vs_code = patch_vertex_shader(vs_code);

    std::string fs_code = read_file(fs_path);

    if (!shader.loadFromMemory(vs_code, fs_code)) {
        abort();
    }
}

void* alloc_buffer(GLuint* index, GLenum target, GLsizeiptr size_bytes) {
    // Generate & bind buffer
    glGenBuffers(1, index);
    glBindBuffer(target, *index);

    // Allocate buffer (uninitialized)
    GLenum usage = (target == GL_SHADER_STORAGE_BUFFER) ? GL_DYNAMIC_DRAW : GL_STATIC_DRAW;
    glBufferData(target, size_bytes, nullptr, usage);

    // Map buffer to memory
    void* ptr = glMapBufferRange(target, 0, size_bytes, GL_MAP_WRITE_BIT);

    return ptr;
}

void unmap_buffer(GLenum target) {
    glUnmapBuffer(target);
}

void free_buffer(GLuint* index) {
    glDeleteBuffers(1, index);
}

void ShaderBuffers::re_alloc(size_t new_elements_count, size_t new_segments_count) {
    if (allocated && (new_elements_count == elements_count && new_segments_count == segments_count)) {
        return;
    }
    elements_count = new_elements_count;
    segments_count = new_segments_count;

    free();

    // Generate SSBO buffer
    auto ssbo_size_bytes = (GLsizeiptr) (sizeof(Element) * new_elements_count);
    void* ssbo_void_ptr = alloc_buffer(&ssbo_index, GL_SHADER_STORAGE_BUFFER, ssbo_size_bytes);
    ssbo_mapped_ptr = static_cast<Element*>(ssbo_void_ptr);

    // SSBO buffer is left with uninitialized data
    // It will be written during ElementParams computation

    // Generate VBO buffer
    vbo_vertices_count = (GLsizei) (new_elements_count * (new_segments_count + 1));
    auto vbo_size_bytes = (GLsizeiptr) (sizeof(VBO_vertex) * vbo_vertices_count);
    void* vbo_void_ptr = alloc_buffer(&vbo_index, GL_ARRAY_BUFFER, vbo_size_bytes);
    auto vbo_mapped_ptr = static_cast<VBO_vertex*>(vbo_void_ptr);

    // Fill VBO buffer
    auto _ptr = vbo_mapped_ptr;
    for (size_t element = 0; element < new_elements_count; ++element) {
        for (size_t si = 0; si <= new_segments_count; ++si) {
            _ptr->element = (float)element;
            _ptr->s = (float)si / (float)new_segments_count;
            ++_ptr;
        }
    }

    // Unmap VBO buffer
    unmap_buffer(GL_ARRAY_BUFFER);

    allocated = true;
}

Element *ShaderBuffers::get_buffer_ptr() {
    return allocated ? ssbo_mapped_ptr : nullptr;
}

void ShaderBuffers::draw(UniformParams up) {
    if (!allocated) {
        return;
    }

    sf::Shader::bind(&shader);

    PACK_UP(up_array, up);
    shader.setUniformArray("up_array", up_array, UP_ARRAY_SIZE);

    glBindBuffer(GL_ARRAY_BUFFER, vbo_index);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_index);
    glDrawArrays(GL_LINE_STRIP, 0, vbo_vertices_count);

    glDisableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    sf::Shader::bind(nullptr);
}

void ShaderBuffers::free() {
    if (!allocated) {
        return;
    }

    unmap_buffer(GL_SHADER_STORAGE_BUFFER);
    ssbo_mapped_ptr = nullptr;
    free_buffer(&ssbo_index);
    ssbo_index = NULL;

    free_buffer(&vbo_index);
    vbo_index = NULL;
    vbo_vertices_count = NULL;

    allocated = false;
}
