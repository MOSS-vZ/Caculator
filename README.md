## 文件结构
```
ImageEditor/
├── CMakeLists.txt          # 构建文件（链接OpenCV、SFML、OCR库）
├── README.md               # 项目说明
├── src/                    # 源代码
│   ├── main.cpp            # 程序入口
│   ├── App.cpp/h           # 主应用类（管理窗口和主循环）
│   ├── gui/                # 界面绘制
│   │   ├── Canvas.cpp/h    # 图片显示区域
│   │   └── Toolbar.cpp/h   # 按钮/菜单栏
│   ├── core/               # 核心数据
│   │   ├── ImageModel.cpp/h # 图像数据封装
│   │   └── History.cpp/h   # 撤销/重做栈
│   ├── processors/         # 图像处理（核心算法）
│   │   ├── Filter.cpp/h    # 灰度、二值化、模糊
│   │   ├── Transform.cpp/h # 旋转、缩放、翻转
│   │   └── Arithmetic.cpp/h# 识别算式并计算
│   └── ocr/                # OCR识别
│       └── OCRWrapper.cpp/h # 封装OCR库（如Tesseract）
├── resources/              # 资源文件（字体、图标）
└── docs/                   # 设计文档存放处
```