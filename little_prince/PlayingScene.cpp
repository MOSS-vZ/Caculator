#include "PlayingScene.h"
#include "Settings.h"
#include <iostream>

PlayingScene::PlayingScene() : returnToTitle(false) {}

void PlayingScene::onEnter() {
    std::cout << "[Game] Entered PLAYING state. Press ESC to return." << std::endl;
}

void PlayingScene::update(float dt) {
    // 无更新逻辑
}

void PlayingScene::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(20, 20, 40));
    sf::RectangleShape panel(sf::Vector2f(600, 300));
    panel.setFillColor(sf::Color(255, 255, 255, 200));
    panel.setOrigin(sf::Vector2f(300, 150));
    panel.setPosition(sf::Vector2f(Settings::WINDOW_WIDTH / 2.0f, Settings::WINDOW_HEIGHT / 2.0f));
    window.draw(panel);
    sf::CircleShape dot(20);
    dot.setFillColor(sf::Color::Green);
    dot.setOrigin(sf::Vector2f(20, 20));
    dot.setPosition(sf::Vector2f(Settings::WINDOW_WIDTH / 2.0f, Settings::WINDOW_HEIGHT / 2.0f - 60));
    window.draw(dot);
}

void PlayingScene::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* keyEvent = event.getIf<sf::Event::KeyPressed>()) {
        if (keyEvent->code == sf::Keyboard::Key::Escape) {
            returnToTitle = true;
        }
    }
}

void PlayingScene::onExit() {
    std::cout << "[PlayingScene] Exiting." << std::endl;
}