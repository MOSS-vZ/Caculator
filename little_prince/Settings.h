#pragma once
#include <string>

namespace Settings {
    inline std::string getBasePath() {
        // 请根据您的实际路径修改，使用相对路径
        return "images/begin/";
    }

    const int WINDOW_WIDTH = 2400;
    const int WINDOW_HEIGHT = 1350;

    const float PRINCE_HEIGHT_RATIO = 0.6f;
    const int BUTTON_WIDTH = 750;           // 原600 * 1.25
    const int PRINCE_LEFT_MARGIN = 431;     // 原345 * 1.25
    const int BTN_RIGHT_MARGIN = 150;       // 原120 * 1.25
    const int BTN_BOTTOM_MARGIN = 375;      // 原300 * 1.25

    const float FADE_DURATION = 0.8f;
    const float TITLE_MOVE_DURATION = 0.6f;

    inline float easeOutQuad(float t) {
        return t * (2.0f - t);
    }
}