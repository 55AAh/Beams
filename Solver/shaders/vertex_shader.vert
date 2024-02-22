// % version 430 core
// The line above is commented to satisfy C preprocessor
// But it needs to be patched to #version before compiling GLSL





// Those definitions would be shared between C and GLSL code
#ifndef ONLY_IMPLEMENT_EQLINK

    struct Basis {
        float t[2];
        float n[2];
    };

    struct SolutionFull {
        float x, y;
        float M;
        float T;
        Basis tn;
        float Fx, Fy;
    };

    struct SolutionBase {
        float u, w;
        float M;
        float T;
        Basis tn;
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

    SolutionFull EQLINK_setup_initial_border(UniformParams up);
    SolutionBase EQLINK_setup_base(UniformParams up, SolutionFull full0);
    SolutionCorr EQLINK_setup_corr(UniformParams up, SolutionFull full0, SolutionBase base0);
    SolutionBase EQLINK_link_base(UniformParams up, SolutionFull full0, SolutionBase base0, float s);
    SolutionCorr EQLINK_link_corr(UniformParams up, SolutionFull full0, SolutionBase base0, SolutionCorr corr0, float s);
    SolutionFull EQLINK_link_full(UniformParams up, SolutionFull full0, SolutionBase base0, SolutionBase base_s, SolutionCorr corr_s, float s);





// This is the GLSL code that integrates EQLINK into shader pipeline
#ifndef INSIDE_C_CODE

    layout (location = 0) in vec2 aPos;
    layout(std430, binding = 0) restrict readonly buffer ElementsBuffer {
        Element elements[];
    };
    uniform float up_array[UP_ARRAY_SIZE];
    uniform float zoom;
    uniform vec2 look_at;

    out vec3 vertexColor;

    void main() {
        UNPACK_UP(up, up_array);

        float s = aPos.x * up.total_length / float(elements.length());
        int element_index = int(aPos.y);

        Element el_0 = elements[element_index];
        SolutionBase base_s = EQLINK_link_base(up, el_0.full, el_0.base, s);
        SolutionCorr corr_s = EQLINK_link_corr(up, el_0.full, el_0.base, el_0.corr, s);
        SolutionFull full_s = EQLINK_link_full(up, el_0.full, el_0.base, base_s, corr_s, s);

        gl_Position = vec4((full_s.x - look_at.x) * zoom, (full_s.y - look_at.y) * zoom, 0.0, 1.0);
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

    SolutionFull EQLINK_setup_initial_border(UniformParams up) {
        // Beam's left end is hinged at a known angle
        float x = 0.0f, y = 0.0f;
        float M = 0.0f;
        float T = up.initial_angle;
        Basis tn;
        tn.t[0] = cos(T); tn.t[1] = sin(T);
        tn.n[0] = -sin(T); tn.n[1] = cos(T);

        // Support reaction force is upward
        float each_stand_load = up.total_weight / 2.0f;
        float Fx = 0.0f;
        float Fy = each_stand_load;

        SolutionFull border;
        border.x = x;
        border.y = y;
        border.M = M;
        border.T = T;
        border.tn = tn;
        border.Fx = Fx;
        border.Fy = Fy;

        return border;
    }

    SolutionBase EQLINK_setup_base(UniformParams up, SolutionFull full0) {
        // Base solution accounts for geometry
        float u = full0.x, w = full0.y;
        float T = full0.T;
        float t[2], n[2];
        Basis tn = full0.tn;

        // Force induces a moment in the middle of the element
        // Since we don't know the curvature yet, we treat the element as straight
        float each_el_length = up.total_length / float(up.elements_count);
        float middle_s = each_el_length / 2.0f;
        float F_arm_x = full0.tn.t[0] * middle_s, F_arm_y = full0.tn.t[1] * middle_s;
        // Moment is <0 when beam goes to the right (because F then induces a counter-clockwise rotation)
        float M = F_arm_x * full0.Fy - F_arm_y * full0.Fx;

        SolutionBase base0;
        base0.u = u;
        base0.w = w;
        base0.M = M;
        base0.T = T;
        base0.tn = tn;

        return base0;
    }

    SolutionCorr EQLINK_setup_corr(UniformParams up, SolutionFull full0, SolutionBase base0) {
        // No offset or rotation at the beginning
        float u = 0.0, w = 0.0;
        float T = 0.0;

        // Since full moment is zero (for hinge), the moment induced by F in base solution
        // should be compensated in correction solution
        float M = -base0.M;

        // Upward force is expressed in basis (at the middle)
        float each_el_length = up.total_length / float(up.elements_count);
        SolutionBase base_mid = EQLINK_link_base(up, full0, base0, each_el_length / 2.0f);
        float N = full0.Fy * base_mid.tn.t[1];
        float Q = full0.Fy * base_mid.tn.n[1];

        // Each element has weight
        float each_el_weight = up.total_weight / float(up.elements_count);
        float P = each_el_weight;

        // Its force is also expressed in basis (at the middle)
        float Pt = P * base_mid.tn.t[1];
        float Pn = P * base_mid.tn.n[1];

        SolutionCorr corr0;
        corr0.u = u;
        corr0.w = w;
        corr0.M = M;
        corr0.T = T;
        corr0.N = N;
        corr0.Q = Q;
        corr0.Pt = Pt;
        corr0.Pn = Pn;

        return corr0;
    }

    float calc_K(UniformParams up, float M) {
        // Moment induces curvature
        float K = M / up.EI;
        return K;
    }

    Basis rotate_basis(Basis tn0, float phi) {
        float rot_mat_s[2][2];
        rot_mat_s[0][0] = cos(phi); rot_mat_s[0][1] = sin(phi);
        rot_mat_s[1][0] = -sin(phi); rot_mat_s[1][1] = cos(phi);

        Basis tn_s;
        tn_s.t[0] = rot_mat_s[0][0] * tn0.t[0] + rot_mat_s[0][1] * tn0.n[0];
        tn_s.t[1] = rot_mat_s[0][0] * tn0.t[1] + rot_mat_s[0][1] * tn0.n[1];
        tn_s.n[0] = rot_mat_s[1][0] * tn0.t[0] + rot_mat_s[1][1] * tn0.n[0];
        tn_s.n[1] = rot_mat_s[1][0] * tn0.t[1] + rot_mat_s[1][1] * tn0.n[1];

        return tn_s;
    }

    SolutionBase EQLINK_link_base(UniformParams up, SolutionFull full0, SolutionBase base0, float s) {
        // Moment is constant
        float M = base0.M;
        float K = calc_K(up, M);

        // This moment induces curvature
        float phi = s * K;

        // Coordinates are shifted
        float shift_mat_s[2];
        shift_mat_s[0] = 1.0f / K * sin(phi);
        shift_mat_s[1] = 1.0f / K * (1.0f - cos(phi));

        float du = shift_mat_s[0] * full0.tn.t[0] + shift_mat_s[1] * full0.tn.n[0];
        float dw = shift_mat_s[0] * full0.tn.t[1] + shift_mat_s[1] * full0.tn.n[1];
        float u_s = base0.u + du;
        float w_s = base0.w + dw;

        // Basis is rotated
        float T = base0.T;
        float T_s = T + phi;
        Basis tn_s = rotate_basis(full0.tn, phi);

        SolutionBase base_s;
        base_s.u = u_s;
        base_s.w = w_s;
        base_s.M = M;
        base_s.T = T_s;
        base_s.tn = tn_s;

        return base_s;
    }

    SolutionCorr EQLINK_link_corr(UniformParams up, SolutionFull full0, SolutionBase base0, SolutionCorr corr0, float s) {
        float K = calc_K(up, base0.M);
        float R = 1.0f / K;
        float phi = s * K;
        float sin_phi = sin(phi), cos_phi = cos(phi);

        float u0 = corr0.u, w0 = corr0.w;
        float T0 = corr0.T;
        float M0 = corr0.M;
        float N0 = corr0.N, Q0 = corr0.Q;
        float Pt = corr0.Pt, Pn = corr0.Pn;
        float f = 0.0f;
        float EJ = up.EI;

        float fact[7] = { 1, 1, 2, 6, 24, 120, 720 };
        float pow_phi[6] = { 1.0f, phi, pow(phi, 2.0f), pow(phi, 3.0f), pow(phi, 4.0f), pow(phi, 5.0f) };
        float pow_s[5] = { 1.0f, s, pow(s, 2.0f), pow(s, 3.0f), pow(s, 4.0f) };

        float u_s = 0.0;
        u_s += u0 * cos_phi;
        u_s += w0 * sin_phi;
        u_s += T0 * s * (phi / 2.0 - pow_phi[3] / fact[4] + pow_phi[5] / fact[6]);
        u_s += Q0 * (pow_s[3] / EJ * (phi / fact[4] - 2.0 * pow_phi[3] / fact[6]) - f * s * (phi / 2.0 - pow_phi[3] / 2.0 / fact[3] + pow_phi[5] / 2.0 / fact[5]));
        u_s += N0 * (-pow_s[3] / EJ * (pow_phi[2] / fact[5]) - f * s * (1.0 - 2.0 * pow_phi[2] / 3.0 + 3.0 * pow_phi[4] / fact[5]));
        u_s += M0 * pow_s[2] / EJ * (phi / fact[3] - pow_phi[3] / fact[5]);
        u_s += Pn * (pow_s[4] / EJ * (phi / fact[5]) + f * pow_s[2] * (-phi / fact[3] + 2.0 * pow_phi[3] / fact[5]));
        u_s += Pt * (-pow_s[4] / EJ * (pow_phi[2] / fact[6]) - f * pow_s[2] * (1.0 / 2.0 - pow_phi[2] / 2.0 / fact[3] + pow_phi[4] / 2.0 / fact[5]));

        float w_s = 0.0;
        w_s += u0 * -sin_phi;
        w_s += w0 * cos_phi;
        w_s += T0 * R * sin_phi;
        w_s += Q0 * (pow_s[3] / EJ * (1.0 / fact[3] - 2.0 * pow_phi[2] / fact[5]) + f * s * (pow_phi[2] / fact[3] - 2.0 * pow_phi[4] / fact[5]));
        w_s += N0 * (-pow_s[3] / EJ * (phi / fact[4] - 2.0 * pow_phi[3] / fact[6]) + f * s * (phi / 2.0 - pow_phi[3] / 2.0 / fact[3] + pow_phi[5] / 2.0 / fact[5]));
        w_s += M0 * pow_s[2] / EJ * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
        w_s += Pn * (pow_s[4] / EJ * (1.0 / fact[4] - 2.0 * pow_phi[2] / fact[6]) + f * pow_s[2] * (pow_phi[2] / fact[4] - 2.0 * pow_phi[4] / fact[6]));
        w_s += Pt * (pow_s[4] / EJ * (-phi / fact[5]) + f * pow_s[2] * (phi / fact[3] - 2.0 * pow_phi[3] / fact[5]));

        float T_s = 0.0;
        T_s += T0;
        T_s += Q0 * pow_s[2] / EJ * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
        T_s += N0 * (-pow_s[2] / EJ * (phi / fact[3] - pow_phi[3] / fact[5]));
        T_s += M0 * s / EJ;
        T_s += Pn * pow_s[3] / EJ * (1.0 / fact[3] - pow_phi[2] / fact[5]);
        T_s += Pt * pow_s[3] / EJ * (-phi / fact[4] + pow_phi[3] / fact[6]);

        float Q_s = 0.0;
        Q_s += Q0 * cos_phi;
        Q_s += N0 * -sin_phi;
        Q_s += Pn * R * sin_phi;
        Q_s += Pt * s * (-phi / 2.0 + pow_phi[3] / fact[4] - pow_phi[5] / fact[6]);

        float N_s = 0.0;
        N_s += Q0 * sin_phi;
        N_s += N0 * cos_phi;
        N_s += Pn * s * (phi / 2.0 - pow_phi[3] / fact[4] + pow_phi[5] / fact[6]);
        N_s += Pt * R * sin_phi;

        float M_s = 0.0;
        M_s += Q0 * R * sin_phi;
        M_s += N0 * s * (-phi / 2.0 + pow_phi[3] / fact[4] - pow_phi[5] / fact[6]) * N0;
        M_s += M0;
        M_s += Pn * pow_s[2] * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
        M_s += Pt * pow_s[2] * (-phi / fact[3] + pow_phi[3] / fact[5]);

        SolutionCorr corr_s;
        corr_s.u = u_s;
        corr_s.w = w_s;
        corr_s.M = M_s;
        corr_s.T = T_s;
        corr_s.N = N_s;
        corr_s.Q = Q_s;
        corr_s.Pt = Pt;
        corr_s.Pn = Pn;

        return corr_s;
    }

    SolutionFull EQLINK_link_full(UniformParams up, SolutionFull full0, SolutionBase base0, SolutionBase base_s, SolutionCorr corr_s, float s) {
        float diff_u = corr_s.u * base_s.tn.t[0] + corr_s.w * base_s.tn.n[0];
        float diff_w = corr_s.u * base_s.tn.t[1] + corr_s.w * base_s.tn.n[1];
        float diff_M = corr_s.M;
        float diff_T = corr_s.T;

        float x_s = base_s.u + diff_u;
        float y_s = base_s.w + diff_w;
        float M_s = base_s.M + diff_M;
        float T_s = base_s.T + diff_T;

        float diff_T_bc = T_s - base0.T;
        Basis tn_s = rotate_basis(full0.tn, diff_T_bc);

        float Fx_s = corr_s.N * base_s.tn.t[0] + corr_s.Q * base_s.tn.n[0];
        float Fy_s = corr_s.N * base_s.tn.t[1] + corr_s.Q * base_s.tn.n[1];

        SolutionFull full_s;
        full_s.x = x_s;
        full_s.y = y_s;
        full_s.M = M_s;
        full_s.T = T_s;
        full_s.tn = tn_s;
        full_s.Fx = Fx_s;
        full_s.Fy = Fy_s;

        return full_s;
    }

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

        SolutionFull full_s;
        full_s.x = base_u;
        full_s.y = base_w;
        full_s.M = 2.0;
        full_s.T = base_t;
        full_s.tn.t[0] = 0.0f;
        full_s.tn.t[1] = 0.0f;
        full_s.tn.n[0] = 0.0f;
        full_s.tn.n[1] = 0.0f;
        full_s.Fx = 0.0f;
        full_s.Fy = 0.0f;

        SolutionBase base_s;
        base_s.u = 0.0f;
        base_s.w = 0.0f;
        base_s.M = 0.0f;
        base_s.T = 0.0f;
        base_s.tn.t[0] = 0.0f;
        base_s.tn.t[1] = 0.0f;
        base_s.tn.n[0] = 0.0f;
        base_s.tn.n[1] = 0.0f;

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