#pragma once
#include <SFML/Graphics.hpp>

class Scene {
public:
    virtual ~Scene() = default;
    virtual void onEnter() = 0;
    virtual void update(float dt) = 0;
    virtual void draw(sf::RenderWindow& window) = 0;
    virtual void onExit() = 0;
    virtual void handleEvent(const sf::Event& event, sf::RenderWindow& window) = 0;
};