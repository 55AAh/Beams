#ifndef SHADERBEAMS_SOLVER_H
#define SHADERBEAMS_SOLVER_H


#define C_USE_DOUBLE_PRECISION 1

#if C_USE_DOUBLE_PRECISION
#define C_float double
#else
#define C_float float
#endif


#define C_Basis_FIELDS t, n
struct C_Basis {
    C_float t[2];
    C_float n[2];
};

#define C_SolutionFull_FIELDS x, y, M, T, tn, Fx, Fy
struct C_SolutionFull {
    C_float x, y;
    C_float M;
    C_float T;
    C_Basis tn;
    C_float Fx, Fy;
};

#define C_SolutionBase_FIELDS u, w, M, T, tn
struct C_SolutionBase {
    C_float u, w;
    C_float M;
    C_float T;
    C_Basis tn;
};

#define C_SolutionCorr_FIELDS u, w, M, T, N, Q, Pt, Pn
struct C_SolutionCorr {
    C_float u, w;
    C_float M;
    C_float T;
    C_float N, Q;
    C_float Pt, Pn;
};

#define C_Element_FIELDS full, base, corr
struct C_Element {
    C_SolutionFull full;
    C_SolutionBase base;
    C_SolutionCorr corr;
};

#define C_UniformParams_FIELDS corr_selector, EI, initial_angle, total_weight, total_length, gap, elements_count
struct C_UniformParams {
    int corr_selector;
    C_float EI;
    C_float initial_angle;
    C_float total_weight;
    C_float total_length;
    C_float gap;
    int elements_count;
};

#define UP_ARRAY_SIZE 7

C_SolutionFull C_EQLINK_setup_initial_border(C_UniformParams up);
C_SolutionBase C_EQLINK_setup_base(C_UniformParams up, C_SolutionFull full0);
C_SolutionCorr C_EQLINK_setup_corr(C_UniformParams up, C_SolutionFull full0, C_SolutionBase base0);
C_SolutionBase C_EQLINK_link_base(C_UniformParams up, C_SolutionFull full0, C_SolutionBase base0, C_float s);
C_SolutionCorr C_EQLINK_link_corr(C_UniformParams up, [[maybe_unused]] C_SolutionFull full0, C_SolutionBase base0, C_SolutionCorr corr0, C_float s);
C_SolutionCorr C_EQLINK_link_corr_linear(C_UniformParams up, [[maybe_unused]] C_SolutionFull full0, C_SolutionBase base0, C_SolutionCorr corr0, C_float s);
C_SolutionCorr C_EQLINK_link_corr_exponential(C_UniformParams up, [[maybe_unused]] C_SolutionFull full0, C_SolutionBase base0, C_SolutionCorr corr0, C_float s);
C_SolutionFull C_EQLINK_link_full([[maybe_unused]] C_UniformParams up, C_SolutionFull full0, C_SolutionBase base0, C_SolutionBase base_s, C_SolutionCorr corr_s,
                                  [[maybe_unused]] C_float s);

const C_float PI = 3.14159265358979f;

class C_Solver {
public:
    void setup(C_UniformParams new_up);

    [[nodiscard]] bool was_setup() const { return _was_setup; }

    void traverse(size_t begin, size_t end) const;

    C_Element get_solution_at(size_t element_i, C_float s) const;

    void forget();

    ~C_Solver() { forget(); }

    C_UniformParams up {};

    C_Element* elements = nullptr;

private:
    void internal_re_alloc(size_t new_elements_count);

    void internal_ensure_free();

    bool _was_setup = false;

    size_t elements_count = 0;
    bool allocated = false;
};


#endif //SHADERBEAMS_SOLVER_H
