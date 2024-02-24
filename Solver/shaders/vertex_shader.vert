#version 430 core


#define GLSL_float float


struct GLSL_Basis {
    GLSL_float t[2];
    GLSL_float n[2];
};

struct GLSL_SolutionFull {
    GLSL_float x, y;
    GLSL_float M;
    GLSL_float T;
    GLSL_Basis tn;
    GLSL_float Fx, Fy;
};

struct GLSL_SolutionBase {
    GLSL_float u, w;
    GLSL_float M;
    GLSL_float T;
    GLSL_Basis tn;
};

struct GLSL_SolutionCorr {
    GLSL_float u, w;
    GLSL_float M;
    GLSL_float T;
    GLSL_float N, Q;
    GLSL_float Pt, Pn;
};

struct GLSL_Element {
    GLSL_SolutionFull full;
    GLSL_SolutionBase base;
    GLSL_SolutionCorr corr;
};

struct GLSL_UniformParams {
    int corr_selector;
    GLSL_float EI;
    GLSL_float initial_angle;
    GLSL_float total_weight;
    GLSL_float total_length;
    GLSL_float gap;
    int elements_count;
};

#define UP_ARRAY_SIZE 7
#define GLSL_UNPACK_UP(up, arr) GLSL_UniformParams up = GLSL_UniformParams(int(arr[0]), arr[1], arr[2], arr[3], arr[4], arr[5], int(arr[6]));

GLSL_SolutionFull GLSL_EQLINK_setup_initial_border(GLSL_UniformParams up);
GLSL_SolutionBase GLSL_EQLINK_setup_base(GLSL_UniformParams up, GLSL_SolutionFull full0);
GLSL_SolutionCorr GLSL_EQLINK_setup_corr(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0);
GLSL_SolutionBase GLSL_EQLINK_link_base(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_float s);
GLSL_SolutionCorr GLSL_EQLINK_link_corr(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_SolutionCorr corr0, GLSL_float s);
GLSL_SolutionCorr GLSL_EQLINK_link_corr_linear(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_SolutionCorr corr0, GLSL_float s);
GLSL_SolutionCorr GLSL_EQLINK_link_corr_exponential(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_SolutionCorr corr0, GLSL_float s);
GLSL_SolutionFull GLSL_EQLINK_link_full(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_SolutionBase base_s, GLSL_SolutionCorr corr_s, GLSL_float s);

const GLSL_float PI = 3.14159265358979f;

GLSL_SolutionFull GLSL_EQLINK_setup_initial_border(GLSL_UniformParams up) {
    // Beam's left end is hinged at a known angle
    GLSL_float x = 0.0, y = 0.0;
    GLSL_float M = 0.0;
    GLSL_float T = up.initial_angle;
    GLSL_Basis tn;
    tn.t[0] = cos(T); tn.t[1] = sin(T);
    tn.n[0] = -sin(T); tn.n[1] = cos(T);

    // Support reaction force is upward
    GLSL_float each_stand_load = up.total_weight / 2.0;
    GLSL_float Fx = 0.0;
    GLSL_float Fy = each_stand_load;

    GLSL_SolutionFull border;
    border.x = x;
    border.y = y;
    border.M = M;
    border.T = T;
    border.tn = tn;
    border.Fx = Fx;
    border.Fy = Fy;

    return border;
}

GLSL_SolutionBase GLSL_EQLINK_setup_base(GLSL_UniformParams up, GLSL_SolutionFull full0) {
    // Base solution accounts for geometry
    GLSL_float u = full0.x, w = full0.y;
    GLSL_float T = full0.T;
    GLSL_Basis tn = full0.tn;

    // Force induces a moment in the middle of the element
    // Since we don't know the curvature yet, we treat the element as straight
    GLSL_float each_el_length = up.total_length / GLSL_float(up.elements_count);
    GLSL_float middle_s = each_el_length / 2.0f;
    GLSL_float F_arm_x = full0.tn.t[0] * middle_s, F_arm_y = full0.tn.t[1] * middle_s;
    // Moment is <0 when beam goes to the right (because F then induces a counter-clockwise rotation)
    GLSL_float M = F_arm_x * full0.Fy - F_arm_y * full0.Fx;

    GLSL_SolutionBase base0;
    base0.u = u;
    base0.w = w;
    base0.M = M;
    base0.T = T;
    base0.tn = tn;

    return base0;
}

GLSL_SolutionCorr GLSL_EQLINK_setup_corr(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0) {
    // No offset or rotation at the beginning
    GLSL_float u = 0.0, w = 0.0;
    GLSL_float T = 0.0;

    // Since full moment is zero (for hinge), the moment induced by F in base solution
    // should be compensated in correction solution
    GLSL_float M = -base0.M;

    // Upward force is expressed in basis (at the middle)
    GLSL_float each_el_length = up.total_length / GLSL_float(up.elements_count);
    GLSL_SolutionBase base_mid = GLSL_EQLINK_link_base(up, full0, base0, each_el_length / 2.0f);
    GLSL_float N = full0.Fy * base_mid.tn.t[1];
    GLSL_float Q = full0.Fy * base_mid.tn.n[1];

    // Each element has weight
    GLSL_float each_el_weight = up.total_weight / GLSL_float(up.elements_count);
    GLSL_float P = each_el_weight;

    // Its force is also expressed in basis (at the middle)
    GLSL_float Pt = P * base_mid.tn.t[1];
    GLSL_float Pn = P * base_mid.tn.n[1];

    GLSL_SolutionCorr corr0;
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

GLSL_float GLSL_calc_K(GLSL_UniformParams up, GLSL_float M) {
    // Moment induces curvature
    GLSL_float K = M / up.EI;
    return K;
}

GLSL_Basis GLSL_rotate_basis(GLSL_Basis tn0, GLSL_float phi) {
    GLSL_float rot_mat_s[2][2];
    rot_mat_s[0][0] = cos(phi); rot_mat_s[0][1] = sin(phi);
    rot_mat_s[1][0] = -sin(phi); rot_mat_s[1][1] = cos(phi);

    GLSL_Basis tn_s;
    tn_s.t[0] = rot_mat_s[0][0] * tn0.t[0] + rot_mat_s[0][1] * tn0.n[0];
    tn_s.t[1] = rot_mat_s[0][0] * tn0.t[1] + rot_mat_s[0][1] * tn0.n[1];
    tn_s.n[0] = rot_mat_s[1][0] * tn0.t[0] + rot_mat_s[1][1] * tn0.n[0];
    tn_s.n[1] = rot_mat_s[1][0] * tn0.t[1] + rot_mat_s[1][1] * tn0.n[1];

    return tn_s;
}

GLSL_SolutionBase GLSL_EQLINK_link_base(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_float s) {
    // Moment is constant
    GLSL_float M = base0.M;
    GLSL_float K = GLSL_calc_K(up, M);

    // This moment induces curvature
    GLSL_float phi = s * K;

    // Coordinates are shifted
    GLSL_float shift_mat_s[2];
    shift_mat_s[0] = 1.0f / K * sin(phi);
    shift_mat_s[1] = 1.0f / K * (1.0f - cos(phi));

    GLSL_float du = shift_mat_s[0] * full0.tn.t[0] + shift_mat_s[1] * full0.tn.n[0];
    GLSL_float dw = shift_mat_s[0] * full0.tn.t[1] + shift_mat_s[1] * full0.tn.n[1];
    GLSL_float u_s = base0.u + du;
    GLSL_float w_s = base0.w + dw;

    // Basis is rotated
    GLSL_float T = base0.T;
    GLSL_float T_s = T + phi;
    GLSL_Basis tn_s = GLSL_rotate_basis(full0.tn, phi);

    GLSL_SolutionBase base_s;
    base_s.u = u_s;
    base_s.w = w_s;
    base_s.M = M;
    base_s.T = T_s;
    base_s.tn = tn_s;

    return base_s;
}

GLSL_SolutionCorr GLSL_EQLINK_link_corr(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_SolutionCorr corr0, GLSL_float s) {
    if (up.corr_selector == 0)
        return GLSL_EQLINK_link_corr_linear(up, full0, base0, corr0, s);
    else
        return GLSL_EQLINK_link_corr_exponential(up, full0, base0, corr0, s);
}

GLSL_SolutionCorr GLSL_EQLINK_link_corr_linear(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_SolutionCorr corr0, GLSL_float s) {
    GLSL_float K = GLSL_calc_K(up, base0.M);
    GLSL_float R = 1.0 / K;
    GLSL_float phi = s * K;
    GLSL_float sin_phi = sin(phi), cos_phi = cos(phi);

    GLSL_float u0 = corr0.u, w0 = corr0.w;
    GLSL_float T0 = corr0.T;
    GLSL_float M0 = corr0.M;
    GLSL_float N0 = corr0.N, Q0 = corr0.Q;
    GLSL_float Pt = corr0.Pt, Pn = corr0.Pn;
    GLSL_float f = 0.0;
    GLSL_float EJ = up.EI;

    GLSL_float fact[7] = { 1, 1, 2, 6, 24, 120, 720 };
    GLSL_float pow_phi[6] = { 1.0, phi, pow(phi, 2.0), pow(phi, 3.0), pow(phi, 4.0), pow(phi, 5.0) };
    GLSL_float pow_s[5] = { 1.0, s, pow(s, 2.0), pow(s, 3.0), pow(s, 4.0) };

    GLSL_float u_s = 0.0;
    u_s += u0 * cos_phi;
    u_s += w0 * sin_phi;
    u_s += T0 * s * (phi / 2.0 - pow_phi[3] / fact[4] + pow_phi[5] / fact[6]);
    u_s += Q0 * (pow_s[3] / EJ * (phi / fact[4] - 2.0 * pow_phi[3] / fact[6]) - f * s * (phi / 2.0 - pow_phi[3] / 2.0 / fact[3] + pow_phi[5] / 2.0 / fact[5]));
    u_s += N0 * (-pow_s[3] / EJ * (pow_phi[2] / fact[5]) - f * s * (1.0 - 2.0 * pow_phi[2] / 3.0 + 3.0 * pow_phi[4] / fact[5]));
    u_s += M0 * pow_s[2] / EJ * (phi / fact[3] - pow_phi[3] / fact[5]);
    u_s += Pn * (pow_s[4] / EJ * (phi / fact[5]) + f * pow_s[2] * (-phi / fact[3] + 2.0 * pow_phi[3] / fact[5]));
    u_s += Pt * (-pow_s[4] / EJ * (pow_phi[2] / fact[6]) - f * pow_s[2] * (1.0 / 2.0 - pow_phi[2] / 2.0 / fact[3] + pow_phi[4] / 2.0 / fact[5]));

    GLSL_float w_s = 0.0;
    w_s += u0 * -sin_phi;
    w_s += w0 * cos_phi;
    w_s += T0 * R * sin_phi;
    w_s += Q0 * (pow_s[3] / EJ * (1.0 / fact[3] - 2.0 * pow_phi[2] / fact[5]) + f * s * (pow_phi[2] / fact[3] - 2.0 * pow_phi[4] / fact[5]));
    w_s += N0 * (-pow_s[3] / EJ * (phi / fact[4] - 2.0 * pow_phi[3] / fact[6]) + f * s * (phi / 2.0 - pow_phi[3] / 2.0 / fact[3] + pow_phi[5] / 2.0 / fact[5]));
    w_s += M0 * pow_s[2] / EJ * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
    w_s += Pn * (pow_s[4] / EJ * (1.0 / fact[4] - 2.0 * pow_phi[2] / fact[6]) + f * pow_s[2] * (pow_phi[2] / fact[4] - 2.0 * pow_phi[4] / fact[6]));
    w_s += Pt * (pow_s[4] / EJ * (-phi / fact[5]) + f * pow_s[2] * (phi / fact[3] - 2.0 * pow_phi[3] / fact[5]));

    GLSL_float T_s = 0.0;
    T_s += T0;
    T_s += Q0 * pow_s[2] / EJ * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
    T_s += N0 * (-pow_s[2] / EJ * (phi / fact[3] - pow_phi[3] / fact[5]));
    T_s += M0 * s / EJ;
    T_s += Pn * pow_s[3] / EJ * (1.0 / fact[3] - pow_phi[2] / fact[5]);
    T_s += Pt * pow_s[3] / EJ * (-phi / fact[4] + pow_phi[3] / fact[6]);

    GLSL_float Q_s = 0.0;
    Q_s += Q0 * cos_phi;
    Q_s += N0 * -sin_phi;
    Q_s += Pn * R * sin_phi;
    Q_s += Pt * s * (-phi / 2.0 + pow_phi[3] / fact[4] - pow_phi[5] / fact[6]);

    GLSL_float N_s = 0.0;
    N_s += Q0 * sin_phi;
    N_s += N0 * cos_phi;
    N_s += Pn * s * (phi / 2.0 - pow_phi[3] / fact[4] + pow_phi[5] / fact[6]);
    N_s += Pt * R * sin_phi;

    GLSL_float M_s = 0.0;
    M_s += Q0 * R * sin_phi;
    M_s += N0 * s * (-phi / 2.0 + pow_phi[3] / fact[4] - pow_phi[5] / fact[6]) * N0;
    M_s += M0;
    M_s += Pn * pow_s[2] * (1.0 / 2.0 - pow_phi[2] / fact[4] + pow_phi[4] / fact[6]);
    M_s += Pt * pow_s[2] * (-phi / fact[3] + pow_phi[3] / fact[5]);

    GLSL_SolutionCorr corr_s;
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

GLSL_SolutionCorr GLSL_EQLINK_link_corr_exponential(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_SolutionCorr corr0, GLSL_float s) {
    GLSL_float K = GLSL_calc_K(up, base0.M);
    GLSL_float R = 1.0 / K;
    GLSL_float phi = s * K;
    GLSL_float sin_phi = sin(phi), cos_phi = cos(phi);

    GLSL_float u0 = corr0.u, w0 = corr0.w;
    GLSL_float T0 = corr0.T;
    GLSL_float M0 = corr0.M;
    GLSL_float N0 = corr0.N, Q0 = corr0.Q;
    GLSL_float Pt = corr0.Pt, Pn = corr0.Pn;
    GLSL_float f = 0.0;
    GLSL_float mu = 1.0;
    GLSL_float sh_mu_phi = sinh(mu*phi), ch_mu_phi = cosh(mu*phi);
    GLSL_float mu_sp1 = pow(mu, 2) + 1;
    GLSL_float EJ = up.EI;

    GLSL_float pow_phi[6] = { 1.0, phi, pow(phi, 2.0), pow(phi, 3.0), pow(phi, 4.0), pow(phi, 5.0) };
    GLSL_float pow_R[4] = { 1.0, R, pow(R, 2.0), pow(R, 3.0) };
    GLSL_float pow_mu[5] = { 1.0, mu, pow(mu, 2.0), pow(mu, 3.0), pow(mu, 4.0) };

    GLSL_float u_s = 0.0;
    u_s += u0 * cos_phi;
    u_s += w0 * sin_phi;
    u_s += T0 * R * (1 - cos_phi);
    u_s += Q0 * (pow_R[3]/(EJ*pow_mu[2])*((ch_mu_phi-cos_phi)/mu_sp1-(1-cos_phi))-f*R*((ch_mu_phi-cos_phi)/mu_sp1));
    u_s += N0 * -(pow_R[3]/(EJ*pow_mu[3])*((sh_mu_phi-mu*sin_phi)/mu_sp1-mu*(phi-sin_phi))+f*R/pow_mu[2]*((pow_mu[4]+2*pow_mu[2])*sin_phi/mu_sp1-mu*sh_mu_phi/mu_sp1));
    u_s += M0 * (pow_R[2]/EJ*(sh_mu_phi/pow_mu[3]-phi/pow_mu[2])-f*(1/mu)*(sh_mu_phi-mu*sin_phi));
    u_s += Pn * R*(pow_R[3]/(EJ*pow_mu[3])*((sh_mu_phi-mu*sin_phi)/mu_sp1-mu*(phi-sin_phi))-f*R/mu*(sh_mu_phi/mu_sp1-mu*sin_phi/mu_sp1));
    u_s += Pt * R*(pow_R[3]/(EJ*pow_mu[4])*(ch_mu_phi/mu_sp1-cos_phi*pow_mu[4]/mu_sp1-pow_mu[2]*pow_phi[2]/2+pow_mu[2]-1)-f*R/pow_mu[2]*((cos_phi-ch_mu_phi)/mu_sp1+mu_sp1*(1-cos_phi)));

    GLSL_float w_s = 0.0;
    w_s += u0 * -sin_phi;
    w_s += w0 * cos_phi;
    w_s += T0 * R * sin_phi;
    w_s += Q0 * (pow_R[3]/(EJ*pow_mu[2])*(mu*sh_mu_phi/mu_sp1-sin_phi*pow_mu[2]/mu_sp1)+f*R/mu*((sh_mu_phi-mu*sin_phi)/mu_sp1));
    w_s += N0 * (pow_R[3]/(EJ*pow_mu[2])*((ch_mu_phi-cos_phi)/mu_sp1-(1-cos_phi))-f*R/pow_mu[2]*((1-cos_phi)*mu_sp1-(ch_mu_phi-cos_phi)/mu_sp1));
    w_s += M0 * (pow_R[2]/EJ*(ch_mu_phi-1)/pow_mu[2]+f*mu_sp1/pow_mu[2]*((ch_mu_phi-cos_phi)/mu_sp1-(1-cos_phi)));
    w_s += Pn * R*(pow_R[3]/(EJ*pow_mu[2])*((ch_mu_phi-cos_phi)/mu_sp1-(1-cos_phi))+f*R/pow_mu[2]*((ch_mu_phi-cos_phi)/mu_sp1-(1-cos_phi)));
    w_s += Pt * R*(pow_R[3]/(EJ*pow_mu[4])*(mu*sh_mu_phi/mu_sp1+pow_mu[4]*sin_phi/mu_sp1-pow_mu[2]*phi)-f*R/pow_mu[3]*(mu_sp1*mu*(phi-sin_phi)-(sh_mu_phi-mu*sin_phi)/mu_sp1));

    GLSL_float T_s = 0.0;
    T_s += T0;
    T_s += Q0 * pow_R[2]/(EJ*pow_mu[2])*(ch_mu_phi-1);
    T_s += N0 * -pow_R[2]/(EJ*pow_mu[3])*(sh_mu_phi-mu*phi);
    T_s += M0 * R/EJ*(phi+mu_sp1/pow_mu[3]*(sh_mu_phi-mu*phi));
    T_s += Pn * pow_R[3]/(EJ*pow_mu[3])*(sh_mu_phi-mu*phi);
    T_s += Pt * -pow_R[3]/(EJ*pow_mu[4])*(ch_mu_phi-pow_mu[2]*pow_phi[2]/2-1);

    GLSL_float Q_s = 0.0;
    Q_s += Q0 * ch_mu_phi;
    Q_s += N0 * -1/mu*sh_mu_phi;
    Q_s += M0 * mu_sp1/(R*mu)*sh_mu_phi;
    Q_s += Pn * R * sh_mu_phi/mu;
    Q_s += Pt * R/pow_mu[2]*(-ch_mu_phi+1);

    GLSL_float N_s = 0.0;
    N_s += Q0 * sh_mu_phi/mu;
    N_s += N0 * (1-(ch_mu_phi-1)/pow_mu[2]);
    N_s += M0 * mu_sp1/(pow_mu[2]*R)*(ch_mu_phi-1);
    N_s += Pn * R*(ch_mu_phi-1)/pow_mu[2];
    N_s += Pt * -R*(sh_mu_phi/pow_mu[3]-mu_sp1/pow_mu[2]*phi);

    GLSL_float M_s = 0.0;
    M_s += Q0 * R/mu*sh_mu_phi;
    M_s += N0 * R/pow_mu[2]*(-ch_mu_phi+1);
    M_s += M0 * (ch_mu_phi+1/pow_mu[2]*(ch_mu_phi-1));
    M_s += Pn * R*R/pow_mu[2]*(ch_mu_phi-1);
    M_s += Pt * R*(-R/pow_mu[3]*sh_mu_phi+R*phi/pow_mu[2]);

    GLSL_SolutionCorr corr_s;
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

GLSL_SolutionFull GLSL_EQLINK_link_full(GLSL_UniformParams up, GLSL_SolutionFull full0, GLSL_SolutionBase base0, GLSL_SolutionBase base_s, GLSL_SolutionCorr corr_s, GLSL_float s) {
    GLSL_float diff_u = corr_s.u * base_s.tn.t[0] + corr_s.w * base_s.tn.n[0];
    GLSL_float diff_w = corr_s.u * base_s.tn.t[1] + corr_s.w * base_s.tn.n[1];
    GLSL_float diff_M = corr_s.M;
    GLSL_float diff_T = corr_s.T;

    GLSL_float x_s = base_s.u + diff_u;
    GLSL_float y_s = base_s.w + diff_w;
    GLSL_float M_s = base_s.M + diff_M;
    GLSL_float T_s = base_s.T + diff_T;

    GLSL_float diff_T_bc = T_s - base0.T;
    GLSL_Basis tn_s = GLSL_rotate_basis(full0.tn, diff_T_bc);

    GLSL_float Fx_s = corr_s.N * base_s.tn.t[0] + corr_s.Q * base_s.tn.n[0];
    GLSL_float Fy_s = corr_s.N * base_s.tn.t[1] + corr_s.Q * base_s.tn.n[1];

    GLSL_SolutionFull full_s;
    full_s.x = x_s;
    full_s.y = y_s;
    full_s.M = M_s;
    full_s.T = T_s;
    full_s.tn = tn_s;
    full_s.Fx = Fx_s;
    full_s.Fy = Fy_s;

    return full_s;
}


layout (location = 0) in vec2 aPos;
layout(std430, binding = 0) restrict readonly buffer ElementsBuffer {
    GLSL_Element elements[];
};
uniform GLSL_float up_array[UP_ARRAY_SIZE];
uniform GLSL_float zoom;
uniform GLSL_float look_at[2];

out vec3 vertexColor;

void main() {
    GLSL_UNPACK_UP(up, up_array);

    GLSL_float s = aPos.x * up.total_length / GLSL_float(elements.length());
    int element_index = int(aPos.y);

    GLSL_Element el_0 = elements[element_index];
    GLSL_SolutionBase base_s = GLSL_EQLINK_link_base(up, el_0.full, el_0.base, s);
    GLSL_SolutionCorr corr_s = GLSL_EQLINK_link_corr(up, el_0.full, el_0.base, el_0.corr, s);
    GLSL_SolutionFull full_s = GLSL_EQLINK_link_full(up, el_0.full, el_0.base, base_s, corr_s, s);

    gl_Position = vec4((full_s.x - look_at[0]) * zoom, (full_s.y - look_at[1]) * zoom, 0.0, 1.0);
    int _color = element_index % 3;
    vertexColor = vec3(_color == 0, _color == 1, _color == 2);
}