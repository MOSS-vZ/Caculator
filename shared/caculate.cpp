#include <string>
#include <stack>
#include <cctype>
#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <opencv2/opencv.hpp>

// ============================================================
//  表达式求值（双栈算法，支持 + - * / 和括号）
// ============================================================

static int precedence(char op) {
    if (op == '+' || op == '-') return 1;
    if (op == '*' || op == '/') return 2;
    return 0;
}

static int applyOp(int a, int b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/':
            if (b == 0) throw std::runtime_error("Division by zero");
            return a / b;
        default: throw std::runtime_error(std::string("Unknown operator: ") + op);
    }
}

int evaluateExpression(const std::string& expr) {
    std::stack<int> values;
    std::stack<char> ops;

    for (size_t i = 0; i < expr.length(); ) {
        char c = expr[i];
        if (std::isspace(c)) { ++i; continue; }

        if (std::isdigit(c)) {
            int num = 0;
            while (i < expr.length() && std::isdigit(expr[i]))
                num = num * 10 + (expr[i++] - '0');
            values.push(num);
            continue;
        }

        if (c == '(') { ops.push(c); ++i; continue; }

        if (c == ')') {
            while (!ops.empty() && ops.top() != '(') {
                int b = values.top(); values.pop();
                int a = values.top(); values.pop();
                values.push(applyOp(a, b, ops.top())); ops.pop();
            }
            if (ops.empty()) throw std::runtime_error("Mismatched parentheses");
            ops.pop(); ++i;
            continue;
        }

        if (c == '+' || c == '-' || c == '*' || c == '/') {
            if (c == '-') {
                    // 一元负号检测
                    bool isUnary = (i == 0);
                    if (i > 0) {
                        size_t j = i;
                        while (j > 0 && std::isspace(expr[--j])) {}
                        isUnary = (expr[j] == '(');
                    }
                    if (isUnary) values.push(0);
                }
            while (!ops.empty() && ops.top() != '(' &&
                   precedence(ops.top()) >= precedence(c)) {
                int b = values.top(); values.pop();
                int a = values.top(); values.pop();
                values.push(applyOp(a, b, ops.top())); ops.pop();
            }
            ops.push(c); ++i;
            continue;
        }

        throw std::runtime_error(std::string("Invalid char: '") + c + "'");
    }

    while (!ops.empty()) {
        if (ops.top() == '(') throw std::runtime_error("Unmatched '('");
        int b = values.top(); values.pop();
        int a = values.top(); values.pop();
        values.push(applyOp(a, b, ops.top())); ops.pop();
    }

    return values.empty() ? 0 : values.top();
}

// ============================================================
//  批改
// ============================================================

bool isExpressionCorrect(const std::string& text, int& result) {
    // 去空白
    std::string clean;
    for (char c : text)
        if (!std::isspace(static_cast<unsigned char>(c))) clean += c;
    if (clean.empty()) {
        std::cout << "  [grade] empty text" << std::endl;
        return false;
    }

    size_t eq = clean.find('=');
    if (eq == std::string::npos || eq == 0 || eq + 1 >= clean.length()) {
        std::cout << "  [grade] no valid '=' in \"" << clean << "\"" << std::endl;
        return false;
    }

    std::string left  = clean.substr(0, eq);
    std::string right = clean.substr(eq + 1);

    // ---- 左侧求值 ----
    int leftVal = 0;
    try {
        leftVal = evaluateExpression(left);
    } catch (const std::exception& e) {
        std::cout << "  [grade] left eval failed: " << e.what()
                  << "  (\"" << left << "\")" << std::endl;
        return false;
    }

    // ---- 右侧提取数字（更稳健） ----
    // 从右侧字符串中提取第一个完整整数（支持前导负号）
    std::string rightDigits;
    bool neg = false;
    for (size_t i = 0; i < right.length(); i++) {
        char c = right[i];
        if (c == '-' && rightDigits.empty() && !neg) {
            neg = true;
            rightDigits += c;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            rightDigits += c;
        } else if (!rightDigits.empty() && rightDigits != "-") {
            // 遇到非数字且已经收集了数字 → 停止（处理 "15✓" 这种情况）
            break;
        }
    }

    if (rightDigits.empty() || rightDigits == "-") {
        std::cout << "  [grade] no digits in right side \""
                  << right << "\"" << std::endl;
        return false;
    }

    int expected = 0;
    try {
        expected = std::stoi(rightDigits);
    } catch (...) {
        std::cout << "  [grade] stoi failed on \"" << rightDigits << "\"" << std::endl;
        return false;
    }

    result = leftVal;
    bool ok = (leftVal == expected);

    std::cout << "  [grade] " << left << " = " << leftVal
              << "  vs  " << expected
              << "  → " << (ok ? "CORRECT" : "WRONG") << std::endl;
    return ok;
}

// ============================================================
//  结构验证
// ============================================================

/// 提取字符串中的数字部分（支持前导负号），用于验证右侧
static std::string extractDigits(const std::string& s) {
    std::string out;
    bool neg = false;
    for (char c : s) {
        if (c == '-' && out.empty() && !neg) {
            neg = true;
            out += c;
        } else if (std::isdigit(static_cast<unsigned char>(c))) {
            out += c;
        }
    }
    if (out == "-") out.clear();
    return out;
}

bool isValidEquation(const std::string& text) {
    if (text.length() < 4) {
        std::cout << "  [valid] too short: \"" << text << "\"" << std::endl;
        return false;
    }

    size_t eq = text.find('=');
    if (eq == std::string::npos || eq == 0 || eq >= text.length() - 1) {
        std::cout << "  [valid] no '=' or at edge: \"" << text << "\"" << std::endl;
        return false;
    }

    std::string left  = text.substr(0, eq);
    std::string right = text.substr(eq + 1);

    // 左侧：至少有一个数字和一个运算符
    if (left.find_first_of("0123456789") == std::string::npos) {
        std::cout << "  [valid] left no digits: \"" << left << "\"" << std::endl;
        return false;
    }
    if (left.find_first_of("+-*/") == std::string::npos) {
        std::cout << "  [valid] left no operator: \"" << left << "\"" << std::endl;
        return false;
    }

    // 右侧：提取纯数字后非空即可（不再要求全字符都是数字）
    std::string rightNum = extractDigits(right);
    if (rightNum.empty()) {
        std::cout << "  [valid] right no digits: \"" << right << "\"" << std::endl;
        return false;
    }

    return true;
}

// ============================================================
//  绘制
// ============================================================

void drawMark(cv::Mat& img, cv::Point point, bool correct, double scale) {
    int face = cv::FONT_HERSHEY_SIMPLEX;
    cv::putText(img, correct ? "V" : "X", point, face, scale,
                correct ? cv::Scalar(0, 255, 0) : cv::Scalar(0, 0, 255),
                static_cast<int>(3 * scale));
}

void drawMarkLine(cv::Mat& img, cv::Point center, bool correct, int size) {
    const int t = std::max(3, size / 10);
    if (correct) {
        cv::line(img, cv::Point(center.x - size / 3, center.y),
                 cv::Point(center.x - size / 6, center.y + size / 3),
                 cv::Scalar(0, 255, 0), t);
        cv::line(img, cv::Point(center.x - size / 6, center.y + size / 3),
                 cv::Point(center.x + size / 3, center.y - size / 3),
                 cv::Scalar(0, 255, 0), t);
    } else {
        int h = size / 2;
        cv::line(img, cv::Point(center.x - h, center.y - h),
                 cv::Point(center.x + h, center.y + h),
                 cv::Scalar(0, 0, 255), t);
        cv::line(img, cv::Point(center.x + h, center.y - h),
                 cv::Point(center.x - h, center.y + h),
                 cv::Scalar(0, 0, 255), t);
    }
}
