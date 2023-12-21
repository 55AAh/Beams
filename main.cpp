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

static int elements_count = 3, segments_count = 5;

void calculate(ElementParams* elements) {
    static bool show_demo_window;
    static float o;
    ImGui::SliderFloat("o", &o, 0, 1);
    ImGui::ShowDemoWindow(&show_demo_window);

    // Compute shader buffers
    if (elements != nullptr) {
        elements[0].u = 0.1f + o;
        elements[1].u = 0.2f;
        elements[2].u = 0.4f;

        elements[0].w = 0.1f;
        elements[1].w = 0.1f;
        elements[2].w = 0.1f;
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
    ShaderBuffers shader_buffers("shaders\\vertex_shader.vert", "shaders\\fragment_shader.frag");

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
        if (ImGui::SliderInt("Elements", &elements_count, 1, 1000) ||
            ImGui::SliderInt("Segments", &segments_count, 1, 1000)) {
            shader_buffers.re_alloc(elements_count, segments_count);
            printf("elements %d, segments %d\n", elements_count, segments_count);
        }
        calculate(shader_buffers.get_buffer_ptr());

        // Clear the window with black color
        window.clear(sf::Color::Black);

        // Draw frame
        window.setActive(true);
        draw_grid_n_axes();
        shader_buffers.draw();
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
