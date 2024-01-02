#ifndef SHADERBEAMS_FORMULAE_H
#define SHADERBEAMS_FORMULAE_H

// INTENDED TO BE INCLUDED ONCE IN CXX & ONCE IN SHADERS

#ifndef IN_SHADER
#include <cmath>
#else
#define FORMULAE_IMPLEMENTED
#endif


#define unpack_params(ep, up)   float u0 = ep.uwt.u, w0 = ep.uwt.w, t0 = ep.uwt.t, \
                                m0 = ep.m, n0 = ep.n, q0 = ep.q, \
                                EI = up.EI, GJ = up.GJ

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


#endif
