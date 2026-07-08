#pragma once
#include <SFML/Graphics.hpp>

class App;

class Screen {
public:
    virtual ~Screen() = default;
    virtual void init() = 0;
    virtual void handleEvent(const sf::Event& event, const sf::RenderWindow& window) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;

    void setApp(App* app) { m_app = app; }

protected:
    App* m_app = nullptr;
};