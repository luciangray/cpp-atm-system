#ifndef ATM_WINDOW_H
#define ATM_WINDOW_H

#include "ATM.h"
#include <QPixmap>
#include <QWidget>

class QLabel;
class QLineEdit;
class QStackedWidget;
class QFrame;

class ATMWindow : public QWidget {
public:
    explicit ATMWindow(ATM& atm, QWidget* parent = nullptr);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    ATM& atm_;
    QStackedWidget* pages_;
    QLineEdit* card_input_;
    QLineEdit* password_input_;
    QLabel* card_label_;
    QLabel* balance_label_;
    QLabel* status_label_;
    QLabel* subtitle_label_;
    QLabel* clock_label_;
    QLabel* date_label_;
    QFrame* clock_panel_;
    QFrame* panel_ = nullptr;
    QPixmap background_;
    double ui_scale_ = 1.0;
    void position_content();
    void build_login_page();
    void build_account_page();
    void login();
    void refresh_account();
    void show_message(const QString& text, bool success);
    void amount_dialog(bool is_deposit);
    void password_dialog();
    void history_dialog();
    void logout();
};

#endif
