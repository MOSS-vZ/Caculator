#include <tesseract/baseapi.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <iostream>
#include <string>
#include <algorithm>
#include <cctype>
#include <cstdio>
#include <climits>
#include "single.h"
#include "caculate.h"

using namespace std;
// ============================================================
//  预处理
// ============================================================

static void addBorder(cv::Mat& img, int b) {
    cv::copyMakeBorder(img, img, b, b, b, b, cv::BORDER_CONSTANT, cv::Scalar(255));
}

cv::Mat preprocessForOCR(const cv::Mat& src) {
    cv::Mat gray;
    if (src.channels() == 3)
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else
        gray = src.clone();

    if (gray.empty()) return gray;

    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    clahe->apply(gray, gray);

    const int MIN_H = 56;
    if (gray.rows < MIN_H) {
        double s = (double)MIN_H / gray.rows;
        cv::resize(gray, gray, cv::Size(), s, s, cv::INTER_CUBIC);
    }
    addBorder(gray, 8);
    return gray;
}

static cv::Mat preprocessBinary(const cv::Mat& src) {
    cv::Mat gray;
    if (src.channels() == 3)
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else
        gray = src.clone();

    if (gray.empty()) return gray;

    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);
    cv::threshold(gray, gray, 0, 255, cv::THRESH_BINARY | cv::THRESH_OTSU);

    const int MIN_H = 56;
    if (gray.rows < MIN_H) {
        double s = (double)MIN_H / gray.rows;
        cv::resize(gray, gray, cv::Size(), s, s, cv::INTER_CUBIC);
    }
    addBorder(gray, 8);
    return gray;
}

// ============================================================
//  运算符后处理
// ============================================================

std::string mapOperators(const std::string& input) {
    std::string s = input;

    std::replace(s.begin(), s.end(), 'x', '*');
    std::replace(s.begin(), s.end(), 'X', '*');
    for (size_t p = 0; (p = s.find("\xC3\x97", p)) != std::string::npos; ++p) s.replace(p, 2, "*");
    for (size_t p = 0; (p = s.find("\xE2\x8B\x85", p)) != std::string::npos; ++p) s.replace(p, 3, "*");
    for (size_t p = 0; (p = s.find("\xC3\xB7", p)) != std::string::npos; ++p) s.replace(p, 2, "/");
    for (size_t p = 0; (p = s.find("--", p)) != std::string::npos; ) s.replace(p, 2, "=");

    // OCR 字符混淆修正
    std::replace(s.begin(), s.end(), 'O', '0');
    std::replace(s.begin(), s.end(), 'o', '0');
    std::replace(s.begin(), s.end(), 'S', '5');
    std::replace(s.begin(), s.end(), 's', '5');
    std::replace(s.begin(), s.end(), 'l', '1');
    std::replace(s.begin(), s.end(), 'I', '1');
    std::replace(s.begin(), s.end(), 'Z', '2');
    std::replace(s.begin(), s.end(), 'z', '2');
    std::replace(s.begin(), s.end(), 'B', '8');

    return s;
}

// ============================================================
//  Tesseract（印刷体）
// ============================================================

static tesseract::TessBaseAPI& getTesseract() {
    static tesseract::TessBaseAPI ocr;
    static bool ok = false;
    if (!ok) {
        if (ocr.Init("tessdata\\", "eng") == 0) {
            ocr.SetVariable("tessedit_ocr_engine_mode", "1");
            ok = true;
        }
    }
    return ocr;
}

static std::pair<std::string, int> tesseractOnce(const cv::Mat& img,
                                                   tesseract::TessBaseAPI& ocr,
                                                   tesseract::PageSegMode psm) {
    ocr.SetVariable("tessedit_char_whitelist", "0123456789+-xX()=*/.%");
    ocr.SetPageSegMode(psm);
    ocr.SetImage(img.data, img.cols, img.rows, 1, img.step);
    char* out = ocr.GetUTF8Text();
    int conf = ocr.MeanTextConf();
    std::string text(out ? out : "");
    delete[] out;
    return {text, conf};
}

static std::string tesseractOCR(const cv::Mat& img) {
    if (img.empty()) return "";
    tesseract::TessBaseAPI& ocr = getTesseract();

    static const tesseract::PageSegMode psms[] = {
        tesseract::PSM_SINGLE_LINE,
        tesseract::PSM_SINGLE_BLOCK,
        tesseract::PSM_SINGLE_WORD,
        tesseract::PSM_SPARSE_TEXT,
    };

    // 灰度 + 二值，取最佳
    std::string best;
    int bestConf = -1;

    cv::Mat gray = preprocessForOCR(img);
    for (auto p : psms) {
        auto [t, c] = tesseractOnce(gray, ocr, p);
        if (c > bestConf) { bestConf = c; best = t; }
        if (bestConf >= 85) break;
    }

    cv::Mat bin = preprocessBinary(img);
    for (auto p : psms) {
        auto [t, c] = tesseractOnce(bin, ocr, p);
        if (c > bestConf) { bestConf = c; best = t; }
        if (bestConf >= 85) break;
    }

    if (bestConf < 50) {
        auto [t, c] = tesseractOnce(gray, ocr, tesseract::PSM_AUTO);
        if (c > bestConf) best = t;
    }

    return best;
}

/**
 * 识别左侧题目（印刷体 Tesseract）
 * 输出已通过 mapOperators 清理
 */
std::string recognizeLeft(const cv::Mat& img) {
    if (img.empty()) {
        std::cout << "  [OCR-L] empty image" << std::endl;
        return "";
    }
    std::string raw = tesseractOCR(img);
    std::cout << "  [OCR-L] raw=\"" << raw << "\"" << std::endl;
    std::string result = mapOperators(raw);
    // 去掉空白和可能带进来的等号
    result.erase(std::remove_if(result.begin(), result.end(),
                                [](unsigned char c) { return std::isspace(c); }),
                 result.end());
    result.erase(std::remove(result.begin(), result.end(), '='), result.end());
    std::cout << "  [OCR-L] cleaned=\"" << result << "\"" << std::endl;
    return result;
}

// ============================================================
//  EasyOCR 桥接
// ============================================================

static std::string execCmd(const std::string& cmd) {
    std::string result;
    FILE* pipe = _popen(cmd.c_str(), "r");
    if (!pipe) return "";
    char buf[4096];
    while (fgets(buf, sizeof(buf), pipe)) result += buf;
    _pclose(pipe);
    while (!result.empty() && (result.back() == '\n' || result.back() == '\r'))
        result.pop_back();
    return result;
}

static std::string easyOCRSingle(const cv::Mat& img) {
    std::string tmp = "temp_ocr.png";
    cv::imwrite(tmp, img);
    std::string out = execCmd("python ocr_bridge.py \"" + tmp + "\"");
    std::remove(tmp.c_str());

    if (out.empty() || out == "[]") return "";
    // 解析 ["text"]
    if (out.size() >= 4 && out[0] == '[' && out[1] == '"') {
        size_t s = out.find('"'), e = out.find('"', s + 1);
        if (s != std::string::npos && e != std::string::npos)
            return out.substr(s + 1, e - s - 1);
    }
    std::string cl;
    for (char c : out)
        if (c != '[' && c != ']' && c != '"') cl += c;
    return cl;
}

/**
 * 识别右侧答案（手写体 EasyOCR → 只保留数字）
 */
std::string recognizeRight(const cv::Mat& img) {
    if (img.empty()) {
        std::cout << "  [OCR-R] empty image" << std::endl;
        return "";
    }

    std::string raw = easyOCRSingle(img);
    std::cout << "  [OCR-R] raw=\"" << raw << "\"" << std::endl;
    std::string digits;
    for (char c : raw)
        if (std::isdigit(static_cast<unsigned char>(c))) digits += c;

    if (digits.empty()) {
        // 再做一次 O→0 等修正
        std::string corrected = mapOperators(raw);
        std::cout << "  [OCR-R] corrected=\"" << corrected << "\"" << std::endl;
        for (char c : corrected)
            if (std::isdigit(static_cast<unsigned char>(c))) digits += c;
    }

    std::cout << "  [OCR-R] digits=\"" << digits << "\"" << std::endl;
    return digits;
}

// ============================================================
//  完整算式识别（兜底：无视觉等号时使用）
// ============================================================

/// 尝试把某个 '-' 换成 '='（从右往左试），验证通过则采纳
static std::string trySwapLastMinus(const std::string& text) {
    // 已经有等号就不处理
    if (text.find('=') != std::string::npos) return text;

    // 从右往左逐个减号尝试
    std::string work = text;
    for (size_t pos = work.rfind('-'); pos != std::string::npos; pos = work.rfind('-', pos)) {
        if (pos == 0) break;
        std::string fixed = work;
        fixed[pos] = '=';
        try {
            int dummy;
            if (isExpressionCorrect(fixed, dummy)) {
                std::cout << "  [trySwapMinus] pos=" << pos
                          << " → \"" << fixed << "\" CORRECT" << std::endl;
                return fixed;
            }
        } catch (...) {}
        if (pos == 0) break;
        pos--;
    }
    return text;
}

/// 穷举插入 '=' 的位置，选 |左-右| 最小的（加验证防瞎猜）
static std::string tryAllSplits(const std::string& text) {
    int bestDiff = INT_MAX;
    size_t bestPos = std::string::npos;

    for (size_t i = 1; i < text.length(); i++) {
        // 不能在运算符后面插入 '='
        if (text[i-1] == '+' || text[i-1] == '-' || text[i-1] == '*' || text[i-1] == '/')
            continue;
        // '=' 后面必须是数字或负号
        if (!std::isdigit(static_cast<unsigned char>(text[i])) && text[i] != '-')
            continue;

        std::string left  = text.substr(0, i);
        std::string right = text.substr(i);

        // 验证左侧：必须有数字和运算符
        if (left.find_first_of("0123456789") == std::string::npos) continue;
        if (left.find_first_of("+-*/") == std::string::npos) continue;

        // 验证右侧：提取数字后必须非空
        std::string rightNum;
        for (char c : right) {
            if (c == '-' && rightNum.empty()) { rightNum += c; continue; }
            if (std::isdigit(static_cast<unsigned char>(c))) rightNum += c;
        }
        if (rightNum.empty() || rightNum == "-") continue;

        try {
            int lv = evaluateExpression(left);
            int rv = std::stoi(rightNum);
            int diff = std::abs(lv - rv);
            if (diff < bestDiff) { bestDiff = diff; bestPos = i; }
            if (diff == 0) break;
        } catch (...) { continue; }
    }

    if (bestPos != std::string::npos) {
        std::string result = text.substr(0, bestPos) + "=" + text.substr(bestPos);
        std::cout << "  [tryAllSplits] bestPos=" << bestPos
                  << " diff=" << bestDiff << " → \"" << result << "\"" << std::endl;
        return result;
    }
    return text;
}

std::string recognizeFull(const cv::Mat& img) {
    if (img.empty()) {
        std::cout << "  [OCR-FULL] empty image" << std::endl;
        return "";
    }

    // EasyOCR 识别整张图
    std::string raw = easyOCRSingle(img);
    std::cout << "  [OCR-FULL] raw=\"" << raw << "\"" << std::endl;
    if (raw.empty()) return "";

    std::string cleaned = mapOperators(raw);
    cleaned.erase(std::remove_if(cleaned.begin(), cleaned.end(),
                                 [](unsigned char c) { return std::isspace(c); }),
                  cleaned.end());
    std::cout << "  [OCR-FULL] cleaned=\"" << cleaned << "\"" << std::endl;

    if (cleaned.find('=') != std::string::npos) return cleaned;

    // 没有等号：尝试推断
    if (cleaned.length() >= 3) {
        std::string inferred = trySwapLastMinus(cleaned);
        if (inferred.find('=') != std::string::npos) {
            std::cout << "  [OCR-FULL] inferred(swapLastMinus)=\"" << inferred << "\"" << std::endl;
            return inferred;
        }

        inferred = tryAllSplits(cleaned);
        std::cout << "  [OCR-FULL] inferred(tryAllSplits)=\"" << inferred << "\"" << std::endl;
        return inferred;
    }

    return cleaned;
}

// ============================================================
//  严格模式 OCR（新流水线）
// ============================================================

// ---- 左侧严格识别 ----

LeftResult recognizeLeftStrict(const cv::Mat& img) {
    LeftResult result;
    result.text     = "";
    result.reliable = false;

    if (img.empty()) {
        cout << "  [L-strict] empty image" << endl;
        return result;
    }

    tesseract::TessBaseAPI& ocr = getTesseract();
    // 使用宽松的 whitelist 让 Tesseract 自然识别，之后再严格验证
    ocr.SetVariable("tessedit_char_whitelist", "0123456789+-()");

    // 只加白边，不做额外预处理（图像已经过 preprocessUnified）
    cv::Mat proc;
    cv::copyMakeBorder(img, proc, 4, 4, 4, 4, cv::BORDER_CONSTANT, cv::Scalar(255));

    // 两次尝试，取置信度更高的
    struct Attempt { std::string text; int conf; };
    vector<Attempt> attempts;

    static const tesseract::PageSegMode psms[] = {
        tesseract::PSM_SINGLE_LINE,
        tesseract::PSM_SINGLE_BLOCK,
    };

    for (auto psm : psms) {
        ocr.SetPageSegMode(psm);
        ocr.SetImage(proc.data, proc.cols, proc.rows, 1, proc.step);
        char* out = ocr.GetUTF8Text();
        std::string text(out ? out : "");
        delete[] out;
        int conf = ocr.MeanTextConf();

        // 去空白
        text.erase(std::remove_if(text.begin(), text.end(),
                    [](unsigned char c) { return std::isspace(c); }),
                   text.end());

        cout << "  [L-strict] PSM=" << (psm == tesseract::PSM_SINGLE_LINE ? "LINE" : "BLOCK")
             << "  conf=" << conf << "  text=\"" << text << "\"" << endl;

        attempts.push_back({text, conf});
    }

    // 选最优结果
    std::string bestText;
    int bestConf = -1;
    bool bestValid = false;

    for (auto& att : attempts) {
        // 验证
        const std::string whitelist = "0123456789+-()";
        bool valid = !att.text.empty();
        for (char c : att.text)
            if (whitelist.find(c) == std::string::npos) { valid = false; break; }
        // 至少有一个数字
        if (att.text.find_first_of("0123456789") == std::string::npos) valid = false;
        // 首尾不能是运算符
        if (!att.text.empty()) {
            char first = att.text[0], last = att.text.back();
            if (first == '+' || first == '-' || first == '*' || first == '/') valid = false;
            if (last  == '+' || last  == '-' || last  == '*' || last  == '/') valid = false;
        }
        // 括号必须配对
        int parens = 0;
        for (char c : att.text) {
            if (c == '(') parens++;
            if (c == ')') parens--;
            if (parens < 0) break;
        }
        if (parens != 0) valid = false;

        if (valid && att.conf > bestConf) {
            bestText  = att.text;
            bestConf  = att.conf;
            bestValid = true;
        }
        // 如果都不合法，保留最高置信度的结果
        if (!bestValid && att.conf > bestConf) {
            bestText = att.text;
            bestConf = att.conf;
        }
    }

    result.text     = bestText;
    result.reliable = bestValid;

    if (bestValid)
        cout << "  [L-strict] PASS → \"" << bestText << "\"" << endl;
    else
        cout << "  [L-strict] FAIL, keeping \"" << bestText
             << "\" as unreliable" << endl;

    return result;
}

// ---- 右侧严格识别 ----

RightResult recognizeRightStrict(const cv::Mat& img) {
    RightResult result;
    result.digits   = "";
    result.reliable = false;

    if (img.empty()) {
        cout << "  [R-strict] empty image" << endl;
        return result;
    }

    // 第一次尝试
    std::string raw = easyOCRSingle(img);
    cout << "  [R-strict] attempt1 raw=\"" << raw << "\"" << endl;

    std::string digits;
    std::string corrected = mapOperators(raw);
    for (char c : corrected)
        if (std::isdigit(static_cast<unsigned char>(c)))
            digits += c;
    if (digits.empty())
        for (char c : raw)
            if (std::isdigit(static_cast<unsigned char>(c)))
                digits += c;

    if (!digits.empty()) {
        result.digits   = digits;
        result.reliable = true;
        cout << "  [R-strict] PASS → digits=\"" << digits << "\"" << endl;
        return result;
    }

    // 第二次尝试：调整预处理后重试
    cout << "  [R-strict] attempt2 with different preprocessing..." << endl;
    cv::Mat enhanced = preprocessForOCR(img);
    raw = easyOCRSingle(enhanced);
    cout << "  [R-strict] attempt2 raw=\"" << raw << "\"" << endl;

    corrected = mapOperators(raw);
    for (char c : corrected)
        if (std::isdigit(static_cast<unsigned char>(c)))
            digits += c;
    if (digits.empty())
        for (char c : raw)
            if (std::isdigit(static_cast<unsigned char>(c)))
                digits += c;

    if (!digits.empty()) {
        result.digits   = digits;
        result.reliable = true;
        cout << "  [R-strict] PASS (retry) → digits=\"" << digits << "\"" << endl;
    } else {
        cout << "  [R-strict] FAIL after 2 attempts, keeping empty" << endl;
    }

    return result;
}

// ---- 后处理：纠错 + 推断 ----

std::string postProcessEquation(const LeftResult& left,
                                 const RightResult& right) {
    cout << "  [post] input: L=\"" << left.text
         << "\" (rel=" << left.reliable
         << ")  R=\"" << right.digits
         << "\" (rel=" << right.reliable << ")" << endl;

    // ---- 1. 纠正常见 OCR 错误 ----
    std::string leftClean  = mapOperators(left.text);
    std::string rightClean = right.digits;

    // ---- 2. 移除小数点 ----
    leftClean.erase(std::remove(leftClean.begin(), leftClean.end(), '.'), leftClean.end());
    rightClean.erase(std::remove(rightClean.begin(), rightClean.end(), '.'), rightClean.end());

    // ---- 3. 去掉首尾运算符 ----
    while (!leftClean.empty() &&
           (leftClean.back() == '+' || leftClean.back() == '-' ||
            leftClean.back() == '*' || leftClean.back() == '/'))
        leftClean.pop_back();
    while (!leftClean.empty() &&
           (leftClean[0] == '+' || leftClean[0] == '*' || leftClean[0] == '/'))
        leftClean.erase(0, 1);
    // 保留开头 '-'（可能是负号）

    cout << "  [post] after clean: L=\"" << leftClean
         << "\"  R=\"" << rightClean << "\"" << endl;

    // ---- 4. 如果左侧无运算符 → 逐个位置、逐个运算符尝试 ----
    if (!leftClean.empty() &&
        leftClean.find_first_of("+-*/") == std::string::npos) {
        cout << "  [post] no operator in left, trying inference..." << endl;

        int bestDiff   = INT_MAX;
        int rightVal   = 0;
        bool hasRight  = false;

        if (!rightClean.empty()) {
            try { rightVal = std::stoi(rightClean); hasRight = true; }
            catch (...) {}
        }

        std::string bestLeft = leftClean;  // 默认保持原样

        // 允许的运算符（现阶段 + -）
        const char ops[] = {'+', '-'};
        // 候选插入位置：数字之间（不在开头）
        for (size_t pos = 1; pos < leftClean.length(); pos++) {
            // 不能在 '(' 后面或 ')' 前面插入
            if (leftClean[pos-1] == '(' || leftClean[pos] == ')') continue;
            // 两侧都必须是数字
            if (!std::isdigit((unsigned char)leftClean[pos-1])) continue;
            if (!std::isdigit((unsigned char)leftClean[pos])) continue;

            for (char op : ops) {
                std::string candidate = leftClean.substr(0, pos) + op + leftClean.substr(pos);
                try {
                    int lv = evaluateExpression(candidate);
                    if (hasRight) {
                        int diff = std::abs(lv - rightVal);
                        if (diff < bestDiff) {
                            bestDiff = diff;
                            bestLeft = candidate;
                        }
                    } else {
                        // 右侧为空 → 选第一个合法的
                        bestLeft = candidate;
                        bestDiff = 0;
                    }
                } catch (...) { continue; }
            }
            if (bestDiff == 0) break;  // 完美匹配，不再尝试
        }

        if (bestDiff < INT_MAX) {
            leftClean = bestLeft;
            cout << "  [post] inferred operator: \""
                 << leftClean << "\"  diff=" << bestDiff << endl;
        }
    }

    // ---- 5. 右侧 OCR 校验：计算结果 vs OCR 结果 ----
    if (!leftClean.empty() && !rightClean.empty() &&
        leftClean.find_first_of("+-*/") != std::string::npos) {
        try {
            int leftVal = evaluateExpression(leftClean);
            int rightVal = std::stoi(rightClean);
            if (leftVal != rightVal) {
                cout << "  [post] OCR mismatch: eval(" << leftClean
                     << ")=" << leftVal << "  vs OCR right=\"" << rightClean
                     << "\"(" << rightVal << ")" << endl;
                // 如果左侧可靠、运算符是推断出来的（右OCR可能不准），用计算值替代
                if (!left.reliable || left.text.find_first_of("+-*/") == std::string::npos) {
                    rightClean = std::to_string(leftVal);
                    cout << "  [post] trusting computed value, correcting right → \""
                         << rightClean << "\"" << endl;
                }
            }
        } catch (...) {}
    }

    // ---- 6. 右侧为空时额外推断 ----
    if (rightClean.empty() && !leftClean.empty()) {
        try {
            int leftVal = evaluateExpression(leftClean);
            rightClean = std::to_string(leftVal);
            cout << "  [post] right empty, using computed value: \""
                 << rightClean << "\"" << endl;
        } catch (...) {
            cout << "  [post] right empty, cannot compute" << endl;
        }
    }

    // ---- 7. 组合最终算式 ----
    std::string fullEquation = leftClean + "=" + rightClean;

    cout << "  [post] final: \"" << fullEquation << "\"" << endl;
    return fullEquation;
}

