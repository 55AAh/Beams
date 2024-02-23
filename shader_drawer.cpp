#include "shader_drawer.h"

#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;


NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VisualParams, VisualParams_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SolverParams, SolverParams_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(C_UniformParams, C_UniformParams_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(C_Basis, C_Basis_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(C_SolutionFull, C_SolutionFull_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(C_SolutionBase, C_SolutionBase_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(C_SolutionCorr, C_SolutionCorr_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(C_Element, C_Element_FIELDS)

GLSL_Basis C2GLSL_Basis(C_Basis c_basis) {
    return GLSL_Basis {
        { GLSL_float(c_basis.t[0]), GLSL_float(c_basis.t[1]) },
        { GLSL_float(c_basis.n[0]), GLSL_float(c_basis.n[1]) },
    };
}

GLSL_SolutionFull C2GLSL_SolutionFull(C_SolutionFull c_full) {
    return GLSL_SolutionFull {
        GLSL_float(c_full.x),
        GLSL_float(c_full.y),
        GLSL_float(c_full.M),
        GLSL_float(c_full.T),
        C2GLSL_Basis(c_full.tn),
        GLSL_float(c_full.Fx),
        GLSL_float(c_full.Fy),
    };
}

GLSL_SolutionBase C2GLSL_SolutionBase(C_SolutionBase c_base) {
    return GLSL_SolutionBase {
            GLSL_float(c_base.u),
            GLSL_float(c_base.w),
            GLSL_float(c_base.M),
            GLSL_float(c_base.T),
            C2GLSL_Basis(c_base.tn),
    };
}

GLSL_SolutionCorr C2GLSL_SolutionCorr(C_SolutionCorr c_corr) {
    return GLSL_SolutionCorr {
            GLSL_float(c_corr.u),
            GLSL_float(c_corr.w),
            GLSL_float(c_corr.M),
            GLSL_float(c_corr.T),
            GLSL_float(c_corr.N),
            GLSL_float(c_corr.Q),
            GLSL_float(c_corr.Pt),
            GLSL_float(c_corr.Pn),
    };
}

GLSL_Element C2GLSL_Element(C_Element c_element) {
    return GLSL_Element {
            C2GLSL_SolutionFull(c_element.full),
            C2GLSL_SolutionBase(c_element.base),
            C2GLSL_SolutionCorr(c_element.corr),
    };
}

GLSL_UniformParams C2GLSL_UniformParams(C_UniformParams c_up) {
    return GLSL_UniformParams {
            GLSL_float(c_up.EI),
            GLSL_float(c_up.initial_angle),
            GLSL_float(c_up.total_weight),
            GLSL_float(c_up.total_length),
            GLSL_float(c_up.gap),
            c_up.elements_count,
    };
}

ShaderDrawer::ShaderDrawer(sf::RenderWindow* new_window, int new_segments_count) {
    vp.segments_count = new_segments_count;
    window = new_window;
    file_load_dialog = ImGui::FileBrowser(ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter);
    file_load_dialog.SetTitle("Load ShaderBeams problem from file");
    file_load_dialog.SetTypeFilters({ ".txt" });
    file_save_dialog = ImGui::FileBrowser(ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir | ImGuiFileBrowserFlags_CloseOnEsc | ImGuiFileBrowserFlags_ConfirmOnEnter);
    file_save_dialog.SetTitle("Save ShaderBeams problem to file");
    file_save_dialog.SetTypeFilters({ ".txt" });
}

void ShaderDrawer::setup(C_UniformParams new_up) {
    sp.solved = false;
    solver.setup(new_up);
    ensure_sb();
}

void ShaderDrawer::tweak(int new_segments_count) {
    vp.segments_count = new_segments_count;
    ensure_sb();
}

void ShaderDrawer::ensure_sb() {
    if (solver.was_setup()) {
        sb.re_alloc(solver.up.elements_count, vp.segments_count);
    }
    else {
        sb.free();
    }
}

void ShaderDrawer::forget() {
    sp.solved = false;
    solver.forget();
    sb.free();
}

void ShaderDrawer::load_from_file(const std::filesystem::path& file_path) {
    std::ifstream i(file_path);
    json j;
    i >> j;

    vp = j["visual_params"];
    sp = j["solver_params"];
    C_UniformParams up = j["problem"];
    json el_j = j["solution"];

    assert(el_j.size() == up.elements_count + 1);

    solver.setup(up);

    ensure_sb();

    C_Element *c_elements = solver.elements;

    for (auto& element : el_j) {
        C_Element c_el = element.template get<C_Element>();
        *c_elements++ = c_el;
    }

    copy_to_shaders(0, up.elements_count);
}

void ShaderDrawer::save_to_file(const std::filesystem::path& file_path) {
    C_Element *c_elements = solver.elements;
    json el_j;
    for (size_t element_i = 0; element_i <= solver.up.elements_count; ++element_i) {
        C_Element c_el = c_elements[element_i];
        el_j.push_back(c_el);
    }

    json j;
    j["visual_params"] = vp;
    j["solver_params"] = sp;
    j["problem"] = solver.up;
    j["solution"] = el_j;

    std::ofstream o(file_path);
    o << std::setw(4) << j << std::endl;
}

void ShaderDrawer::process_event(sf::Event event) {
    vp.process_event(event, window);
}

void ShaderDrawer::process_gui() {
    // Calculate the new frame

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("Load from file")) {
                file_load_dialog.Open();
            }
            if (ImGui::MenuItem("Save to file")) {
                file_save_dialog.Open();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Menu"))
        {
            if (ImGui::MenuItem("Show demo window")) {
                show_demo_window = true;
            }
            ImGui::EndMenu();
        }
    }
    ImGui::EndMainMenuBar();

    if (file_load_dialog.HasSelected()) {
        std::filesystem::path file_path = file_load_dialog.GetSelected();
        load_from_file(file_path);
        file_load_dialog.ClearSelected();
    }

    if (file_save_dialog.HasSelected()) {
        std::filesystem::path file_path = file_save_dialog.GetSelected();
        if (!file_path.has_extension())
            file_path.replace_extension("txt");
        save_to_file(file_path);
        file_save_dialog.ClearSelected();
    }

    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    ImGui::Begin("Beams");

    if (ImGui::CollapsingHeader("Visual")) {
        bool segments_count_changed = ImGui::SliderInt("Segments", &vp.segments_count, 1, 10);
        if (segments_count_changed) {
            tweak(vp.segments_count);
        }

        ImGui::Checkbox("Dashed lines", &vp.dashed);
    }

    bool should_compute = false;
    if (ImGui::CollapsingHeader("Problem", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (solver.was_setup()) {
            if (ImGui::Button("Forget problem")) {
                forget();
            }
        }
        else {
            static bool debug_auto_setup = true;
            if (ImGui::Button("Setup problem") || debug_auto_setup) {
                debug_auto_setup = false;

                C_UniformParams up {};

                // Problem statement
                up.total_weight = 100 * PI * 4;
                up.total_length = 10;
                up.gap = 6;
                up.initial_angle = 0;
                up.EI = 1000;

                // FEM parameters
                up.elements_count = 10;

                setup(up);
            }
        }

        bool up_changed = false;

        up_changed |= ImGui_Slider("Total weight", &solver.up.total_weight, 0.1, 10000.0);
        up_changed |= ImGui_Slider("Total length", &solver.up.total_length, 0.1, 30.0);
        up_changed |= ImGui_Slider("EI", &solver.up.EI, 1.0, 10000.0);
        solver.up.initial_angle = fmod(solver.up.initial_angle, 2.0 * PI);
        up_changed |= ImGui_Slider("Theta", &solver.up.initial_angle, -PI / 2, PI / 2);

        if (up_changed) {
            sp.solved = false;
        }

        ImGui::Spacing();

        bool elements_count_changed = ImGui::SliderInt("Elements", &solver.up.elements_count, 1, 100);
        if (elements_count_changed) {
            setup(solver.up);
        }

        if (solver.was_setup()) {
            should_compute = sp.should_compute(&solver);
        }
    }

    // Compute
    if (should_compute && !file_load_dialog.IsOpened() && !file_save_dialog.IsOpened()) {
        compute(0, solver.up.elements_count);
        copy_to_shaders(0, solver.up.elements_count);
    }

    ImGui::End(); // MainWindow
}

void ShaderDrawer::compute(size_t begin, size_t end) {
    solver.traverse(begin, end);
    sp.solved = true;
    sp.accept_solution(&solver);
}

void ShaderDrawer::copy_to_shaders(size_t begin, size_t end) {
    GLSL_Element *glsl_elements = sb.get_buffer_ptr();
    if (glsl_elements != nullptr) {
        for (size_t element_i = begin; element_i < end; ++element_i) {
            C_Element c_element = solver.elements[element_i];
            GLSL_Element glsl_element = C2GLSL_Element(c_element);
            glsl_elements[element_i] = glsl_element;
        }
    }
}

void draw_grid_n_axes(C_float zoom, std::array<C_float, 2> look_at, C_float line_gap = 0.1) {
    // Grid
    glBegin(GL_LINES);
    glColor3f(0.1, 0.1, 0.1);
    int lines_cnt = int(1.0 / line_gap / zoom);
    for (int i = -lines_cnt - 1; i <= lines_cnt + 1; ++i) {
        glVertex2d(-1, ((C_float)i * line_gap - fmod(look_at[1], line_gap)) * zoom);
        glVertex2d(+1, ((C_float)i * line_gap - fmod(look_at[1], line_gap)) * zoom);
        glVertex2d(((C_float)i * line_gap - fmod(look_at[0], line_gap)) * zoom, -1);
        glVertex2d(((C_float)i * line_gap - fmod(look_at[0], line_gap)) * zoom, +1);
    }

    // Axes
    glColor3f(1, 1, 1);
    glVertex2d(-1, -look_at[1] * zoom);
    glVertex2d(+1, -look_at[1] * zoom);
    glVertex2d(-look_at[0] * zoom, -1);
    glVertex2d(-look_at[0] * zoom, +1);
    glEnd();
}

void ShaderDrawer::draw() {
    draw_grid_n_axes(vp.zoom, vp.look_at, 0.1);

    if (sp.solved) {
        C_UniformParams c_up = solver.up;
        GLSL_UniformParams glsl_up = C2GLSL_UniformParams(c_up);
        sb.draw(glsl_up, GLSL_float(vp.zoom), { GLSL_float(vp.look_at[0]), GLSL_float(vp.look_at[1]) }, vp.dashed);
    }
    file_load_dialog.Display();
    file_save_dialog.Display();
}
