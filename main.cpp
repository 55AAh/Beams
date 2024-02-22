#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui-SFML.h>

#include "shader_buffers.h"


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

static int elements_count = 10, segments_count = 5;

static bool debug_auto_setup = true;

void draw_beams(Solver* solver, ShaderBuffers* shader_buffers) {
    if (solver->was_setup()) {
        if (ImGui::Button("Delete problem")) {
            shader_buffers->free();
            solver->forget();
        }
    }
    else {
        if (ImGui::Button("Setup problem") || debug_auto_setup) {
            debug_auto_setup = false;
            UniformParams new_up { 1, 0, 1000, 3 * 3.1415926 / 4, 10, elements_count };
            solver->setup(new_up);
            shader_buffers->re_alloc(elements_count, segments_count);
        }
    }

    // Calculate the new frame
    static bool show_demo_window = true;
    ImGui::ShowDemoWindow(&show_demo_window);
    bool elements_count_changed = ImGui::SliderInt("Elements", &elements_count, 1, 1000);
    bool segments_count_changed = ImGui::SliderInt("Segments", &segments_count, 1, 10);
    if (elements_count_changed || segments_count_changed) {
        shader_buffers->re_alloc(elements_count, segments_count);
    }

    if (!solver->was_setup()) {
        return;
    }

    static float theta = 0;
    ImGui::SliderFloat("theta", &theta, 0, PI / 2);

    // Compute shader buffers
    ElementParams* element_params = shader_buffers->get_buffer_ptr();
    if (element_params != nullptr) {
        solver->traverse(theta, element_params, elements_count, 0, elements_count - 1);
    }
}

int main() {
    // Setup SFML window
    sf::RenderWindow window(sf::VideoMode(800, 800), "BeamsSFML");
    window.setVerticalSyncEnabled(true);
    window.setFramerateLimit(60);

    // Initialize GLEW
    GLenum err = glewInit();
    if (err != GLEW_OK) {
        fprintf(stderr, "glewInit error: %s\n", glewGetErrorString(err));
        abort();
    }

    // Initialize ImGui
    if (!ImGui::SFML::Init(window)) {
        fprintf(stderr, "ImGui::SFML::Init error");
        abort();
    }

    // Load & compile shaders
    ShaderBuffers shader_buffers;

    Solver solver;

    sf::Clock deltaClock;
    bool running = true;
    while (running) {

        // Check all the window's events that were triggered since the last iteration of the loop
        sf::Event event{};
        while (window.pollEvent(event)) {

            // Pass events to ImGui
            ImGui::SFML::ProcessEvent(window, event);

            // "Close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                running = false;
            // Window was resized
            else if (event.type == sf::Event::Resized)
                glViewport(0, 0, (GLsizei) event.size.width, (GLsizei) event.size.height);

        }

        // Pass mouse & display_size & time to ImGui
        ImGui::SFML::Update(window, deltaClock.restart());

        // Process ImGui & generate draw lists
        // Also calculate the solution
        draw_beams(&solver, &shader_buffers);

        // Clear the window with black color
        window.clear(sf::Color::Black);

        // Draw frame
        window.setActive(true);
        draw_grid_n_axes();
        if (solver.was_setup()) {
            shader_buffers.draw(solver.up);
        }
        window.setActive(false);

        // Draw ImGui lists
        ImGui::SFML::Render(window);

        // End the current frame (swap buffers)
        window.display();
    }

    // Shutdown ImGui
    ImGui::SFML::Shutdown();

    return 0;
}
