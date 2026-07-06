#include "Game.h"
#include "Settings.h"
#include "TitleScene.h"
#include "PlayingScene.h"
#include <iostream>

Game::Game() {
    sf::VideoMode vm;
    vm.size.x = Settings::WINDOW_WIDTH;
    vm.size.y = Settings::WINDOW_HEIGHT;
    vm.bitsPerPixel = 32;
    window.create(vm, "Little Prince - SFML");
    window.setFramerateLimit(60);
    currentScene = std::make_unique<TitleScene>();
    currentScene->onEnter();
}

void Game::run() {
    sf::Clock deltaClock;
    while (window.isOpen()) {
        processEvents();
        float dt = deltaClock.restart().asSeconds();
        if (dt > 0.033f) dt = 0.033f;
        update(dt);
        draw();
    }
}

void Game::processEvents() {
    while (const auto event = window.pollEvent()) {
        if (event->is<sf::Event::Closed>())
            window.close();
        if (currentScene)
            currentScene->handleEvent(*event, window);
    }
}

void Game::update(float dt) {
    if (currentScene)
        currentScene->update(dt);

    // 标题场景切换到游戏场景
    if (auto* titleScene = dynamic_cast<TitleScene*>(currentScene.get())) {
        if (titleScene->isStartClicked()) {
            changeScene(std::make_unique<PlayingScene>());
        }
    }
    // 游戏场景切换回标题场景
    if (auto* playingScene = dynamic_cast<PlayingScene*>(currentScene.get())) {
        if (playingScene->shouldReturnToTitle()) {
            changeScene(std::make_unique<TitleScene>());
        }
    }
}

void Game::draw() {
    window.clear();
    if (currentScene)
        currentScene->draw(window);
    window.display();
}

void Game::changeScene(std::unique_ptr<Scene> newScene) {
    if (currentScene)
        currentScene->onExit();
    currentScene = std::move(newScene);
    if (currentScene)
        currentScene->onEnter();
}