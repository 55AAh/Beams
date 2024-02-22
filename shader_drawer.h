#ifndef SHADERBEAMS_SHADER_DRAWER_H
#define SHADERBEAMS_SHADER_DRAWER_H

#include "Solver.h"
#include "shader_buffers.h"

#include <SFML/Graphics.hpp>
#include "SFML/Window/Event.hpp"


class ShaderDrawer {
public:
    ShaderDrawer(sf::RenderWindow* window, int new_segments_count = 10);

    void setup(UniformParams new_up);

    void tweak(int new_segments_count);

    void process_event(sf::Event event);

    void process_gui();

    void draw();

    void forget();

    ~ShaderDrawer() { forget(); }

private:
    void ensure_sb();

    Solver solver;
    ShaderBuffers sb;
    int segments_count;
    sf::RenderWindow *window;
    float zoom;
    sf::Vector2f look_at;
    bool mouse_pressed;
    sf::Vector2i mouse_initial;
    sf::Vector2f look_at_initial;
    bool show_demo_window;
};


#endif //SHADERBEAMS_SHADER_DRAWER_H
