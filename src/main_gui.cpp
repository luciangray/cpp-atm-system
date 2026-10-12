#include "ATMWindow.h"
#include <QApplication>
#include <QMessageBox>

int main(int argc, char* argv[]) {
    // 本项目在 WSLg 中演示，使用已验证的 X11 窗口后端。
    if (qEnvironmentVariableIsSet("WSL_DISTRO_NAME") && qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM")) {
        qputenv("QT_QPA_PLATFORM", "xcb");
    }
    QApplication app(argc, argv);
    std::string data_file = ATM_DEFAULT_DATA_FILE;
    QStringList args = app.arguments();
    if (args.size() == 3 && args[1] == "--data") {
        data_file = args[2].toStdString();
    } else if (args.size() != 1) {
        QMessageBox::warning(nullptr, "参数错误", "用法：atm_gui [--data 数据文件路径]");
        return 1;
    }
    ATM atm(data_file);
    if (!atm.load()) {
        QMessageBox::critical(nullptr, "数据读取失败", QString::fromStdString(atm.message()));
        return 1;
    }
    ATMWindow window(atm);
    window.show();
    return app.exec();
}
