"""
OCR 桥接脚本 —— C++ 通过命令行调用，使用 EasyOCR 引擎

用法：
    python ocr_bridge.py <image_path1> [image_path2 ...]

输出（stdout）：
    一个 JSON 数组，顺序对应输入图片
    例如：["3+5=8", "12-7=5"]

首次运行会自动下载 EasyOCR 模型到 %USERPROFILE%/.EasyOCR/model/。
"""

import sys
import json
import os
import warnings

# 必须在 import torch 之前设置（修复 PyTorch 与 OpenCV 的 OpenMP DLL 冲突）
os.environ["KMP_DUPLICATE_LIB_OK"] = "TRUE"

# 抑制 PyTorch pin_memory 警告
warnings.filterwarnings("ignore", message=".*pin_memory.*")
warnings.filterwarnings("ignore", message=".*accelerator.*")


def correct_arithmetic(text: str) -> str:
    """修正 OCR 在算术上下文中常见的字符混淆（字母→数字）"""
    corrections = {
        'O': '0', 'o': '0',
        'S': '5', 's': '5',
        'l': '1', 'I': '1',
        'Z': '2', 'z': '2',
        'B': '8',
        'g': '9', 'q': '9',
    }
    return "".join(corrections.get(ch, ch) for ch in text)


def init_reader():
    """初始化 EasyOCR Reader（仅英文，CPU 模式）"""
    import easyocr
    return easyocr.Reader(
        ['en'],
        gpu=False,
        verbose=False,
    )


def ocr_single(reader, image_path: str) -> str:
    """识别单张图片，拼接所有文本块返回"""
    try:
        results = reader.readtext(image_path)
    except Exception as e:
        print(f"EasyOCR error on {image_path}: {e}", file=sys.stderr)
        return ""

    if not results:
        return ""

    # results: [(bbox, text, confidence), ...]
    texts = []
    for item in results:
        if len(item) >= 2:
            t = item[1]
            if isinstance(t, list):
                t = " ".join(str(x) for x in t)
            if t:
                texts.append(str(t))

    # 拼接并去空格，然后修正常见混淆
    raw = "".join("".join(str(t).split()) for t in texts)
    return correct_arithmetic(raw)


def main():
    if len(sys.argv) < 2:
        print("[]", flush=True)
        return

    reader = init_reader()

    results = []
    for path in sys.argv[1:]:
        if os.path.isfile(path):
            text = ocr_single(reader, path)
            results.append(text)
        else:
            results.append("")

    print(json.dumps(results, ensure_ascii=False), flush=True)


if __name__ == "__main__":
    main()
