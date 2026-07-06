#pragma once
#include "Scene.h"

class PlayingScene : public Scene {
public:
    PlayingScene();
    void onEnter() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    void onExit() override;
    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;
    bool shouldReturnToTitle() const { return returnToTitle; }
private:
    bool returnToTitle = false;
};