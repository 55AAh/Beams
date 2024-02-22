#ifndef SHADERBEAMS_SHADER_DRAWER_H
#define SHADERBEAMS_SHADER_DRAWER_H

#include "Solver.h"
#include "shader_buffers.h"

#include <SFML/Graphics.hpp>
#include "SFML/Window/Event.hpp"


class ShaderDrawer {
public:
    explicit ShaderDrawer(sf::RenderWindow* window, int new_segments_count = 10);

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
    sf::RenderWindow *window;

    bool show_demo_window = false;

    int segments_count;
    bool dashed = false;
    float zoom = 0.1f;
    sf::Vector2f look_at = sf::Vector2f(0.0f, 0.0f);
    bool mouse_pressed = false;
    sf::Vector2i mouse_initial = sf::Vector2i(0, 0);
    sf::Vector2f look_at_initial = sf::Vector2f(0.0f, 0.0f);

    bool solved = false;
    bool auto_solve = true;
    bool auto_fit_angle = true;
    float fit_threshold = 1e-3f;
    float fit_rate = 0.1f;
    float fit_deviation = 0.0f;
};


#endif //SHADERBEAMS_SHADER_DRAWER_H
