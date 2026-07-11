#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

class SplashScreen : public Screen {
public:
    SplashScreen();

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    std::vector<std::string> m_filePaths;   // sorted frame file paths
    std::vector<sf::Texture> m_textures;    // loaded textures
    std::unique_ptr<sf::Sprite> m_sprite;
    int m_totalFrames = 0;
    int m_loadIdx = 0;          // next frame index to load
    int m_currentFrame = 0;
    float m_frameTimer = 0.f;
    bool m_loadingDone = false;
    bool m_done = false;

    void loadNextBatch(int count);

    static constexpr float FRAME_DELAY = 1.f / 24.f;
    static constexpr float WINDOW_WIDTH  = 2048.f;
    static constexpr float WINDOW_HEIGHT = 1536.f;
};
