#include <algorithm>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <memory>
#include <random>
#include <vector>

#include <SFML/Graphics.hpp>

const int WINDOW_WIDTH = 800;
const int WINDOW_HEIGHT = 800;
const int FPS_LIMIT = 30;

// Number of frames for one left -> right animation (60 frames @ 30 FPS = 2 s)
const int FRAMES_PER_ANIM = 60;

const float PI = 3.14159265358979f;

// global tween function
std::function<float(float, float, float)> tween = [](float a, float b, float t) {
    return (1 - t) * a + t * b;
};

void handleInput(sf::Window& window, bool& shouldQuit) {
    while (const std::optional<sf::Event> event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>()) {
            window.close();
            shouldQuit = true;
        }

        // Q2: keys 1-9 replace the global tween with a different easing.
        // Each lambda computes an eased value e = f(t) in [0,1] and then
        // interpolates between a and b with it.
        if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
            switch (key->code) {
                case sf::Keyboard::Key::Num1:  // linear
                    tween = [](float a, float b, float t) {
                        return a + (b - a) * t;
                    };
                    break;
                case sf::Keyboard::Key::Num2:  // ease in quad
                    tween = [](float a, float b, float t) {
                        float e = t * t;
                        return a + (b - a) * e;
                    };
                    break;
                case sf::Keyboard::Key::Num3:  // ease out quad
                    tween = [](float a, float b, float t) {
                        float e = 1.0f - (1.0f - t) * (1.0f - t);
                        return a + (b - a) * e;
                    };
                    break;
                case sf::Keyboard::Key::Num4:  // ease in-out cubic
                    tween = [](float a, float b, float t) {
                        float e = t < 0.5f ? 4.0f * t * t * t
                                           : 1.0f - std::pow(-2.0f * t + 2.0f, 3.0f) / 2.0f;
                        return a + (b - a) * e;
                    };
                    break;
                case sf::Keyboard::Key::Num5:  // ease in sine
                    tween = [](float a, float b, float t) {
                        float e = 1.0f - std::cos(t * PI / 2.0f);
                        return a + (b - a) * e;
                    };
                    break;
                case sf::Keyboard::Key::Num6:  // ease out sine
                    tween = [](float a, float b, float t) {
                        float e = std::sin(t * PI / 2.0f);
                        return a + (b - a) * e;
                    };
                    break;
                case sf::Keyboard::Key::Num7:  // ease in-out sine
                    tween = [](float a, float b, float t) {
                        float e = -(std::cos(PI * t) - 1.0f) / 2.0f;
                        return a + (b - a) * e;
                    };
                    break;
                case sf::Keyboard::Key::Num8:  // ease out back (overshoots past 1)
                    tween = [](float a, float b, float t) {
                        const float c1 = 1.70158f;
                        const float c3 = c1 + 1.0f;
                        float e = 1.0f + c3 * std::pow(t - 1.0f, 3.0f) +
                                  c1 * std::pow(t - 1.0f, 2.0f);
                        return a + (b - a) * e;
                    };
                    break;
                case sf::Keyboard::Key::Num9:  // ease out bounce
                    tween = [](float a, float b, float t) {
                        const float n1 = 7.5625f;
                        const float d1 = 2.75f;
                        float e;
                        if (t < 1.0f / d1) {
                            e = n1 * t * t;
                        } else if (t < 2.0f / d1) {
                            t -= 1.5f / d1;
                            e = n1 * t * t + 0.75f;
                        } else if (t < 2.5f / d1) {
                            t -= 2.25f / d1;
                            e = n1 * t * t + 0.9375f;
                        } else {
                            t -= 2.625f / d1;
                            e = n1 * t * t + 0.984375f;
                        }
                        return a + (b - a) * e;
                    };
                    break;
                default:
                    break;
            }
        }
    }
}

void render(sf::RenderWindow& window, int frame) {
    // Clear with blue background (sky)
    window.clear(sf::Color::Black);

    // Progress through the current animation, in [0, 1). The modulo makes it repeat.
    float t = static_cast<float>(frame % FRAMES_PER_ANIM) / FRAMES_PER_ANIM;

    // Q1: circle moving left -> right, 1/3 from the top, x given by tween
    const float radius = 20.0f;
    float x = tween(radius, WINDOW_WIDTH - radius, t);
    float y = WINDOW_HEIGHT / 3.0f;

    sf::CircleShape circle(radius);
    circle.setOrigin({radius, radius});
    circle.setFillColor(sf::Color::Cyan);
    circle.setPosition({x, y});
    window.draw(circle);

    // Q3: plot the tween function on [0,1] x [0,1] at the bottom of the window
    const float left = 80.0f;
    const float right = WINDOW_WIDTH - 80.0f;
    const float bottom = WINDOW_HEIGHT - 80.0f;
    const float top = WINDOW_HEIGHT - 330.0f;

    // Map a point in the unit square to screen coords (y is flipped in SFML)
    auto toScreen = [&](float u, float v) {
        return sf::Vector2f{left + u * (right - left), bottom - v * (bottom - top)};
    };

    // Axes
    sf::VertexArray axes(sf::PrimitiveType::Lines, 4);
    axes[0] = sf::Vertex{toScreen(0.0f, 0.0f), sf::Color::White};
    axes[1] = sf::Vertex{toScreen(1.0f, 0.0f), sf::Color::White};
    axes[2] = sf::Vertex{toScreen(0.0f, 0.0f), sf::Color::White};
    axes[3] = sf::Vertex{toScreen(0.0f, 1.0f), sf::Color::White};
    window.draw(axes);

    // Curve: sample tween(0, 1, u) for u in [0, 1]
    const int SAMPLES = 200;
    sf::VertexArray curve(sf::PrimitiveType::LineStrip);
    for (int i = 0; i <= SAMPLES; ++i) {
        float u = static_cast<float>(i) / SAMPLES;
        curve.append(sf::Vertex{toScreen(u, tween(0.0f, 1.0f, u)), sf::Color::Cyan});
    }
    window.draw(curve);

    // Dot at the current animation position on the curve
    const float dotRadius = 5.0f;
    sf::CircleShape dot(dotRadius);
    dot.setOrigin({dotRadius, dotRadius});
    dot.setFillColor(sf::Color::Yellow);
    dot.setPosition(toScreen(t, tween(0.0f, 1.0f, t)));
    window.draw(dot);

    window.display();
}

int main() {
    sf::RenderWindow window;

    try {
        // Initialize window
        window.create(sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}), "Tween");
        window.setFramerateLimit(FPS_LIMIT);
        // Prevent key repeats.
        window.setKeyRepeatEnabled(false);

        bool shouldQuit = false;
        int frame = 0;
        // Main game loop
        while (window.isOpen()) {
            handleInput(window, shouldQuit);
            if (shouldQuit) {
                break;
            }
            render(window, frame);
            ++frame;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return -1;
    }
    return 0;
}
