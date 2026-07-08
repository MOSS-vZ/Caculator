#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <string>
#include <memory>
#include <vector>

class ResultScreen : public Screen {
public:
    ResultScreen(const sf::Image& resultImg, float accuracy, int wrongCount, int total, const std::string& wrongInfo);

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    sf::Image m_resultImage;
    sf::Texture m_resultTexture;
    std::unique_ptr<sf::Sprite> m_resultSprite;

    float m_accuracy;
    int m_wrongCount;
    int m_totalQuestions;
    std::string m_wrongInfo;

    // ---- UI ----
    sf::CircleShape m_backButton;
    sf::Font m_font;
    std::unique_ptr<sf::Text> m_statText;
    std::unique_ptr<sf::Text> m_wrongText;

    // ---- 背景花纹 ----
    struct Pattern {
        sf::CircleShape shape;
        sf::Vector2f velocity;
        float phase;
        float radiusSpeed;
    };
    std::vector<Pattern> m_patterns;
    void initPatterns();

    bool m_mousePressed = false;
    sf::Vector2f m_mousePos;

    static constexpr float WINDOW_WIDTH = 2048.f;
    static constexpr float WINDOW_HEIGHT = 1536.f;
};