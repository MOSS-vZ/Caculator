#include "ResultScreen.h"
#include "App.h"
#include "MainScreen.h"
#include <iostream>
#include <memory>
#include <cmath>

ResultScreen::ResultScreen(const sf::Image& resultImg, float accuracy, int wrongCount, int total, const std::string& wrongInfo)
    : m_resultImage(resultImg), m_accuracy(accuracy), m_wrongCount(wrongCount), m_totalQuestions(total), m_wrongInfo(wrongInfo) {
}

// ---- 初始化背景花纹 ----
void ResultScreen::initPatterns() {
    m_patterns.clear();

    struct ColorSpec {
        std::uint8_t r, g, b, a;
        float radius;
    };
    std::vector<ColorSpec> specs = {
        {180, 210, 255, 30, 500.f},
        {220, 180, 255, 25, 400.f},
        {255, 210, 220, 28, 600.f},
        {180, 255, 220, 22, 350.f},
        {255, 230, 180, 25, 450.f},
        {200, 200, 255, 20, 550.f},
    };

    for (const auto& spec : specs) {
        Pattern p;
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

        m_patterns.push_back(p);
    }
}

void ResultScreen::init() {
    // ---- 加载结果图片 ----
    m_resultTexture.loadFromImage(m_resultImage);
    m_resultSprite = std::make_unique<sf::Sprite>(m_resultTexture);
    float maxW = 1800.f, maxH = 1000.f;
    float scaleX = maxW / static_cast<float>(m_resultTexture.getSize().x);
    float scaleY = maxH / static_cast<float>(m_resultTexture.getSize().y);
    float scale = std::min(scaleX, scaleY);
    m_resultSprite->setScale(sf::Vector2f(scale, scale));
    m_resultSprite->setPosition(sf::Vector2f(
        (WINDOW_WIDTH - static_cast<float>(m_resultTexture.getSize().x) * scale) / 2.f,
        50.f
    ));

    // ---- 返回按钮（浅色风格） ----
    m_backButton.setRadius(40.f);
    m_backButton.setFillColor(sf::Color(200, 200, 200, 180));
    m_backButton.setOutlineColor(sf::Color(150, 150, 160));
    m_backButton.setOutlineThickness(2.f);
    m_backButton.setPosition(sf::Vector2f(20.f, 20.f));

    // ---- 加载字体 ----
    bool loaded = false;
    if (m_font.openFromFile("C:/Windows/Fonts/msyh.ttf")) loaded = true;
    if (!loaded && m_font.openFromFile("C:/Windows/Fonts/arial.ttf")) loaded = true;
    if (!loaded) std::cerr << "Font not loaded" << std::endl;

    if (loaded) {
        // ---- 统计信息文本（深色，在白色背景上清晰） ----
        m_statText = std::make_unique<sf::Text>(m_font);
        m_statText->setCharacterSize(44);
        m_statText->setFillColor(sf::Color(40, 40, 60));
        std::string statStr = "Accuracy: " + std::to_string(static_cast<int>(m_accuracy)) +
            "%   Wrong: " + std::to_string(m_wrongCount) +
            "/" + std::to_string(m_totalQuestions);
        m_statText->setString(statStr);
        m_statText->setPosition(sf::Vector2f(100.f, 1200.f));

        // ---- 错题详情 ----
        m_wrongText = std::make_unique<sf::Text>(m_font);
        m_wrongText->setCharacterSize(34);
        m_wrongText->setFillColor(sf::Color(200, 60, 60));
        m_wrongText->setString("Wrong IDs: " + m_wrongInfo);
        m_wrongText->setPosition(sf::Vector2f(100.f, 1280.f));
    }

    // ---- 初始化花纹 ----
    initPatterns();

    std::cout << "ResultScreen init done" << std::endl;
}

void ResultScreen::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (pressed->button == sf::Mouse::Button::Left) {
            m_mousePressed = true;
            m_mousePos = sf::Vector2f(
                static_cast<float>(pressed->position.x),
                static_cast<float>(pressed->position.y)
            );

            sf::Vector2f center = m_backButton.getPosition() + sf::Vector2f(40.f, 40.f);
            if (std::hypot(m_mousePos.x - center.x, m_mousePos.y - center.y) <= m_backButton.getRadius()) {
                std::cout << "Return to main menu" << std::endl;
                if (m_app) {
                    m_app->switchScreen(std::make_unique<MainScreen>());
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
}

void ResultScreen::update(float /*deltaTime*/) {
    // 更新花纹漂移
    for (auto& p : m_patterns) {
        sf::Vector2f pos = p.shape.getPosition();
        pos.x += p.velocity.x * 0.016f * 60.f;
        pos.y += p.velocity.y * 0.016f * 60.f;
        float r = p.shape.getRadius();
        if (pos.x < r) { pos.x = r; p.velocity.x = -p.velocity.x; }
        if (pos.x > WINDOW_WIDTH - r) { pos.x = WINDOW_WIDTH - r; p.velocity.x = -p.velocity.x; }
        if (pos.y < r) { pos.y = r; p.velocity.y = -p.velocity.y; }
        if (pos.y > WINDOW_HEIGHT - r) { pos.y = WINDOW_HEIGHT - r; p.velocity.y = -p.velocity.y; }
        p.shape.setPosition(pos);

        p.phase += 0.016f * p.radiusSpeed;
        float radiusVar = std::sin(p.phase) * 20.f;
        float newRadius = std::max(30.f, p.shape.getRadius() + radiusVar);
        p.shape.setRadius(newRadius);
        p.shape.setOrigin({ newRadius, newRadius });
    }
}

void ResultScreen::draw(sf::RenderWindow& window) {
    // ---- 白色背景 ----
    window.clear(sf::Color::White);

    // ---- 绘制花纹 ----
    for (const auto& p : m_patterns) {
        window.draw(p.shape);
    }

    // ---- 绘制结果图片 ----
    if (m_resultSprite) {
        window.draw(*m_resultSprite);
    }

    // ---- 返回按钮 ----
    window.draw(m_backButton);
    sf::ConvexShape arrow;
    arrow.setPointCount(3);
    sf::Vector2f basePos = m_backButton.getPosition() + sf::Vector2f(20.f, 20.f);
    arrow.setPoint(0, basePos + sf::Vector2f(30.f, 5.f));
    arrow.setPoint(1, basePos + sf::Vector2f(10.f, 20.f));
    arrow.setPoint(2, basePos + sf::Vector2f(30.f, 35.f));
    arrow.setFillColor(sf::Color(100, 100, 110));
    window.draw(arrow);

    // ---- 绘制统计信息 ----
    if (m_statText) window.draw(*m_statText);
    if (m_wrongText) window.draw(*m_wrongText);
}