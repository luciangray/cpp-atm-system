# ATM 银行模拟系统

C++ 面向对象课程设计。提供控制台版和简洁的中文 Qt 图形界面，两者共用相同的银行卡类、ATM 类和数据文件。

## 功能

GUI 使用用户提供的燕山大学校园照片：宽窗口登录面板靠左，小窗口居中，账户操作面板居中。以 1080×720 为基准，窗口变大时统一放大面板、字体、控件、间距和图标，最高 2.5 倍；小窗口保留基本可读尺寸。包含圆角面板、柔和阴影、银行卡样式、简洁图标和按钮反馈。右上角以两行显示较大的时间和日期；没有校园标语、英文装饰标题或课程说明页脚。弹窗、交易表格与余额曲线随主窗口比例放大。

- 卡号、密码登录；查询余额；存款；取款；修改密码；退卡。
- 查看本账户的存取款记录，按交易顺序绘制余额变化曲线。
- 文本文件保存账户、密码和交易记录，重启后恢复。
- 预置三个模拟账户，无注册、管理员、转账和网络功能。

| 卡号 | 初始密码 | 初始余额 |
| --- | --- | --- |
| 6222020000000001 | 123456 | 1000.00 元 |
| 6222020000000002 | 234567 | 2500.00 元 |
| 6222020000000003 | 345678 | 500.00 元 |

密码为 6 位数字。存款支持到分；取款必须为 100 元的整数倍。金额内部使用 `long long` 保存整数分，余额上限为 10 亿元。改密需要原密码和两次相同的新密码。退卡只退出登录，不删除账户。

控制台使用终端的备用屏幕，每次操作后刷新页面，退出时恢复原命令行画面；菜单下方保留最近一次操作结果，存取款显示具体金额与当前余额。交易记录每页 5 笔，输入 1 上一页、2 下一页，按回车返回。使用少量终端控制序列，不需要额外库；输出重定向到文件或管道时不使用这些序列。

## 开发环境

在 Windows 上通过 VS Code 的 WSL 扩展打开 Ubuntu 中的项目目录，左下角应显示 WSL。推荐安装 C/C++ 扩展用于 F5 调试。

本项目验证环境：Ubuntu 24.04 / WSL2、GCC 13.3、CMake 3.28、Qt 6.4、WSLg。Qt 只使用 Widgets 和 QPainter；Qt Test 用于界面测试，不是业务依赖。Python 3 用于控制台进程测试。

新环境安装依赖：

```bash
sudo apt-get update
sudo apt-get install -y g++ cmake qt6-base-dev qt6-base-dev-tools gdb fonts-noto-cjk python3
```

## 编译和运行

```bash
cd ~/MyCodes/cpp-atm-system
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j 4
./build/atm_gui
# 或运行控制台版
./build/atm_console
```

首次运行会自动生成 `data/atm_data.txt`。默认数据位置由 CMake 根据源码目录设置，因此从其他当前目录启动也可正常读取。将源码移动到其他目录后，应重新配置、编译；提交的是完整源码工程，而不是单独一个 Linux 可执行文件。

两个版本使用同一个默认数据文件，演示时依次使用即可。需要独立账户数据时指定路径：

```bash
./build/atm_gui --data /tmp/atm-demo/data.txt
./build/atm_console --data /tmp/atm-demo/data.txt
```

需要恢复默认演示账户时，先关闭程序，再执行下面的命令。它会恢复初始密码、余额，并清空历史记录：

```bash
cp data/demo_data.txt data/atm_data.txt
```

## VS Code 使用

- `Ctrl+Shift+B`：配置并编译整个项目。
- “终端 → 运行任务”：选择“ATM: 运行图形界面”“ATM: 运行控制台”或“ATM: 运行测试”。
- F5：选择“ATM: 调试图形界面”或“ATM: 调试控制台”，可以在核心业务代码中设置断点。

工程包含多个源文件，使用这里提供的整项目任务，不要使用“只编译当前文件”的运行按钮。

## 测试

```bash
ctest --test-dir build --output-on-failure
```

测试覆盖金额输入、登录、存取款、改密、退卡、账户隔离、保存失败和重启恢复。控制台测试启动真实可执行程序；Qt 测试操作真实按钮、输入框和对话框。测试使用临时文件，不改动默认账户。

在 WSLg 中运行界面测试并保存截图：

```bash
QT_QPA_PLATFORM=xcb ./build/gui_tests --screenshots output/screenshots
```

## 目录和阅读顺序

```text
src/
  Money.h / Money.cpp               金额输入、显示、密码格式
  BankCard.h / BankCard.cpp         银行卡数据与基本操作
  Transaction.h                    交易记录结构体
  ATM.h / ATM.cpp                   登录状态、业务操作、文件保存
  main_console.cpp                 控制台菜单
  ATMWindow.h / ATMWindow.cpp       Qt 中文界面
  BalanceChart.h / BalanceChart.cpp QPainter 曲线
  main_gui.cpp                     GUI 入口
assets/campus.jpg                  原始校园照片
resources.qrc                      将照片打包为 Qt 内嵌资源
tests/                             核心、控制台和界面测试
data/demo_data.txt                 初始模拟数据
docs/                              代码讲解和测试说明
.vscode/                           项目编译、运行、调试配置
```

建议按“Money → BankCard → ATM → 控制台 → Qt 界面 → 曲线”的顺序理解。Qt 的重点是布局、信号槽和 `paintEvent`，可以结合 `docs/代码讲解.md` 阅读。

## 课程交付

源码压缩包包含 `src`、`tests`、`assets`、`resources.qrc`、`data/demo_data.txt`、`docs`、`.vscode`、CMake 和 README，排除构建目录与已修改的账户数据。照片编译进程序，运行时不依赖网络或当前目录中的图片文件。视频和正式报告留到后续阶段。

本项目为本地课程模拟，密码按文本保存。封装用于明确类的职责，不表示生产环境安全体系。继承和运行时多态体现在 `BalanceChart : public QWidget` 与 `paintEvent` 重写中。
