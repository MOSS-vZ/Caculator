#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>
#include <tesseract/baseapi.h>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <climits>
#include "split.h"

using namespace std;
using namespace cv;

// ============================================================
//  图像预处理
// ============================================================

cv::Mat deskewImage(const cv::Mat& src) {
    cv::Mat gray;
    if (src.channels() == 3)
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else
        gray = src.clone();

    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    cv::Mat coords;
    cv::findNonZero(binary, coords);
    if (coords.total() < 10) return src.clone();

    cv::RotatedRect rect = cv::minAreaRect(coords);
    double angle = rect.angle;
    if (rect.size.width < rect.size.height)
        angle = 90.0 + angle;

    cv::Mat result = src.clone();
    const double MAX_A = 45.0, MIN_A = 0.5;
    if (std::abs(angle) >= MIN_A && std::abs(angle) <= MAX_A) {
        double corr = -angle;
        cv::Point2f c(src.cols / 2.f, src.rows / 2.f);
        cv::Mat rot = cv::getRotationMatrix2D(c, corr, 1.0);
        cv::warpAffine(src, result, rot, src.size(),
                       cv::INTER_LINEAR, cv::BORDER_CONSTANT, cv::Scalar(255,255,255));
    }

    if (isUpsideDown(result)) {
        cv::rotate(result, result, cv::ROTATE_180);
    }
    return result;
}

cv::Mat preprocessUnified(const cv::Mat& src, int minHeight) {
    // 1. 灰度化
    cv::Mat gray;
    if (src.channels() == 3)
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else
        gray = src.clone();

    // 2. CLAHE 增强对比度
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(2.0, cv::Size(8, 8));
    clahe->apply(gray, gray);

    // 3. 保证最小高度（放大到 Tesseract 可读尺寸）
    if (gray.rows < minHeight) {
        double scale = (double)minHeight / gray.rows;
        cv::resize(gray, gray, cv::Size(), scale, scale, cv::INTER_CUBIC);
    }

    cout << "[preprocess] " << src.cols << "x" << src.rows
         << " → " << gray.cols << "x" << gray.rows
         << "  (CLAHE + minH=" << minHeight << ")" << endl;
    return gray;
}

bool isUpsideDown(const cv::Mat& input) {
    cv::Mat gray;
    if (input.channels() == 3)
        cv::cvtColor(input, gray, cv::COLOR_BGR2GRAY);
    else
        gray = input.clone();

    cv::Mat binary;
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    vector<vector<cv::Point>> contours;
    cv::findContours(binary.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    long long above = 0, below = 0;
    for (const auto& cnt : contours) {
        cv::Rect r = cv::boundingRect(cnt);
        if (r.width < 5 || r.height < 5) continue;
        if (r.width > binary.cols * 0.8 && r.height > binary.rows * 0.8) continue;

        int mid = r.y + r.height / 2;
        for (int y = r.y; y < r.y + r.height; y++) {
            const uchar* row = binary.ptr<uchar>(y);
            for (int x = r.x; x < r.x + r.width; x++) {
                if (row[x] > 0) {
                    if (y < mid) above++; else below++;
                }
            }
        }
    }
    if (above + below == 0) return false;
    return (above > below * 1.25);
}

vector<cv::Rect> findTextLines(const cv::Mat& binary) {
    vector<cv::Rect> lines;

    // 垂直膨胀，填平行内小缝隙（如 '=' 两横之间），避免把一行切成两行
    cv::Mat dilated;
    cv::Mat vKernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(1, 5));
    cv::dilate(binary, dilated, vKernel);

    vector<int> proj(dilated.rows, 0);
    for (int r = 0; r < dilated.rows; r++)
        proj[r] = cv::countNonZero(dilated.row(r));

    long long total = 0; int nr = 0;
    for (int v : proj) { total += v; if (v > 0) nr++; }
    if (nr == 0) return lines;
    // 降低阈值百分比（15%→8%），减少误切割
    int threshold = std::max(3, static_cast<int>((double)total / nr * 0.08));

    const int MIN_H   = 12;   // 最小行高
    const int MIN_GAP = 3;    // 连续低于阈值 >= 3 行才算行间缝隙（滞后）

    bool in = false; int start = 0; int belowCount = 0;
    for (int r = 0; r < dilated.rows; r++) {
        if (proj[r] > threshold) {
            if (!in) { start = r; in = true; }
            belowCount = 0;
        } else if (in) {
            belowCount++;
            if (belowCount >= MIN_GAP) {
                if (r - belowCount - start + 1 >= MIN_H)
                    lines.push_back(cv::Rect(0, start, binary.cols, r - belowCount + 1 - start));
                in = false;
                belowCount = 0;
            }
        }
    }
    if (in && dilated.rows - start >= MIN_H)
        lines.push_back(cv::Rect(0, start, binary.cols, dilated.rows - start));

    // 合并且得过近的行（gap < 行高均值的 30%）
    if (lines.size() > 1) {
        int avgH = 0;
        for (auto& l : lines) avgH += l.height;
        avgH /= (int)lines.size();
        int mergeGap = std::max(5, static_cast<int>(avgH * 0.30));

        vector<cv::Rect> merged = {lines[0]};
        for (size_t i = 1; i < lines.size(); i++) {
            int gap = lines[i].y - (merged.back().y + merged.back().height);
            if (gap < mergeGap)
                merged.back().height = lines[i].y + lines[i].height - merged.back().y;
            else merged.push_back(lines[i]);
        }
        lines = std::move(merged);
    }
    return lines;
}

// ============================================================
//  等号检测（自适应阈值 + 全局最优配对）
// ============================================================

vector<EqSign> findEqualsSigns(const cv::Mat& binary) {
    vector<EqSign> results;

    // ---- 1. 轮廓提取（RETR_LIST 防止等号横线与字符粘连时丢失） ----
    vector<vector<cv::Point>> contours;
    cv::Mat tmp = binary.clone();
    cv::findContours(tmp, contours, cv::RETR_LIST, cv::CHAIN_APPROX_SIMPLE);

    // ---- 2. 从轮廓统计参考尺寸（自适应基准） ----
    vector<int> charH, charW;
    for (const auto& cnt : contours) {
        cv::Rect r = cv::boundingRect(cnt);
        // "字符级"轮廓：高度明显大于宽度，且面积合理
        if (r.height > r.width && r.area() > 20 && r.area() < binary.cols * binary.rows / 4) {
            charH.push_back(r.height);
            charW.push_back(r.width);
        }
    }
    int refH = 20, refW = 8;  // 默认值（小图兜底）
    if (charH.size() >= 3) {
        sort(charH.begin(), charH.end());
        sort(charW.begin(), charW.end());
        refH = charH[charH.size() / 2];  // 中位数
        refW = charW[charW.size() / 2];
    }
    // 限制自适应范围，防止极端值
    refH = std::max(10, std::min(refH, 120));
    refW = std::max(4,  std::min(refW, 60));

    // ---- 3. 自适应 bar 筛选阈值 ----
    const int    BAR_MIN_W  = std::max(4,  refW / 2);         // 最小宽度
    const int    BAR_MAX_H  = std::max(3,  refH / 5 + 1);    // 最大高度
    const double BAR_MIN_AR = 2.5;                            // 最小宽高比（比例，分辨率无关）

    cout << "[eqsign] refH=" << refH << "px  refW=" << refW
         << "px  barMinW=" << BAR_MIN_W << "  barMaxH=" << BAR_MAX_H << endl;

    // ---- 4. 提取候选横条 ----
    struct Bar { cv::Rect r; cv::Point c; };
    vector<Bar> bars;
    for (const auto& cnt : contours) {
        cv::Rect r = cv::boundingRect(cnt);
        double ar = (double)r.width / std::max(r.height, 1);
        bool wOk = r.width >= BAR_MIN_W;
        bool hOk = r.height >= 1 && r.height <= BAR_MAX_H;
        bool arOk = ar >= BAR_MIN_AR;
        if (wOk && hOk && arOk) {
            bars.push_back({r, cv::Point(r.x + r.width/2, r.y + r.height/2)});
            cout << "  [bar] x=" << r.x << " y=" << r.y
                 << " w=" << r.width << " h=" << r.height
                 << " ar=" << ar << "  ACCEPT" << endl;
        }
    }

    cout << "[eqsign] " << bars.size() << " bars accepted (from "
         << contours.size() << " contours)" << endl;
    if (bars.size() < 2) return results;

    // ---- 5. 自适应配对阈值 ----
    const int    PAIR_GAP_MIN  = std::max(2,  refH / 20 + 1);   // 最小垂直间距
    const int    PAIR_GAP_MAX  = std::max(6,  refH / 3);        // 最大垂直间距
    const int    PAIR_DX_MAX   = std::max(3,  refW / 3);        // 最大水平偏移
    const double PAIR_OL_MIN   = 0.70;                          // 最小重叠度
    const double PAIR_WR_MIN   = 0.65;                          // 最小宽度比

    // ---- 6. 全局评分配对（分数排序，高分优先，避免贪心局部最优） ----
    struct Pair { int i, j; double score; };
    vector<Pair> pairs;

    for (size_t i = 0; i < bars.size(); i++) {
        for (size_t j = i + 1; j < bars.size(); j++) {
            int vg = std::abs(bars[i].c.y - bars[j].c.y);
            if (vg < PAIR_GAP_MIN || vg > PAIR_GAP_MAX) continue;

            int hc = std::abs(bars[i].c.x - bars[j].c.x);
            if (hc > PAIR_DX_MAX) continue;

            int ol = std::max(bars[i].r.x, bars[j].r.x);
            int or_ = std::min(bars[i].r.x + bars[i].r.width, bars[j].r.x + bars[j].r.width);
            if (or_ <= ol) continue;
            double overlap = (double)(or_ - ol) / std::min(bars[i].r.width, bars[j].r.width);
            if (overlap < PAIR_OL_MIN) continue;

            int maxW = std::max(bars[i].r.width, bars[j].r.width);
            int minW = std::min(bars[i].r.width, bars[j].r.width);
            double wRatio = (double)minW / maxW;
            if (wRatio < PAIR_WR_MIN) continue;

            // 综合评分：重叠度 + 宽度比 − 偏移惩罚
            double score = overlap * 0.4 + wRatio * 0.4
                         + (1.0 - (double)hc / PAIR_DX_MAX) * 0.2;
            pairs.push_back({(int)i, (int)j, score});
        }
    }

    // 按评分降序，高分优先选中
    sort(pairs.begin(), pairs.end(),
         [](const Pair& a, const Pair& b) { return a.score > b.score; });

    vector<bool> used(bars.size(), false);
    for (const auto& p : pairs) {
        if (used[p.i] || used[p.j]) continue;
        used[p.i] = used[p.j] = true;

        int vg = std::abs(bars[p.i].c.y - bars[p.j].c.y);
        int hc = std::abs(bars[p.i].c.x - bars[p.j].c.x);

        EqSign eq;
        eq.box.x = std::min(bars[p.i].r.x, bars[p.j].r.x);
        eq.box.y = std::min(bars[p.i].r.y, bars[p.j].r.y);
        eq.box.width  = std::max(bars[p.i].r.x + bars[p.i].r.width,
                                  bars[p.j].r.x + bars[p.j].r.width) - eq.box.x;
        eq.box.height = std::max(bars[p.i].r.y + bars[p.i].r.height,
                                  bars[p.j].r.y + bars[p.j].r.height) - eq.box.y;
        eq.center.x = eq.box.x + eq.box.width / 2;
        eq.center.y = eq.box.y + eq.box.height / 2;
        results.push_back(eq);

        cout << "  [eqsign] x=" << eq.center.x << " y=" << eq.center.y
             << "  w=" << eq.box.width << " h=" << eq.box.height
             << "  bars:(" << bars[p.i].r.width << "x" << bars[p.i].r.height
             << ", " << bars[p.j].r.width << "x" << bars[p.j].r.height
             << ") gap=" << vg << " dx=" << hc
             << " score=" << p.score << endl;
    }

    // ---- 7. 自适应排序 ----
    const int SORT_Y_THRESH = std::max(15, refH / 2);
    std::sort(results.begin(), results.end(), [=](const EqSign& a, const EqSign& b) {
        if (std::abs(a.center.y - b.center.y) > SORT_Y_THRESH)
            return a.center.y < b.center.y;
        return a.center.x < b.center.x;
    });
    return results;
}

// ============================================================
//  算式检测（主函数）
//  —— 等号锚点 + 间隙兜底
// ============================================================

static bool rectsOverlap(const cv::Rect& a, const cv::Rect& b, int margin = 2) {
    int aEndX = a.x + a.width;
    int aEndY = a.y + a.height;
    int bEndX = b.x + b.width;
    int bEndY = b.y + b.height;
    return (a.x - margin < bEndX && aEndX + margin > b.x &&
            a.y - margin < bEndY && aEndY + margin > b.y);
}

static void mergeOverlappingBoxes(vector<cv::Rect>& boxes) {
    for (size_t i = 0; i < boxes.size(); i++) {
        for (size_t j = i + 1; j < boxes.size(); j++) {
            if (rectsOverlap(boxes[i], boxes[j])) {
                int iEnd = boxes[i].x + boxes[i].width;
                int jEnd = boxes[j].x + boxes[j].width;
                int iBot = boxes[i].y + boxes[i].height;
                int jBot = boxes[j].y + boxes[j].height;
                boxes[i].x = std::min(boxes[i].x, boxes[j].x);
                boxes[i].y = std::min(boxes[i].y, boxes[j].y);
                boxes[i].width  = std::max(iEnd, jEnd) - boxes[i].x;
                boxes[i].height = std::max(iBot, jBot) - boxes[i].y;
                boxes.erase(boxes.begin() + j);
                j--;
            }
        }
    }
}

// ---- 辅助：在两个等号之间找到最佳分界间隙 ----
// 在 [xLeft, xRight] 范围内找最大的字符间隙；找不到则返回 -1
static int findBestBoundary(const vector<cv::Rect>& charBoxes,
                             int xLeft, int xRight) {
    int bestX = -1, bestGap = -1;
    for (size_t i = 1; i < charBoxes.size(); i++) {
        int gapStart = charBoxes[i-1].x + charBoxes[i-1].width;
        int gapEnd   = charBoxes[i].x;
        // 间隙必须与搜索范围有交集
        if (gapEnd <= xLeft || gapStart >= xRight) continue;
        int gapSize = gapEnd - gapStart;
        if (gapSize > bestGap) {
            bestGap = gapSize;
            bestX = (gapStart + gapEnd) / 2;
        }
    }
    return bestX;
}

// ---- 辅助：按 x 范围收集字符外接矩形 ----
static cv::Rect collectCharBounds(const vector<cv::Rect>& charBoxes,
                                   int xMin, int xMax) {
    int rx = xMax, ry = INT_MAX, rRight = xMin, rBot = 0;
    bool found = false;
    for (const auto& b : charBoxes) {
        int mid = b.x + b.width / 2;
        if (mid >= xMin && mid <= xMax) {
            rx = std::min(rx, b.x);
            ry = std::min(ry, b.y);
            rRight = std::max(rRight, b.x + b.width);
            rBot   = std::max(rBot,  b.y + b.height);
            found = true;
        }
    }
    if (!found) {
        rx = xMin;  rRight = xMax;
        ry = 0;     rBot  = 20;
    }
    return cv::Rect(rx, ry, rRight - rx, rBot - ry);
}

// ---- 辅助：输出 region ----
static void emitRegion(vector<EqRegion>& out,
                        const cv::Mat& src,
                        const cv::Rect& line,
                        const cv::Rect& bounds,
                        const cv::Point& eqCenter) {
    const int MIN_W = 30, MIN_H = 12;
    if (bounds.width < MIN_W || bounds.height < MIN_H) return;

    const int PAD = 8;
    int gx = std::max(0, line.x + bounds.x - PAD);
    int gy = std::max(0, line.y + bounds.y - PAD);
    int gw = std::min(src.cols - gx, bounds.width  + 2 * PAD);
    int gh = std::min(src.rows - gy, bounds.height + 2 * PAD);

    EqRegion reg;
    reg.rect    = cv::Rect(gx, gy, gw, gh);
    reg.image   = src(reg.rect).clone();
    reg.eqLocal = (eqCenter.x >= 0)
        ? cv::Point(eqCenter.x - gx, eqCenter.y - gy)
        : cv::Point(-1, -1);
    out.push_back(reg);
}

// ============================================================
//  主入口
// ============================================================

vector<EqRegion> detectEquations(const cv::Mat& src) {
    vector<EqRegion> results;

    // 1. 二值化
    cv::Mat gray, binary;
    if (src.channels() == 3)
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    else
        gray = src.clone();
    cv::threshold(gray, binary, 0, 255, cv::THRESH_BINARY_INV | cv::THRESH_OTSU);

    // 2. 文字行
    auto lines = findTextLines(binary);
    if (lines.empty()) lines.push_back(cv::Rect(0, 0, binary.cols, binary.rows));

    // 3. 等号检测
    auto eqSigns = findEqualsSigns(binary);

    cout << "[detect] " << lines.size() << " line(s), "
         << eqSigns.size() << " equals sign(s)." << endl;

    // 4. 逐行处理
    for (const auto& line : lines) {
        cv::Mat lineBin = binary(line).clone();

        // 4a. 提取字符框
        vector<vector<cv::Point>> charContours;
        cv::findContours(lineBin, charContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

        vector<cv::Rect> charBoxes;
        for (const auto& cnt : charContours) {
            cv::Rect r = cv::boundingRect(cnt);
            if (r.width >= 3 && r.height >= 5) charBoxes.push_back(r);
        }
        if (charBoxes.size() < 2) continue;

        mergeOverlappingBoxes(charBoxes);
        sort(charBoxes.begin(), charBoxes.end(),
             [](const cv::Rect& a, const cv::Rect& b) { return a.x < b.x; });

        // 4b. 找出本行内的等号（转行内坐标）
        vector<EqSign> lineEqs;
        for (const auto& eq : eqSigns) {
            if (eq.center.y >= line.y &&
                eq.center.y <= line.y + line.height) {
                EqSign local = eq;
                local.center.x -= line.x;
                local.center.y -= line.y;
                local.box.x -= line.x;
                local.box.y -= line.y;
                lineEqs.push_back(local);
            }
        }
        sort(lineEqs.begin(), lineEqs.end(),
             [](const EqSign& a, const EqSign& b) { return a.center.x < b.center.x; });

        // 4c. 参考量
        vector<int> gaps;
        for (size_t i = 1; i < charBoxes.size(); i++) {
            int g = charBoxes[i].x - (charBoxes[i-1].x + charBoxes[i-1].width);
            if (g >= 0) gaps.push_back(g);
        }
        int medianGap = gaps.empty() ? 5 :
            (sort(gaps.begin(), gaps.end()), gaps[gaps.size() / 2]);

        int avgW = 0;
        for (const auto& b : charBoxes) avgW += b.width;
        avgW /= (int)charBoxes.size();

        if (!lineEqs.empty()) {
            // ================================================
            //  有等号：以等号为锚分割
            // ================================================
            cout << "[detect] y=" << line.y << "  boxes=" << charBoxes.size()
                 << "  eqs=" << lineEqs.size()
                 << "  medianGap=" << medianGap << "px  avgW=" << avgW
                 << "px  (eq-anchored)" << endl;

            int prevBoundary = 0;

            for (size_t k = 0; k < lineEqs.size(); k++) {
                int eqX = lineEqs[k].center.x;

                // 右边界
                int nextBoundary;
                if (k + 1 < lineEqs.size()) {
                    int nextEqX = lineEqs[k+1].center.x;
                    int mid = (eqX + nextEqX) / 2;

                    // 在两等号之间找最大字符间隙
                    int best = findBestBoundary(charBoxes, eqX, nextEqX);

                    if (best >= 0) {
                        // 找到了间隙 → 用它做分界
                        nextBoundary = best;
                        cout << "  [boundary] eq[" << k << "]↔eq[" << (k+1)
                             << "]  mid=" << mid << "  gapX=" << best << endl;
                    } else {
                        // 没找到正向间隙（字符重叠或无gap）→ 用中点
                        nextBoundary = mid;
                        cout << "  [boundary] eq[" << k << "]↔eq[" << (k+1)
                             << "]  mid=" << mid << "  (no gap, using midpoint)"
                             << endl;
                    }
                } else {
                    nextBoundary = line.width;
                }

                cv::Rect bounds = collectCharBounds(charBoxes,
                                                     prevBoundary,
                                                     nextBoundary);

                emitRegion(results, src, line, bounds,
                           cv::Point(lineEqs[k].center.x,
                                     lineEqs[k].center.y));

                prevBoundary = nextBoundary;
            }
        } else {
            // ================================================
            //  无等号：整行作为一个区域（不冒险拆分）
            // ================================================
            cv::Rect bounds = collectCharBounds(charBoxes, 0, line.width);
            cout << "[detect] y=" << line.y << "  boxes=" << charBoxes.size()
                 << "  avgW=" << avgW << "px  (no eq, whole-line)" << endl;
            emitRegion(results, src, line, bounds, cv::Point(-1, -1));
        }
    }

    // 5. 统计
    int withEq = 0;
    for (const auto& r : results)
        if (r.eqLocal.x >= 0) withEq++;

    cout << "[detect] " << results.size() << " region(s), "
         << withEq << " with visual '='." << endl;
    return results;
}

// ============================================================
//  Tesseract OCR 全局等号检测 + ROI 切分
// ============================================================

vector<EqSplit> findEqualsByOCR(const cv::Mat& preprocessedImg) {
    vector<EqSplit> results;

    // ---- 1. 初始化 Tesseract ----
    static tesseract::TessBaseAPI ocr;
    static bool tesseractOk = false;
    if (!tesseractOk) {
        if (ocr.Init("tessdata\\", "eng") == 0) {
            ocr.SetVariable("tessedit_ocr_engine_mode", "1");
            tesseractOk = true;
        } else {
            cerr << "[eq-ocr] Tesseract init FAILED" << endl;
            return results;
        }
    }

    // ---- 2. 全局 OCR（RIL_SYMBOL 获取每个字符的 box） ----
    ocr.SetPageSegMode(tesseract::PSM_SINGLE_BLOCK);
    ocr.SetImage(preprocessedImg.data, preprocessedImg.cols,
                 preprocessedImg.rows, 1, preprocessedImg.step);
    ocr.Recognize(0);

    tesseract::ResultIterator* it = ocr.GetIterator();
    if (!it) return results;

    // ---- 3. 收集所有 '=' 的包围盒 ----
    struct EqBox { cv::Rect box; cv::Point center; };
    vector<EqBox> eqBoxes;

    do {
        char* word = it->GetUTF8Text(tesseract::RIL_SYMBOL);
        if (word) {
            std::string s(word);
            delete[] word;
            // 匹配 '=' 符号
            if (s == "=") {
                int x1, y1, x2, y2;
                if (it->BoundingBox(tesseract::RIL_SYMBOL, &x1, &y1, &x2, &y2)) {
                    EqBox eb;
                    eb.box = cv::Rect(x1, y1, x2 - x1, y2 - y1);
                    eb.center = cv::Point(x1 + (x2 - x1) / 2, y1 + (y2 - y1) / 2);
                    eqBoxes.push_back(eb);
                }
            }
        }
    } while (it->Next(tesseract::RIL_SYMBOL));
    delete it;

    cout << "[eq-ocr] found " << eqBoxes.size() << " '=' symbol(s)" << endl;
    if (eqBoxes.empty()) return results;

    // ---- 4. 合并重叠框（同一个等号可能被多次识别） ----
    // 按 x 排序后合并水平重叠的框
    sort(eqBoxes.begin(), eqBoxes.end(),
         [](const EqBox& a, const EqBox& b) { return a.box.x < b.box.x; });

    vector<EqBox> merged;
    for (const auto& eb : eqBoxes) {
        bool found = false;
        for (auto& m : merged) {
            // 如果两个框水平重叠且垂直接近 → 合并
            if (m.box.x + m.box.width >= eb.box.x &&
                eb.box.x + eb.box.width >= m.box.x &&
                std::abs(m.center.y - eb.center.y) < 15) {
                m.box.x = std::min(m.box.x, eb.box.x);
                m.box.y = std::min(m.box.y, eb.box.y);
                int newR = std::max(m.box.x + m.box.width,
                                    eb.box.x + eb.box.width);
                int newB = std::max(m.box.y + m.box.height,
                                    eb.box.y + eb.box.height);
                m.box.width  = newR - m.box.x;
                m.box.height = newB - m.box.y;
                m.center.x = m.box.x + m.box.width / 2;
                m.center.y = m.box.y + m.box.height / 2;
                found = true;
                break;
            }
        }
        if (!found) merged.push_back(eb);
    }

    cout << "[eq-ocr] after merge: " << merged.size() << " unique '='" << endl;

    // ---- 5. 获取文本行边界 ----
    cv::Mat binary;
    cv::threshold(preprocessedImg, binary, 0, 255,
                  cv::THRESH_BINARY_INV | cv::THRESH_OTSU);
    auto lines = findTextLines(binary);
    if (lines.empty())
        lines.push_back(cv::Rect(0, 0, preprocessedImg.cols, preprocessedImg.rows));

    // 将等号分配到对应的行
    struct LineEqs { cv::Rect line; vector<EqBox> eqs; };
    vector<LineEqs> lineData;
    for (const auto& line : lines) {
        LineEqs le;
        le.line = line;
        for (const auto& eb : merged) {
            if (eb.center.y >= line.y && eb.center.y <= line.y + line.height)
                le.eqs.push_back(eb);
        }
        if (!le.eqs.empty()) {
            sort(le.eqs.begin(), le.eqs.end(),
                 [](const EqBox& a, const EqBox& b) { return a.center.x < b.center.x; });
            lineData.push_back(le);
        }
    }

    // ---- 6. 生成 ROI ----
    // 使用所有等号的包围盒中位数高度来估算行高
    int eqH = 10;
    if (!merged.empty()) {
        vector<int> heights;
        for (const auto& eb : merged) heights.push_back(eb.box.height);
        sort(heights.begin(), heights.end());
        eqH = heights[heights.size() / 2];
    }

    for (auto& ld : lineData) {
        // 竖直范围：行上下边界 + 少量 padding
        int top  = ld.line.y;
        int bot  = ld.line.y + ld.line.height;
        int padV = std::max(4, eqH);

        for (size_t k = 0; k < ld.eqs.size(); k++) {
            EqSplit split;
            split.eqCenter = ld.eqs[k].center;
            split.eqBox    = ld.eqs[k].box;
            split.lineIdx  = (int)results.size();  // 调试用

            // 水平分界
            int leftBound, rightBound;
            if (k == 0)
                leftBound = 0;
            else
                leftBound = (ld.eqs[k-1].center.x + ld.eqs[k].center.x) / 2;

            if (k + 1 < ld.eqs.size())
                rightBound = (ld.eqs[k].center.x + ld.eqs[k+1].center.x) / 2;
            else
                rightBound = preprocessedImg.cols;

            // 构建 fullROI
            int x = leftBound;
            int y = std::max(0, top - padV);
            int w = std::min(preprocessedImg.cols - x, rightBound - leftBound);
            int h = std::min(preprocessedImg.rows - y, (bot - top) + 2 * padV);

            split.fullROI = cv::Rect(x, y, w, h);

            cout << "  [eq-split] #" << results.size()
                 << "  eq@(" << split.eqCenter.x << "," << split.eqCenter.y << ")"
                 << "  ROI=[" << x << "," << y << " " << w << "x" << h << "]"
                 << "  left=" << leftBound << " right=" << rightBound << endl;

            results.push_back(split);
        }
    }

    return results;
}
