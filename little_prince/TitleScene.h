#pragma once
#include "Scene.h"
#include <vector>
#include <string>

class TitleScene : public Scene {
public:
    TitleScene();
    ~TitleScene() override = default;

    void onEnter() override;
    void update(float dt) override;
    void draw(sf::RenderWindow& window) override;
    void onExit() override;
    void handleEvent(const sf::Event& event, sf::RenderWindow& window) override;

    bool isStartClicked() const { return startClicked; }

private:
    // 动画状态
    enum AnimState {
        ANIM_IDLE,
        ANIM_TITLE_MOVE,
        ANIM_PRINCE_FADE,
        ANIM_BUTTON_FADE,
        ANIM_COMPLETE
    };
    AnimState animState;
    sf::Clock animClock;
    float animProgress;

    sf::Vector2f titleStartPos, titleTargetPos;

    // 星星粒子
    struct Star {
        sf::Vector2f pos;
        float speed;
        float size;
        float alpha;
    };
    std::vector<Star> stars;
    void initStars();
    void updateStars(float dt);
    void drawStars(sf::RenderWindow& window);

    // 资源
    sf::Texture titleBackgroundTex;
    sf::Texture wordButtonTex;
    sf::Texture titleTextTex;
    std::vector<sf::Texture> princeTextures;

    float princeScaleX, princeScaleY;
    sf::Vector2f princePosition;
    sf::Vector2f btnPosition, btnSize;

    // 小王子动画控制
    int currentPrinceFrame;
    int animDirection;
    sf::Clock animFrameClock;
    float frameDuration;

    bool startClicked;

    // 内部辅助函数
    bool loadResources();
    void resetAnimation();
};