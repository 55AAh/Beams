#include <SFML/Graphics.hpp>
#include <GL/glew.h>


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

    bool running = true;
    while (running) {

        // Check all the window's events that were triggered since the last iteration of the loop
        sf::Event event{};
        while (window.pollEvent(event)) {
            // "Close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                running = false;
            // Window was resized
            else if (event.type == sf::Event::Resized)
                glViewport(0, 0, (GLsizei) event.size.width, (GLsizei) event.size.height);
        }

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

        // End the current frame (swap buffers)
        window.display();
    }

    return 0;
}
