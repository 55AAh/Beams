#ifndef SHADERBEAMS_SOLVER_H
#define SHADERBEAMS_SOLVER_H


const float PI = 3.14159265358979f;

struct ElementUWT {
    float u, w, t;
};
ElementUWT construct_uwt(float u, float w, float t);

struct ElementParams {
    ElementUWT uwt;
    float m;
    float q, n;
};

struct UniformParams {
    float EI, GJ;
    float total_weight;
    float total_length;
};
#define UP_ARRAY_SIZE 4
#define UP_ARRAY(name, up) float name[UP_ARRAY_SIZE] { up.EI, up.GJ, up.total_weight, up.total_length }

class Solver {
public:
    void setup(UniformParams new_up) {
        up = new_up;
        _was_setup = true;
    }

    [[nodiscard]] bool was_setup() const { return _was_setup; }

    void traverse(float theta, ElementParams* elements, size_t elements_count, size_t begin, size_t end) const;

    void forget();

    ~Solver() { forget(); }

    UniformParams up {};

private:
    bool _was_setup = false;
};


#endif //SHADERBEAMS_SOLVER_H
