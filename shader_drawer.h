#ifndef SHADERBEAMS_SHADER_DRAWER_H
#define SHADERBEAMS_SHADER_DRAWER_H

#include "Solver.h"
#include "shader_buffers.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <imgui.h>
#include <imfilebrowser.h>


#if C_USE_DOUBLE_PRECISION
#define C_ImGuiDataType ImGuiDataType_Double
#else
#define C_ImGuiDataType ImGuiDataType_Float
#endif

inline bool ImGui_Slider(const char* label, C_float* v, C_float v_min, C_float v_max, const char* format = "%.3f", ImGuiSliderFlags flags = 0) {
    return ImGui::SliderScalar(label, C_ImGuiDataType, v, &v_min, &v_max, format, flags);
}

#define VisualParams_FIELDS segments_count, dashed, zoom, look_at, mouse_pressed, mouse_initial, look_at_initial
struct VisualParams {
    int segments_count = 0;
    bool dashed = false;
    C_float zoom = 0.1f;
    std::array<C_float, 2> look_at = {0.0, 0.0};
    bool mouse_pressed = false;
    std::array<int, 2> mouse_initial = {0, 0};
    std::array<C_float, 2> look_at_initial = {0.0, 0.0};

    void process_event(sf::Event event, sf::RenderWindow* window) {
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
};


#define SolverParams_FIELDS solved, auto_solve, auto_fit_angle, fit_threshold, fit_rate, fit_deviation
struct SolverParams {
    bool solved = false;
    bool auto_solve = true;
    bool auto_fit_angle = true;
    C_float fit_threshold = 1e-3;
    C_float fit_rate = 0.1;
    C_float fit_deviation = 0.0;

    bool should_compute(C_Solver* solver) {
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

    void accept_solution(C_Solver* solver) {
        if (auto_fit_angle) {
            fit_deviation = solver->elements[solver->up.elements_count].full.y;
            C_float deviation_factor = fit_deviation / solver->up.total_length;
            C_float angle_factor = (PI / 2.0) * deviation_factor;
            C_float angle_delta = angle_factor * fit_rate;
            solver->up.initial_angle -= angle_delta;
        }
    }
};


class ShaderDrawer {
public:
    explicit ShaderDrawer(sf::RenderWindow* window, int new_segments_count = 10);

    void setup(C_UniformParams new_up);

    void tweak(int new_segments_count);

    void load_from_file(const std::filesystem::path& file_path);

    void save_to_file(const std::filesystem::path& file_path);

    void process_event(sf::Event event);

    void process_gui();

    void draw();

    void forget();

    ~ShaderDrawer() { forget(); }

private:
    void ensure_sb();

    void compute(size_t begin, size_t end);

    void copy_to_shaders(size_t begin, size_t end);

    C_Solver solver;
    ShaderBuffers sb;
    sf::RenderWindow *window;

    bool show_demo_window = false;

    VisualParams vp;
    SolverParams sp;

    ImGui::FileBrowser file_load_dialog, file_save_dialog;
};


#endif //SHADERBEAMS_SHADER_DRAWER_H
