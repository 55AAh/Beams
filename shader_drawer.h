#ifndef SHADERBEAMS_SHADER_DRAWER_H
#define SHADERBEAMS_SHADER_DRAWER_H

#include "Solver.h"
#include "shader_buffers.h"


class ShaderDrawer {
public:
    explicit ShaderDrawer(int new_segments_count = 10);

    void setup(UniformParams new_up);

    void tweak(int new_segments_count);

    void process_gui();

    void draw();

    void forget();

    ~ShaderDrawer() { forget(); }

private:
    void ensure_sb();

    Solver solver;
    ShaderBuffers sb;
    int segments_count;
    float zoom;
};


#endif //SHADERBEAMS_SHADER_DRAWER_H
