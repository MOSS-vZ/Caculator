#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>
#include <vector>
#include <future>

class CaptureScreen : public Screen {
public:
    CaptureScreen();
    ~CaptureScreen();

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    cv::VideoCapture m_cap;
    cv::Mat m_frameMat;
    std::unique_ptr<sf::Texture> m_frameTexture;
    std::unique_ptr<sf::Sprite> m_frameSprite;
    bool m_cameraReady = false;
    bool m_cameraOpened = false;

    sf::VertexArray m_backgroundGradient;
    struct Pattern {
        sf::CircleShape shape;
        sf::Vector2f    velocity;
        float           phase;
        float           radiusSpeed;
    };
    std::vector<Pattern> m_patterns;

    sf::Texture m_backTexture;
    std::unique_ptr<sf::Sprite> m_backSprite;
    bool m_usingCustomBack = false;
    sf::CircleShape m_backButtonFallback;

    sf::CircleShape m_captureButton;
    sf::CircleShape m_captureButtonShadow;

    sf::RectangleShape m_cameraToggleBtn;
    sf::RectangleShape m_cameraToggleBtnShadow;
    std::unique_ptr<sf::Text> m_cameraToggleText;

    sf::RectangleShape m_inputBox;
    std::unique_ptr<sf::Text> m_inputText;
    std::string m_inputString;
    bool m_isInputActive = false;
    bool m_showCursor = true;
    sf::Clock m_cursorClock;

    sf::RectangleShape m_browseBtn;
    std::unique_ptr<sf::Text> m_browseBtnText;
    void openFileDialog();

    sf::RectangleShape m_processButton;
    std::unique_ptr<sf::Text> m_processButtonText;

    size_t m_cursorPos = 0;
    size_t m_selectionStart = 0;
    bool   m_hasSelection = false;

    sf::Vector2f m_mousePos;
    bool m_mousePressed = false;

    std::string m_pendingImagePath;
    int m_pendingFrames = -1;

    bool m_isProcessing = false;
    std::future<sf::Image> m_futureResult;
    sf::Image m_originalImage;
    sf::Image m_resultImage;

    sf::Texture m_answerBtnTexture;
    std::unique_ptr<sf::Sprite> m_answerBtnSprite;
    bool m_isAnswerProcessing = false;
    std::future<bool> m_futureAnswer;
    bool m_showingAnswer = false;
    sf::RectangleShape m_ansOverlay;
    sf::Texture m_ansInputTex, m_ansOutputTex;
    std::unique_ptr<sf::Sprite> m_ansInputSprite, m_ansOutputSprite;
    sf::CircleShape m_ansCloseBtn;
    std::unique_ptr<sf::Text> m_ansCloseX;
    std::unique_ptr<sf::Text> m_ansTitleInput, m_ansTitleOutput;
    void runAnswerScript();
    void loadAnswerResult();

    float m_scanLineY = 0.f;
    float m_scanSpeed = 300.f;

    void updateFrame();
    std::unique_ptr<sf::Texture> matToTexture(const cv::Mat& mat);
    void processImageFromPath(const std::string& imagePath);
    void clearPhotoFolder();
    bool copyFileToTest(const std::string& sourcePath);
    bool runPythonScript();
    sf::Image loadResultImage();
    bool loadImageToSprite(const std::string& imagePath);

    void updateInputDisplay();
    void clearSelection();
    void deleteSelection();
    std::string getSelectedText() const;

    sf::Font m_font;
    static constexpr float WINDOW_WIDTH = 2048.f;
    static constexpr float WINDOW_HEIGHT = 1536.f;
};
