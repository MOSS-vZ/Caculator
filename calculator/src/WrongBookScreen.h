#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

class WrongBookScreen : public Screen {
public:
    WrongBookScreen();

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    // A single wrong question
    struct WrongEntry {
        int         questionNumber = 0;
        std::string studentAnswer;
        std::string correctAnswer;
        std::string imagePath;
        sf::Image   image;
        sf::Texture texture;
        std::unique_ptr<sf::Sprite> sprite;
        bool valid = false;
    };

    // A batch of wrong questions from one grading session
    struct WrongBatch {
        std::string timestamp;               // "2026-07-11 10:42:50"
        std::string batchId;                 // "20260711_104250"
        std::vector<WrongEntry> entries;     // sorted by question number
    };

    std::vector<WrongBatch> m_batches;

    enum class DragMode { None, Thumb };

    struct Pattern {
        sf::CircleShape shape;
        sf::Vector2f    velocity;
        float           phase = 0.f;
        float           radiusSpeed = 0.f;
    };
    std::vector<Pattern> m_patterns;
    void initPatterns();

    // ---- Fixed UI ----
    sf::Font m_font;
    sf::Texture m_backTexture;
    std::unique_ptr<sf::Sprite> m_backSprite;
    std::unique_ptr<sf::Text> m_fixedTitle;
    sf::RectangleShape m_titleBg;

    // ---- Scrollable content ----
    std::vector<std::unique_ptr<sf::Text>> m_batchTitles;       // per-batch timestamp
    std::vector<std::unique_ptr<sf::Text>> m_questionNumbers;   // per-entry "Question N"
    std::vector<std::unique_ptr<sf::Text>> m_studentAnswers;    // per-entry "Your Answer: X" (red)
    std::vector<std::unique_ptr<sf::Text>> m_correctAnswers;    // per-entry "Correct Answer: Y" (green)

    // ---- Scroll state ----
    float m_scrollOffset = 0.f;
    float m_maxScroll = 0.f;
    float m_contentHeight = 0.f;

    // ---- Drag state ----
    DragMode m_dragMode = DragMode::None;
    float m_dragAnchorY = 0.f;
    float m_dragAnchorOffset = 0.f;

    // ---- Scrollbar ----
    sf::RectangleShape m_scrollTrack;
    sf::RectangleShape m_scrollThumb;
    bool m_isHoveringThumb = false;
    float m_trackX = 0.f, m_trackY = 0.f, m_trackH = 0.f;

    // ---- Mouse ----
    sf::Vector2f m_mousePos;
    bool m_mousePressed = false;

    void scanWrongDir();
    void layoutContent();
    void updateScrollThumb();
    void setScrollOffset(float newOffset);
    bool isOnThumb(sf::Vector2f pos) const;
    float mouseYToScroll(float mouseY) const;

    static constexpr float WINDOW_WIDTH   = 2048.f;
    static constexpr float WINDOW_HEIGHT  = 1536.f;
    static constexpr float MAX_IMG_W      = 1600.f;
    static constexpr float MAX_IMG_H      = 800.f;
    static constexpr float SLOT_GAP       = 40.f;
    static constexpr float TITLE_GAP      = 16.f;
    static constexpr float BATCH_GAP      = 60.f;
    static constexpr float SCROLLBAR_W    = 14.f;
    static constexpr float FIXED_HEADER_H = 100.f;
};
