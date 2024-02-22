#ifndef SHADERBEAMS_SOLVER_H
#define SHADERBEAMS_SOLVER_H


#define INSIDE_C_CODE
#include "shaders/vertex_shader.vert" // This will export type definitions, but not implement EQLINK
#undef INSIDE_C_CODE


#include <string>


const float PI = 3.14159265358979f;

class Solver {
public:
    void setup(UniformParams new_up) {
        up = new_up;
        _was_setup = true;
    }

    [[nodiscard]] bool was_setup() const { return _was_setup; }

    void traverse(float theta, ElementParams* elements, size_t begin, size_t end) const;

    void forget();

    ~Solver() { forget(); }

    UniformParams up {};

private:
    bool _was_setup = false;
};

#endif //SHADERBEAMS_SOLVER_H
