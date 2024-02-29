#include <SFML/Graphics.hpp>
#include <GL/glew.h>
#include <imgui.h>
#include <imgui-SFML.h>

#include "shader_drawer.h"


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

    ShaderDrawer sd(&window);

    sf::Clock deltaClock;
    while (sd.running) {

        // Check all the window's events that were triggered since the last iteration of the loop
        sf::Event event{};
        while (window.pollEvent(event)) {

            // Pass events to ImGui
            ImGui::SFML::ProcessEvent(window, event);

            // "Close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                sd.running = false;
            // Window was resized
            else if (event.type == sf::Event::Resized)
                glViewport(0, 0, (GLsizei) event.size.width, (GLsizei) event.size.height);

            sd.process_event(event);

        }

        // Pass mouse & display_size & time to ImGui
        ImGui::SFML::Update(window, deltaClock.restart());

        // Process ImGui & generate draw lists
        // Also calculate the solution
        sd.process_gui();

        // Clear the window with black color
        window.clear(sf::Color::Black);

        // Draw frame
        window.setActive(true);
        sd.draw();
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
