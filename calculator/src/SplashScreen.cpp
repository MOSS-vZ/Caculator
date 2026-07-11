#include "SplashScreen.h"
#include "App.h"
#include "MainScreen.h"
#include <iostream>
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

SplashScreen::SplashScreen() {}

void SplashScreen::loadNextBatch(int count) {
    int end = std::min(m_loadIdx + count, m_totalFrames);
    for (; m_loadIdx < end; ++m_loadIdx) {
        sf::Texture tex;
        if (tex.loadFromFile(m_filePaths[m_loadIdx]))
            m_textures.push_back(std::move(tex));
    }
    if (m_loadIdx >= m_totalFrames) {
        m_loadingDone = true;
        std::cout << "Splash: all " << m_textures.size() << " frames loaded" << std::endl;
    }
}

void SplashScreen::init() {
    // Try multiple paths for the frames directory
    std::vector<std::string> dirPaths = {
        "assets/splash",
        "../assets/splash",
        "images/output",
        fs::current_path().string() + "/assets/splash"
    };
    std::string framesDir;
    for (const auto& p : dirPaths) {
        if (fs::exists(p)) { framesDir = p; break; }
    }
    if (framesDir.empty()) {
        std::cerr << "Splash: no frames directory found" << std::endl;
        m_done = true;
        return;
    }

    // Collect and sort frame file paths
    for (const auto& entry : fs::directory_iterator(framesDir)) {
        if (entry.is_regular_file())
            m_filePaths.push_back(entry.path().string());
    }
    std::sort(m_filePaths.begin(), m_filePaths.end());
    m_totalFrames = static_cast<int>(m_filePaths.size());

    if (m_totalFrames == 0) {
        std::cerr << "Splash: no frames found" << std::endl;
        m_done = true;
        return;
    }

    std::cout << "Splash: " << m_totalFrames << " frames, loading first batch..." << std::endl;

    // Load first few frames immediately so we can show something
    loadNextBatch(10);

    if (m_textures.empty()) {
        m_done = true;
        return;
    }

    // Set up sprite with first frame
    m_sprite = std::make_unique<sf::Sprite>(m_textures[0]);
    auto sz = m_textures[0].getSize();
    float sc = std::min(WINDOW_WIDTH / static_cast<float>(sz.x),
                        WINDOW_HEIGHT / static_cast<float>(sz.y));
    m_sprite->setScale(sf::Vector2f(sc, sc));
    sf::FloatRect gb = m_sprite->getGlobalBounds();
    m_sprite->setPosition(sf::Vector2f(
        (WINDOW_WIDTH  - gb.size.x) * 0.5f,
        (WINDOW_HEIGHT - gb.size.y) * 0.5f));
}

void SplashScreen::handleEvent(const sf::Event& event, const sf::RenderWindow&) {
    if (m_done) return;
    if (event.getIf<sf::Event::MouseButtonPressed>() ||
        event.getIf<sf::Event::KeyPressed>()) {
        m_done = true;
    }
}

void SplashScreen::update(float deltaTime) {
    if (m_done) {
        if (m_app) m_app->switchScreen(std::make_unique<MainScreen>());
        return;
    }

    // Keep loading frames in background
    if (!m_loadingDone) {
        loadNextBatch(5);
    }

    if (m_textures.empty()) return;

    // Advance animation
    int loadedCount = static_cast<int>(m_textures.size());
    m_frameTimer += deltaTime;
    while (m_frameTimer >= FRAME_DELAY && m_currentFrame < loadedCount - 1) {
        m_frameTimer -= FRAME_DELAY;
        ++m_currentFrame;
        m_sprite->setTexture(m_textures[m_currentFrame], true);
    }

    // Wait for loading to finish, then auto-end after last frame
    if (m_loadingDone && m_currentFrame >= m_totalFrames - 1) {
        if (m_frameTimer >= FRAME_DELAY * 3.f) {
            m_done = true;
        }
    }
}

void SplashScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color::Black);
    if (m_sprite)
        window.draw(*m_sprite);
}
