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

#define VisualParams_FIELDS disabled, segments_count, dashed, zoom, look_at, mouse_pressed, mouse_initial, look_at_initial
struct VisualParams {
    bool disabled = false;
    int segments_count = 0;
    bool dashed = false;
    C_float zoom = 0.1f;
    std::array<C_float, 2> look_at = {0.0, 0.0};
    bool mouse_pressed = false;
    std::array<int, 2> mouse_initial = {0, 0};
    std::array<C_float, 2> look_at_initial = {0.0, 0.0};

    void process_event(sf::Event event, sf::RenderWindow* window);
};


#define SolverParams_FIELDS solved, auto_solve, auto_fit_angle, fit_threshold, fit_rate, fit_deviation
struct SolverParams {
    bool solved = false;
    bool auto_solve = true;
    bool auto_fit_angle = true;
    C_float fit_threshold = 1e-3;
    C_float fit_rate = 0.1;
    C_float fit_deviation = 0.0;

    bool should_compute(C_Solver* solver);

    void accept_solution(C_Solver* solver);
};


class ShaderDrawer {
public:
    bool running = true;

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

    void free_sb();

    void compute(size_t begin, size_t end);

    void copy_to_shaders(size_t begin, size_t end);

    C_Solver solver;
    ShaderBuffers sb;
    sf::RenderWindow *window;

    bool show_demo_window = false;

    VisualParams vp;
    SolverParams sp;

    ImGui::FileBrowser file_load_dialog, file_save_dialog;
    bool matplotlib = false;
};


#endif //SHADERBEAMS_SHADER_DRAWER_H
