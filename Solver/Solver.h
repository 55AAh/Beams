#ifndef SHADERBEAMS_SOLVER_H
#define SHADERBEAMS_SOLVER_H


struct ElementParams {
    float u, w;
    float t, m;
    float q, n;
};

class Solver {
public:
    Solver() {}
    void traverse(float theta, ElementParams* elements, size_t begin, size_t end);

private:
    float total_length;
};


#endif //SHADERBEAMS_SOLVER_H
