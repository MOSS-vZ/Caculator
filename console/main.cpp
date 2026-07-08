//! [includes]
#include <opencv2/core.hpp>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

#include <iostream>
#include "split.h"
#include "single.h"
#include "caculate.h"

using namespace cv;
using namespace std;
//! [includes]

int main()
{
    // ============================================================
    // 1. 加载 & 预处理
    // ============================================================
    cv::Mat src = cv::imread("images\\items\\test.jpg");
    if (src.empty()) {
        std::cerr << "[ERROR] Cannot read input image." << std::endl;
        return -1;
    }

    // 倾斜校正（可选）
    // cv::Mat img = deskewImage(src);
    cv::Mat img = src;
    std::cout << "[main] original: " << img.cols << "x" << img.rows << std::endl;

    // 统一预处理：CLAHE + resize（保证 Tesseract 可读）
    cv::Mat proc = preprocessUnified(img, 56);
    double scaleX = (double)proc.cols / img.cols;
    double scaleY = (double)proc.rows / img.rows;
    std::cout << "[main] preprocessed: " << proc.cols << "x" << proc.rows
              << "  scale=" << scaleX << "x" << scaleY << std::endl;

    // ============================================================
    // 2. 算式检测（三级递进兜底）
    // ============================================================
    cv::Mat display;
    cv::cvtColor(proc, display, cv::COLOR_GRAY2BGR);
    int correctCount = 0;
    int eqIdx = 0;

    // L1: Tesseract 字符级等号检测
    std::vector<EqSplit> splits = findEqualsByOCR(proc);
    int level = 1;

    // L2: OpenCV 轮廓等号检测（不依赖OCR）
    std::vector<EqRegion> regions;
    if (splits.empty()) {
        level = 2;
        std::cout << "[main] L1 failed, trying L2: OpenCV equals detection..." << std::endl;
        regions = detectEquations(img);
    }

    // L3: 全图当作一道题（等号位置未知，OCR推断）
    bool useL3 = (splits.empty() && regions.empty());
    if (useL3) {
        level = 3;
        std::cout << "[main] L2 failed, trying L3: full-image as one equation..." << std::endl;
    }

    // ---- 处理 L1/L2: 基于等号位置的识别 ----
    if (level <= 2) {
        int totalItems = (level == 1) ? (int)splits.size() : (int)regions.size();
        std::cout << "[main] L" << level << ": " << totalItems
                  << " equation(s) to process." << std::endl;

        for (int i = 0; i < totalItems; i++) {
            std::cout << "\n--- Eq #" << eqIdx << " ---" << std::endl;

            cv::Mat fullImg;
            cv::Rect roi;
            cv::Point eqPt;

            if (level == 1) {
                auto& sp = splits[i];
                roi = sp.fullROI;
                fullImg = proc(roi).clone();
                eqPt = cv::Point(sp.eqCenter.x - roi.x, sp.eqCenter.y - roi.y);
            } else {
                auto& reg = regions[i];
                roi = reg.rect;
                fullImg = reg.image.clone();
                eqPt = reg.eqLocal;  // 可能是 (-1,-1)
            }

            std::string fullEquation;

            if (eqPt.x >= 0 && eqPt.y >= 0) {
                // 有等号位置 → 切分左右分别识别
                int rightStart = std::min(eqPt.x + 2, fullImg.cols);
                cv::Rect leftR(0, 0, eqPt.x, fullImg.rows);
                cv::Rect rightR(rightStart, 0, fullImg.cols - rightStart, fullImg.rows);

                cv::Mat leftImg  = leftR.width > 0 ? fullImg(leftR).clone() : cv::Mat();
                cv::Mat rightImg = rightR.width > 0 ? fullImg(rightR).clone() : cv::Mat();

                LeftResult  leftRes  = recognizeLeftStrict(leftImg);
                RightResult rightRes = recognizeRightStrict(rightImg);
                fullEquation = postProcessEquation(leftRes, rightRes);
            } else {
                // 无等号位置 → 全图 EasyOCR + 等号推断
                fullEquation = recognizeFull(fullImg);
            }

            if (fullEquation.empty() || fullEquation.find('=') == std::string::npos) {
                std::cout << "[main] L" << level << " eq#" << eqIdx
                          << " failed to get valid equation, skipping" << std::endl;
                continue;
            }

            // 批改
            int computed = 0;
            bool correct = isExpressionCorrect(fullEquation, computed);
            std::cout << "[main] \"" << fullEquation << "\"  "
                      << (correct ? "✓ CORRECT" : "✗ WRONG")
                      << "  (expect=" << computed << ")" << std::endl;
            if (correct) correctCount++;

            // 画标记
            int markSize = std::max(22, std::min(static_cast<int>(roi.height * 0.8), 100));
            cv::Point mark(roi.x + roi.width + markSize / 2,
                           roi.y + roi.height / 2);
            if (mark.x > display.cols - markSize / 2)
                mark.x = roi.x + roi.width - markSize / 2;
            if (mark.y < markSize / 2)
                mark.y = roi.y + roi.height / 2;
            drawMarkLine(display, mark, correct, markSize);

            cv::rectangle(display, roi,
                          correct ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 2);
            eqIdx++;
        }
    }

    // ---- 处理 L3: 全图作为一道题 ----
    if (useL3) {
        std::cout << "\n--- Eq #0 (L3 fallback) ---" << std::endl;

        // 先尝试 EasyOCR 全图识别
        std::string fullEquation = recognizeFull(proc);

        // 如果还是不行，Tesseract 全图 + 后处理推断
        if (fullEquation.empty() || fullEquation.find('=') == std::string::npos) {
            std::cout << "[main] L3 EasyOCR failed, trying Tesseract full..." << std::endl;
            LeftResult lr = recognizeLeftStrict(proc);
            RightResult rr;  // 右侧为空，postProcess 会尝试推断
            rr.digits = "";
            rr.reliable = false;
            fullEquation = postProcessEquation(lr, rr);
        }

        if (!fullEquation.empty() && fullEquation.find('=') != std::string::npos) {
            int computed = 0;
            bool correct = isExpressionCorrect(fullEquation, computed);
            std::cout << "[main] \"" << fullEquation << "\"  "
                      << (correct ? "✓ CORRECT" : "✗ WRONG")
                      << "  (expect=" << computed << ")" << std::endl;
            if (correct) correctCount++;

            cv::Rect roi(0, 0, display.cols, display.rows);
            int markSize = std::max(22, std::min(static_cast<int>(display.rows * 0.3), 100));
            drawMarkLine(display,
                         cv::Point(display.cols * 3 / 4, display.rows / 2),
                         correct, markSize);
            cv::rectangle(display, roi,
                          correct ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255), 2);
            eqIdx++;
        } else {
            std::cout << "[main] L3 all attempts failed, giving up." << std::endl;
        }
    }

    if (eqIdx == 0) {
        std::cerr << "[ERROR] No equations could be processed." << std::endl;
        return -1;
    }

    // ============================================================
    // 4. 统计 & 保存
    // ============================================================
    std::cout << "\n[main] Total: " << eqIdx
              << "  Correct: " << correctCount
              << "  Wrong: " << (eqIdx - correctCount) << std::endl;
    if (eqIdx > 0)
        std::cout << "[main] Accuracy: "
                  << (100.0 * correctCount / eqIdx) << "%" << std::endl;

    cv::imwrite("marked_full.png", display);
    cv::imshow("Marked", display);
    cv::waitKey(0);
    return 0;
}
