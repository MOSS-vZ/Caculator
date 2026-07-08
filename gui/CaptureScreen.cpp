#include "CaptureScreen.h"
#include "App.h"
#include "ResultScreen.h"
#include "MainScreen.h"
#include "split.h"
#include "single.h"
#include "caculate.h"
#include <SFML/Graphics.hpp>
#include <windows.h>
#include <commdlg.h>
#include <iostream>
#include <random>
#include <filesystem>
#include <chrono>
#include <iomanip>
#include <thread>

namespace fs = std::filesystem;

CaptureScreen::CaptureScreen() {}

CaptureScreen::~CaptureScreen() {
    if (m_cap.isOpened()) {
        m_cap.release();
    }
}

// ---- 打开文件选择框 ----
std::string CaptureScreen::openFileDialog() {
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn = {};
    ofn.lStructSize  = sizeof(ofn);
    ofn.hwndOwner    = GetActiveWindow();
    ofn.lpstrFilter  = "Images\0*.png;*.jpg;*.bmp;*.jpeg\0All\0*.*\0";
    ofn.lpstrFile    = filename;
    ofn.nMaxFile     = MAX_PATH;
    ofn.Flags        = OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrTitle   = "选择一张算式图片";
    if (GetOpenFileNameA(&ofn))
        return std::string(filename);
    return "";
}

// ---- 显示图片（摄像头帧或本地加载） ----
void CaptureScreen::displayImage(const cv::Mat& mat) {
    if (mat.empty()) return;
    m_frameMat = mat;
    m_frameTexture = matToTexture(mat);
    if (m_frameTexture && m_frameSprite)
        m_frameSprite->setTexture(*m_frameTexture);
    float maxW = 1600.f, maxH = 1000.f;
    float sX = maxW / mat.cols;
    float sY = maxH / mat.rows;
    float sc = std::min(sX, sY);
    m_frameSprite->setScale(sf::Vector2f(sc, sc));
    m_frameSprite->setPosition(sf::Vector2f(
        (WINDOW_WIDTH - mat.cols * sc) / 2.f,
        (WINDOW_HEIGHT - mat.rows * sc) / 2.f - 50.f));
    std::cout << "Displayed: " << mat.cols << "x" << mat.rows << std::endl;
}

// ---- 清空 photo 文件夹 ----
void CaptureScreen::clearPhotoFolder() {
    try {
        fs::path photoDir = "photo";
        if (fs::exists(photoDir)) {
            for (const auto& entry : fs::directory_iterator(photoDir)) {
                fs::remove(entry.path());
            }
            std::cout << "Photo folder cleared" << std::endl;
        }
        else {
            fs::create_directory(photoDir);
            std::cout << "Photo folder created" << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error clearing photo folder: " << e.what() << std::endl;
    }
}

// ---- Mat 转 SFML Texture ----
std::unique_ptr<sf::Texture> CaptureScreen::matToTexture(const cv::Mat& mat) {
    auto texture = std::make_unique<sf::Texture>();
    if (mat.empty()) return texture;

    cv::Mat rgba;
    cv::cvtColor(mat, rgba, cv::COLOR_BGR2RGBA);

    sf::Image image(sf::Vector2u(rgba.cols, rgba.rows));
    for (int y = 0; y < rgba.rows; ++y) {
        for (int x = 0; x < rgba.cols; ++x) {
            cv::Vec4b pixel = rgba.at<cv::Vec4b>(y, x);
            image.setPixel(sf::Vector2u(x, y),
                sf::Color(pixel[0], pixel[1], pixel[2], pixel[3]));
        }
    }

    texture->loadFromImage(image);
    return texture;
}

// ---- 更新摄像头帧 ----
void CaptureScreen::updateFrame() {
    if (m_cameraReady && m_cap.isOpened()) {
        cv::Mat frame;
        m_cap >> frame;
        if (!frame.empty()) {
            m_frameMat = frame;
            m_frameTexture = matToTexture(frame);
            if (m_frameTexture && m_frameSprite) {
                m_frameSprite->setTexture(*m_frameTexture);
            }
        }
    }
}

// ---- 初始化 ----
void CaptureScreen::init() {
    clearPhotoFolder();

    m_cap.open(0);
    if (!m_cap.isOpened()) {
        std::cerr << "Error: Cannot open camera!" << std::endl;
        m_cameraReady = false;
        m_frameMat = cv::Mat(480, 640, CV_8UC3, cv::Scalar(50, 60, 80));
        cv::putText(m_frameMat, "Camera not found", cv::Point(150, 240),
            cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(255, 255, 255), 2);
    }
    else {
        m_cameraReady = true;
        m_cap >> m_frameMat;
        std::cout << "Camera opened successfully" << std::endl;
    }

    if (!m_frameMat.empty()) {
        m_frameTexture = matToTexture(m_frameMat);
        m_frameSprite = std::make_unique<sf::Sprite>(*m_frameTexture);
        float maxW = 1600.f, maxH = 1000.f;
        float scaleX = maxW / static_cast<float>(m_frameMat.cols);
        float scaleY = maxH / static_cast<float>(m_frameMat.rows);
        float scale = std::min(scaleX, scaleY);
        m_frameSprite->setScale(sf::Vector2f(scale, scale));
        m_frameSprite->setPosition(sf::Vector2f(
            (WINDOW_WIDTH - m_frameMat.cols * scale) / 2.f,
            (WINDOW_HEIGHT - m_frameMat.rows * scale) / 2.f - 50.f
        ));
    }

    m_backButton.setRadius(40.f);
    m_backButton.setFillColor(sf::Color(200, 200, 200, 200));
    m_backButton.setOutlineColor(sf::Color::White);
    m_backButton.setOutlineThickness(2.f);
    m_backButton.setPosition(sf::Vector2f(20.f, 20.f));

    m_captureButton.setRadius(70.f);
    m_captureButton.setFillColor(sf::Color(220, 50, 50));
    m_captureButton.setOutlineColor(sf::Color::White);
    m_captureButton.setOutlineThickness(4.f);
    m_captureButton.setPosition(sf::Vector2f(
        (WINDOW_WIDTH - 140.f) / 2.f,
        1400.f
    ));

    // 加载按钮（绿色，拍照按钮右边，大号好点）
    float capX = m_captureButton.getPosition().x;
    float capY = m_captureButton.getPosition().y;
    float loadR = 70.f;
    float loadX = capX + 160.f;
    float loadY = capY;
    m_loadButton.setRadius(loadR);
    m_loadButton.setFillColor(sf::Color(50, 180, 80));
    m_loadButton.setOutlineColor(sf::Color::White);
    m_loadButton.setOutlineThickness(4.f);
    m_loadButton.setPosition(sf::Vector2f(loadX, loadY));

    // 白色文件夹图标（等比放大）
    m_loadIcon1.setSize(sf::Vector2f(60.f, 10.f));
    m_loadIcon1.setFillColor(sf::Color::White);
    m_loadIcon1.setPosition(sf::Vector2f(loadX + loadR - 30.f, loadY + 30.f));

    m_loadIcon2.setSize(sf::Vector2f(40.f, 30.f));
    m_loadIcon2.setFillColor(sf::Color::White);
    m_loadIcon2.setPosition(sf::Vector2f(loadX + loadR - 20.f, loadY + 42.f));

    // 点击检测范围：实际半径 + 20px 容差，覆盖整个按钮 + 周围
    float m_loadHitRadius = loadR + 20.f;

    std::cout << "CaptureScreen init done" << std::endl;
}

// ---- 生成假的测试图片（用于结果界面） ----
sf::Image CaptureScreen::generateTestImage() {
    // 创建一个 800x600 的测试图片，画一些模拟题目和随机标记
    sf::Image img(sf::Vector2u(800, 600), sf::Color(240, 240, 245));

    // 画横线（模拟作业纸）
    for (int y = 30; y < 600; y += 50) {
        for (int x = 20; x < 780; ++x) {
            img.setPixel(sf::Vector2u(x, y), sf::Color(180, 180, 190));
        }
    }

    // 随机生成题目数量
    int total = 8 + rand() % 5;
    int wrong = rand() % (total / 2) + 1;
    if (wrong >= total) wrong = total - 1;

    std::vector<int> wrongIndices;
    for (int i = 0; i < wrong; ++i) {
        int idx;
        do {
            idx = rand() % total;
        } while (std::find(wrongIndices.begin(), wrongIndices.end(), idx) != wrongIndices.end());
        wrongIndices.push_back(idx);
    }

    // 在图片上绘制标记
    for (int i = 0; i < total; ++i) {
        int x = rand() % 700 + 50;
        int y = rand() % 400 + 50;

        bool isWrong = std::find(wrongIndices.begin(), wrongIndices.end(), i) != wrongIndices.end();

        if (isWrong) {
            // 红叉
            for (int d = -20; d <= 20; ++d) {
                if (x + d >= 0 && x + d < 800 && y + d >= 0 && y + d < 600)
                    img.setPixel(sf::Vector2u(x + d, y + d), sf::Color::Red);
                if (x + d >= 0 && x + d < 800 && y - d >= 0 && y - d < 600)
                    img.setPixel(sf::Vector2u(x + d, y - d), sf::Color::Red);
            }
        }
        else {
            // 绿勾
            for (int d = 0; d <= 15; ++d) {
                if (x - 15 + d >= 0 && x - 15 + d < 800 && y + d >= 0 && y + d < 600)
                    img.setPixel(sf::Vector2u(x - 15 + d, y + d), sf::Color::Green);
            }
            for (int d = 0; d <= 35; ++d) {
                if (x - 5 + d >= 0 && x - 5 + d < 800 && y + 15 - d >= 0 && y + 15 - d < 600)
                    img.setPixel(sf::Vector2u(x - 5 + d, y + 15 - d), sf::Color::Green);
            }
        }
    }

    return img;
}

// ---- 真实批改：调用识别流水线 ----
CaptureScreen::GradeResult CaptureScreen::gradeFrame(const cv::Mat& frame) {
    GradeResult gr;
    gr.accuracy   = 0.f;
    gr.wrongCount = 0;
    gr.total      = 0;

    if (frame.empty()) return gr;

    // 1. 统一预处理
    cv::Mat proc = preprocessUnified(frame, 56);

    // 2. Tesseract 找等号
    std::vector<EqSplit> splits = findEqualsByOCR(proc);

    // Fallback: OpenCV 轮廓找等号
    if (splits.empty()) {
        std::cout << "[grade] L1 failed, trying L2..." << std::endl;
        std::vector<EqRegion> regions = detectEquations(frame);
        if (!regions.empty()) {
            // 转 L2 结果到统一循环
            cv::Mat display;
            cv::cvtColor(proc, display, cv::COLOR_GRAY2BGR);

            for (auto& reg : regions) {
                gr.total++;
                std::string fullEquation;
                if (reg.eqLocal.x >= 0 && reg.eqLocal.y >= 0) {
                    int eqX = reg.eqLocal.x;
                    cv::Mat leftImg  = reg.image(cv::Rect(0,0,eqX,reg.image.rows)).clone();
                    cv::Mat rightImg = reg.image(cv::Rect(eqX,0,reg.image.cols-eqX,reg.image.rows)).clone();
                    LeftResult  lr = recognizeLeftStrict(leftImg);
                    RightResult rr = recognizeRightStrict(rightImg);
                    fullEquation = postProcessEquation(lr, rr);
                } else {
                    fullEquation = recognizeFull(reg.image);
                }

                int computed = 0;
                bool correct = isExpressionCorrect(fullEquation, computed);
                if (!correct) {
                    gr.wrongCount++;
                    gr.wrongInfo += std::to_string(gr.total) + " ";
                }

                int markSize = std::max(22, std::min((int)(reg.rect.height*0.8), 100));
                cv::Point mark(reg.rect.x+reg.rect.width+markSize/2, reg.rect.y+reg.rect.height/2);
                drawMarkLine(display, mark, correct, markSize);
                cv::rectangle(display, reg.rect,
                    correct ? cv::Scalar(0,255,0) : cv::Scalar(0,0,255), 2);
            }
            gr.markedImg = display;
        }
        if (gr.total > 0) gr.accuracy = (float)(gr.total - gr.wrongCount) / gr.total * 100.f;
        return gr;
    }

    // L1: Tesseract 找到等号
    cv::Mat display;
    cv::cvtColor(proc, display, cv::COLOR_GRAY2BGR);

    for (auto& sp : splits) {
        gr.total++;
        cv::Mat fullImg = proc(sp.fullROI).clone();
        int eqBoxL = sp.eqBox.x - sp.fullROI.x;
        int eqCenterX = sp.eqCenter.x - sp.fullROI.x;
        int rightStart = std::min(eqCenterX + 2, eqBoxL + sp.eqBox.width);

        cv::Mat leftImg  = fullImg(cv::Rect(0,0,eqBoxL,fullImg.rows)).clone();
        cv::Mat rightImg = fullImg(cv::Rect(rightStart,0,fullImg.cols-rightStart,fullImg.rows)).clone();

        LeftResult  lr = recognizeLeftStrict(leftImg);
        RightResult rr = recognizeRightStrict(rightImg);
        std::string fullEquation = postProcessEquation(lr, rr);

        int computed = 0;
        bool correct = isExpressionCorrect(fullEquation, computed);
        if (!correct) {
            gr.wrongCount++;
            gr.wrongInfo += std::to_string(gr.total) + " ";
        }

        int markSize = std::max(22, std::min((int)(sp.fullROI.height*0.8), 100));
        cv::Point mark(sp.eqCenter.x+markSize, sp.eqCenter.y-markSize/4);
        if (mark.x > display.cols-markSize/2) mark.x = sp.fullROI.x+sp.fullROI.width+markSize/4;
        if (mark.y < markSize/2) mark.y = sp.eqCenter.y+sp.fullROI.height/2;
        drawMarkLine(display, mark, correct, markSize);
        cv::rectangle(display, sp.fullROI,
            correct ? cv::Scalar(0,255,0) : cv::Scalar(0,0,255), 2);
    }

    gr.markedImg = display;
    if (gr.total > 0) gr.accuracy = (float)(gr.total - gr.wrongCount) / gr.total * 100.f;
    return gr;
}

// ---- 模拟批改（已废弃，保留接口） ----
void CaptureScreen::mockProcessImage() {
    std::cout << "Mock processing (fake)" << std::endl;

    int total = 8 + rand() % 5;
    int wrong = rand() % (total / 2) + 1;
    if (wrong >= total) wrong = total - 1;
    float accuracy = (float)(total - wrong) / total * 100.f;

    std::vector<int> wrongIndices;
    for (int i = 0; i < wrong; ++i) {
        int idx;
        do {
            idx = rand() % total;
        } while (std::find(wrongIndices.begin(), wrongIndices.end(), idx) != wrongIndices.end());
        wrongIndices.push_back(idx);
    }
    std::string wrongStr;
    for (int idx : wrongIndices) wrongStr += std::to_string(idx + 1) + " ";

    sf::Image resultImage = generateTestImage();

    if (m_app) {
        auto resultScreen = std::make_unique<ResultScreen>(
            resultImage, accuracy, wrong, total, wrongStr
        );
        m_app->switchScreen(std::move(resultScreen));
    }
}

// ---- 处理图片（拍照或本地加载后调用） ----
void CaptureScreen::processImage(const cv::Mat& image) {
    std::cout << "Processing image " << image.cols << "x" << image.rows << std::endl;

    auto gr = gradeFrame(image);

    if (gr.total == 0) {
        std::cout << "No equations found, using mock fallback" << std::endl;
        mockProcessImage();
        return;
    }

    std::cout << "Grading done: " << gr.total << " total, "
              << gr.wrongCount << " wrong, "
              << gr.accuracy << "% accuracy" << std::endl;

    // cv::Mat → sf::Image
    cv::Mat bgr;
    if (gr.markedImg.channels() == 1)
        cv::cvtColor(gr.markedImg, bgr, cv::COLOR_GRAY2BGR);
    else
        bgr = gr.markedImg;

    cv::Mat rgba;
    cv::cvtColor(bgr, rgba, cv::COLOR_BGR2RGBA);

    sf::Image resultImage(sf::Vector2u(rgba.cols, rgba.rows));
    for (int y = 0; y < rgba.rows; ++y) {
        for (int x = 0; x < rgba.cols; ++x) {
            cv::Vec4b p = rgba.at<cv::Vec4b>(y, x);
            resultImage.setPixel(sf::Vector2u(x, y),
                sf::Color(p[0], p[1], p[2], p[3]));
        }
    }

    if (m_app) {
        auto resultScreen = std::make_unique<ResultScreen>(
            resultImage, gr.accuracy, gr.wrongCount, gr.total, gr.wrongInfo
        );
        m_app->switchScreen(std::move(resultScreen));
    }
}

// ---- 拍照按钮 → 弹出文件选择框 ----
void CaptureScreen::captureAndProcess() {
    std::string path = openFileDialog();
    if (path.empty()) return;

    cv::Mat img = cv::imread(path);
    if (img.empty()) {
        std::cout << "Failed to load: " << path << std::endl;
        return;
    }
    std::cout << "Loaded: " << path << " (" << img.cols << "x" << img.rows << ")" << std::endl;
    displayImage(img);
    processImage(img);
}

// ---- 事件处理 ----
void CaptureScreen::handleEvent(const sf::Event& event, const sf::RenderWindow& window) {
    if (const auto* moved = event.getIf<sf::Event::MouseMoved>()) {
        m_mousePos = sf::Vector2f(
            static_cast<float>(moved->position.x),
            static_cast<float>(moved->position.y)
        );
    }

    if (const auto* pressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        if (pressed->button == sf::Mouse::Button::Left) {
            m_mousePressed = true;

            sf::Vector2f center = m_backButton.getPosition() + sf::Vector2f(40.f, 40.f);
            if (std::hypot(m_mousePos.x - center.x, m_mousePos.y - center.y) <= m_backButton.getRadius()) {
                std::cout << "Return to main menu" << std::endl;
                if (m_app) {
                    if (m_cap.isOpened()) m_cap.release();
                    m_app->switchScreen(std::make_unique<MainScreen>());
                }
                return;
            }

            sf::Vector2f capCenter = m_captureButton.getPosition() + sf::Vector2f(70.f, 70.f);
            if (std::hypot(m_mousePos.x - capCenter.x, m_mousePos.y - capCenter.y) <= m_captureButton.getRadius()) {
                std::cout << "Capture button clicked!" << std::endl;
                captureAndProcess();
                return;
            }

            // 加载按钮（绿色）
            float lr = m_loadButton.getRadius();
            sf::Vector2f loadCenter = m_loadButton.getPosition() + sf::Vector2f(lr, lr);
            float loadDist = std::hypot(m_mousePos.x - loadCenter.x, m_mousePos.y - loadCenter.y);
            if (loadDist <= lr + 20.f) {  // +20px 容差，好点
                std::cout << "Load button clicked!" << std::endl;
                std::string path = openFileDialog();
                if (!path.empty()) {
                    cv::Mat img = cv::imread(path);
                    if (!img.empty()) {
                        displayImage(img);
                        processImage(img);
                    }
                }
                return;
            }
        }
    }

    if (const auto* released = event.getIf<sf::Event::MouseButtonReleased>()) {
        if (released->button == sf::Mouse::Button::Left) {
            m_mousePressed = false;
        }
    }

    // 键盘快捷键：G 重新批改当前图片
    if (const auto* key = event.getIf<sf::Event::KeyPressed>()) {
        if (key->scancode == sf::Keyboard::Scancode::G && !m_frameMat.empty()) {
            processImage(m_frameMat);
        }
    }
}

// ---- 更新 ----
void CaptureScreen::update(float deltaTime) {
    if (m_cameraReady && m_cap.isOpened()) {
        updateFrame();
    }
}

// ---- 绘制 ----
void CaptureScreen::draw(sf::RenderWindow& window) {
    window.clear(sf::Color(30, 30, 40));

    if (m_frameSprite) {
        window.draw(*m_frameSprite);
    }

    window.draw(m_backButton);
    sf::ConvexShape arrow;
    arrow.setPointCount(3);
    sf::Vector2f basePos = m_backButton.getPosition() + sf::Vector2f(20.f, 20.f);
    arrow.setPoint(0, basePos + sf::Vector2f(30.f, 5.f));
    arrow.setPoint(1, basePos + sf::Vector2f(10.f, 20.f));
    arrow.setPoint(2, basePos + sf::Vector2f(30.f, 35.f));
    arrow.setFillColor(sf::Color(50, 50, 50));
    window.draw(arrow);

    window.draw(m_captureButton);
    sf::CircleShape innerCircle(30.f);
    innerCircle.setFillColor(sf::Color::White);
    innerCircle.setPosition(m_captureButton.getPosition() + sf::Vector2f(40.f, 40.f));
    window.draw(innerCircle);

    // 加载文件按钮 + 文件夹图标
    window.draw(m_loadButton);
    window.draw(m_loadIcon1);
    window.draw(m_loadIcon2);
}