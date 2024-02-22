#include "shader_drawer.h"

#include "imgui.h"


ShaderDrawer::ShaderDrawer(sf::RenderWindow* new_window, int new_segments_count) {
    segments_count = new_segments_count;
    window = new_window;
    zoom = 0.1f;
    mouse_pressed = false;
    mouse_initial = sf::Vector2i(0, 0);
    look_at_initial = sf::Vector2f(0.0f, 0.0f);
    show_demo_window = false;
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

void ShaderDrawer::process_event(sf::Event event) {
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
            mouse_initial = sf::Vector2i(event.mouseButton.x, event.mouseButton.y);
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
            look_at.x = look_at_initial.x - float(event.mouseMove.x - mouse_initial.x) / float(window_size.x) * 2.0f / zoom;
            look_at.y = look_at_initial.y + float(event.mouseMove.y - mouse_initial.y) / float(window_size.y) * 2.0f / zoom;
        }
    }
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
    ImGui::Checkbox("Show demo window", &show_demo_window);
    if (show_demo_window) {
        ImGui::ShowDemoWindow(&show_demo_window);
    }

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

    ImGui::SliderFloat("initial_angle", &solver.up.initial_angle, -PI / 2, PI / 2);

    // Compute shader buffers
    Element* elements = sb.get_buffer_ptr();
    if (elements != nullptr) {
        solver.traverse(elements, 0, solver.up.elements_count - 1);
    }
}

void draw_grid_n_axes(float zoom, sf::Vector2f look_at, float line_gap = 0.1) {
    // Grid
    glBegin(GL_LINES);
    glColor3f(0.1, 0.1, 0.1);
    int lines_cnt = int(1.0 / line_gap / zoom);
    for (int i = -lines_cnt - 1; i <= lines_cnt + 1; ++i) {
        glVertex2f(-1, ((float)i * line_gap - fmodf(look_at.y, line_gap)) * zoom);
        glVertex2f(+1, ((float)i * line_gap - fmodf(look_at.y, line_gap)) * zoom);
        glVertex2f(((float)i * line_gap - fmodf(look_at.x, line_gap)) * zoom, -1);
        glVertex2f(((float)i * line_gap - fmodf(look_at.x, line_gap)) * zoom, +1);
    }

    // Axes
    glColor3f(1, 1, 1);
    glVertex2d(-1, -look_at.y * zoom);
    glVertex2d(+1, -look_at.y * zoom);
    glVertex2d(-look_at.x * zoom, -1);
    glVertex2d(-look_at.x * zoom, +1);
    glEnd();
}

void ShaderDrawer::draw() {
    draw_grid_n_axes(zoom, look_at, 0.1);
    sb.draw(solver.up, zoom, look_at);
}
