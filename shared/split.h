#pragma once
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

// ---- 数据结构 ----

/// 检测到的等号（OpenCV 轮廓方式）
struct EqSign {
    cv::Rect box;       // 包围盒
    cv::Point center;   // 中心点
};

/// 提取出的算式区域
struct EqRegion {
    cv::Rect  rect;       // 在原图中的区域
    cv::Mat   image;      // 裁剪出的图像
    cv::Point eqLocal;    // 等号在 image 中的坐标（-1,-1 表示未检测到视觉等号）
};

/// Tesseract OCR 检测到的等号切分信息
struct EqSplit {
    cv::Point eqCenter;   // 等号中心（全局坐标）
    cv::Rect  eqBox;      // 等号包围盒（用于精确切分，排除 '=' 本身）
    cv::Rect  fullROI;    // 完整算式区域（用于裁剪）
    int       lineIdx;    // 所属行号（调试用）
};

// ---- 图像预处理 ----
cv::Mat deskewImage(const cv::Mat& src);
bool    isUpsideDown(const cv::Mat& gray);
std::vector<cv::Rect> findTextLines(const cv::Mat& binary);

/// 统一预处理：灰度化 + CLAHE 增强对比度 + resize 保证最小高度
cv::Mat preprocessUnified(const cv::Mat& src, int minHeight = 56);

// ---- 等号检测 ----

/// OpenCV 轮廓方式检测等号（保留，暂不作为兜底）
std::vector<EqSign> findEqualsSigns(const cv::Mat& binary);

/// Tesseract OCR 全局等号检测 + 切分 ROI
/// 返回的每个 EqSplit 对应一道算式，fullROI 可直接用于裁剪
std::vector<EqSplit> findEqualsByOCR(const cv::Mat& preprocessedImg);

/// 以等号为锚或行投影为兜底，提取所有算式区域（原有函数）
std::vector<EqRegion> detectEquations(const cv::Mat& src);
