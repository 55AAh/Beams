#include "shader_drawer.h"

#include <nlohmann/json.hpp>
#include <fstream>

using json = nlohmann::json;


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

void ShaderDrawer::setup(UniformParams new_up) {
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

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(VisualParams, VisualParams_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SolverParams, SolverParams_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(UniformParams, UniformParams_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Basis, Basis_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SolutionFull, SolutionFull_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SolutionBase, SolutionBase_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SolutionCorr, SolutionCorr_FIELDS)
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Element, Element_FIELDS)

void ShaderDrawer::load_from_file(const std::filesystem::path& file_path) {
    std::ifstream i(file_path);
    json j;
    i >> j;

    vp = j["visual_params"];
    sp = j["solver_params"];
    UniformParams up = j["problem"];
    json el_j = j["solution"];

    assert(el_j.size() == up.elements_count + 1);

    solver.setup(up);

    ensure_sb();
    Element *elements = sb.get_buffer_ptr();

    for (auto& element : el_j) {
        Element el = element.template get<Element>();
        *elements++ = el;
    }
}

void ShaderDrawer::save_to_file(const std::filesystem::path& file_path) {
    Element *elements = sb.get_buffer_ptr();
    json el_j;
    for (size_t element_i = 0; element_i <= solver.up.elements_count; ++element_i) {
        Element el = elements[element_i];
        el_j.push_back(el);
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

                UniformParams up {};

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

        up_changed |= ImGui::SliderFloat("Total weight", &solver.up.total_weight, 0.1f, 10000.0f);
        up_changed |= ImGui::SliderFloat("Total length", &solver.up.total_length, 0.1f, 30.0f);
        up_changed |= ImGui::SliderFloat("EI", &solver.up.EI, 1.0f, 10000.0f);
        solver.up.initial_angle = fmodf(solver.up.initial_angle, 2.0f * PI);
        up_changed |= ImGui::SliderFloat("Theta", &solver.up.initial_angle, -PI / 2, PI / 2);

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
        Element *elements = sb.get_buffer_ptr();
        if (elements != nullptr) {
            solver.traverse(elements, 0, solver.up.elements_count - 1);
            sp.solved = true;

            sp.accept_solution(&solver, elements);
        }
    }

    ImGui::End(); // MainWindow
}

void draw_grid_n_axes(float zoom, std::array<float, 2> look_at, float line_gap = 0.1) {
    // Grid
    glBegin(GL_LINES);
    glColor3f(0.1, 0.1, 0.1);
    int lines_cnt = int(1.0 / line_gap / zoom);
    for (int i = -lines_cnt - 1; i <= lines_cnt + 1; ++i) {
        glVertex2f(-1, ((float)i * line_gap - fmodf(look_at[1], line_gap)) * zoom);
        glVertex2f(+1, ((float)i * line_gap - fmodf(look_at[1], line_gap)) * zoom);
        glVertex2f(((float)i * line_gap - fmodf(look_at[0], line_gap)) * zoom, -1);
        glVertex2f(((float)i * line_gap - fmodf(look_at[0], line_gap)) * zoom, +1);
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
        sb.draw(solver.up, vp.zoom, vp.look_at, vp.dashed);
    }
    file_load_dialog.Display();
    file_save_dialog.Display();
}
