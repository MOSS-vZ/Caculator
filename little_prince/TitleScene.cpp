#include "TitleScene.h"
#include "Settings.h"
#include <iostream>
#include <cmath>
#include <cstdlib>

TitleScene::TitleScene()
    : animState(ANIM_IDLE), animProgress(0.0f),
    princeScaleX(1.0f), princeScaleY(1.0f),
    currentPrinceFrame(0), animDirection(1),
    frameDuration(2.0f / 23.0f),
    startClicked(false) {
}

void TitleScene::onEnter() {
    resetAnimation();
    if (!loadResources()) {
        std::cerr << "Failed to load resources in TitleScene" << std::endl;
    }
    initStars();
}

void TitleScene::resetAnimation() {
    animState = ANIM_TITLE_MOVE;
    animClock.restart();
    animProgress = 0.0f;
    currentPrinceFrame = 0;
    animDirection = 1;
    animFrameClock.restart();
}

bool TitleScene::loadResources() {
    std::string basePath = Settings::getBasePath();
    if (!titleBackgroundTex.loadFromFile(basePath + "begin.png")) return false;
    if (!wordButtonTex.loadFromFile(basePath + "word.png")) return false;
    if (!titleTextTex.loadFromFile(basePath + "title.png")) return false;

    sf::Vector2u btnSrcSize = wordButtonTex.getSize();
    float btnScale = (float)Settings::BUTTON_WIDTH / btnSrcSize.x;
    btnSize = sf::Vector2f(btnSrcSize.x * btnScale, btnSrcSize.y * btnScale);
    btnPosition = sf::Vector2f(Settings::WINDOW_WIDTH - btnSize.x - Settings::BTN_RIGHT_MARGIN,
        Settings::WINDOW_HEIGHT - btnSize.y - Settings::BTN_BOTTOM_MARGIN);

    princeTextures.resize(24);
    for (int i = 0; i < 24; ++i) {
        std::string path = basePath + "prince\\" + std::to_string(i + 1) + ".png";
        if (!princeTextures[i].loadFromFile(path)) return false;
    }

    sf::Vector2u princeSrcSize = princeTextures[0].getSize();
    float targetH = Settings::WINDOW_HEIGHT * Settings::PRINCE_HEIGHT_RATIO;
    princeScaleY = targetH / princeSrcSize.y;
    princeScaleX = princeScaleY;
    sf::Sprite tempSprite(princeTextures[0]);
    tempSprite.setScale(sf::Vector2f(princeScaleX, princeScaleY));
    sf::FloatRect princeBounds = tempSprite.getGlobalBounds();
    princePosition = sf::Vector2f(Settings::PRINCE_LEFT_MARGIN,
        (Settings::WINDOW_HEIGHT - princeBounds.size.y) / 2);
    return true;
}

void TitleScene::initStars() {
    stars.resize(150);
    for (auto& s : stars) {
        s.pos.x = rand() % Settings::WINDOW_WIDTH;
        s.pos.y = rand() % Settings::WINDOW_HEIGHT;
        s.speed = 20 + rand() % 60;
        s.size = 3 + rand() % 6;
        s.alpha = 150 + rand() % 105;
    }
}

void TitleScene::updateStars(float dt) {
    for (auto& s : stars) {
        s.pos.y += s.speed * dt;
        if (s.pos.y > Settings::WINDOW_HEIGHT) {
            s.pos.y = 0;
            s.pos.x = rand() % Settings::WINDOW_WIDTH;
            s.speed = 20 + rand() % 60;
            s.size = 3 + rand() % 6;
            s.alpha = 150 + rand() % 105;
        }
    }
}

void TitleScene::drawStars(sf::RenderWindow& window) {
    for (const auto& s : stars) {
        sf::CircleShape star(s.size);
        star.setFillColor(sf::Color(255, 255, 255, static_cast<unsigned char>(s.alpha)));
        star.setPosition(s.pos);
        window.draw(star);
    }
}

void TitleScene::update(float dt) {
    // 小王子帧动画
    static float timeAcc = 0.0f;
    timeAcc += dt;
    if (timeAcc >= frameDuration) {
        timeAcc -= frameDuration;
        currentPrinceFrame += animDirection;
        if (currentPrinceFrame >= 24) {
            currentPrinceFrame = 23;
            animDirection = -1;
        }
        else if (currentPrinceFrame < 0) {
            currentPrinceFrame = 0;
            animDirection = 1;
        }
    }

    // 标题进场动画
    if (animState != ANIM_COMPLETE) {
        float elapsed = animClock.getElapsedTime().asSeconds();
        switch (animState) {
        case ANIM_TITLE_MOVE:
            animProgress = std::min(1.0f, elapsed / Settings::TITLE_MOVE_DURATION);
            if (elapsed >= Settings::TITLE_MOVE_DURATION) {
                animState = ANIM_PRINCE_FADE;
                animClock.restart();
            }
            break;
        case ANIM_PRINCE_FADE:
            animProgress = std::min(1.0f, elapsed / Settings::FADE_DURATION);
            if (elapsed >= Settings::FADE_DURATION) {
                animState = ANIM_BUTTON_FADE;
                animClock.restart();
            }
            break;
        case ANIM_BUTTON_FADE:
            animProgress = std::min(1.0f, elapsed / Settings::FADE_DURATION);
            if (elapsed >= Settings::FADE_DURATION) {
                animState = ANIM_COMPLETE;
            }
            break;
        default:
            break;
        }
    }

    updateStars(dt);
}

void TitleScene::draw(sf::RenderWindow& window) {
    window.clear();

    // 背景
    sf::Sprite bgSprite(titleBackgroundTex);
    sf::Vector2u bgSize = titleBackgroundTex.getSize();
    bgSprite.setScale(sf::Vector2f((float)Settings::WINDOW_WIDTH / bgSize.x,
        (float)Settings::WINDOW_HEIGHT / bgSize.y));
    window.draw(bgSprite);

    // 星星
    drawStars(window);

    // 标题文字
    sf::Sprite titleSprite(titleTextTex);
    float titleWidth = Settings::WINDOW_WIDTH * 0.6f;
    float titleScale = titleWidth / titleTextTex.getSize().x;
    titleSprite.setScale(sf::Vector2f(titleScale, titleScale));
    sf::FloatRect titleBounds = titleSprite.getGlobalBounds();
    titleTargetPos = sf::Vector2f((Settings::WINDOW_WIDTH - titleBounds.size.x) / 2, 80);
    titleStartPos = sf::Vector2f(titleTargetPos.x, -titleBounds.size.y);
    sf::Vector2f titlePos = (animState == ANIM_TITLE_MOVE)
        ? titleStartPos + (titleTargetPos - titleStartPos) * Settings::easeOutQuad(animProgress)
        : titleTargetPos;
    titleSprite.setPosition(titlePos);
    if (animState == ANIM_COMPLETE) {
        float breath = 0.7f + 0.3f * std::sin(animClock.getElapsedTime().asSeconds() * 1.5f);
        titleSprite.setColor(sf::Color(255, 255, 255, static_cast<unsigned char>(255 * breath)));
    }
    else {
        titleSprite.setColor(sf::Color(255, 255, 255, 255));
    }
    window.draw(titleSprite);

    // 光晕
    if (animState >= ANIM_PRINCE_FADE) {
        sf::Sprite princeTemp(princeTextures[currentPrinceFrame]);
        princeTemp.setScale(sf::Vector2f(princeScaleX, princeScaleY));
        sf::FloatRect princeBounds = princeTemp.getGlobalBounds();
        float centerX = princePosition.x + princeBounds.size.x / 2;
        float centerY = princePosition.y + princeBounds.size.y / 2;
        float radius = std::max(princeBounds.size.x, princeBounds.size.y) * 0.8f;
        sf::CircleShape halo(radius);
        halo.setFillColor(sf::Color(255, 255, 255, 60));
        halo.setOrigin(sf::Vector2f(radius, radius));
        halo.setPosition(sf::Vector2f(centerX, centerY));
        window.draw(halo);
    }

    // 小王子
    sf::Sprite princeSprite(princeTextures[currentPrinceFrame]);
    princeSprite.setScale(sf::Vector2f(princeScaleX, princeScaleY));
    princeSprite.setPosition(princePosition);
    if (animState >= ANIM_PRINCE_FADE) {
        float princeAlpha = 0;
        if (animState == ANIM_PRINCE_FADE) {
            float t = animProgress;
            if (t < 0.01f) princeAlpha = 0;
            else princeAlpha = 255 * Settings::easeOutQuad(t);
        }
        else {
            princeAlpha = 255;
        }
        princeSprite.setColor(sf::Color(255, 255, 255, static_cast<unsigned char>(princeAlpha)));
    }
    else {
        princeSprite.setColor(sf::Color(255, 255, 255, 0));
    }
    window.draw(princeSprite);

    // 按钮
    sf::Sprite btnSprite(wordButtonTex);
    float btnScale = (float)Settings::BUTTON_WIDTH / wordButtonTex.getSize().x;
    btnSprite.setScale(sf::Vector2f(btnScale, btnScale));
    btnSprite.setPosition(btnPosition);

    // 悬停放大效果
    sf::Vector2i mousePos = sf::Mouse::getPosition(window);
    bool hovering = (mousePos.x >= btnPosition.x && mousePos.x <= btnPosition.x + btnSize.x &&
        mousePos.y >= btnPosition.y && mousePos.y <= btnPosition.y + btnSize.y);
    if (hovering && animState == ANIM_COMPLETE) {
        btnSprite.setScale(sf::Vector2f(btnScale * 1.05f, btnScale * 1.05f));
        sf::FloatRect bounds = btnSprite.getGlobalBounds();
        float newX = btnPosition.x - (bounds.size.x - btnSize.x) / 2;
        float newY = btnPosition.y - (bounds.size.y - btnSize.y) / 2;
        btnSprite.setPosition(sf::Vector2f(newX, newY));
    }

    if (animState >= ANIM_BUTTON_FADE) {
        float btnAlpha = 0;
        if (animState == ANIM_BUTTON_FADE) {
            float t = animProgress;
            if (t < 0.01f) btnAlpha = 0;
            else btnAlpha = 255 * Settings::easeOutQuad(t);
        }
        else {
            btnAlpha = 255;
        }
        btnSprite.setColor(sf::Color(255, 255, 255, static_cast<unsigned char>(btnAlpha)));
    }
    else {
        btnSprite.setColor(sf::Color(255, 255, 255, 0));
    }
    window.draw(btnSprite);
}

void TitleScene::handleEvent(const sf::Event& event, sf::RenderWindow& window) {
    if (const auto* mouseBtn = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (mouseBtn->button == sf::Mouse::Button::Left && animState == ANIM_COMPLETE) {
            sf::Vector2i mousePos = sf::Mouse::getPosition(window);
            if (mousePos.x >= btnPosition.x && mousePos.x <= btnPosition.x + btnSize.x &&
                mousePos.y >= btnPosition.y && mousePos.y <= btnPosition.y + btnSize.y) {
                startClicked = true;
            }
        }
    }
}

void TitleScene::onExit() {
    // 无需清理
}