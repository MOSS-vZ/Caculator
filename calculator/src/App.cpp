#include "App.h"
#include "SplashScreen.h"
#include "MainScreen.h"

App::App()
    : m_window(sf::VideoMode({ 2048, 1536 }), "Math Grading System", sf::Style::Default),
      m_view(sf::Vector2f(DESIGN_WIDTH / 2.f, DESIGN_HEIGHT / 2.f),
             sf::Vector2f(DESIGN_WIDTH, DESIGN_HEIGHT)) {
    m_window.setFramerateLimit(60);
    updateView();
}

void App::updateView() {
    sf::Vector2u winSize = m_window.getSize();
    float winAspect = static_cast<float>(winSize.x) / static_cast<float>(winSize.y);
    float designAspect = DESIGN_WIDTH / DESIGN_HEIGHT;

    // 始终保持设计宽高比，上下左右均匀留黑，界面居中
    float w = 1.f, h = 1.f;
    if (winAspect > designAspect) {
        w = designAspect / winAspect;   // 左右留黑
    } else {
        h = winAspect / designAspect;   // 上下留黑
    }
    m_view.setViewport(sf::FloatRect({ (1.f - w) / 2.f, (1.f - h) / 2.f }, { w, h }));
}

void App::run() {
    auto splash = std::make_unique<SplashScreen>();
    splash->setApp(this);
    splash->init();
    m_currentScreen = std::move(splash);

    while (m_window.isOpen()) {
        while (const std::optional event = m_window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                m_window.close();
            if (const auto* resized = event->getIf<sf::Event::Resized>())
                updateView();
            if (m_currentScreen)
                m_currentScreen->handleEvent(*event, m_window);
        }

        float dt = m_clock.restart().asSeconds();
        if (m_currentScreen)
            m_currentScreen->update(dt);

        m_window.clear(sf::Color::Black);
        m_window.setView(m_view);
        if (m_currentScreen)
            m_currentScreen->draw(m_window);
        m_window.display();
    }
}

void App::switchScreen(std::unique_ptr<Screen> newScreen) {
    m_currentScreen = std::move(newScreen);
    if (m_currentScreen) {
        m_currentScreen->setApp(this);
        m_currentScreen->init();
    }
}