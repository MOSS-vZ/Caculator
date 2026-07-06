#pragma once
#include <memory>
#include "Scene.h"

class Game {
public:
    Game();
    void run();
    void changeScene(std::unique_ptr<Scene> newScene);
private:
    sf::RenderWindow window;
    std::unique_ptr<Scene> currentScene;
    void processEvents();
    void update(float dt);
    void draw();
};