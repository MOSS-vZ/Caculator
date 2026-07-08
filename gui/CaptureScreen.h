#pragma once
#include "Screen.h"
#include <SFML/Graphics.hpp>
#include <opencv2/opencv.hpp>
#include <memory>
#include <string>

class CaptureScreen : public Screen {
public:
    CaptureScreen();
    ~CaptureScreen();

    void init() override;
    void handleEvent(const sf::Event& event, const sf::RenderWindow& window) override;
    void update(float deltaTime) override;
    void draw(sf::RenderWindow& window) override;

private:
    // ---- 摄像头 ----
    cv::VideoCapture m_cap;
    cv::Mat m_frameMat;
    std::unique_ptr<sf::Texture> m_frameTexture;
    std::unique_ptr<sf::Sprite> m_frameSprite;
    bool m_cameraReady = false;

    // ---- UI ----
    sf::CircleShape m_backButton;
    sf::CircleShape m_captureButton;
    sf::CircleShape m_loadButton;
    sf::RectangleShape m_loadIcon1;
    sf::RectangleShape m_loadIcon2;

    // ---- 鼠标状态 ----
    sf::Vector2f m_mousePos;
    bool m_mousePressed = false;

    // ---- 辅助函数 ----
    void updateFrame();
    void displayImage(const cv::Mat& mat);
    std::unique_ptr<sf::Texture> matToTexture(const cv::Mat& mat);
    void captureAndProcess();
    void processImage(const cv::Mat& image);
    void clearPhotoFolder();
    std::string openFileDialog();

    // ---- 真实批改（调用识别流水线） ----
    struct GradeResult {
        cv::Mat  markedImg;
        float    accuracy;
        int      wrongCount;
        int      total;
        std::string wrongInfo;
    };
    GradeResult gradeFrame(const cv::Mat& frame);

    // ---- 假操作（不读取文件，已废弃） ----
    sf::Image generateTestImage();
    void mockProcessImage();

    static constexpr float WINDOW_WIDTH = 2048.f;
    static constexpr float WINDOW_HEIGHT = 1536.f;
};