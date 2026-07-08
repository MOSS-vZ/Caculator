#include "App.h"
#include "MainScreen.h"

App::App()
    : m_window(sf::VideoMode({ 2048, 1536 }), "Math Grading System") {
    m_window.setFramerateLimit(60);
}

void App::run() {
    auto mainScreen = std::make_unique<MainScreen>();
    mainScreen->setApp(this);
    mainScreen->init();
    m_currentScreen = std::move(mainScreen);

    while (m_window.isOpen()) {
        while (const std::optional event = m_window.pollEvent()) {
            if (event->is<sf::Event::Closed>())
                m_window.close();
            if (m_currentScreen)
                m_currentScreen->handleEvent(*event, m_window);
        }

        float dt = m_clock.restart().asSeconds();
        if (m_currentScreen)
            m_currentScreen->update(dt);

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