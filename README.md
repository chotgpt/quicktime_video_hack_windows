# quicktime_video_hack_windows

将 iOS 设备通过 **有线方式投屏到 Windows**，并实时捕获设备端 **音频与视频流**。  
项目基于 QuickTime 协议的原理，用 **C++** 重新实现，并提供 **命令行与 Qt GUI 示例**。

---

## 功能简介
- 通过数据线在 Windows 上采集 iOS 屏幕画面（视频流和音频流）  
- 支持多设备
- 支持二次开发，提供音视频流回调接口  
- 附带 Qt 简易投屏界面示例  

---

## 致谢
本项目的实现参考并致谢于 Daniel Paulus 的 Go 项目：  
👉 [danielpaulus/quicktime_video_hack](https://github.com/danielpaulus/quicktime_video_hack)

本仓库为 **基于该项目思路，使用 C++ 重新开发** 的版本。  
感谢原作者的逆向研究成果与公开文档。

---

## 仓库结构
```
/
├─ ios_line_cast_screen/     # 投屏核心库（C++ DLL）与测试示例程序
├─ libimobiledevice_win/     # 重新编译并修改的 usbmuxd 版本（用于进入 QuickTime 模式）
├─ packages/                 # 第三方依赖库与自编译的静态/动态库
├─ qt_ios_line_cast_screen/  # Qt 图形界面测试程序（视频预览界面）
├─ release/                  # 构建产物与最终打包发布目录
├─ tool/                     # 驱动安装器及辅助工具（usbmuxd、ideviceinfo等）


```
## 使用方法
### 1. 启动 usbmuxd 服务
进入 `tool` 目录并运行：usbmuxd.exe

该进程会监听端口 **37015**，并在检测到 iOS 设备连接时自动进入 QuickTime 模式。

### 2. 连接并解锁手机
确保设备通过数据线连接电脑，并解锁屏幕。  
首次连接需在手机上点击“信任此电脑”。

### 3. 安装驱动
使用 `tool\驱动安装器` 安装驱动。可选择：
- **libusb0 驱动**
- **libusb 过滤层驱动**

> 注意：libusb 驱动与 Apple 官方驱动会冲突。

### 4. 运行投屏程序
可以选择运行以下任意程序进行测试：

#### 命令行版本
release\test_x64Debug.exe

程序启动后会自动检测设备，并通过回调输出音视频流。  
开发者可在源码中实现自定义回调函数来处理流数据（保存或显示）。

#### Qt 界面版本
release\qt_ios_line_cast_screen.exe

Qt 示例提供简单的画面预览窗口，可直接显示来自 iOS 的实时视频流。（作者音视频解码开发水平有限，仅供预览界面）
```
