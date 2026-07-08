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
    sf::RenderWindow m_window;
    std::unique_ptr<Screen> m_currentScreen;
    sf::Clock m_clock;
};