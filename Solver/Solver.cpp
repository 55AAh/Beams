#include "Solver.h"

#define INSIDE_C_CODE
#define ONLY_IMPLEMENT_EQLINK
#include "shaders/vertex_shader.vert" // This will implement EQLINK
#undef ONLY_IMPLEMENT_EQLINK
#undef INSIDE_C_CODE


ElementUWT construct_uwt(float u, float w, float t) {
    return ElementUWT { u, w, t };
}

void Solver::traverse(float theta, ElementParams *elements, size_t begin, size_t end) const {
    float each_length = up.total_length / (float)up.elements_count;

    ElementParams ep0 = begin == 0 ?
            ElementParams{ ElementUWT { 0, 0, theta }, 0.0, 0, 0 }
            : elements[begin - 1];

    for (size_t element_i = begin; element_i <= end; ++element_i) {
        elements[element_i] = ep0;

        ElementUWT uwt1 = calc_uwt(ep0, up, each_length);
        ElementParams ep1 { uwt1, 2.0, 0, 0 };

        ep0 = ep1;
    }
}

void Solver::forget() {
    _was_setup = false;
}