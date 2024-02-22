// % version 430 core
// The line above is commented to satisfy C preprocessor
// But it needs to be patched to #version before compiling GLSL





// Those definitions would be shared between C and GLSL code
#ifndef ONLY_IMPLEMENT_EQLINK

    struct SolutionFull {
        float x, y;
        float M;
        float T;
        float t[2], n[2];
        float Fx, Fy;
    };

    struct SolutionBase {
        float u, w;
        float M;
        float T;
        float t[2], n[2];
    };

    struct SolutionCorr {
        float u, w;
        float M;
        float T;
        float N, Q;
        float Pt, Pn;
    };

    struct Element {
        SolutionFull full;
        SolutionBase base;
        SolutionCorr corr;
    };

    struct UniformParams {
        float EI;
        float initial_angle;
        float total_weight;
        float total_length;
        float gap;
        int elements_count;
    };

    #define UP_ARRAY_SIZE 6

#ifdef INSIDE_C_CODE
    #define PACK_UP(arr, up) float arr[UP_ARRAY_SIZE] { up.EI, up.initial_angle, up.total_weight, up.total_length, up.gap, float(up.elements_count) }
#else // INSIDE_C_CODE
    #define UNPACK_UP(up, arr) UniformParams up = UniformParams(arr[0], arr[1], arr[2], arr[3], arr[4], int(arr[5]));
#endif

    Element calc_EQLINK(Element el, UniformParams up, float s);





// This is the GLSL code that integrates EQLINK into shader pipeline
#ifndef INSIDE_C_CODE

    layout (location = 0) in vec2 aPos;
    layout(std430, binding = 0) restrict readonly buffer ElementsBuffer {
        Element elements[];
    };
    uniform float up_array[UP_ARRAY_SIZE];
    uniform float zoom;

    out vec3 vertexColor;

    void main() {
        UNPACK_UP(up, up_array);

        float s = aPos.x * up.total_length / float(elements.length());
        int element_index = int(aPos.y);

        Element el_s = calc_EQLINK(elements[element_index], up, s);

        gl_Position = vec4(el_s.full.x, el_s.full.y, 0.0, 1.0);
    //    gl_Position = vec4(elements[element].uwt.u + s / 3, elements[element].uwt.w + s*s / 3, 0.0, 1.0);
        int _color = element_index % 3;
        vertexColor = vec3(_color == 0, _color == 1, _color == 2);
    }

#endif // INSIDE_C_CODE
#endif // ONLY_IMPLEMENT_EQLINK





// This is needed so that EQLINK is not implemented twice in different C units
#if !defined(INSIDE_C_CODE) || defined(ONLY_IMPLEMENT_EQLINK)

    #ifdef INSIDE_C_CODE
        #include <cmath> // Trig functions are available in GLSL by default
    #endif // INSIDE_C_CODE

    Element calc_EQLINK(Element el0, UniformParams up, float s) {
        float u0 = el0.full.x, w0 = el0.full.y, t0 = el0.full.T, m0 = el0.full.M, n0 = 0.0, q0 = 0.0, EI = up.EI;

        float K = m0 / EI;

        float base_cu, base_cw, base_ct;
        if (K == 0) {
            base_cu = s;
            base_cw = 0;
            base_ct = 0;
        }
        else {
            float r = 1 / K;
            float phi = s / r;

            base_cu = r * sin(phi);
            base_cw = r * (1 - cos(phi));
            base_ct = -phi;
        }

        float base_du = cos(t0) * base_cu + sin(t0) * base_cw;
        float base_dw = -sin(t0) * base_cu + cos(t0) * base_cw;
        float base_dt = base_ct;

        float base_u = u0 + base_du;
        float base_w = w0 + base_dw;
        float base_t = t0 + base_dt;

        // construct
        SolutionFull full_s;
        full_s.x = base_u;
        full_s.y = base_w;
        full_s.M = 2.0;
        full_s.T = base_t;
        full_s.t[0] = 0.0f;
        full_s.t[1] = 0.0f;
        full_s.n[0] = 0.0f;
        full_s.n[1] = 0.0f;
        full_s.Fx = 0.0f;
        full_s.Fy = 0.0f;

        SolutionBase base_s;
        base_s.u = 0.0f;
        base_s.w = 0.0f;
        base_s.M = 0.0f;
        base_s.T = 0.0f;
        base_s.t[0] = 0.0f;
        base_s.t[1] = 0.0f;
        base_s.n[0] = 0.0f;
        base_s.n[1] = 0.0f;

        SolutionCorr corr_s;
        corr_s.u = 0.0f;
        corr_s.w = 0.0f;
        corr_s.M = 0.0f;
        corr_s.T = 0.0f;
        corr_s.N = 0.0f;
        corr_s.Q = 0.0f;
        corr_s.Pt = 0.0f;
        corr_s.Pn = 0.0f;

        Element element_s;
        element_s.full = full_s;
        element_s.base = base_s;
        element_s.corr = corr_s;

        return element_s;
    }

#endif // EQLINK