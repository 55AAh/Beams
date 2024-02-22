#include "shader_drawer.h"

#include "imgui.h"


ShaderDrawer::ShaderDrawer(int new_segments_count) {
    segments_count = new_segments_count;
    zoom = 0.1f;
}

void ShaderDrawer::setup(UniformParams new_up) {
    solver.setup(new_up);
    ensure_sb();
}

void ShaderDrawer::tweak(int new_segments_count) {
    segments_count = new_segments_count;
    ensure_sb();
}

void ShaderDrawer::ensure_sb() {
    if (solver.was_setup()) {
        sb.re_alloc(solver.up.elements_count, segments_count);
    }
    else {
        sb.free();
    }
}

void ShaderDrawer::forget() {
    solver.forget();
    sb.free();
}

static bool debug_auto_setup = true;

void ShaderDrawer::process_gui() {
    if (solver.was_setup()) {
        if (ImGui::Button("Forget problem")) {
            forget();
        }
    }
    else {
        if (ImGui::Button("Setup problem") || debug_auto_setup) {
            debug_auto_setup = false;

            UniformParams up {};

            // Problem statement
            up.total_weight = 100 * PI * 4;
            up.total_length = 10;
            up.gap = 6;
            up.initial_angle = -1.3762567175136284;
            up.EI = 1000;

            // FEM parameters
            up.elements_count = 10;

            // UniformParams new_up { 1, 0, 1000, 3 * 3.1415926 / 4, 10, 10 };
            setup(up);
        }
    }

    // Calculate the new frame
    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);
    bool elements_count_changed = ImGui::SliderInt("Elements", &solver.up.elements_count, 1, 1000);
    if (elements_count_changed) {
        setup(solver.up);
    }
    bool segments_count_changed = ImGui::SliderInt("Segments", &segments_count, 1, 10);
    if (segments_count_changed) {
        tweak(segments_count);
    }

    ImGui::SliderFloat("Zoom", &zoom, 0.1, 10);

    if (!solver.was_setup()) {
        return;
    }

    ImGui::SliderFloat("initial_angle", &solver.up.initial_angle, -PI / 2, PI / 2);

    // Compute shader buffers
    Element* elements = sb.get_buffer_ptr();
    if (elements != nullptr) {
        solver.traverse(elements, 0, solver.up.elements_count - 1);
    }
}

void draw_grid_n_axes(float zoom, float line_gap = 0.1) {
    // Grid
    glBegin(GL_LINES);
    glColor3f(0.1, 0.1, 0.1);
    int lines_cnt = int(1.0 / line_gap / zoom);
    float scale_factor = zoom * line_gap;
    for (int i = -lines_cnt; i <= lines_cnt; ++i) {
        glVertex2f(-1, (float)i * scale_factor);
        glVertex2f(+1, (float)i * scale_factor);
        glVertex2f((float)i * scale_factor, -1);
        glVertex2f((float)i * scale_factor, +1);
    }

    // Axes
    glColor3f(1, 1, 1);
    glVertex2d(-1, 0);
    glVertex2d(1, 0);
    glVertex2d(0, -1);
    glVertex2d(0, 1);
    glEnd();
}

void ShaderDrawer::draw() {
    draw_grid_n_axes(zoom, 0.1);
    sb.draw(solver.up, zoom);
}
