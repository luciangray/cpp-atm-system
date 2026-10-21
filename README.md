# ATM 银行模拟系统

C++ 课程设计，包含控制台和 Qt 图形界面，共用同一套业务代码。

## 功能与演示账户

支持登录、查询余额、存款、取款、修改密码、退卡，以及交易记录和余额曲线。账户与交易记录保存到文本文件，重启后恢复。

| 卡号 | 初始密码 | 初始余额 |
| --- | --- | --- |
| 6222020000000001 | 123456 | 1000.00 元 |
| 6222020000000002 | 234567 | 2500.00 元 |
| 6222020000000003 | 345678 | 500.00 元 |

密码为 6 位数字；存款支持到分，取款必须为 100 元的整数倍。程序内部用整数分表示金额。

## 编译和运行

环境：Ubuntu / WSL，GCC、CMake、Qt 6。首次安装依赖：

```bash
sudo apt install g++ cmake qt6-base-dev qt6-base-dev-tools gdb fonts-noto-cjk python3
```

在项目根目录执行：

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build -j 4
./build/atm_console
./build/atm_gui
```

两个界面默认共用 `data/atm_data.txt`，依次运行即可。文件不存在时自动创建演示账户。关闭程序后，可恢复初始账户并清空记录：

```bash
cp data/demo_data.txt data/atm_data.txt
```

也可以通过 `--data 文件路径` 使用独立数据。数据文件由程序维护，不建议手工修改；密码以文本保存，仅用于本地课程演示。

## 主要代码

| 文件 | 用途 |
| --- | --- |
| `BankCard.h/.cpp` | 银行卡信息、余额和密码操作 |
| `Money.h/.cpp` | 金额解析、格式化和密码格式检查 |
| `Transaction.h` | 一笔交易的数据结构 |
| `ATM.h/.cpp` | 登录、存取款、交易记录和文件读写 |
| `main_console.cpp` | 控制台菜单 |
| `ATMWindow.h/.cpp` | Qt 界面和按钮响应 |
| `BalanceChart.h/.cpp` | 余额曲线 |
| `main_gui.cpp` | 图形界面入口 |

以上文件位于 `src/`。校园图片位于 `assets/`，通过 `resources.qrc` 编入程序。

## 调试和测试

VS Code 通过 WSL 打开项目，在 WSL 中安装 Microsoft C/C++ 扩展。`Ctrl+Shift+B` 编译整个项目；F5 选择 `ATM - GDB Source Debug`，自动编译并停在控制台入口。调试使用独立的 `build/debug-session/accounts.txt`，不改默认账户。

```bash
ctest --test-dir build --output-on-failure
```

`tests/` 包含核心业务、控制台和 Qt 界面测试，使用临时数据文件。
