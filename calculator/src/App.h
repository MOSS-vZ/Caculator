#pragma once
#include <SFML/Graphics.hpp>
#include <memory>
#include "Screen.h"

class App {
public:
    App();
    void run();
    void switchScreen(std::unique_ptr<Screen> newScreen);

private:
    void updateView();

    sf::RenderWindow m_window;
    std::unique_ptr<Screen> m_currentScreen;
    sf::Clock m_clock;
    sf::View m_view;

    static constexpr float DESIGN_WIDTH = 2048.f;
    static constexpr float DESIGN_HEIGHT = 1536.f;
};