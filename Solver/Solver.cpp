#include "Solver.h"

#define INSIDE_C_CODE
#define ONLY_IMPLEMENT_EQLINK
#include "shaders/vertex_shader.vert" // This will implement EQLINK
#undef ONLY_IMPLEMENT_EQLINK
#undef INSIDE_C_CODE


void Solver::traverse(Element *elements, size_t begin, size_t end) const {
    float each_length = up.total_length / (float)up.elements_count;

    SolutionFull border {};
    if (begin > 0) {
        border = elements[begin - 1].full; // EQLINK TO END! // TODO
    }
    else {
        border = EQLINK_setup_initial_border(up);
    }

    for (size_t element_i = begin; element_i <= end; ++element_i) {
        SolutionFull full0 = border;
        SolutionBase base0 = EQLINK_setup_base(up, full0);
        SolutionCorr corr0 = EQLINK_setup_corr(up, full0, base0);
        Element el0 {full0, base0, corr0 };

        elements[element_i] = el0;

        SolutionBase base1 = EQLINK_link_base(up, full0, base0, each_length);
        SolutionCorr corr1 = EQLINK_link_corr(up, full0, base0, corr0, each_length);
        SolutionFull full1 = EQLINK_link_full(up, full0, base0,base1, corr1, each_length);
        border = full1;
    }
}

void Solver::forget() {
    _was_setup = false;
}