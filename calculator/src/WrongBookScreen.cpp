#include "WrongBookScreen.h"
#include "App.h"
#include "MainScreen.h"
#include "ResourceManager.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <regex>
#include <map>
#include <random>

namespace fs = std::filesystem;

WrongBookScreen::WrongBookScreen() {}

// ========== scan photo/wrong ==========
void WrongBookScreen::scanWrongDir() {
    m_batches.clear();
    std::string wrongDir = "photo/wrong";

    if (!fs::exists(wrongDir)) {
        std::cout << "WrongBookScreen: no wrong/ directory" << std::endl;
        return;
    }

    // Find all *_YYYYMMDD_HHMMSS.txt summary files (avoid Chinese in regex)
    std::regex summaryPattern(R"(_(\d{8}_\d{6})\.txt)");
    std::smatch match;
    std::vector<std::pair<std::string, std::string>> summaryFiles; // {filename, batchId}

    for (const auto& entry : fs::directory_iterator(wrongDir)) {
        if (!entry.is_regular_file()) continue;
        std::string filename = entry.path().filename().string();
        if (std::regex_search(filename, match, summaryPattern))
            summaryFiles.emplace_back(filename, match[1].str());
    }

    // Sort descending by batchId (newest first)
    std::sort(summaryFiles.begin(), summaryFiles.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });

    for (const auto& [filename, batchId] : summaryFiles) {

        // Parse display timestamp
        std::string displayTime =
            batchId.substr(0, 4) + "-" + batchId.substr(4, 2) + "-" + batchId.substr(6, 2) +
            " " +
            batchId.substr(9, 2) + ":" + batchId.substr(11, 2) + ":" + batchId.substr(13, 2);

        // Read the summary txt file
        std::string txtPath = wrongDir + "/" + filename;
        std::ifstream file(txtPath);
        if (!file.is_open()) continue;

        WrongBatch batch;
        batch.timestamp = displayTime;
        batch.batchId   = batchId;

        std::string line;
        bool firstLine = true;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            // Skip the first non-empty line (header: "题号 | 学生答案 | ...")
            if (firstLine) {
                firstLine = false;
                // Only skip if it looks like a header (contains '|' and no leading digit+pipe)
                if (line.find('|') != std::string::npos &&
                    (line.empty() || !std::isdigit(static_cast<unsigned char>(line[0])))) {
                    continue;
                }
            }

            // Parse: "6 | 3 | 4 | wrong_20260711_104250_6.png"
            std::regex entryPattern(R"((\d+)\s*\|\s*(\S+)\s*\|\s*(\S+)\s*\|\s*(.+))");
            std::smatch em;
            if (!std::regex_match(line, em, entryPattern)) continue;

            WrongEntry we;
            we.questionNumber = std::stoi(em[1].str());
            we.studentAnswer  = em[2].str();
            we.correctAnswer  = em[3].str();
            we.imagePath      = wrongDir + "/" + em[4].str();

            // Load image if it exists
            if (fs::exists(we.imagePath))
                we.valid = we.image.loadFromFile(we.imagePath);

            batch.entries.push_back(std::move(we));
        }

        if (!batch.entries.empty()) {
            // Sort entries by question number ascending
            std::sort(batch.entries.begin(), batch.entries.end(),
                [](const WrongEntry& a, const WrongEntry& b) {
                    return a.questionNumber < b.questionNumber;
                });
            m_batches.push_back(std::move(batch));
        }
    }

    int totalEntries = 0;
    for (const auto& b : m_batches) totalEntries += static_cast<int>(b.entries.size());
    std::cout << "WrongBookScreen: " << m_batches.size() << " batches, "
              << totalEntries << " entries" << std::endl;
}

// ========== background patterns ==========
void WrongBookScreen::initPatterns() {
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
void WrongBookScreen::init() {
    m_font.openFromFile("C:/Windows/Fonts/msyh.ttf") ||
        m_font.openFromFile("C:/Windows/Fonts/arial.ttf");

    scanWrongDir();
    initPatterns();

    // ---- Fixed header ----
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
    m_fixedTitle->setString("Wrong Book");
    m_fixedTitle->setCharacterSize(42);
    m_fixedTitle->setFillColor(sf::Color(40, 40, 60));
    sf::FloatRect tb = m_fixedTitle->getLocalBounds();
    m_fixedTitle->setOrigin(sf::Vector2f(tb.size.x * 0.5f, tb.size.y * 0.5f));
    m_fixedTitle->setPosition(sf::Vector2f(WINDOW_WIDTH * 0.5f, FIXED_HEADER_H * 0.5f));

    // ---- Create textures / sprites / labels per entry ----
    for (auto& batch : m_batches) {
        // Batch timestamp title
        auto batchTitle = std::make_unique<sf::Text>(m_font);
        batchTitle->setString(batch.timestamp);
        batchTitle->setCharacterSize(34);
        batchTitle->setFillColor(sf::Color(50, 50, 70));
        m_batchTitles.push_back(std::move(batchTitle));

        for (auto& entry : batch.entries) {
            // Load texture + sprite
            if (entry.valid) {
                entry.texture.loadFromImage(entry.image);
                entry.sprite = std::make_unique<sf::Sprite>(entry.texture);
                auto sz = entry.texture.getSize();
                float iw = static_cast<float>(sz.x);
                float ih = static_cast<float>(sz.y);
                float sc = std::min(MAX_IMG_W / iw, MAX_IMG_H / ih);
                entry.sprite->setScale(sf::Vector2f(sc, sc));
            }

            // "Question N" label
            auto qLabel = std::make_unique<sf::Text>(m_font);
            qLabel->setString("Question " + std::to_string(entry.questionNumber));
            qLabel->setCharacterSize(36);
            qLabel->setFillColor(sf::Color(80, 80, 100));
            m_questionNumbers.push_back(std::move(qLabel));

            // "Your Answer: X" — red
            auto studentAns = std::make_unique<sf::Text>(m_font);
            studentAns->setString("Your Answer: " + entry.studentAnswer);
            studentAns->setCharacterSize(32);
            studentAns->setFillColor(sf::Color(200, 50, 50));
            m_studentAnswers.push_back(std::move(studentAns));

            // "Correct Answer: Y" — green
            auto correctAns = std::make_unique<sf::Text>(m_font);
            correctAns->setString("Correct Answer: " + entry.correctAnswer);
            correctAns->setCharacterSize(32);
            correctAns->setFillColor(sf::Color(40, 150, 40));
            m_correctAnswers.push_back(std::move(correctAns));
        }
    }

    // ---- Scrollbar geometry ----
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

    layoutContent();
    m_scrollOffset = 0.f;
    updateScrollThumb();

    std::cout << "WrongBookScreen init done, batches=" << m_batches.size()
              << " contentH=" << m_contentHeight << std::endl;
}

// ========== layout ==========
void WrongBookScreen::layoutContent() {
    float curY = FIXED_HEADER_H + 30.f;
    float centerX = WINDOW_WIDTH * 0.5f;

    int entryIdx = 0;  // global index into m_questionNumbers / m_answerLines

    for (size_t bi = 0; bi < m_batches.size(); ++bi) {
        auto& batch = m_batches[bi];

        // Batch timestamp title (centered)
        if (bi < m_batchTitles.size()) {
            sf::FloatRect tr = m_batchTitles[bi]->getLocalBounds();
            m_batchTitles[bi]->setPosition(sf::Vector2f(centerX - tr.size.x * 0.5f, curY));
            curY += tr.size.y + BATCH_GAP;
        }

        // Each entry in this batch
        for (size_t ei = 0; ei < batch.entries.size(); ++ei) {
            auto& entry = batch.entries[ei];

            // "Question N" label (centered)
            if (entryIdx < static_cast<int>(m_questionNumbers.size())) {
                sf::FloatRect qr = m_questionNumbers[entryIdx]->getLocalBounds();
                m_questionNumbers[entryIdx]->setPosition(
                    sf::Vector2f(centerX - qr.size.x * 0.5f, curY));
                curY += qr.size.y + TITLE_GAP;
            }

            // Question image (centered)
            if (entry.valid) {
                sf::FloatRect sr = entry.sprite->getGlobalBounds();
                entry.sprite->setPosition(sf::Vector2f(centerX - sr.size.x * 0.5f, curY));
                curY += sr.size.y + TITLE_GAP;
            }

            // Answer line: Your Answer (red) + gap + Correct Answer (green), centered as a group
            if (entryIdx < static_cast<int>(m_studentAnswers.size()) &&
                entryIdx < static_cast<int>(m_correctAnswers.size())) {
                float sw = m_studentAnswers[entryIdx]->getLocalBounds().size.x;
                float cw = m_correctAnswers[entryIdx]->getLocalBounds().size.x;
                float gap = 60.f;
                float groupW = sw + gap + cw;
                float groupX = centerX - groupW * 0.5f;

                m_studentAnswers[entryIdx]->setPosition(sf::Vector2f(groupX, curY));
                m_correctAnswers[entryIdx]->setPosition(sf::Vector2f(groupX + sw + gap, curY));

                float lineH = std::max(m_studentAnswers[entryIdx]->getLocalBounds().size.y,
                                       m_correctAnswers[entryIdx]->getLocalBounds().size.y);
                curY += lineH + SLOT_GAP;
            }

            ++entryIdx;
        }
    }

    m_contentHeight = curY;
    m_maxScroll = (m_contentHeight > WINDOW_HEIGHT) ? (m_contentHeight - WINDOW_HEIGHT) : 0.f;
}

// ========== scroll helpers ==========
void WrongBookScreen::setScrollOffset(float newOffset) {
    m_scrollOffset = std::max(0.f, std::min(newOffset, m_maxScroll));
    updateScrollThumb();
}

void WrongBookScreen::updateScrollThumb() {
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

bool WrongBookScreen::isOnThumb(sf::Vector2f pos) const {
    return m_scrollThumb.getGlobalBounds().contains(pos);
}

float WrongBookScreen::mouseYToScroll(float mouseY) const {
    if (m_maxScroll <= 0.f) return 0.f;

    float thumbH = m_scrollThumb.getGlobalBounds().size.y;
    float maxThumbTravel = m_trackH - thumbH;
    if (maxThumbTravel <= 0.f) return 0.f;

    float thumbCenterY = mouseY - thumbH * 0.5f;
    float ratio = (thumbCenterY - m_trackY) / maxThumbTravel;
    ratio = std::max(0.f, std::min(ratio, 1.f));
    return ratio * m_maxScroll;
}

// ========== events ==========
void WrongBookScreen::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        sf::Vector2f newPos = window.mapPixelToCoords(moved->position);
        m_mousePos = newPos;

        if (m_dragMode == DragMode::Thumb) {
            float newOffset = mouseYToScroll(newPos.y);
            setScrollOffset(newOffset);
        }

        m_isHoveringThumb = isOnThumb(newPos);
        m_scrollThumb.setFillColor(m_isHoveringThumb
            ? sf::Color(120, 120, 140, 240)
            : sf::Color(160, 160, 180, 200));
    }

    if (const auto* wheel = event.getIf<sf::Event::MouseWheelScrolled>()) {
        float newOff = m_scrollOffset - wheel->delta * 50.f;
        setScrollOffset(newOff);
    }

    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (pressed->button == sf::Mouse::Button::Left) {
            m_mousePressed = true;
            sf::Vector2f clickPos = window.mapPixelToCoords(pressed->position);

            // 1) Back button
            if (m_backSprite->getGlobalBounds().contains(clickPos)) {
                if (m_app) m_app->switchScreen(std::make_unique<MainScreen>());
                return;
            }

            // 2) Thumb drag
            if (isOnThumb(clickPos)) {
                m_dragMode = DragMode::Thumb;
                m_dragAnchorY = clickPos.y;
                m_dragAnchorOffset = m_scrollOffset;
                return;
            }

            // 3) Track click → jump
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
void WrongBookScreen::update(float dt) {
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
void WrongBookScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(250, 252, 255));

    for (const auto& p : m_patterns)
        window.draw(p.shape);

    sf::View appView = window.getView();   // 复制一份，不保留引用

    // ---- Scrollable content ----
    sf::View scrollView = appView;
    scrollView.setCenter(sf::Vector2f(
        WINDOW_WIDTH * 0.5f,
        WINDOW_HEIGHT * 0.5f + m_scrollOffset));
    scrollView.setSize(sf::Vector2f(WINDOW_WIDTH, WINDOW_HEIGHT));
    window.setView(scrollView);

    int entryIdx = 0;
    for (size_t bi = 0; bi < m_batches.size(); ++bi) {
        // Batch title
        if (bi < m_batchTitles.size())
            window.draw(*m_batchTitles[bi]);

        for (size_t ei = 0; ei < m_batches[bi].entries.size(); ++ei) {
            auto& entry = m_batches[bi].entries[ei];

            if (entryIdx < static_cast<int>(m_questionNumbers.size()))
                window.draw(*m_questionNumbers[entryIdx]);

            if (entry.valid)
                window.draw(*entry.sprite);

            if (entryIdx < static_cast<int>(m_studentAnswers.size()))
                window.draw(*m_studentAnswers[entryIdx]);
            if (entryIdx < static_cast<int>(m_correctAnswers.size()))
                window.draw(*m_correctAnswers[entryIdx]);

            ++entryIdx;
        }
    }

    // ---- Fixed UI ----
    window.setView(appView);

    window.draw(m_titleBg);
    window.draw(*m_fixedTitle);

    // Back button
    window.draw(*m_backSprite);

    // Scrollbar
    window.draw(m_scrollTrack);
    window.draw(m_scrollThumb);
}
