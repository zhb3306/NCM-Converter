# NCM Converter

**NCM Converter** 是一款基于 Qt 和 C++ 开发的图形化工具，用于将网易云音乐加密的 `.ncm` 文件解密并转换为常见的音频格式（FLAC 或 MP3）。它提供了简洁、现代化的界面，支持批量处理、自定义输出目录，并集成了 FFmpeg 以实现高质量的 MP3 转换。

![截图占位](screenshot.png)

---

## 📌 功能特性

- ✅ **解密 .ncm 文件**：基于 `libncmdump` 核心库，兼容新版网易云音乐格式。
- ✅ **批量转换**：支持同时添加多个文件或整个文件夹（含子目录）。
- ✅ **输出格式选择**：可直接输出为源格式（FLAC/MP3），或一键转换为 MP3（192kbps）。
- ✅ **保留 FLAC 选项**：转换为 MP3 时可选择保留原始 FLAC 文件。
- ✅ **现代化 UI**：采用毛玻璃、圆角、渐变等设计，视觉风格明亮友好。
- ✅ **多平台支持**：使用 Qt 5.15.2 开发，可在 Windows 7 及以上系统运行。

---

## 📦 依赖组件

| 组件 | 用途 | 许可证 |
|------|------|--------|
| [libncmdump](https://github.com/taurusxin/ncmdump) | 核心解密引擎 | MIT |
| [FFmpeg](https://ffmpeg.org/) | MP3 编码转换 | LGPL-2.1+ |
| [Qt 5.15.2](https://www.qt.io/) | GUI 框架及工具链 | LGPL-3.0 / GPL-3.0 |

---

## 🔧 编译指南

### 环境要求
- Windows 7 / 10 / 11（开发测试基于 Win7）
- Visual Studio 2019 或 2026（推荐）
- Qt 5.15.2（MSVC 2019 64-bit）
- CMake（可选，使用 qmake 时无需）

### 步骤
1. **克隆源码**
   ```bash
   git clone https://github.com/zhb3306/NCM-Converter.git
   cd NCM-Converter

2. **打开项目**  
- 使用 Visual Studio 打开 `.sln` 文件（如果使用 qmake，则打开 `.pro` 文件）。  
- 确保已配置 Qt 版本（`Qt VS Tools` 中指向 `C:\Qt\5.15.2\msvc2019_64`）。
   
---

3. **配置编译选项**  
   - 平台工具集：`Visual Studio 2019 (v142)`  
   - 预处理器定义中添加：
            _SILENCE_ALL_MS_EXT_DEPRECATION_WARNINGS
            _HAS_STD_BYTE=0
            _STL_WARN_LEGACY_STDEXT_CHECKED_ITERATORS=0

     
4. **编译与运行**
- 选择 `Release x64` 配置，点击“生成解决方案”。
- 将 `libncmdump.dll` 和 `ffmpeg.exe` 复制到输出目录（与 `.exe` 同级）。
- 运行 `NCM-Converter.exe`。

---

## 🚀 使用方法

1. **启动软件**：双击运行 `NCM-Converter.exe`。
2. **添加文件**：点击“添加文件”或“添加文件夹”按钮，选择 `.ncm` 文件。
3. **设置输出目录**（可选）：在输入框中指定输出路径，留空则保存在源文件目录。
4. **选择转换选项**：
- 勾选“转换为MP3”可将解密后的 FLAC 转为 MP3（192kbps）。
- 勾选“也保留FLAC文件”可在转换 MP3 后保留原始 FLAC。
5. **开始转换**：点击“开始转换”按钮，进度条会显示处理进度。
6. **查看日志**：转换过程中的状态信息会显示在底部日志区域。

---

 ## 📂 打包发布
 使用 `windeployqt` 工具收集 Qt 运行时依赖：
   windeployqt.exe --release NCM-Converter.exe

---

## 📄 关于与贡献

### 许可证
本项目采用 **GNU General Public License v3.0** 进行许可。详细信息请查看项目根目录的 `LICENSE` 文件。

### 致谢
- [taurusxin](https://github.com/taurusxin) 开发的 [libncmdump](https://github.com/taurusxin/ncmdump)
- FFmpeg 团队
- Qt 开源社区

### 作者
**ZHB3306**  
- GitHub: [zhb3306](https://github.com/zhb3306)  
- 项目主页: [NCM-Converter](https://github.com/zhb3306/NCM-Converter)

### 反馈与贡献
如果你遇到任何问题或有改进建议，欢迎提交 [Issue](https://github.com/zhb3306/NCM-Converter/issues) 或 Pull Request。
