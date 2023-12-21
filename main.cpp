#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui-SFML.h>


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
        static bool show_demo_window;
        ImGui::ShowDemoWindow(&show_demo_window);

        // Clear the window with black color
        window.clear(sf::Color::Black);

        // Draw frame
        window.setActive(true);
        glBegin(GL_TRIANGLES);
        glColor3f(1.0, 0.0, 0.0);
        glVertex2f(-0.5, -0.5);
        glColor3f(0.0, 1.0, 0.0);
        glVertex2f(+0.5, -0.5);
        glColor3f(0.0, 0.0, 1.0);
        glVertex2f(+0.0, +0.5);
        glEnd();
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
