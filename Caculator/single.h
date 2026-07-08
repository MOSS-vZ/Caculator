#pragma once
#include <tesseract/baseapi.h>
#include <opencv2/opencv.hpp>
#include <string>

// ---- 严格模式识别结果 ----

struct LeftResult {
    std::string text;      // 识别文本
    bool        reliable;  // 是否通过验证
};

struct RightResult {
    std::string digits;    // 提取的数字
    bool        reliable;  // 是否通过验证
};

// ---- 原有 OCR 识别 ----

/// 识别左侧题目（印刷体 → Tesseract）
std::string recognizeLeft(const cv::Mat& img);

/// 识别右侧答案（手写体 → EasyOCR，只返回数字）
std::string recognizeRight(const cv::Mat& img);

/// 识别整道算式（兜底：无视觉等号时用，EasyOCR + 等号推断）
std::string recognizeFull(const cv::Mat& img);

// ---- 严格模式 OCR（新流水线用） ----

/// 左侧严格识别：Tesseract，whitelist="0123456789+-()"
/// 首次失败自动换 PSM 重试一次
LeftResult recognizeLeftStrict(const cv::Mat& img);

/// 右侧严格识别：EasyOCR，只取数字
/// 首次失败自动增强预处理重试一次
RightResult recognizeRightStrict(const cv::Mat& img);

// ---- 后处理 ----

/// 组合左右识别结果，纠错 + 推断缺失运算符
/// 返回最终算式字符串（含 '='）
std::string postProcessEquation(const LeftResult& left,
                                 const RightResult& right);

// ---- 工具 ----
std::string mapOperators(const std::string& input);
cv::Mat preprocessForOCR(const cv::Mat& src);
