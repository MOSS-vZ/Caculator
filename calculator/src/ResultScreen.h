#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <string>
#include <vector>

class ResultScreen : public Screen {
public:
    ResultScreen(const sf::Image& originalImg,
                 const sf::Image& resultImg,
                 const sf::Image& pieImg,
                 const sf::Image& trendImg);
    ResultScreen(const sf::Image& resultImg,
                 const sf::Image& pieImg,
                 const sf::Image& trendImg);
    ResultScreen(const sf::Image& resultImg);

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    struct ImageSlot {
        std::string title;
        sf::Image  image;
        sf::Texture texture;
        std::unique_ptr<sf::Sprite> sprite;
        bool        valid = false;
    };
    std::vector<ImageSlot> m_slots;

    enum class DragMode { None, Thumb };

    struct Pattern {
        sf::CircleShape shape;
        sf::Vector2f    velocity;
        float           phase = 0.f;
        float           radiusSpeed = 0.f;
    };
    std::vector<Pattern> m_patterns;
    void initPatterns();

    sf::Font m_font;
    std::vector<std::unique_ptr<sf::Text>> m_titles;
    std::vector<std::unique_ptr<sf::Text>> m_placeholders;
    sf::Texture m_backTexture;
    std::unique_ptr<sf::Sprite> m_backSprite;

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

    // ---- Scrollbar geometry (fixed) ----
    float m_trackX = 0.f;
    float m_trackY = 0.f;
    float m_trackH = 0.f;

    // ---- Mouse ----
    sf::Vector2f m_mousePos;
    bool m_mousePressed = false;

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

    void layoutContent();
    void updateScrollThumb();
    void setScrollOffset(float newOffset);
    bool isOnThumb(sf::Vector2f pos) const;
    float mouseYToScroll(float mouseY) const;
    ImageHitResult hitTestImages(sf::Vector2f pos) const;
    void enterZoom(const sf::Texture* tex, const std::string& title);
    void exitZoom();
    void addSlot(const std::string& title, const sf::Image& img);

    static constexpr float WINDOW_WIDTH  = 2048.f;
    static constexpr float WINDOW_HEIGHT = 1536.f;
    static constexpr float MAX_IMG_W     = 900.f;
    static constexpr float MAX_IMG_H     = 700.f;
    static constexpr float SLOT_GAP      = 60.f;
    static constexpr float TITLE_GAP     = 20.f;
    static constexpr float COL_GAP       = 40.f;
    static constexpr float SCROLLBAR_W   = 14.f;
};
