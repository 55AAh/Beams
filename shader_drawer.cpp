#include "shader_drawer.h"

#include "imgui.h"


ShaderDrawer::ShaderDrawer(int new_segments_count) {
    segments_count = new_segments_count;
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
            UniformParams new_up { 1, 0, 1000, 3 * 3.1415926 / 4, 10, 10 };
            setup(new_up);
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

    if (!solver.was_setup()) {
        return;
    }

    static float theta = 0;
    ImGui::SliderFloat("theta", &theta, 0, PI / 2);

    // Compute shader buffers
    ElementParams* element_params = sb.get_buffer_ptr();
    if (element_params != nullptr) {
        solver.traverse(theta, element_params, 0, solver.up.elements_count - 1);
    }
}

void draw_grid_n_axes() {
    // Grid
    glBegin(GL_LINES);
    glColor3f(0.1, 0.1, 0.1);
    for (int i = -10; i <= 10; ++i) {
        glVertex2f(-1, (float)i / 10);
        glVertex2f(+1, (float)i / 10);
        glVertex2f((float)i / 10, -1);
        glVertex2f((float)i / 10, +1);
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
    draw_grid_n_axes();
    sb.draw(solver.up);
}
