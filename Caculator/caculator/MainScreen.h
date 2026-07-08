#pragma once
#include "Screen.h"
#include "Button.h"
#include <memory>
#include <vector>

class MainScreen : public Screen {
public:
    MainScreen();

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    // ========== 尺寸参数 ==========
    static constexpr float WINDOW_WIDTH = 2048.f;
    static constexpr float WINDOW_HEIGHT = 1536.f;

    static constexpr float COMPUTER_SCALE = 0.6f;
    static constexpr float WORD_SCALE = 0.35f;
    static constexpr float BUTTON_SCALE = 0.3f;
    static constexpr float BUTTON_HOVER_SCALE = 0.33f;   // 悬停放大到 1.1 倍

    // ===== 坐标 =====
    static constexpr float COMPUTER_X = 900.f;
    static constexpr float COMPUTER_Y = 300.f;
    static constexpr float WORD_X = 150.f;
    static constexpr float WORD_Y = 200.f;
    static constexpr float BTN_START_X = 130.f;
    static constexpr float BTN_START_Y = 900.f;
    static constexpr float BTN_PHOTO_X = 135.f;
    static constexpr float BTN_PHOTO_Y = 590.f;

    // ========== 动画相关 ==========
    sf::Clock m_animClock;
    float m_animDuration = 2.0f;      // 总动画时长（秒）
    bool m_animFinished = false;

    // 缓动函数：ease-out 平方
    float easeOut(float t) const;

    // ========== 背景花纹 ==========
    struct Pattern {
        sf::CircleShape shape;
        sf::Vector2f velocity;
        float phase;           // 用于呼吸效果
        float radiusSpeed;     // 半径变化速度
    };
    std::vector<Pattern> m_patterns;
    void initPatterns();

    // ========== 鼠标状态 ==========
    sf::Vector2f m_mousePos;
    bool m_isStartHovered = false;
    bool m_isPhotoHovered = false;

    // ========== UI 元素 ==========
    sf::Sprite m_computer;
    sf::Sprite m_word;
    std::unique_ptr<Button> m_btnStartPractice;
    std::unique_ptr<Button> m_btnTakePhoto;

    bool m_mousePressed = false;
};