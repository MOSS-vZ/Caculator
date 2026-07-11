#include "HistoryScreen.h"
#include "App.h"
#include "MainScreen.h"
#include "ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <regex>
#include <random>

namespace fs = std::filesystem;

HistoryScreen::HistoryScreen() {}

// ========== scan photo/in and photo/out ==========
void HistoryScreen::scanHistory() {
    m_entries.clear();
    std::string inDir  = "photo/in";
    std::string outDir = "photo/out";

    if (!fs::exists(inDir)) return;

    std::vector<std::string> inputFiles;
    std::regex inputPattern(R"(input_(\d{8})_(\d{6})\.png)");
    std::smatch match;

    for (const auto& entry : fs::directory_iterator(inDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        if (std::regex_match(filename, match, inputPattern))
            inputFiles.push_back(filename);
    }

    std::sort(inputFiles.begin(), inputFiles.end(), std::greater<std::string>());

    for (const auto& filename : inputFiles) {
        if (!std::regex_match(filename, match, inputPattern)) continue;

        std::string dateStr = match[1].str();
        std::string timeStr = match[2].str();

        std::string displayTime =
            dateStr.substr(0, 4) + "-" + dateStr.substr(4, 2) + "-" + dateStr.substr(6, 2) +
            " " +
            timeStr.substr(0, 2) + ":" + timeStr.substr(2, 2) + ":" + timeStr.substr(4, 2);

        std::string suffix = dateStr + "_" + timeStr;

        HistoryEntry he;
        he.timestamp  = displayTime;
        he.inputPath  = inDir + "/" + filename;
        he.outputPath = outDir + "/output_" + suffix + ".png";

        he.inputValid = he.inputImage.loadFromFile(he.inputPath);
        if (fs::exists(he.outputPath))
            he.outputValid = he.outputImage.loadFromFile(he.outputPath);

        m_entries.push_back(std::move(he));
    }

    std::string trendPath = "photo/trend.png";
    if (fs::exists(trendPath))
        m_trend.valid = m_trend.image.loadFromFile(trendPath);

    std::cout << "HistoryScreen: " << m_entries.size() << " entries, trend="
              << (m_trend.valid ? "yes" : "no") << std::endl;
}

// ========== background patterns ==========
void HistoryScreen::initPatterns() {
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

// ========== init ==========
void HistoryScreen::init() {
    m_font.openFromFile("C:/Windows/Fonts/msyh.ttf") ||
        m_font.openFromFile("C:/Windows/Fonts/arial.ttf");

    scanHistory();
    initPatterns();

    // ---- Fixed header background ----
    m_titleBg.setSize(sf::Vector2f(WINDOW_WIDTH, FIXED_HEADER_H));
    m_titleBg.setFillColor(sf::Color(250, 250, 252, 240));
    m_titleBg.setPosition(sf::Vector2f(0.f, 0.f));

    // ---- Back button ----
    ResourceManager::loadTextureWithFallback(m_backTexture, "return.png");
    m_backSprite = std::make_unique<sf::Sprite>(m_backTexture);
    float backScale = 80.f / m_backTexture.getSize().x;
    m_backSprite->setScale(sf::Vector2f(backScale, backScale));
    m_backSprite->setPosition(sf::Vector2f(20.f, 20.f));

    // ---- Fixed title ----
    m_fixedTitle = std::make_unique<sf::Text>(m_font);
    m_fixedTitle->setString("History");
    m_fixedTitle->setCharacterSize(42);
    m_fixedTitle->setFillColor(sf::Color(40, 40, 60));
    sf::FloatRect tb = m_fixedTitle->getLocalBounds();
    m_fixedTitle->setOrigin(sf::Vector2f(tb.size.x * 0.5f, tb.size.y * 0.5f));
    m_fixedTitle->setPosition(sf::Vector2f(WINDOW_WIDTH * 0.5f, FIXED_HEADER_H * 0.5f));

    // ---- Trend section ----
    if (m_trend.valid) {
        m_trend.texture.loadFromImage(m_trend.image);
        m_trend.sprite = std::make_unique<sf::Sprite>(m_trend.texture);
        auto texSize = m_trend.texture.getSize();
        float iw = static_cast<float>(texSize.x);
        float ih = static_cast<float>(texSize.y);
        float sc = std::min(1.0f, std::min(TREND_MAX_W / iw, TREND_MAX_H / ih));
        m_trend.sprite->setScale(sf::Vector2f(sc, sc));

        m_trendTitle = std::make_unique<sf::Text>(m_font);
        m_trendTitle->setString("Accuracy Trend");
        m_trendTitle->setCharacterSize(36);
        m_trendTitle->setFillColor(sf::Color(60, 60, 80));
    }

    // ---- History entries ----
    for (auto& entry : m_entries) {
        if (entry.inputValid) {
            entry.inputTexture.loadFromImage(entry.inputImage);
            entry.inputSprite = std::make_unique<sf::Sprite>(entry.inputTexture);
            auto sz = entry.inputTexture.getSize();
            float sc = std::min(1.0f, std::min(
                MAX_IMG_W / static_cast<float>(sz.x),
                MAX_IMG_H / static_cast<float>(sz.y)));
            entry.inputSprite->setScale(sf::Vector2f(sc, sc));
        }
        if (entry.outputValid) {
            entry.outputTexture.loadFromImage(entry.outputImage);
            entry.outputSprite = std::make_unique<sf::Sprite>(entry.outputTexture);
            auto sz = entry.outputTexture.getSize();
            float sc = std::min(1.0f, std::min(
                MAX_IMG_W / static_cast<float>(sz.x),
                MAX_IMG_H / static_cast<float>(sz.y)));
            entry.outputSprite->setScale(sf::Vector2f(sc, sc));
        }

        auto title = std::make_unique<sf::Text>(m_font);
        title->setString(entry.timestamp);
        title->setCharacterSize(30);
        title->setFillColor(sf::Color(50, 50, 70));
        m_entryTitles.push_back(std::move(title));

        auto inLabel = std::make_unique<sf::Text>(m_font);
        inLabel->setString("Original");
        inLabel->setCharacterSize(22);
        inLabel->setFillColor(sf::Color(120, 120, 140));
        m_entryInputLabels.push_back(std::move(inLabel));

        auto outLabel = std::make_unique<sf::Text>(m_font);
        outLabel->setString("Graded");
        outLabel->setCharacterSize(22);
        outLabel->setFillColor(sf::Color(120, 120, 140));
        m_entryOutputLabels.push_back(std::move(outLabel));
    }

    // ---- Scrollbar geometry (fixed, computed once) ----
    m_trackX = WINDOW_WIDTH - SCROLLBAR_W - 16.f;
    m_trackY = FIXED_HEADER_H + 8.f;
    m_trackH = WINDOW_HEIGHT - FIXED_HEADER_H - 16.f;

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

    // Hover highlight
    m_hoverHighlight.setFillColor(sf::Color::Transparent);
    m_hoverHighlight.setOutlineColor(sf::Color(80, 140, 240, 180));
    m_hoverHighlight.setOutlineThickness(3.f);

    layoutContent();
    m_scrollOffset = 0.f;
    updateScrollThumb();

    std::cout << "HistoryScreen init done, entries=" << m_entries.size()
              << " contentH=" << m_contentHeight << " maxScroll=" << m_maxScroll << std::endl;
}

// ========== layout scrollable content ==========
void HistoryScreen::layoutContent() {
    float curY = FIXED_HEADER_H + 30.f;
    float centerX = WINDOW_WIDTH * 0.5f;

    // ---- Trend image ----
    if (m_trend.valid) {
        sf::FloatRect tr = m_trendTitle->getLocalBounds();
        m_trendTitle->setPosition(sf::Vector2f(centerX - tr.size.x * 0.5f, curY));
        curY += tr.size.y + TITLE_GAP;

        sf::FloatRect sr = m_trend.sprite->getGlobalBounds();
        m_trend.sprite->setPosition(sf::Vector2f(centerX - sr.size.x * 0.5f, curY));
        curY += sr.size.y + SLOT_GAP;
    }

    if (m_entries.empty()) {
        m_contentHeight = curY;
        m_maxScroll = 0.f;
        return;
    }

    // Measure label height from first label
    float labelH = 26.f;
    if (!m_entryInputLabels.empty())
        labelH = m_entryInputLabels[0]->getLocalBounds().size.y;
    if (labelH < 18.f) labelH = 26.f;

    for (size_t i = 0; i < m_entries.size(); ++i) {
        auto& entry = m_entries[i];
        bool hasInput  = entry.inputValid;
        bool hasOutput = entry.outputValid;
        bool paired    = hasInput && hasOutput;

        // Timestamp title (centered)
        if (i < m_entryTitles.size()) {
            sf::FloatRect tr = m_entryTitles[i]->getLocalBounds();
            m_entryTitles[i]->setPosition(sf::Vector2f(centerX - tr.size.x * 0.5f, curY));
            curY += tr.size.y + TITLE_GAP;
        }

        if (paired) {
            float w0 = entry.inputSprite->getGlobalBounds().size.x;
            float h0 = entry.inputSprite->getGlobalBounds().size.y;
            float w1 = entry.outputSprite->getGlobalBounds().size.x;
            float h1 = entry.outputSprite->getGlobalBounds().size.y;

            float rowW = w0 + COL_GAP + w1;
            float rowStartX = (WINDOW_WIDTH - rowW) * 0.5f;

            if (i < m_entryInputLabels.size()) {
                sf::FloatRect lr = m_entryInputLabels[i]->getLocalBounds();
                m_entryInputLabels[i]->setPosition(
                    sf::Vector2f(rowStartX + w0 * 0.5f - lr.size.x * 0.5f, curY));
            }
            if (i < m_entryOutputLabels.size()) {
                sf::FloatRect lr = m_entryOutputLabels[i]->getLocalBounds();
                m_entryOutputLabels[i]->setPosition(
                    sf::Vector2f(rowStartX + w0 + COL_GAP + w1 * 0.5f - lr.size.x * 0.5f, curY));
            }
            curY += labelH + 8.f;

            entry.inputSprite->setPosition(sf::Vector2f(rowStartX, curY));
            entry.outputSprite->setPosition(sf::Vector2f(rowStartX + w0 + COL_GAP, curY));
            curY += std::max(h0, h1) + SLOT_GAP;

        } else if (hasInput) {
            if (i < m_entryInputLabels.size()) {
                sf::FloatRect lr = m_entryInputLabels[i]->getLocalBounds();
                m_entryInputLabels[i]->setPosition(sf::Vector2f(centerX - lr.size.x * 0.5f, curY));
                curY += labelH + 8.f;
            }
            sf::FloatRect sr = entry.inputSprite->getGlobalBounds();
            entry.inputSprite->setPosition(sf::Vector2f(centerX - sr.size.x * 0.5f, curY));
            curY += sr.size.y + SLOT_GAP;

        } else if (hasOutput) {
            if (i < m_entryOutputLabels.size()) {
                sf::FloatRect lr = m_entryOutputLabels[i]->getLocalBounds();
                m_entryOutputLabels[i]->setPosition(sf::Vector2f(centerX - lr.size.x * 0.5f, curY));
                curY += labelH + 8.f;
            }
            sf::FloatRect sr = entry.outputSprite->getGlobalBounds();
            entry.outputSprite->setPosition(sf::Vector2f(centerX - sr.size.x * 0.5f, curY));
            curY += sr.size.y + SLOT_GAP;
        }
    }

    m_contentHeight = curY;
    m_maxScroll = (m_contentHeight > WINDOW_HEIGHT) ? (m_contentHeight - WINDOW_HEIGHT) : 0.f;
}

// ========== clamp scroll offset and update thumb ==========
void HistoryScreen::setScrollOffset(float newOffset) {
    m_scrollOffset = std::max(0.f, std::min(newOffset, m_maxScroll));
    updateScrollThumb();
}

// ========== update scrollbar thumb position & size ==========
void HistoryScreen::updateScrollThumb() {
    if (m_maxScroll <= 0.f) {
        // Content fits: thumb fills entire track
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

// ========== hit-test: is mouse on the thumb? ==========
bool HistoryScreen::isOnThumb(sf::Vector2f pos) const {
    return m_scrollThumb.getGlobalBounds().contains(pos);
}

// ========== map mouse Y (in track) → scroll offset ==========
float HistoryScreen::mouseYToScroll(float mouseY) const {
    if (m_maxScroll <= 0.f) return 0.f;

    float thumbH = m_scrollThumb.getGlobalBounds().size.y;
    float maxThumbTravel = m_trackH - thumbH;
    if (maxThumbTravel <= 0.f) return 0.f;

    // Click at center of thumb
    float thumbCenterY = mouseY - thumbH * 0.5f;
    float ratio = (thumbCenterY - m_trackY) / maxThumbTravel;
    ratio = std::max(0.f, std::min(ratio, 1.f));
    return ratio * m_maxScroll;
}

// ========== hit-test all images in the scrollable content ==========
HistoryScreen::ImageHitResult HistoryScreen::hitTestImages(sf::Vector2f pos) const {
    ImageHitResult result;
    // Convert from fixed appView coords to scroll-space
    sf::Vector2f sp(pos.x, pos.y + m_scrollOffset);

    // Check trend image
    if (m_trend.valid && m_trend.sprite) {
        sf::FloatRect b = m_trend.sprite->getGlobalBounds();
        if (b.contains(sp)) {
            result.hit = true;
            result.texture = &m_trend.texture;
            result.title = "Accuracy Trend";
            result.bounds = b;
            return result;
        }
    }

    // Check history entry images
    for (size_t i = 0; i < m_entries.size(); ++i) {
        auto& entry = m_entries[i];
        if (entry.inputValid && entry.inputSprite) {
            sf::FloatRect b = entry.inputSprite->getGlobalBounds();
            if (b.contains(sp)) {
                result.hit = true;
                result.texture = &entry.inputTexture;
                result.title = entry.timestamp + " - Original";
                result.bounds = b;
                return result;
            }
        }
        if (entry.outputValid && entry.outputSprite) {
            sf::FloatRect b = entry.outputSprite->getGlobalBounds();
            if (b.contains(sp)) {
                result.hit = true;
                result.texture = &entry.outputTexture;
                result.title = entry.timestamp + " - Graded";
                result.bounds = b;
                return result;
            }
        }
    }

    return result;
}

// ========== enter zoom mode ==========
void HistoryScreen::enterZoom(const sf::Texture* tex, const std::string& title) {
    if (!tex) return;
    m_zoomed = true;
    m_zoomTexture = tex;
    m_zoomTitle = title;

    // Create large sprite centered on screen
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

    // Close button: centered on the top-right corner of the image
    float btnCX = imgX + gb.size.x;
    float btnCY = imgY;
    m_zoomCloseBtn.setPosition(sf::Vector2f(btnCX - 28.f, btnCY - 28.f));

    // "X" text centered in close button
    sf::FloatRect xb = m_zoomCloseX->getLocalBounds();
    m_zoomCloseX->setOrigin(sf::Vector2f(xb.size.x * 0.5f, xb.size.y * 0.5f));
    m_zoomCloseX->setPosition(sf::Vector2f(btnCX, btnCY));

    // Title at top-left of image, outside the image
    m_zoomTitleText->setString(title);
    m_zoomTitleText->setPosition(sf::Vector2f(imgX, imgY - 38.f));
}

// ========== exit zoom mode ==========
void HistoryScreen::exitZoom() {
    m_zoomed = false;
    m_zoomTexture = nullptr;
    m_zoomSprite.reset();
}

// ========== events ==========
void HistoryScreen::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    // ---- Mouse moved ----
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f newPos = window.mapPixelToCoords(moved->position);
        m_mousePos = newPos;

        if (m_zoomed) {
            // Track close button hover
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

        // Thumb hover
        m_isHoveringThumb = isOnThumb(newPos);
        m_scrollThumb.setFillColor(m_isHoveringThumb
            ? sf::Color(120, 120, 140, 240)
            : sf::Color(160, 160, 180, 200));

        // Image hover → highlight
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

            // ---- Zoom mode clicks ----
            if (m_zoomed) {
                // Close button
                sf::FloatRect cb = m_zoomCloseBtn.getGlobalBounds();
                if (cb.contains(clickPos) || std::hypot(
                        clickPos.x - (m_zoomCloseBtn.getPosition().x + 28.f),
                        clickPos.y - (m_zoomCloseBtn.getPosition().y + 28.f))
                        <= m_zoomCloseBtn.getRadius() + 4.f) {
                    exitZoom();
                    return;
                }
                // Click outside the zoomed image → exit zoom
                if (m_zoomSprite) {
                    sf::FloatRect zb = m_zoomSprite->getGlobalBounds();
                    if (!zb.contains(clickPos)) {
                        exitZoom();
                        return;
                    }
                }
                return;
            }

            // ---- Normal mode clicks ----
            // 1) Back button
            if (m_backSprite->getGlobalBounds().contains(clickPos)) {
                if (m_app) m_app->switchScreen(std::make_unique<MainScreen>());
                return;
            }

            // 2) Image click → zoom
            ImageHitResult imgHit = hitTestImages(clickPos);
            if (imgHit.hit) {
                enterZoom(imgHit.texture, imgHit.title);
                return;
            }

            // 3) Scrollbar thumb → thumb drag
            if (isOnThumb(clickPos)) {
                m_dragMode = DragMode::Thumb;
                m_dragAnchorY = clickPos.y;
                m_dragAnchorOffset = m_scrollOffset;
                return;
            }

            // 4) Scrollbar track → jump then thumb drag
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

    // ---- Mouse button released ----
    if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (released->button == sf::Mouse::Button::Left) {
            m_mousePressed = false;
            m_dragMode = DragMode::None;
        }
    }
}

// ========== update ==========
void HistoryScreen::update(float dt) {
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
void HistoryScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(250, 252, 255));

    // Draw background patterns first (before any view change)
    for (const auto& p : m_patterns)
        window.draw(p.shape);

    //const sf::View& appView = window.getView();
    sf::View appView = window.getView();   // 复制一份，不保留引用

    // ---- Scrollable content view ----
    sf::View scrollView = appView;
    scrollView.setCenter(sf::Vector2f(
        WINDOW_WIDTH * 0.5f,
        WINDOW_HEIGHT * 0.5f + m_scrollOffset));
    scrollView.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setView(scrollView);

    // Draw trend
    if (m_trend.valid) {
        window.draw(*m_trendTitle);
        window.draw(*m_trend.sprite);
    }

    // Draw history entries
    for (size_t i = 0; i < m_entries.size(); ++i) {
        if (i < m_entryTitles.size())
            window.draw(*m_entryTitles[i]);

        auto& entry = m_entries[i];

        if (entry.inputValid && entry.outputValid) {
            if (i < m_entryInputLabels.size())
                window.draw(*m_entryInputLabels[i]);
            if (i < m_entryOutputLabels.size())
                window.draw(*m_entryOutputLabels[i]);
            window.draw(*entry.inputSprite);
            window.draw(*entry.outputSprite);
        } else if (entry.inputValid) {
            if (i < m_entryInputLabels.size())
                window.draw(*m_entryInputLabels[i]);
            window.draw(*entry.inputSprite);
        } else if (entry.outputValid) {
            if (i < m_entryOutputLabels.size())
                window.draw(*m_entryOutputLabels[i]);
            window.draw(*entry.outputSprite);
        }
    }

    // ---- Fixed UI (restore app view) ----
    window.setView(appView);

    window.draw(m_titleBg);
    window.draw(*m_fixedTitle);

    // Back button
    window.draw(*m_backSprite);

    // Hover highlight on images (drawn in scroll view)
    if (m_hasHoveredImage && !m_zoomed) {
        window.setView(scrollView);  // highlight is in scroll-space
        window.draw(m_hoverHighlight);
        window.setView(appView);
    }

    // Scrollbar — always visible
    window.draw(m_scrollTrack);
    window.draw(m_scrollThumb);

    // ---- Zoom overlay (drawn last, on top of everything) ----
    if (m_zoomed && m_zoomSprite) {
        window.draw(m_zoomOverlay);
        window.draw(*m_zoomSprite);
        window.draw(m_zoomCloseBtn);
        window.draw(*m_zoomCloseX);
        window.draw(*m_zoomTitleText);
    }
}
