#include "shader_drawer.h"

#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>

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
            c_up.corr_selector,
            GLSL_float(c_up.EI),
            GLSL_float(c_up.initial_angle),
            GLSL_float(c_up.total_weight),
            GLSL_float(c_up.total_length),
            GLSL_float(c_up.gap),
            c_up.elements_count,
    };
}

void VisualParams::process_event(sf::Event event, sf::RenderWindow *window) {
    if (ImGui::GetIO().WantCaptureMouse) {
        return;
    }

    if (event.type == sf::Event::MouseWheelMoved) {
        zoom *= pow(2.0, (C_float)event.mouseWheel.delta / 10.0);
        zoom = fmin(fmax(zoom, pow(2, -10)), pow(2, 10));
    }
    else if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Button::Left) {
            mouse_pressed = true;
            mouse_initial = { event.mouseButton.x, event.mouseButton.y };
            look_at_initial = look_at;
        }
    }
    else if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Button::Left) {
            mouse_pressed = false;
        }
    }
    else if (event.type == sf::Event::MouseMoved) {
        if (mouse_pressed) {
            sf::Vector2u window_size = window->getSize();
            look_at[0] = look_at_initial[0] - C_float(event.mouseMove.x - mouse_initial[0]) / C_float(window_size.x) * 2.0 / zoom;
            look_at[1] = look_at_initial[1] + C_float(event.mouseMove.y - mouse_initial[1]) / C_float(window_size.y) * 2.0 / zoom;
        }
    }
}

bool SolverParams::should_compute(C_Solver *solver) {
    bool force_solve = false;
    bool was_fit = fabs(fit_deviation) < fit_threshold;

    if (ImGui::CollapsingHeader("Solver")) {
        ImGui::Checkbox("Auto-solve", &auto_solve);
        force_solve = ImGui::Button("Solve");

        ImGui::Spacing();

        if (ImGui::Checkbox("AutoFit angle", &auto_fit_angle)) {
            was_fit = false;
        }
        ImGui_Slider("Fit threshold", &fit_threshold, solver->up.total_length * 1e-5, solver->up.total_length / 10.0, "%.3g", ImGuiSliderFlags_Logarithmic);
        ImGui_Slider("Fit rate", &fit_rate, 0.01, 1.0, "%.3g", ImGuiSliderFlags_Logarithmic);
        if (auto_fit_angle) {
            ImGui::Text("Fitting%s", was_fit ? " finished" : "...");
            ImGui::Text("Theta: %f"
                        "\nVertical deviation: % f"
                        "\nThreshold:           %f",
                        solver->up.initial_angle, fit_deviation, fit_threshold);
        }
    }

    if (force_solve)
        return true;

    if (!auto_solve)
        return false;

    if (!solved)
        return true;

    if (auto_fit_angle && !was_fit)
        return true;

    return false;
}

void SolverParams::accept_solution(C_Solver *solver) {
    if (auto_fit_angle) {
        fit_deviation = solver->elements[solver->up.elements_count].full.y;
        C_float deviation_factor = fit_deviation / solver->up.total_length;
        C_float angle_factor = (PI / 2.0) * deviation_factor;
        C_float angle_delta = angle_factor * fit_rate;
        solver->up.initial_angle -= angle_delta;
    }
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
    if (solver.was_setup() && !vp.disabled) {
        sb.re_alloc(solver.up.elements_count, vp.segments_count);
    }
    else {
        free_sb();
    }
}

void ShaderDrawer::free_sb() {
    sb.free();
}

void ShaderDrawer::forget() {
    sp.solved = false;
    solver.forget();
    free_sb();
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
    json j;
    j["visual_params"] = vp;
    j["solver_params"] = sp;
    j["problem"] = solver.up;

    auto j_elements = nlohmann::json::array();
    C_Element *c_elements = solver.elements;
    for (size_t element_i = 0; element_i <= solver.up.elements_count; ++element_i) {
        C_Element c_el = c_elements[element_i];
        j_elements.push_back(c_el);
    }
    j["solution"] = j_elements;

    if (matplotlib) {
        auto j_elements_seg_outer = nlohmann::json::array();
        C_float each_length = solver.up.total_length / (C_float)solver.up.elements_count;

        for (size_t element_i = 0; element_i <= solver.up.elements_count; ++element_i) {
            auto j_elements_seg_inner = nlohmann::json::array();

            for (size_t segment_i = 0; segment_i <= vp.segments_count; ++segment_i) {
                C_float s = each_length * (C_float)segment_i / vp.segments_count;
                C_Element c_seg_full = solver.get_solution_at(element_i, s);

                j_elements_seg_inner.push_back(c_seg_full);
            }

            j_elements_seg_outer.push_back(j_elements_seg_inner);
        }
        j["solution_seg"] = j_elements_seg_outer;
    }

    std::ofstream o(file_path);
    o << std::setw(4) << j << std::endl;
}

void ShaderDrawer::process_event(sf::Event event) {
    vp.process_event(event, window);
}

void open_in_matplotlib(const std::filesystem::path& file_path) {
    std::string command = "python-tools\\run_nowait python-tools\\plot.py ";
    command += file_path.string();
    system(command.c_str());
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
                matplotlib = false;
                file_save_dialog.Open();
            }
            if (ImGui::MenuItem("Exit")) {
                running = false;
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Matplotlib")) {
            if (ImGui::MenuItem("Export")) {
                matplotlib = true;
                std::filesystem::path file_path = "matplotlib.txt";
                save_to_file(file_path);
                open_in_matplotlib(file_path);
            }
            if (ImGui::MenuItem("Export as")) {
                matplotlib = true;
                file_save_dialog.Open();
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Help"))
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

        if (matplotlib) {
            open_in_matplotlib(file_path);
            matplotlib = false;
        }
    }

    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

    ImGui::Begin("Beams");

    if (ImGui::CollapsingHeader("Visual")) {
        bool sb_changed = false;

        sb_changed |= ImGui::Checkbox("Disabled", &vp.disabled);

        if (!vp.disabled) {
            sb_changed |= ImGui::SliderInt("Segments", &vp.segments_count, 1, 10);
            ImGui::Checkbox("Dashed lines", &vp.dashed);
        }

        if (sb_changed) {
            tweak(vp.segments_count);
        }

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
                up.corr_selector = 0;

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

        bool fem_changed = false;

        fem_changed |= ImGui::SliderInt("Elements", &solver.up.elements_count, 1, 100);
        fem_changed |= ImGui::SliderInt("Corr solution", &solver.up.corr_selector, 0, 1);

        if (fem_changed) {
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
