#include "ATMWindow.h"
#include "BalanceChart.h"
#include <QApplication>
#include <QDialog>
#include <QDir>
#include <QFrame>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>
#include <functional>
#include <iostream>
#include <stdexcept>

#define CHECK(condition) do { if (!(condition)) { \
    throw std::runtime_error(std::string("检查失败：") + #condition + "，行 " + std::to_string(__LINE__)); \
} } while (false)

template<class T>
T* child(QWidget* parent, const char* name) {
    T* result = parent->findChild<T*>(name);
    CHECK(result != nullptr);
    return result;
}

void click(QWidget* parent, const char* name) {
    QTest::mouseClick(child<QPushButton>(parent, name), Qt::LeftButton);
    QApplication::processEvents();
}

void enter(QWidget* parent, const char* name, const QString& text) {
    QLineEdit* field = child<QLineEdit>(parent, name);
    field->clear();
    QTest::keyClicks(field, text);
}

void modal_action(ATMWindow& window, const char* button,
                  const std::function<void(QDialog*)>& action) {
    std::string error;
    QTimer::singleShot(80, [&]() {
        QDialog* dialog = qobject_cast<QDialog*>(QApplication::activeModalWidget());
        try {
            CHECK(dialog != nullptr);
            action(dialog);
        } catch (const std::exception& exception) {
            error = exception.what();
            if (dialog) { dialog->reject(); }
        }
    });
    click(&window, button);
    CHECK(error.empty());
}

void amount(ATMWindow& window, const char* button, const QString& value) {
    modal_action(window, button, [&](QDialog* dialog) {
        enter(dialog, "amountInput", value);
        click(dialog, "confirmButton");
    });
}

void change_password(ATMWindow& window, const QString& old_password,
                     const QString& new_password, const QString& confirmation) {
    modal_action(window, "passwordButton", [&](QDialog* dialog) {
        enter(dialog, "oldPasswordInput", old_password);
        enter(dialog, "newPasswordInput", new_password);
        enter(dialog, "confirmPasswordInput", confirmation);
        click(dialog, "confirmButton");
    });
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    QString screenshot_dir;
    if (app.arguments().size() == 3 && app.arguments()[1] == "--screenshots") {
        screenshot_dir = app.arguments()[2];
        CHECK(QDir().mkpath(screenshot_dir));
    }
    QTemporaryDir temp;
    try {
        CHECK(temp.isValid());
        ATM atm((temp.path() + "/data.txt").toStdString());
        CHECK(atm.load());
        ATMWindow window(atm);
        window.show();
        QTest::qWait(150);
        auto screenshot = [&](QWidget* widget, const QString& name) {
            if (!screenshot_dir.isEmpty()) {
                CHECK(widget->grab().save(screenshot_dir + "/" + name + ".png"));
            }
        };
        auto status = [&]() { return child<QLabel>(&window, "statusLabel")->text(); };
        auto login = [&](const QString& number, const QString& password) {
            enter(&window, "cardInput", number);
            enter(&window, "passwordInput", password);
            click(&window, "loginButton");
        };
        CHECK(!QPixmap(":/images/campus.jpg").isNull());
        auto check_layout = [&]() {
            QFrame* panel = child<QFrame>(&window, "contentPanel");
            CHECK(window.rect().contains(panel->geometry()));
            double scale = qBound(1.0, qMin(window.width() / 1080.0, window.height() / 720.0), 2.5);
            CHECK(panel->width() <= qRound((atm.is_logged_in() ? 660 : 450) * scale));
            CHECK(panel->width() >= qRound((atm.is_logged_in() ? 580 : 450) * scale));
            QFrame* clock = child<QFrame>(&window, "clockPanel");
            if (window.width() / scale >= 1040) {
                CHECK(clock->isVisible());
                CHECK(window.rect().contains(clock->geometry()));
                CHECK(!clock->geometry().intersects(panel->geometry()));
                for (const char* name : {"clockLabel", "dateLabel"}) {
                    QLabel* label = child<QLabel>(&window, name);
                    CHECK(clock->rect().contains(QRect(label->mapTo(clock, QPoint(0, 0)), label->size())));
                }
            }
            const char* inputs[] = {"cardInput", "passwordInput", "loginButton"};
            const char* buttons[] = {"balanceButton", "depositButton", "withdrawButton", "passwordButton", "historyButton", "logoutButton"};
            auto check_widget = [&](const char* name) {
                QWidget* widget = child<QWidget>(&window, name);
                CHECK(widget->isVisible());
                QRect bounds(widget->mapTo(&window, QPoint(0, 0)), widget->size());
                CHECK(panel->geometry().contains(bounds));
                CHECK(widget->height() >= 30);
                QWidget* page = child<QStackedWidget>(&window, "pages")->currentWidget();
                if (page->isAncestorOf(widget)) {
                    QRect page_bounds(widget->mapTo(page, QPoint(0, 0)), widget->size());
                    CHECK(page->rect().contains(page_bounds));
                }
            };
            if (atm.is_logged_in()) {
                for (const char* name : buttons) { check_widget(name); }
            } else {
                for (const char* name : inputs) { check_widget(name); }
            }
            check_widget("statusLabel");
        };
        screenshot(&window, "01-login");
        for (QSize size : {QSize(640, 620), QSize(1080, 720), QSize(1920, 1080), QSize(2560, 1600)}) {
            window.resize(size);
            QTest::qWait(220);
            check_layout();
            screenshot(&window, QString("login-%1x%2").arg(size.width()).arg(size.height()));
            if (size.width() >= 1000) {
                CHECK(child<QFrame>(&window, "contentPanel")->x() < window.width() / 4);
                CHECK(!child<QLabel>(&window, "clockLabel")->text().isEmpty());
                CHECK(child<QLabel>(&window, "clockLabel")->text().size() == 5);
                CHECK(child<QLabel>(&window, "dateLabel")->text().size() == 10);
                CHECK(child<QLabel>(&window, "clockLabel")->font().pixelSize() >= 30);
            }
            if (size.width() >= 1920) {
                CHECK(child<QFrame>(&window, "contentPanel")->width() >= 675);
                CHECK(child<QLineEdit>(&window, "cardInput")->font().pixelSize() >= 21);
                CHECK(child<QLabel>(&window, "clockLabel")->font().pixelSize() >= 45);
            }
        }
        for (QLabel* label : window.findChildren<QLabel*>()) {
            CHECK(!label->text().contains("YSU"));
            CHECK(!label->text().contains("CAMPUS"));
            CHECK(!label->text().contains("校园一景"));
            CHECK(!label->text().contains("课程设计"));
        }
        window.showMaximized();
        QTest::qWait(220);
        check_layout();
        screenshot(&window, "login-maximized");
        window.showNormal();
        window.resize(1080, 720);
        QTest::qWait(220);
        CHECK(child<QLineEdit>(&window, "passwordInput")->echoMode() == QLineEdit::Password);
        click(&window, "loginButton");
        CHECK(!atm.is_logged_in());
        login("6222020000000001", "000000");
        CHECK(!atm.is_logged_in());
        CHECK(status().contains("错误"));
        CHECK(child<QLineEdit>(&window, "passwordInput")->text().isEmpty());
        enter(&window, "cardInput", "6222020000000001");
        enter(&window, "passwordInput", "123456");
        QTest::keyClick(child<QLineEdit>(&window, "passwordInput"), Qt::Key_Return);
        CHECK(atm.is_logged_in());
        CHECK(child<QStackedWidget>(&window, "pages")->currentIndex() == 1);
        CHECK(child<QLabel>(&window, "balanceLabel")->text().contains("1000.00"));
        click(&window, "balanceButton");
        CHECK(status().contains("1000.00"));
        screenshot(&window, "02-account");
        for (QSize size : {QSize(640, 620), QSize(1080, 720), QSize(1920, 1080), QSize(2560, 1600)}) {
            window.resize(size);
            QTest::qWait(220);
            check_layout();
            QFrame* panel = child<QFrame>(&window, "contentPanel");
            CHECK(qAbs(panel->geometry().center().x() - window.rect().center().x()) <= 1);
            screenshot(&window, QString("account-%1x%2").arg(size.width()).arg(size.height()));
            if (size.width() >= 1920) {
                CHECK(child<QLabel>(&window, "balanceLabel")->font().pixelSize() >= 49);
                CHECK(child<QPushButton>(&window, "depositButton")->height() >= 72);
            }
        }
        window.showMaximized();
        QTest::qWait(220);
        check_layout();
        screenshot(&window, "account-maximized");
        window.showNormal();
        window.resize(1080, 720);
        QTest::qWait(220);
        CHECK(child<QLabel>(&window, "balanceLabel")->font().pixelSize() == 33);
        window.resize(1920, 1080);
        QTest::qWait(220);
        modal_action(window, "depositButton", [&](QDialog* dialog) {
            CHECK(dialog->width() >= 630);
            CHECK(child<QLineEdit>(dialog, "amountInput")->font().pixelSize() >= 21);
            screenshot(dialog, "deposit-large");
            dialog->reject();
        });
        modal_action(window, "passwordButton", [&](QDialog* dialog) {
            CHECK(dialog->width() >= 645);
            CHECK(child<QLineEdit>(dialog, "oldPasswordInput")->font().pixelSize() >= 21);
            screenshot(dialog, "password-large");
            dialog->reject();
        });
        modal_action(window, "historyButton", [&](QDialog* dialog) {
            CHECK(dialog->width() >= 1290);
            CHECK(child<QTableWidget>(dialog, "recordsTable")->verticalHeader()->defaultSectionSize() >= 54);
            screenshot(dialog, "history-large");
            click(dialog, "closeHistoryButton");
        });
        window.resize(1080, 720);
        QTest::qWait(220);
        std::cout << "PASS 各尺寸整体缩放、双行时钟、清理装饰文字、弹窗及曲线同步放大\n";
        std::cout << "PASS 界面登录、错误提示、回车登录、余额查询\n";

        modal_action(window, "historyButton", [&](QDialog* dialog) {
            CHECK(child<QTableWidget>(dialog, "recordsTable")->rowCount() == 0);
            CHECK(atm.balance_history().size() == 1);
            screenshot(dialog, "03-empty-history");
            click(dialog, "closeHistoryButton");
        });
        modal_action(window, "depositButton", [&](QDialog* dialog) { dialog->reject(); });
        CHECK(atm.current_card()->get_balance() == 100000);
        amount(window, "depositButton", "0");
        CHECK(status().contains("有效"));
        amount(window, "depositButton", "abc");
        CHECK(status().contains("有效"));
        amount(window, "depositButton", "1.234");
        CHECK(status().contains("有效"));
        modal_action(window, "depositButton", [&](QDialog* dialog) {
            enter(dialog, "amountInput", "200.50");
            screenshot(dialog, "04-deposit");
            click(dialog, "confirmButton");
        });
        CHECK(atm.current_card()->get_balance() == 120050);
        CHECK(status().contains("200.50 元") && status().contains("1200.50 元"));
        CHECK(child<QLabel>(&window, "balanceLabel")->text().contains("1200.50"));
        amount(window, "withdrawButton", "50");
        CHECK(status().contains("100 元"));
        amount(window, "withdrawButton", "2000");
        CHECK(status().contains("余额不足"));
        amount(window, "withdrawButton", "100");
        CHECK(atm.current_card()->get_balance() == 110050);
        CHECK(status().contains("100.00 元") && status().contains("1100.50 元"));
        check_layout();
        CHECK(atm.current_records().size() == 2);
        screenshot(&window, "05-after-transactions");
        modal_action(window, "historyButton", [&](QDialog* dialog) {
            QTableWidget* table = child<QTableWidget>(dialog, "recordsTable");
            CHECK(table->rowCount() == 2);
            CHECK(table->item(0, 1)->text() == "取款");
            CHECK(table->item(0, 2)->text() == "100.00");
            CHECK(table->item(0, 3)->text() == "1100.50");
            CHECK(table->item(1, 1)->text() == "存款");
            CHECK(atm.balance_history() == std::vector<long long>({100000, 120050, 110050}));
            CHECK(dialog->findChild<QWidget*>("balanceChart") != nullptr);
            screenshot(dialog, "06-history-chart");
            click(dialog, "closeHistoryButton");
        });
        std::cout << "PASS 取消、非法金额、存款、取款、余额不足、交易表和真实曲线\n";

        change_password(window, "123456", "111111", "222222");
        CHECK(status().contains("不一致"));
        change_password(window, "000000", "111111", "111111");
        CHECK(status().contains("原密码错误"));
        change_password(window, "123456", "abc123", "abc123");
        CHECK(status().contains("6 位数字"));
        change_password(window, "123456", "012345", "012345");
        CHECK(status().contains("修改成功"));
        click(&window, "logoutButton");
        CHECK(!atm.is_logged_in());
        CHECK(child<QStackedWidget>(&window, "pages")->currentIndex() == 0);
        CHECK(child<QLineEdit>(&window, "cardInput")->text().isEmpty());
        CHECK(child<QLabel>(&window, "balanceLabel")->text().isEmpty());
        login("6222020000000001", "123456");
        CHECK(!atm.is_logged_in());
        login("6222020000000001", "012345");
        CHECK(atm.is_logged_in());
        click(&window, "logoutButton");
        login("6222020000000002", "234567");
        CHECK(atm.current_card()->get_balance() == 250000);
        CHECK(atm.current_records().empty());
        modal_action(window, "historyButton", [&](QDialog* dialog) {
            CHECK(child<QTableWidget>(dialog, "recordsTable")->rowCount() == 0);
            click(dialog, "closeHistoryButton");
        });
        window.close();
        ATM restored((temp.path() + "/data.txt").toStdString());
        CHECK(restored.load());
        ATMWindow second_window(restored);
        second_window.show();
        enter(&second_window, "cardInput", "6222020000000001");
        enter(&second_window, "passwordInput", "012345");
        click(&second_window, "loginButton");
        CHECK(restored.is_logged_in());
        CHECK(child<QLabel>(&second_window, "balanceLabel")->text().contains("1100.50"));
        second_window.close();
        std::cout << "PASS 改密、退卡、账户切换、重新创建窗口后恢复数据\n";
        std::cout << "全部界面测试通过，平台：" << app.platformName().toStdString() << '\n';
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
