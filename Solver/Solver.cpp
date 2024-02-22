#include "Solver.h"

#define INSIDE_C_CODE
#define ONLY_IMPLEMENT_EQLINK
#include "shaders/vertex_shader.vert" // This will implement EQLINK
#undef ONLY_IMPLEMENT_EQLINK
#undef INSIDE_C_CODE


void Solver::traverse(Element *elements, size_t begin, size_t end) const {
    float each_length = up.total_length / (float)up.elements_count;

    Element el0 = begin == 0 ?
            Element { 0, 0, 0.0, up.initial_angle, 0.0, 0.0 }
            : elements[begin - 1];

    for (size_t element_i = begin; element_i <= end; ++element_i) {
        elements[element_i] = el0;

        Element el1 = calc_EQLINK(el0, up, each_length);

        el0 = el1;
    }
}

void Solver::forget() {
    _was_setup = false;
}