#include "ResultScreen.h"
#include "App.h"
#include "MainScreen.h"
#include "ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>

// ========== background patterns ==========
void ResultScreen::initPatterns() {
    m_patterns.clear();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> xDist(100.f, WINDOW_WIDTH - 100.f);
    std::uniform_real_distribution<float> yDist(100.f, WINDOW_HEIGHT - 100.f);
    std::uniform_real_distribution<float> rDist(80.f, 300.f);
    std::uniform_real_distribution<float> sDist(10.f, 40.f);
    std::uniform_int_distribution<int> aDist(20, 50);

    struct Spec { uint8_t r, g, b, a; };
    std::vector<Spec> specs = {
        {130, 210, 255, 30}, {220, 180, 255, 28}, {255, 210, 220, 30},
        {180, 255, 220, 25}, {255, 230, 180, 30}, {200, 200, 255, 28},
        {255, 190, 210, 25}, {190, 240, 255, 28}, {250, 210, 230, 30},
        {210, 250, 200, 25}
    };
    for (const auto& spec : specs) {
        Pattern p;
        float r = rDist(gen);
        p.shape.setRadius(r);
        p.shape.setFillColor(sf::Color(spec.r, spec.g, spec.b, static_cast<uint8_t>(aDist(gen))));
        p.shape.setOrigin(sf::Vector2f(r, r));
        p.shape.setPosition(sf::Vector2f(xDist(gen), yDist(gen)));
        float angle = static_cast<float>(gen()) / std::mt19937::max() * 6.2832f;
        float speed = sDist(gen);
        p.velocity = sf::Vector2f(std::cos(angle) * speed, std::sin(angle) * speed);
        p.phase = static_cast<float>(gen()) / std::mt19937::max() * 6.2832f;
        p.radiusSpeed = 0.2f + static_cast<float>(gen()) / std::mt19937::max() * 0.3f;
        m_patterns.push_back(p);
    }
}

// ========== Constructors ==========
ResultScreen::ResultScreen(const sf::Image& originalImg,
                           const sf::Image& resultImg,
                           const sf::Image& pieImg,
                           const sf::Image& trendImg) {
    addSlot("Original Image",      originalImg);
    addSlot("Grading Result",      resultImg);
    addSlot("Accuracy Distribution", pieImg);
    addSlot("History Trend",       trendImg);
}

ResultScreen::ResultScreen(const sf::Image& resultImg,
                           const sf::Image& pieImg,
                           const sf::Image& trendImg) {
    addSlot("Grading Result",        resultImg);
    addSlot("Accuracy Distribution", pieImg);
    addSlot("History Trend",         trendImg);
}

ResultScreen::ResultScreen(const sf::Image& resultImg) {
    addSlot("Grading Result", resultImg);
}

void ResultScreen::addSlot(const std::string& title, const sf::Image& img) {
    ImageSlot slot;
    slot.title = title;
    slot.image = img;
    slot.valid = (img.getSize().x > 0);
    m_slots.push_back(std::move(slot));
}

// ========== init ==========
void ResultScreen::init() {
    m_font.openFromFile("C:/Windows/Fonts/msyh.ttf") ||
        m_font.openFromFile("C:/Windows/Fonts/arial.ttf");

    initPatterns();

    // ---- Back button ----
    ResourceManager::loadTextureWithFallback(m_backTexture, "return.png");
    m_backSprite = std::make_unique<sf::Sprite>(m_backTexture);
    float backScale = 80.f / m_backTexture.getSize().x;
    m_backSprite->setScale(sf::Vector2f(backScale, backScale));
    m_backSprite->setPosition(sf::Vector2f(20.f, 20.f));

    // ---- Create titles / textures / sprites ----
    for (auto& slot : m_slots) {
        auto titleText = std::make_unique<sf::Text>(m_font);
        titleText->setString(slot.title);
        titleText->setCharacterSize(40);
        titleText->setFillColor(sf::Color(40, 40, 60));
        m_titles.push_back(std::move(titleText));

        if (slot.valid) {
            slot.texture.loadFromImage(slot.image);
            slot.sprite = std::make_unique<sf::Sprite>(slot.texture);
            auto texSize = slot.texture.getSize();
            float iw = static_cast<float>(texSize.x);
            float ih = static_cast<float>(texSize.y);
            float sc = std::min(1.0f, std::min(MAX_IMG_W / iw, MAX_IMG_H / ih));
            slot.sprite->setScale(sf::Vector2f(sc, sc));
            m_placeholders.push_back(nullptr);
        } else {
            auto ph = std::make_unique<sf::Text>(m_font);
            ph->setString("No Data");
            ph->setCharacterSize(30);
            ph->setFillColor(sf::Color(160, 160, 170));
            m_placeholders.push_back(std::move(ph));
        }
    }

    // ---- Scrollbar geometry (fixed) ----
    m_trackX = WINDOW_WIDTH - SCROLLBAR_W - 16.f;
    m_trackY = 80.f;
    m_trackH = WINDOW_HEIGHT - 100.f;

    m_scrollTrack.setSize(sf::Vector2f(SCROLLBAR_W, m_trackH));
    m_scrollTrack.setPosition(sf::Vector2f(m_trackX, m_trackY));
    m_scrollTrack.setFillColor(sf::Color(210, 210, 220, 120));
    m_scrollTrack.setOutlineColor(sf::Color(180, 180, 190, 80));
    m_scrollTrack.setOutlineThickness(1.f);

    m_scrollThumb.setSize(sf::Vector2f(SCROLLBAR_W - 2.f, 60.f));
    m_scrollThumb.setPosition(sf::Vector2f(m_trackX + 1.f, m_trackY));
    m_scrollThumb.setFillColor(sf::Color(160, 160, 180, 200));

    // ---- Zoom overlay (hidden by default) ----
    m_zoomOverlay.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    m_zoomOverlay.setFillColor(sf::Color(0, 0, 0, 160));
    m_zoomOverlay.setPosition(sf::Vector2f(0.f, 0.f));

    m_zoomCloseBtn.setRadius(28.f);
    m_zoomCloseBtn.setFillColor(sf::Color(80, 80, 80, 200));
    m_zoomCloseBtn.setOutlineColor(sf::Color(200, 200, 200, 180));
    m_zoomCloseBtn.setOutlineThickness(2.f);

    m_zoomTitleText = std::make_unique<sf::Text>(m_font);
    m_zoomTitleText->setCharacterSize(30);
    m_zoomTitleText->setFillColor(sf::Color(255, 255, 255, 230));

    m_zoomCloseX = std::make_unique<sf::Text>(m_font);
    m_zoomCloseX->setString("X");
    m_zoomCloseX->setCharacterSize(32);
    m_zoomCloseX->setFillColor(sf::Color(255, 255, 255, 230));

    m_hoverHighlight.setFillColor(sf::Color::Transparent);
    m_hoverHighlight.setOutlineColor(sf::Color(80, 140, 240, 180));
    m_hoverHighlight.setOutlineThickness(3.f);

    layoutContent();
    m_scrollOffset = 0.f;
    updateScrollThumb();

    std::cout << "ResultScreen init done, slots=" << m_slots.size()
              << " contentH=" << m_contentHeight << std::endl;
}

// ========== layout (preserved original paired-row logic) ==========
void ResultScreen::layoutContent() {
    float curY = 100.f;
    float centerX = WINDOW_WIDTH * 0.5f;

    const bool pairedRow = (m_slots.size() >= 2 && m_slots[0].valid && m_slots[1].valid);

    if (pairedRow) {
        float titleMaxH = 0.f;
        for (int ri = 0; ri < 2; ++ri) {
            if (ri < static_cast<int>(m_titles.size())) {
                sf::FloatRect tr = m_titles[ri]->getLocalBounds();
                if (tr.size.y > titleMaxH) titleMaxH = tr.size.y;
            }
        }

        float w0 = m_slots[0].sprite->getGlobalBounds().size.x;
        float h0 = m_slots[0].sprite->getGlobalBounds().size.y;
        float w1 = m_slots[1].sprite->getGlobalBounds().size.x;
        float h1 = m_slots[1].sprite->getGlobalBounds().size.y;

        float rowW = w0 + COL_GAP + w1;
        float rowStartX = (WINDOW_WIDTH - rowW) * 0.5f;
        float imagesY = curY + titleMaxH + TITLE_GAP;

        if (m_titles.size() > 0) {
            sf::FloatRect tr = m_titles[0]->getLocalBounds();
            m_titles[0]->setPosition(sf::Vector2f(rowStartX + w0 * 0.5f - tr.size.x * 0.5f, curY));
        }
        if (m_titles.size() > 1) {
            sf::FloatRect tr = m_titles[1]->getLocalBounds();
            m_titles[1]->setPosition(sf::Vector2f(rowStartX + w0 + COL_GAP + w1 * 0.5f - tr.size.x * 0.5f, curY));
        }

        m_slots[0].sprite->setPosition(sf::Vector2f(rowStartX, imagesY));
        m_slots[1].sprite->setPosition(sf::Vector2f(rowStartX + w0 + COL_GAP, imagesY));

        curY += titleMaxH + TITLE_GAP + std::max(h0, h1) + SLOT_GAP;

        for (size_t i = 0; i < 2 && i < m_slots.size(); ++i) {
            if (!m_slots[i].valid && i < m_placeholders.size() && m_placeholders[i]) {
                sf::FloatRect pr = m_placeholders[i]->getLocalBounds();
                m_placeholders[i]->setPosition(sf::Vector2f(centerX - pr.size.x * 0.5f, curY));
                curY += 80.f + SLOT_GAP;
            }
        }
    }

    // Remaining slots: vertical centering
    for (size_t i = pairedRow ? 2 : 0; i < m_slots.size(); ++i) {
        if (i < m_titles.size()) {
            sf::FloatRect tr = m_titles[i]->getLocalBounds();
            m_titles[i]->setPosition(sf::Vector2f(centerX - tr.size.x * 0.5f, curY));
            curY += tr.size.y + TITLE_GAP;
        }

        if (m_slots[i].valid) {
            sf::FloatRect sr = m_slots[i].sprite->getGlobalBounds();
            m_slots[i].sprite->setPosition(sf::Vector2f(centerX - sr.size.x * 0.5f, curY));
            curY += sr.size.y + SLOT_GAP;
        } else if (i < m_placeholders.size() && m_placeholders[i]) {
            sf::FloatRect pr = m_placeholders[i]->getLocalBounds();
            m_placeholders[i]->setPosition(sf::Vector2f(centerX - pr.size.x * 0.5f, curY));
            curY += 80.f + SLOT_GAP;
        }
    }

    m_contentHeight = curY;
    m_maxScroll = (m_contentHeight > WINDOW_HEIGHT) ? (m_contentHeight - WINDOW_HEIGHT) : 0.f;
}

// ========== clamp scroll offset and update thumb ==========
void ResultScreen::setScrollOffset(float newOffset) {
    m_scrollOffset = std::max(0.f, std::min(newOffset, m_maxScroll));
    updateScrollThumb();
}

// ========== update scrollbar thumb ==========
void ResultScreen::updateScrollThumb() {
    if (m_maxScroll <= 0.f) {
        m_scrollThumb.setSize(sf::Vector2f(SCROLLBAR_W - 2.f, m_trackH));
        m_scrollThumb.setPosition(sf::Vector2f(m_trackX + 1.f, m_trackY));
        return;
    }

    float visibleRatio = WINDOW_HEIGHT / m_contentHeight;
    float thumbH = std::max(40.f, visibleRatio * m_trackH);
    float maxThumbTravel = m_trackH - thumbH;
    float scrollRatio = m_scrollOffset / m_maxScroll;
    float thumbY = m_trackY + scrollRatio * maxThumbTravel;

    m_scrollThumb.setSize(sf::Vector2f(SCROLLBAR_W - 2.f, thumbH));
    m_scrollThumb.setPosition(sf::Vector2f(m_trackX + 1.f, thumbY));
}

// ========== hit-test thumb ==========
bool ResultScreen::isOnThumb(sf::Vector2f pos) const {
    return m_scrollThumb.getGlobalBounds().contains(pos);
}

// ========== mouse Y → scroll offset ==========
float ResultScreen::mouseYToScroll(float mouseY) const {
    if (m_maxScroll <= 0.f) return 0.f;

    float thumbH = m_scrollThumb.getGlobalBounds().size.y;
    float maxThumbTravel = m_trackH - thumbH;
    if (maxThumbTravel <= 0.f) return 0.f;

    float thumbCenterY = mouseY - thumbH * 0.5f;
    float ratio = (thumbCenterY - m_trackY) / maxThumbTravel;
    ratio = std::max(0.f, std::min(ratio, 1.f));
    return ratio * m_maxScroll;
}

// ========== hit-test all images ==========
ResultScreen::ImageHitResult ResultScreen::hitTestImages(sf::Vector2f pos) const {
    ImageHitResult result;
    sf::Vector2f sp(pos.x, pos.y + m_scrollOffset);  // appView → scroll-space
    for (size_t i = 0; i < m_slots.size(); ++i) {
        if (m_slots[i].valid && m_slots[i].sprite) {
            sf::FloatRect b = m_slots[i].sprite->getGlobalBounds();
            if (b.contains(sp)) {
                result.hit = true;
                result.texture = &m_slots[i].texture;
                result.title = m_slots[i].title;
                result.bounds = b;
                return result;
            }
        }
    }
    return result;
}

// ========== enter / exit zoom ==========
void ResultScreen::enterZoom(const sf::Texture* tex, const std::string& title) {
    if (!tex) return;
    m_zoomed = true;
    m_zoomTexture = tex;
    m_zoomTitle = title;

    m_zoomSprite = std::make_unique<sf::Sprite>(*tex);
    auto sz = tex->getSize();
    float iw = static_cast<float>(sz.x);
    float ih = static_cast<float>(sz.y);
    float maxW = WINDOW_WIDTH * 0.78f;
    float maxH = WINDOW_HEIGHT * 0.72f;
    float sc = std::min(maxW / iw, maxH / ih);
    m_zoomSprite->setScale(sf::Vector2f(sc, sc));

    sf::FloatRect gb = m_zoomSprite->getGlobalBounds();
    float imgX = (WINDOW_WIDTH - gb.size.x) * 0.5f;
    float imgY = (WINDOW_HEIGHT - gb.size.y) * 0.5f;
    m_zoomSprite->setPosition(sf::Vector2f(imgX, imgY));

    float btnCX = imgX + gb.size.x;
    float btnCY = imgY;
    m_zoomCloseBtn.setPosition(sf::Vector2f(btnCX - 28.f, btnCY - 28.f));

    sf::FloatRect xb = m_zoomCloseX->getLocalBounds();
    m_zoomCloseX->setOrigin(sf::Vector2f(xb.size.x * 0.5f, xb.size.y * 0.5f));
    m_zoomCloseX->setPosition(sf::Vector2f(btnCX, btnCY));

    m_zoomTitleText->setString(title);
    m_zoomTitleText->setPosition(sf::Vector2f(imgX, imgY - 38.f));
}

void ResultScreen::exitZoom() {
    m_zoomed = false;
    m_zoomTexture = nullptr;
    m_zoomSprite.reset();
}

// ========== events ==========
void ResultScreen::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    // ---- Mouse moved ----
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f newPos = window.mapPixelToCoords(moved->position);
        m_mousePos = newPos;

        if (m_zoomed) {
            sf::FloatRect cb = m_zoomCloseBtn.getGlobalBounds();
            m_zoomCloseHovered = cb.contains(newPos);
            m_zoomCloseBtn.setFillColor(m_zoomCloseHovered
                ? sf::Color(200, 60, 60, 220)
                : sf::Color(80, 80, 80, 200));
            return;
        }

        if (m_dragMode == DragMode::Thumb) {
            float newOffset = mouseYToScroll(newPos.y);
            setScrollOffset(newOffset);
        }

        m_isHoveringThumb = isOnThumb(newPos);
        m_scrollThumb.setFillColor(m_isHoveringThumb
            ? sf::Color(120, 120, 140, 240)
            : sf::Color(160, 160, 180, 200));

        ImageHitResult imgHit = hitTestImages(newPos);
        m_hasHoveredImage = imgHit.hit;
        if (m_hasHoveredImage) {
            m_hoverHighlight.setPosition(imgHit.bounds.position);
            m_hoverHighlight.setSize(imgHit.bounds.size);
        }
    }

    // ---- Mouse wheel ----
    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        if (!m_zoomed) {
            float newOff = m_scrollOffset - wheel->delta * 50.f;
            setScrollOffset(newOff);
        }
    }

    // ---- Mouse button pressed ----
    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (pressed->button == sf::Mouse::Button::Left) {
            m_mousePressed = true;
            sf::Vector2f clickPos = window.mapPixelToCoords(pressed->position);

            // ---- Zoom mode ----
            if (m_zoomed) {
                sf::FloatRect cb = m_zoomCloseBtn.getGlobalBounds();
                if (cb.contains(clickPos) || std::hypot(
                        clickPos.x - (m_zoomCloseBtn.getPosition().x + 28.f),
                        clickPos.y - (m_zoomCloseBtn.getPosition().y + 28.f))
                        <= m_zoomCloseBtn.getRadius() + 4.f) {
                    exitZoom(); return;
                }
                if (m_zoomSprite && !m_zoomSprite->getGlobalBounds().contains(clickPos)) {
                    exitZoom(); return;
                }
                return;
            }

            // ---- Normal mode ----
            if (m_backSprite->getGlobalBounds().contains(clickPos)) {
                if (m_app) m_app->switchScreen(std::make_unique<MainScreen>());
                return;
            }

            ImageHitResult imgHit = hitTestImages(clickPos);
            if (imgHit.hit) {
                enterZoom(imgHit.texture, imgHit.title);
                return;
            }

            if (isOnThumb(clickPos)) {
                m_dragMode = DragMode::Thumb;
                m_dragAnchorY = clickPos.y;
                m_dragAnchorOffset = m_scrollOffset;
                return;
            }

            sf::FloatRect trackBounds = m_scrollTrack.getGlobalBounds();
            if (trackBounds.contains(clickPos)) {
                float newOffset = mouseYToScroll(clickPos.y);
                setScrollOffset(newOffset);
                m_dragMode = DragMode::Thumb;
                m_dragAnchorY = clickPos.y;
                m_dragAnchorOffset = m_scrollOffset;
                return;
            }
        }
    }

    if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (released->button == sf::Mouse::Button::Left) {
            m_mousePressed = false;
            m_dragMode = DragMode::None;
        }
    }
}

// ========== update ==========
void ResultScreen::update(float dt) {
    for (auto& p : m_patterns) {
        sf::Vector2f pos = p.shape.getPosition();
        pos += p.velocity * dt;
        float r = p.shape.getRadius();
        if (pos.x < r) { pos.x = r; p.velocity.x = -p.velocity.x; }
        if (pos.x > WINDOW_WIDTH - r) { pos.x = WINDOW_WIDTH - r; p.velocity.x = -p.velocity.x; }
        if (pos.y < r) { pos.y = r; p.velocity.y = -p.velocity.y; }
        if (pos.y > WINDOW_HEIGHT - r) { pos.y = WINDOW_HEIGHT - r; p.velocity.y = -p.velocity.y; }
        p.shape.setPosition(pos);
        p.phase += dt * p.radiusSpeed;
    }
}

// ========== draw ==========
void ResultScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(250, 252, 255));

    for (const auto& p : m_patterns)
        window.draw(p.shape);

    //const sf::View& appView = window.getView();
    sf::View appView = window.getView();   // 复制一份，不保留引用

    // ---- Scrollable content ----
    sf::View scrollView = appView;
    scrollView.setCenter(sf::Vector2f(
        WINDOW_WIDTH * 0.5f,
        WINDOW_HEIGHT * 0.5f + m_scrollOffset));
    scrollView.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setView(scrollView);

    for (size_t i = 0; i < m_slots.size(); ++i) {
        if (i < m_titles.size())
            window.draw(*m_titles[i]);
        if (m_slots[i].valid)
            window.draw(*m_slots[i].sprite);
        else if (i < m_placeholders.size() && m_placeholders[i])
            window.draw(*m_placeholders[i]);
    }

    // ---- Fixed UI (restore app view) ----
    window.setView(appView);

    // Back button
    window.draw(*m_backSprite);

    // Hover highlight
    if (m_hasHoveredImage && !m_zoomed) {
        window.setView(scrollView);
        window.draw(m_hoverHighlight);
        window.setView(appView);
    }

    // Scrollbar — always visible
    window.draw(m_scrollTrack);
    window.draw(m_scrollThumb);

    // ---- Zoom overlay ----
    if (m_zoomed && m_zoomSprite) {
        window.draw(m_zoomOverlay);
        window.draw(*m_zoomSprite);
        window.draw(m_zoomCloseBtn);
        window.draw(*m_zoomCloseX);
        window.draw(*m_zoomTitleText);
    }
}
