#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

class HistoryScreen : public Screen {
public:
    HistoryScreen();

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct HistoryEntry {
        std::string timestamp;
        std::string inputPath;
        std::string outputPath;
        sf::Image  inputImage;
        sf::Image  outputImage;
        sf::Texture inputTexture;
        sf::Texture outputTexture;
        std::unique_ptr<sf::Sprite> inputSprite;
        std::unique_ptr<sf::Sprite> outputSprite;
        bool inputValid  = false;
        bool outputValid = false;
    };

    struct TrendSlot {
        sf::Image  image;
        sf::Texture texture;
        std::unique_ptr<sf::Sprite> sprite;
        bool valid = false;
    };

    enum class DragMode { None, Thumb };

    // ---- Background patterns ----
    struct Pattern {
        sf::CircleShape shape;
        sf::Vector2f    velocity;
        float           phase = 0.f;
        float           radiusSpeed = 0.f;
    };
    std::vector<Pattern> m_patterns;
    void initPatterns();

    std::vector<HistoryEntry> m_entries;
    TrendSlot m_trend;

    // ---- Fixed UI ----
    sf::Font m_font;
    sf::Texture m_backTexture;
    std::unique_ptr<sf::Sprite> m_backSprite;
    std::unique_ptr<sf::Text> m_fixedTitle;
    sf::RectangleShape m_titleBg;

    // ---- Scrollable content ----
    std::unique_ptr<sf::Text> m_trendTitle;
    std::vector<std::unique_ptr<sf::Text>> m_entryTitles;
    std::vector<std::unique_ptr<sf::Text>> m_entryInputLabels;
    std::vector<std::unique_ptr<sf::Text>> m_entryOutputLabels;

    // ---- Scroll state ----
    float m_scrollOffset = 0.f;       // current scroll position [0, m_maxScroll]
    float m_maxScroll = 0.f;          // max scrollable distance
    float m_contentHeight = 0.f;      // total content height

    // ---- Drag state ----
    DragMode m_dragMode = DragMode::None;
    float m_dragAnchorY = 0.f;        // mouse Y where drag started
    float m_dragAnchorOffset = 0.f;   // scrollOffset at drag start

    // ---- Scrollbar ----
    sf::RectangleShape m_scrollTrack;
    sf::RectangleShape m_scrollThumb;
    bool m_isHoveringThumb = false;

    // ---- Mouse ----
    sf::Vector2f m_mousePos;
    bool m_mousePressed = false;

    // ---- Scrollbar geometry (computed once) ----
    float m_trackX = 0.f;
    float m_trackY = 0.f;
    float m_trackH = 0.f;

    // ---- Image hover / zoom ----
    struct ImageHitResult {
        bool hit = false;
        const sf::Texture* texture = nullptr;
        std::string title;
        sf::FloatRect bounds;
    };
    bool m_zoomed = false;
    bool m_zoomCloseHovered = false;
    const sf::Texture* m_zoomTexture = nullptr;
    std::string m_zoomTitle;
    sf::RectangleShape m_zoomOverlay;
    std::unique_ptr<sf::Sprite> m_zoomSprite;
    sf::CircleShape m_zoomCloseBtn;
    std::unique_ptr<sf::Text> m_zoomTitleText;
    std::unique_ptr<sf::Text> m_zoomCloseX;

    sf::RectangleShape m_hoverHighlight;
    bool m_hasHoveredImage = false;

    void scanHistory();
    void layoutContent();
    void updateScrollThumb();
    void setScrollOffset(float newOffset);

    bool isOnThumb(sf::Vector2f pos) const;
    float mouseYToScroll(float mouseY) const;
    ImageHitResult hitTestImages(sf::Vector2f pos) const;
    void enterZoom(const sf::Texture* tex, const std::string& title);
    void exitZoom();

    static constexpr float WINDOW_WIDTH   = 2048.f;
    static constexpr float WINDOW_HEIGHT  = 1536.f;
    static constexpr float MAX_IMG_W      = 750.f;
    static constexpr float MAX_IMG_H      = 550.f;
    static constexpr float TREND_MAX_W    = 1100.f;
    static constexpr float TREND_MAX_H    = 500.f;
    static constexpr float SLOT_GAP       = 50.f;
    static constexpr float TITLE_GAP      = 20.f;
    static constexpr float COL_GAP        = 40.f;
    static constexpr float SCROLLBAR_W    = 14.f;
    static constexpr float FIXED_HEADER_H = 100.f;
};
