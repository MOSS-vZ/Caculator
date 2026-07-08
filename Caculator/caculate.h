#pragma once
#include <string>
#include <opencv2/opencv.hpp>

// ---- 数学计算 ----
int evaluateExpression(const std::string& expr);

// ---- 批改 ----
bool isExpressionCorrect(const std::string& text, int& result);

// ---- 结构验证 ----
// 检查一段文本是否满足"完整算式"的结构要求：
//   有 '='   ∧   左侧有运算符和数字   ∧   右侧全是数字且非空
bool isValidEquation(const std::string& text);

// ---- 绘制 ----
void drawMark(cv::Mat& img, cv::Point point, bool correct, double scale = 1.0);
void drawMarkLine(cv::Mat& img, cv::Point center, bool correct, int size = 30);
