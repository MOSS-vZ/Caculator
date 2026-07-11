# Arithmetic Practice Grading System

口算作业批改系统 - 基于 SFML + OpenCV 的 C++ 桌面应用。

## Features

- 摄像头拍照 / 文件路径导入题目
- AI 批改（通过 Qwen-VL 视觉模型）
- AI 答案生成
- 错题本管理
- 历史记录与正确率趋势图
- 口算练习模式

## Dependencies

- **SFML 3.x** — Graphics / Window / System / Audio
- **OpenCV 4.12.0** — Camera capture
- **Python 3** — AI grading scripts (`scripts/`)
  - `openai`, `matplotlib`, `Pillow`

## Build

Open `caculator.slnx` in Visual Studio 2022, build with Debug|x64 or Release|x64.

## Project Structure

```
caculator/
  src/            C++ source code
  assets/
    images/       UI images (buttons, etc.)
    splash/       Startup animation frames
  bin/            Runtime DLLs
  scripts/        Python AI scripts
  lib/            Static libraries (.lib)
  include/        Third-party headers
  photo/          Runtime data (gitignored)
```

## 🛠️ 开发者环境部署指南 (For Developers)

如果你想克隆此仓库并在本地编译、运行和修改代码，请按照以下步骤配置环境：

### 1. 获取代码
```bash
git clone https://github.com/MOSS-vZ/Calculator.git
```

### 2. 开发工具准备
Visual Studio：本项目使用 Visual Studio 2026 开发，需要安装 C++ 桌面开发 工作负载。

Python：本项目依赖 Python 脚本执行特定功能，请确保电脑已安装 Python (本项目使用的是3.13)。

### 3. C++ 环境配置 (SFML 库)
本项目使用了 SFML 多媒体库。请在 VS 中正确配置链接：

下载 SFML (建议使用 3.1.0 版本) 并解压到你的电脑某个目录（例如 D:\SFML-3.1.0）。

打开 VS 项目 caculator.sln。

配置属性：右键项目 -> 属性。确保配置选择为 Release，平台选择 x64。

在 C/C++ -> 常规 -> 附加包含目录 中添加：D:\SFML-3.1.0\include

在 链接器 -> 常规 -> 附加库目录 中添加：D:\SFML-3.1.0\lib

在 链接器 -> 输入 -> 附加依赖项 中添加（注意不要带 -d）：

text
sfml-system.lib;sfml-window.lib;sfml-graphics.lib;sfml-audio.lib;sfml-network.lib
将解压目录 D:\SFML-3.1.0\bin 中不带 -d 的 .dll 文件复制到项目目录 x64\Release 下。

### 4. Python 运行环境配置
本项目调用了 Python 脚本，因此需要安装第三方依赖库。

在项目根目录下打开命令行（CMD 或 PowerShell）。

如果没有 requirements.txt，请手动创建一个，并填入以下依赖：
```txt
# requirements.txt 内容示例 (请根据实际使用的第三方库填写)
matplotlib
```
执行以下命令安装依赖：

```bash
pip install -r requirements.txt
```
### 5. 编译与运行
在 VS 顶部的工具栏，将模式切换为 Release 和 x64。

点击菜单栏 生成 -> 重新生成解决方案。

进入 x64\Release 文件夹，双击 caculator.exe 即可运行。