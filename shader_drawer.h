#ifndef SHADERBEAMS_SHADER_DRAWER_H
#define SHADERBEAMS_SHADER_DRAWER_H

#include "Solver.h"
#include "shader_buffers.h"

#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <imgui.h>
#include <imfilebrowser.h>


#define VisualParams_FIELDS segments_count, dashed, zoom, look_at, mouse_pressed, mouse_initial, look_at_initial
struct VisualParams {
    int segments_count = 0;
    bool dashed = false;
    float zoom = 0.1f;
    std::array<float, 2> look_at = {0.0f, 0.0f};
    bool mouse_pressed = false;
    std::array<int, 2> mouse_initial = {0, 0};
    std::array<float, 2> look_at_initial = {0.0f, 0.0f};

    void process_event(sf::Event event, sf::RenderWindow* window) {
        if (ImGui::GetIO().WantCaptureMouse) {
            return;
        }

        if (event.type == sf::Event::MouseWheelMoved) {
            zoom *= pow(2.0f, (float)event.mouseWheel.delta / 10.0f);
            zoom = fminf(fmaxf(zoom, powf(2, -10)), powf(2, 10));
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
                look_at[0] = look_at_initial[0] - float(event.mouseMove.x - mouse_initial[0]) / float(window_size.x) * 2.0f / zoom;
                look_at[1] = look_at_initial[1] + float(event.mouseMove.y - mouse_initial[1]) / float(window_size.y) * 2.0f / zoom;
            }
        }
    }
};


#define SolverParams_FIELDS solved, auto_solve, auto_fit_angle, fit_threshold, fit_rate, fit_deviation
struct SolverParams {
    bool solved = false;
    bool auto_solve = true;
    bool auto_fit_angle = true;
    float fit_threshold = 1e-3f;
    float fit_rate = 0.1f;
    float fit_deviation = 0.0f;

    bool should_compute(Solver* solver) {
        bool force_solve = false;
        bool was_fit = fabsf(fit_deviation) < fit_threshold;

        if (ImGui::CollapsingHeader("Solver")) {
            ImGui::Checkbox("Auto-solve", &auto_solve);
            force_solve = ImGui::Button("Solve");

            ImGui::Spacing();

            if (ImGui::Checkbox("AutoFit angle", &auto_fit_angle)) {
                was_fit = false;
            }
            ImGui::SliderFloat("Fit threshold", &fit_threshold, solver->up.total_length * 1e-5f, solver->up.total_length / 10.0f, "%.3g", ImGuiSliderFlags_Logarithmic);
            ImGui::SliderFloat("Fit rate", &fit_rate, 0.01f, 1.0f, "%.3g", ImGuiSliderFlags_Logarithmic);
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

    void accept_solution(Solver* solver, Element *elements) {
        if (auto_fit_angle) {
            fit_deviation = elements[solver->up.elements_count].full.y;
            float deviation_factor = fit_deviation / solver->up.total_length;
            float angle_factor = (PI / 2.0f) * deviation_factor;
            float angle_delta = angle_factor * fit_rate;
            solver->up.initial_angle -= angle_delta;
        }
    }
};


class ShaderDrawer {
public:
    explicit ShaderDrawer(sf::RenderWindow* window, int new_segments_count = 10);

    void setup(UniformParams new_up);

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

    Solver solver;
    ShaderBuffers sb;
    sf::RenderWindow *window;

    bool show_demo_window = false;

    VisualParams vp;
    SolverParams sp;

    ImGui::FileBrowser file_load_dialog, file_save_dialog;
};


#endif //SHADERBEAMS_SHADER_DRAWER_H
