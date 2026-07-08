#include "MainScreen.h"
#include "ResourceManager.h"
#include "CaptureScreen.h"
#include "PracticeScreen.h"   // 新增：用于开始练习跳转
#include "App.h"
#include <iostream>
#include <cmath>
#include <cstdint>

// ---------- Constructor ----------
MainScreen::MainScreen()
    : m_computer(ResourceManager::getInstance().getTexture("computer.png")),
    m_word(ResourceManager::getInstance().getTexture("word.png")) {
    m_computer.setColor(sf::Color(255, 255, 255, 0));
    m_word.setColor(sf::Color(255, 255, 255, 0));
}

// ---------- Easing function ----------
float MainScreen::easeOut(float t) const {
    return 1.f - (1.f - t) * (1.f - t);
}

// ---------- Initialize background patterns ----------
void MainScreen::initPatterns() {
    m_patterns.clear();

    struct ColorSpec {
        std::uint8_t r, g, b, a;
        float radius;
    };
    std::vector<ColorSpec> specs = {
        {180, 210, 255, 40, 500.f},
        {220, 180, 255, 35, 400.f},
        {255, 210, 220, 38, 600.f},
        {180, 255, 220, 32, 350.f},
        {255, 230, 180, 35, 450.f},
        {200, 200, 255, 30, 550.f},
    };

    for (size_t i = 0; i < specs.size(); ++i) {
        Pattern p;
        const auto& spec = specs[i];
        float x = 100.f + static_cast<float>(rand() % 1800);
        float y = 100.f + static_cast<float>(rand() % 1300);
        p.shape.setPosition({ x, y });
        p.shape.setRadius(spec.radius);
        p.shape.setFillColor(sf::Color(spec.r, spec.g, spec.b, spec.a));
        p.shape.setOrigin({ spec.radius, spec.radius });

        float angle = static_cast<float>(rand()) / RAND_MAX * 6.2832f;
        float speed = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.6f;
        p.velocity = { std::cos(angle) * speed, std::sin(angle) * speed };

        p.phase = static_cast<float>(rand()) / RAND_MAX * 6.2832f;
        p.radiusSpeed = 0.2f + static_cast<float>(rand()) / RAND_MAX * 0.3f;

        m_patterns.push_back(std::move(p));
    }
}

// ---------- Initialization ----------
void MainScreen::init() {
    auto& rm = ResourceManager::getInstance();

    m_computer.setPosition({ COMPUTER_X, COMPUTER_Y });
    m_computer.setScale({ COMPUTER_SCALE, COMPUTER_SCALE });

    m_word.setPosition({ WORD_X, WORD_Y });
    m_word.setScale({ WORD_SCALE, WORD_SCALE });

    const sf::Texture& texStart = rm.getTexture("startpractice.png");
    const sf::Texture& texPhoto = rm.getTexture("takephoto.png");

    m_btnStartPractice = std::make_unique<Button>(texStart, BTN_START_X, BTN_START_Y);
    m_btnStartPractice->setScale(BUTTON_SCALE, BUTTON_SCALE);
    m_btnStartPractice->setColor(sf::Color(255, 255, 255, 0));

    m_btnTakePhoto = std::make_unique<Button>(texPhoto, BTN_PHOTO_X, BTN_PHOTO_Y);
    m_btnTakePhoto->setScale(BUTTON_SCALE, BUTTON_SCALE);
    m_btnTakePhoto->setColor(sf::Color(255, 255, 255, 0));

    initPatterns();

    m_animClock.restart();
    m_animFinished = false;

    std::cout << "MainScreen init done (with animations)" << std::endl;
}

// ---------- Event handling ----------
void MainScreen::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    // Mouse move: update position first
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        m_mousePos = { static_cast<float>(moved->position.x),
                      static_cast<float>(moved->position.y) };
    }

    // Mouse button pressed
    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (pressed->button == sf::Mouse::Button::Left) {
            m_mousePressed = true;

            // ---- 开始练习按钮 ----
            if (m_btnStartPractice && m_btnStartPractice->isHovered(m_mousePos)) {
                std::cout << "Start Practice button clicked!" << std::endl;
                if (m_app) {
                    m_app->switchScreen(std::make_unique<PracticeScreen>());  // 跳转到练习界面
                }
                return;
            }

            // ---- 拍照搜题按钮 ----
            if (m_btnTakePhoto && m_btnTakePhoto->isHovered(m_mousePos)) {
                std::cout << "Take Photo button clicked!" << std::endl;
                if (m_app) {
                    m_app->switchScreen(std::make_unique<CaptureScreen>());   // 跳转到拍照界面
                }
                return;
            }
        }
    }

    if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (released->button == sf::Mouse::Button::Left) {
            m_mousePressed = false;
        }
    }

    // Press R key to reset animation (for debugging)
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        if (keyPressed->scancode == sf::Keyboard::Scancode::R) {
            m_animClock.restart();
            m_animFinished = false;
            std::cout << "Animation reset" << std::endl;
        }
    }
}

// ---------- Update ----------
void MainScreen::update(float deltaTime) {
    float elapsed = m_animClock.getElapsedTime().asSeconds();
    float progress = std::min(elapsed / m_animDuration, 1.0f);

    if (progress >= 1.0f && !m_animFinished) {
        m_animFinished = true;
        std::cout << "Animation finished!" << std::endl;
    }

    if (!m_animFinished) {
        float fadeProgress = std::min(progress / 0.7f, 1.0f);
        float fadeAlpha = easeOut(fadeProgress) * 255.f;
        std::uint8_t alpha = static_cast<std::uint8_t>(fadeAlpha);

        m_computer.setColor(sf::Color(255, 255, 255, alpha));
        m_word.setColor(sf::Color(255, 255, 255, alpha));

        float btn1Progress = (progress - 0.4f) / 0.6f;
        btn1Progress = std::max(0.f, std::min(btn1Progress, 1.f));
        float btn1Alpha = easeOut(btn1Progress) * 255.f;
        float btn1Y = BTN_START_Y - 300.f * (1.f - easeOut(btn1Progress));

        m_btnStartPractice->setPosition(BTN_START_X, btn1Y);
        m_btnStartPractice->setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(btn1Alpha)));

        float btn2Progress = (progress - 0.2f) / 0.6f;
        btn2Progress = std::max(0.f, std::min(btn2Progress, 1.f));
        float btn2Alpha = easeOut(btn2Progress) * 255.f;
        float btn2Y = BTN_PHOTO_Y - 300.f * (1.f - easeOut(btn2Progress));

        m_btnTakePhoto->setPosition(BTN_PHOTO_X, btn2Y);
        m_btnTakePhoto->setColor(sf::Color(255, 255, 255, static_cast<std::uint8_t>(btn2Alpha)));

        m_btnStartPractice->setScale(BUTTON_SCALE, BUTTON_SCALE);
        m_btnTakePhoto->setScale(BUTTON_SCALE, BUTTON_SCALE);
    }
    else {
        m_computer.setColor(sf::Color::White);
        m_word.setColor(sf::Color::White);
        m_btnStartPractice->setPosition(BTN_START_X, BTN_START_Y);
        m_btnStartPractice->setColor(sf::Color::White);
        m_btnTakePhoto->setPosition(BTN_PHOTO_X, BTN_PHOTO_Y);
        m_btnTakePhoto->setColor(sf::Color::White);

        bool startHover = m_btnStartPractice->isHovered(m_mousePos);
        bool photoHover = m_btnTakePhoto->isHovered(m_mousePos);

        if (startHover) {
            m_btnStartPractice->setScale(BUTTON_HOVER_SCALE, BUTTON_HOVER_SCALE);
        }
        else {
            m_btnStartPractice->setScale(BUTTON_SCALE, BUTTON_SCALE);
        }

        if (photoHover) {
            m_btnTakePhoto->setScale(BUTTON_HOVER_SCALE, BUTTON_HOVER_SCALE);
        }
        else {
            m_btnTakePhoto->setScale(BUTTON_SCALE, BUTTON_SCALE);
        }
    }

    // Update background patterns
    for (auto& p : m_patterns) {
        sf::Vector2f pos = p.shape.getPosition();
        pos.x += p.velocity.x * deltaTime * 60.f;
        pos.y += p.velocity.y * deltaTime * 60.f;
        float r = p.shape.getRadius();
        if (pos.x < r) { pos.x = r; p.velocity.x = -p.velocity.x; }
        if (pos.x > WINDOW_WIDTH - r) { pos.x = WINDOW_WIDTH - r; p.velocity.x = -p.velocity.x; }
        if (pos.y < r) { pos.y = r; p.velocity.y = -p.velocity.y; }
        if (pos.y > WINDOW_HEIGHT - r) { pos.y = WINDOW_HEIGHT - r; p.velocity.y = -p.velocity.y; }
        p.shape.setPosition(pos);

        p.phase += deltaTime * p.radiusSpeed;
    }
}

// ---------- Drawing ----------
void MainScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color::White);

    for (const auto& p : m_patterns) {
        window.draw(p.shape);
    }

    window.draw(m_computer);
    window.draw(m_word);

    if (m_btnStartPractice) {
        m_btnStartPractice->draw(window);
    }
    if (m_btnTakePhoto) {
        m_btnTakePhoto->draw(window);
    }
}