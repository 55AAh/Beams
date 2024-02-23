#include "Solver.h"

#include <cmath>


C_SolutionFull C_EQLINK_setup_initial_border(C_UniformParams up) {
    // Beam's left end is hinged at a known angle
    C_float x = 0.0f, y = 0.0f;
    C_float M = 0.0f;
    C_float T = up.initial_angle;
    C_Basis tn{};
    tn.t[0] = cos(T); tn.t[1] = sin(T);
    tn.n[0] = -sin(T); tn.n[1] = cos(T);

    // Support reaction force is upward
    C_float each_stand_load = up.total_weight / 2.0f;
    C_float Fx = 0.0f;
    C_float Fy = each_stand_load;

    C_SolutionFull border{};
    border.x = x;
    border.y = y;
    border.M = M;
    border.T = T;
    border.tn = tn;
    border.Fx = Fx;
    border.Fy = Fy;

    return border;
}

C_SolutionBase C_EQLINK_setup_base(C_UniformParams up, C_SolutionFull full0) {
    // Base solution accounts for geometry
    C_float u = full0.x, w = full0.y;
    C_float T = full0.T;
    C_Basis tn = full0.tn;

    // Force induces a moment in the middle of the element
    // Since we don't know the curvature yet, we treat the element as straight
    C_float each_el_length = up.total_length / C_float(up.elements_count);
    C_float middle_s = each_el_length / 2.0f;
    C_float F_arm_x = full0.tn.t[0] * middle_s, F_arm_y = full0.tn.t[1] * middle_s;
    // Moment is <0 when beam goes to the right (because F then induces a counter-clockwise rotation)
    C_float M = F_arm_x * full0.Fy - F_arm_y * full0.Fx;

    C_SolutionBase base0{};
    base0.u = u;
    base0.w = w;
    base0.M = M;
    base0.T = T;
    base0.tn = tn;

    return base0;
}

C_SolutionCorr C_EQLINK_setup_corr(C_UniformParams up, C_SolutionFull full0, C_SolutionBase base0) {
    // No offset or rotation at the beginning
    C_float u = 0.0, w = 0.0;
    C_float T = 0.0;

    // Since full moment is zero (for hinge), the moment induced by F in base solution
    // should be compensated in correction solution
    C_float M = -base0.M;

    // Upward force is expressed in basis (at the middle)
    C_float each_el_length = up.total_length / C_float(up.elements_count);
    C_SolutionBase base_mid = C_EQLINK_link_base(up, full0, base0, each_el_length / 2.0f);
    C_float N = full0.Fy * base_mid.tn.t[1];
    C_float Q = full0.Fy * base_mid.tn.n[1];

    // Each element has weight
    C_float each_el_weight = up.total_weight / C_float(up.elements_count);
    C_float P = each_el_weight;

    // Its force is also expressed in basis (at the middle)
    C_float Pt = P * base_mid.tn.t[1];
    C_float Pn = P * base_mid.tn.n[1];

    C_SolutionCorr corr0{};
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

C_float C_calc_K(C_UniformParams up, C_float M) {
    // Moment induces curvature
    C_float K = M / up.EI;
    return K;
}

C_Basis C_rotate_basis(C_Basis tn0, C_float phi) {
    C_float rot_mat_s[2][2];
    rot_mat_s[0][0] = cos(phi); rot_mat_s[0][1] = sin(phi);
    rot_mat_s[1][0] = -sin(phi); rot_mat_s[1][1] = cos(phi);

    C_Basis tn_s{};
    tn_s.t[0] = rot_mat_s[0][0] * tn0.t[0] + rot_mat_s[0][1] * tn0.n[0];
    tn_s.t[1] = rot_mat_s[0][0] * tn0.t[1] + rot_mat_s[0][1] * tn0.n[1];
    tn_s.n[0] = rot_mat_s[1][0] * tn0.t[0] + rot_mat_s[1][1] * tn0.n[0];
    tn_s.n[1] = rot_mat_s[1][0] * tn0.t[1] + rot_mat_s[1][1] * tn0.n[1];

    return tn_s;
}

C_SolutionBase C_EQLINK_link_base(C_UniformParams up, C_SolutionFull full0, C_SolutionBase base0, C_float s) {
    // Moment is constant
    C_float M = base0.M;
    C_float K = C_calc_K(up, M);

    // This moment induces curvature
    C_float phi = s * K;

    // Coordinates are shifted
    C_float shift_mat_s[2];
    shift_mat_s[0] = 1.0f / K * sin(phi);
    shift_mat_s[1] = 1.0f / K * (1.0f - cos(phi));

    C_float du = shift_mat_s[0] * full0.tn.t[0] + shift_mat_s[1] * full0.tn.n[0];
    C_float dw = shift_mat_s[0] * full0.tn.t[1] + shift_mat_s[1] * full0.tn.n[1];
    C_float u_s = base0.u + du;
    C_float w_s = base0.w + dw;

    // Basis is rotated
    C_float T = base0.T;
    C_float T_s = T + phi;
    C_Basis tn_s = C_rotate_basis(full0.tn, phi);

    C_SolutionBase base_s{};
    base_s.u = u_s;
    base_s.w = w_s;
    base_s.M = M;
    base_s.T = T_s;
    base_s.tn = tn_s;

    return base_s;
}

C_SolutionCorr C_EQLINK_link_corr(C_UniformParams up, [[maybe_unused]] C_SolutionFull full0, C_SolutionBase base0, C_SolutionCorr corr0, C_float s) {
    C_float K = C_calc_K(up, base0.M);
    C_float R = 1.0f / K;
    C_float phi = s * K;
    C_float sin_phi = sin(phi), cos_phi = cos(phi);

    C_float u0 = corr0.u, w0 = corr0.w;
    C_float T0 = corr0.T;
    C_float M0 = corr0.M;
    C_float N0 = corr0.N, Q0 = corr0.Q;
    C_float Pt = corr0.Pt, Pn = corr0.Pn;
    C_float f = 0.0f;
    C_float EJ = up.EI;

    C_float fact[7] = { 1, 1, 2, 6, 24, 120, 720 };
    C_float pow_phi[6] = { 1.0f, phi, pow(phi, 2.0f), pow(phi, 3.0f), pow(phi, 4.0f), pow(phi, 5.0f) };
    C_float pow_s[5] = { 1.0f, s, pow(s, 2.0f), pow(s, 3.0f), pow(s, 4.0f) };

    C_float u_s = 0.0;
    u_s += u0 * cos_phi;
    u_s += w0 * sin_phi;
    u_s += T0 * s * (phi / 2.0 - pow_phi[3] / fact[4] + pow_phi[5] / fact[6]);
    u_s += Q0 * (pow_s[3] / EJ * (phi / fact[4] - 2.0 * pow_phi[3] / fact[6]) - f * s * (phi / 2.0 - pow_phi[3] / 2.0 / fact[3] + pow_phi[5] / 2.0 / fact[5]));
    u_s += N0 * (-pow_s[3] / EJ * (pow_phi[2] / fact[5]) - f * s * (1.0 - 2.0 * pow_phi[2] / 3.0 + 3.0 * pow_phi[4] / fact[5]));
    u_s += M0 * pow_s[2] / EJ * (phi / fact[3] - pow_phi[3] / fact[5]);
    u_s += Pn * (pow_s[4] / EJ * (phi / fact[5]) + f * pow_s[2] * (-phi / fact[3] + 2.0 * pow_phi[3] / fact[5]));
    u_s += Pt * (-pow_s[4] / EJ * (pow_phi[2] / fact[6]) - f * pow_s[2] * (1.0 / 2.0 - pow_phi[2] / 2.0 / fact[3] + pow_phi[4] / 2.0 / fact[5]));

    C_float w_s = 0.0;
    w_s += u0 * -sin_phi;
    w_s += w0 * cos_phi;
    w_s += T0 * R * sin_phi;
    w_s += Q0 * (pow_s[3] / EJ * (1.0 / fact[3] - 2.0 * pow_phi[2] / fact[5]) + f * s * (pow_phi[2] / fact[3] - 2.0 * pow_phi[4] / fact[5]));
    w_s += N0 * (-pow_s[3] / EJ * (phi / fact[4] - 2.0 * pow_phi[3] / fact[6]) + f * s * (phi / 2.0 - pow_phi[3] / 2.0 / fact[3] + pow_phi[5] / 2.0 / fact[5]));
    w_s += M0 * pow_s[2] / EJ * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
    w_s += Pn * (pow_s[4] / EJ * (1.0 / fact[4] - 2.0 * pow_phi[2] / fact[6]) + f * pow_s[2] * (pow_phi[2] / fact[4] - 2.0 * pow_phi[4] / fact[6]));
    w_s += Pt * (pow_s[4] / EJ * (-phi / fact[5]) + f * pow_s[2] * (phi / fact[3] - 2.0 * pow_phi[3] / fact[5]));

    C_float T_s = 0.0;
    T_s += T0;
    T_s += Q0 * pow_s[2] / EJ * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
    T_s += N0 * (-pow_s[2] / EJ * (phi / fact[3] - pow_phi[3] / fact[5]));
    T_s += M0 * s / EJ;
    T_s += Pn * pow_s[3] / EJ * (1.0 / fact[3] - pow_phi[2] / fact[5]);
    T_s += Pt * pow_s[3] / EJ * (-phi / fact[4] + pow_phi[3] / fact[6]);

    C_float Q_s = 0.0;
    Q_s += Q0 * cos_phi;
    Q_s += N0 * -sin_phi;
    Q_s += Pn * R * sin_phi;
    Q_s += Pt * s * (-phi / 2.0 + pow_phi[3] / fact[4] - pow_phi[5] / fact[6]);

    C_float N_s = 0.0;
    N_s += Q0 * sin_phi;
    N_s += N0 * cos_phi;
    N_s += Pn * s * (phi / 2.0 - pow_phi[3] / fact[4] + pow_phi[5] / fact[6]);
    N_s += Pt * R * sin_phi;

    C_float M_s = 0.0;
    M_s += Q0 * R * sin_phi;
    M_s += N0 * s * (-phi / 2.0 + pow_phi[3] / fact[4] - pow_phi[5] / fact[6]) * N0;
    M_s += M0;
    M_s += Pn * pow_s[2] * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
    M_s += Pt * pow_s[2] * (-phi / fact[3] + pow_phi[3] / fact[5]);

    C_SolutionCorr corr_s{};
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

C_SolutionFull C_EQLINK_link_full([[maybe_unused]] C_UniformParams up, C_SolutionFull full0, C_SolutionBase base0, C_SolutionBase base_s, C_SolutionCorr corr_s, [[maybe_unused]] C_float s) {
    C_float diff_u = corr_s.u * base_s.tn.t[0] + corr_s.w * base_s.tn.n[0];
    C_float diff_w = corr_s.u * base_s.tn.t[1] + corr_s.w * base_s.tn.n[1];
    C_float diff_M = corr_s.M;
    C_float diff_T = corr_s.T;

    C_float x_s = base_s.u + diff_u;
    C_float y_s = base_s.w + diff_w;
    C_float M_s = base_s.M + diff_M;
    C_float T_s = base_s.T + diff_T;

    C_float diff_T_bc = T_s - base0.T;
    C_Basis tn_s = C_rotate_basis(full0.tn, diff_T_bc);

    C_float Fx_s = corr_s.N * base_s.tn.t[0] + corr_s.Q * base_s.tn.n[0];
    C_float Fy_s = corr_s.N * base_s.tn.t[1] + corr_s.Q * base_s.tn.n[1];

    C_SolutionFull full_s{};
    full_s.x = x_s;
    full_s.y = y_s;
    full_s.M = M_s;
    full_s.T = T_s;
    full_s.tn = tn_s;
    full_s.Fx = Fx_s;
    full_s.Fy = Fy_s;

    return full_s;
}


C_Element border_element(C_SolutionFull border) {
    C_SolutionBase base_undef{};
    C_SolutionCorr corr_undef{};
    C_Element el0 { border, base_undef, corr_undef };
    return el0;
}


void C_Solver::setup(C_UniformParams new_up) {
    internal_re_alloc((size_t) new_up.elements_count);
    up = new_up;
    _was_setup = true;
}

void C_Solver::traverse(size_t begin, size_t end) const {
    C_float each_length = up.total_length / (float)up.elements_count;

    if (begin == 0) {
        C_SolutionFull border = C_EQLINK_setup_initial_border(up);
        elements[0] = border_element(border);
    }

    for (size_t element_i = begin; element_i < end; ++element_i) {
        C_SolutionFull full0 = elements[element_i].full;
        C_SolutionBase base0 = C_EQLINK_setup_base(up, full0);
        C_SolutionCorr corr0 = C_EQLINK_setup_corr(up, full0, base0);
        C_Element el0 {full0, base0, corr0 };

        elements[element_i] = el0;

        C_SolutionBase base1 = C_EQLINK_link_base(up, full0, base0, each_length);
        C_SolutionCorr corr1 = C_EQLINK_link_corr(up, full0, base0, corr0, each_length);
        C_SolutionFull full1 = C_EQLINK_link_full(up, full0, base0,base1, corr1, each_length);

        elements[element_i + 1] = border_element(full1);
    }
}

void C_Solver::forget() {
    internal_ensure_free();
    _was_setup = false;
}

void C_Solver::internal_re_alloc(size_t new_elements_count) {
    if (allocated && new_elements_count == elements_count) {
        return;
    }

    internal_ensure_free();

    elements = new C_Element[new_elements_count + 1];
    allocated = true;
    elements_count = new_elements_count;
}

void C_Solver::internal_ensure_free() {
    if (!allocated) {
        return;
    }

    delete[] elements;
    elements = nullptr;
    elements_count = NULL;

    allocated = false;
}