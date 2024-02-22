// % version 430 core
// The line above is commented to satisfy C preprocessor
// But it needs to be patched to #version before compiling GLSL





// Those definitions would be shared between C and GLSL code
#ifndef ONLY_IMPLEMENT_EQLINK

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

    ElementUWT calc_uwt(ElementParams ep, UniformParams up, float s);





// This is the GLSL code that integrates EQLINK into shader pipeline
#ifndef INSIDE_C_CODE

    ElementUWT construct_uwt(float u, float w, float t) {
        return ElementUWT(u, w, t);
    }

    layout (location = 0) in vec2 aPos;
    layout(std430, binding = 0) restrict readonly buffer ElementsBuffer {
        ElementParams elements[];
    };
    uniform float up_array[UP_ARRAY_SIZE];
    uniform float zoom;

    out vec3 vertexColor;

    void main() {
        UNPACK_UP(up, up_array);

        float s = aPos.x * up.total_length / float(elements.length());
        int element = int(aPos.y);

        ElementUWT uwt = calc_uwt(elements[element], up, s);

        gl_Position = vec4(uwt.u, uwt.w, 0.0, 1.0);
    //    gl_Position = vec4(elements[element].uwt.u + s / 3, elements[element].uwt.w + s*s / 3, 0.0, 1.0);
        int _color = element % 3;
        vertexColor = vec3(_color == 0, _color == 1, _color == 2);
    }

#endif // INSIDE_C_CODE
#endif // ONLY_IMPLEMENT_EQLINK





// This is needed so that EQLINK is not implemented twice in different C units
#if !defined(INSIDE_C_CODE) || defined(ONLY_IMPLEMENT_EQLINK)

    #ifdef INSIDE_C_CODE
        #include <cmath> // Trig functions are available in GLSL by default
    #endif // INSIDE_C_CODE

    #define unpack_params(ep, up)   float u0 = ep.uwt.u, w0 = ep.uwt.w, t0 = ep.uwt.t, \
                                    m0 = ep.m, n0 = ep.n, q0 = ep.q, \
                                    EI = up.EI

    ElementUWT calc_uwt(ElementParams ep, UniformParams up, float s) {
        unpack_params(ep, up);

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

        return construct_uwt(base_u, base_w, base_t);
    }

#endif // EQLINK