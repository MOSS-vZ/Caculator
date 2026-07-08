#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <memory>
#include <vector>
#include <string>

class PracticeScreen : public Screen {
public:
    PracticeScreen();
    ~PracticeScreen();

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    enum class State {
        ModeSelect,
        Playing,
        WrongShow
    };

    State m_state = State::ModeSelect;

    std::string m_operator = "+";
    int m_left = 0, m_right = 0, m_answer = 0;
    std::vector<int> m_options;
    int m_correctOptionIndex = -1;

    int m_score = 0;

    sf::Clock m_timerClock;
    float m_timeLimit = 5.0f;
    float m_remainingTime = 5.0f;

    sf::CircleShape m_backButton;

    struct ModeButton {
        sf::RectangleShape rect;
        std::string op;
        bool hovered = false;
    };
    std::vector<ModeButton> m_modeButtons;

    struct OptionButton {
        sf::RectangleShape rect;
        int value;
        bool hovered = false;
    };
    std::vector<OptionButton> m_optionButtons;

    sf::RectangleShape m_okButton;

    sf::Font m_font;
    std::unique_ptr<sf::Text> m_titleText;
    std::unique_ptr<sf::Text> m_scoreText;
    std::unique_ptr<sf::Text> m_timerText;
    std::unique_ptr<sf::Text> m_questionText;
    std::unique_ptr<sf::Text> m_wrongText;
    std::unique_ptr<sf::Text> m_okText;
    std::vector<std::unique_ptr<sf::Text>> m_optionLabels;

    struct Pattern {
        sf::CircleShape shape;
        sf::Vector2f velocity;
        float phase;
        float radiusSpeed;
    };
    std::vector<Pattern> m_patterns;
    void initPatterns();

    void generateQuestion();
    void generateOptions();
    void checkAnswer(int selectedIndex);
    void returnToMainMenu();

    sf::Vector2f m_mousePos;
    bool m_mousePressed = false;

    static constexpr float WINDOW_WIDTH = 2048.f;
    static constexpr float WINDOW_HEIGHT = 1536.f;
};