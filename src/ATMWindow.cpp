#include "ATMWindow.h"
#include "BalanceChart.h"
#include "Money.h"
#include <QDateTime>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QFrame>
#include <QGraphicsDropShadowEffect>
#include <QGridLayout>
#include <QHeaderView>
#include <QIcon>
#include <QLabel>
#include <QLineEdit>
#include <QPainter>
#include <QRegularExpression>
#include <QPushButton>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTimer>
#include <QVariant>
#include <QVBoxLayout>

namespace {
// 把样式中的像素尺寸乘以统一比例；颜色和文字内容保持不变。
QString scaled_style(const QString& style, double scale) {
    QRegularExpression pixels("([0-9]+)px");
    auto matches = pixels.globalMatch(style);
    QString result;
    int previous = 0;
    while (matches.hasNext()) {
        auto match = matches.next();
        result += style.mid(previous, match.capturedStart() - previous);
        result += QString::number(qRound(match.captured(1).toInt() * scale)) + "px";
        previous = match.capturedEnd();
    }
    return result + style.mid(previous);
}

void scale_widgets(QWidget* root, double scale, bool scale_root_size = true) {
    QList<QWidget*> widgets = root->findChildren<QWidget*>();
    widgets.prepend(root);
    for (QWidget* widget : widgets) {
        // 保存原始尺寸，每次都从原值计算，避免反复缩放产生累积误差。
        if (!widget->property("baseStyle").isValid()) {
            widget->setProperty("baseStyle", widget->styleSheet());
        }
        if (!widget->property("baseMinimum").isValid()) {
            widget->setProperty("baseMinimum", widget->minimumSize());
            if (QPushButton* button = qobject_cast<QPushButton*>(widget)) {
                button->setProperty("baseIconSize", button->iconSize());
            }
        }
        QString style = widget->property("baseStyle").toString();
        if (!style.isEmpty()) { widget->setStyleSheet(scaled_style(style, scale)); }
        if (widget != root || scale_root_size) {
            QSize minimum = widget->property("baseMinimum").toSize();
            widget->setMinimumSize(qRound(minimum.width() * scale), qRound(minimum.height() * scale));
        }
        if (QPushButton* button = qobject_cast<QPushButton*>(widget)) {
            QSize icon = button->property("baseIconSize").toSize();
            button->setIconSize(QSize(qRound(icon.width() * scale), qRound(icon.height() * scale)));
        }
    }
    for (QLayout* layout : root->findChildren<QLayout*>()) {
        if (!layout->property("baseMargins").isValid()) {
            QMargins margins = layout->contentsMargins();
            layout->setProperty("baseMargins", QVariantList{margins.left(), margins.top(), margins.right(), margins.bottom()});
            layout->setProperty("baseSpacing", layout->spacing());
        }
        QVariantList margins = layout->property("baseMargins").toList();
        layout->setContentsMargins(qRound(margins[0].toInt() * scale), qRound(margins[1].toInt() * scale),
                                   qRound(margins[2].toInt() * scale), qRound(margins[3].toInt() * scale));
        int spacing = layout->property("baseSpacing").toInt();
        if (spacing >= 0) { layout->setSpacing(qRound(spacing * scale)); }
        if (QFormLayout* form = qobject_cast<QFormLayout*>(layout)) {
            if (!form->property("baseVerticalSpacing").isValid()) {
                form->setProperty("baseVerticalSpacing", form->verticalSpacing());
            }
            int vertical = form->property("baseVerticalSpacing").toInt();
            if (vertical >= 0) { form->setVerticalSpacing(qRound(vertical * scale)); }
        }
    }
}

QLineEdit* password_field(const QString& name, QWidget* parent) {
    QLineEdit* field = new QLineEdit(parent);
    field->setObjectName(name);
    field->setEchoMode(QLineEdit::Password);
    field->setMaxLength(6);
    field->setPlaceholderText("请输入 6 位数字密码");
    return field;
}

QDialogButtonBox* dialog_buttons(QDialog& dialog, QVBoxLayout* layout) {
    QDialogButtonBox* buttons = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    buttons->button(QDialogButtonBox::Ok)->setText("确认");
    buttons->button(QDialogButtonBox::Ok)->setObjectName("confirmButton");
    buttons->button(QDialogButtonBox::Ok)->setIcon(QIcon());
    buttons->button(QDialogButtonBox::Cancel)->setText("取消");
    buttons->button(QDialogButtonBox::Cancel)->setObjectName("cancelButton");
    buttons->button(QDialogButtonBox::Cancel)->setIcon(QIcon());
    QObject::connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    QObject::connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    return buttons;
}

// 小图标直接用 QPainter 绘制，避免增加图标库。
QIcon action_icon(int action) {
    QPixmap pixmap(84, 84);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.scale(3, 3);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(QPen(QColor("#286c8b"), 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    if (action == 0) {
        painter.drawRoundedRect(QRectF(3, 6, 22, 16), 3, 3);
        painter.drawLine(4, 11, 24, 11);
        painter.drawLine(7, 17, 12, 17);
    } else if (action == 1 || action == 2) {
        painter.drawEllipse(QRectF(4, 4, 20, 20));
        painter.drawLine(9, 14, 19, 14);
        if (action == 1) { painter.drawLine(14, 9, 14, 19); }
    } else if (action == 3) {
        painter.drawRoundedRect(QRectF(5, 12, 18, 12), 3, 3);
        painter.drawArc(QRectF(9, 3, 10, 16), 0, 180 * 16);
        painter.drawLine(14, 17, 14, 20);
    } else if (action == 4) {
        painter.drawLine(5, 4, 5, 23);
        painter.drawLine(5, 23, 24, 23);
        painter.drawPolyline(QPolygonF({QPointF(8, 18), QPointF(13, 12), QPointF(18, 15), QPointF(24, 6)}));
    } else {
        painter.drawLine(13, 4, 5, 4);
        painter.drawLine(5, 4, 5, 24);
        painter.drawLine(5, 24, 13, 24);
        painter.drawLine(11, 14, 24, 14);
        painter.drawLine(19, 9, 24, 14);
        painter.drawLine(19, 19, 24, 14);
    }
    return QIcon(pixmap);
}
}

ATMWindow::ATMWindow(ATM& atm, QWidget* parent) : QWidget(parent), atm_(atm) {
    Q_INIT_RESOURCE(resources);
    background_.load(":/images/campus.jpg");
    setObjectName("atmWindow");
    setWindowTitle("ATM 银行模拟系统");
    setMinimumSize(640, 620);
    resize(1080, 720);
    setStyleSheet(
        "QWidget { font-family: 'Noto Sans CJK SC'; font-size: 14px; color: #213e52; }"
        "QFrame#contentPanel { background: rgba(250,253,255,247); border: 1px solid rgba(255,255,255,220); border-radius: 22px; }"
        "QLabel { background: transparent; border: none; }"
        "QWidget#loginPage, QWidget#accountPage, QStackedWidget { background: transparent; border: none; }"
        "QDialog { background: #f4f8fb; }"
        "QFrame#clockPanel { background: rgba(12,32,48,110); border: 1px solid rgba(255,255,255,70); border-radius: 12px; }"
        "QLineEdit { background: #ffffff; border: 1px solid #d3e0e9; border-radius: 8px; padding: 10px 12px; selection-background-color: #2b738f; }"
        "QLineEdit:focus { border: 1px solid #318cac; background: #fbfeff; }"
        "QPushButton { background: #226b89; color: white; border: 1px solid #226b89; border-radius: 9px; padding: 9px 14px; font-weight: bold; }"
        "QPushButton:hover { background: #2c809e; border-color: #2c809e; }"
        "QPushButton:pressed { background: #194f69; border-color: #194f69; }"
        "QPushButton:focus { border: 1px solid #60adca; }"
        "QPushButton#cancelButton { background: #e8eff4; color: #4c6577; border-color: #e8eff4; }"
        "QPushButton#cancelButton:hover { background: #dbe7ef; }"
        "QPushButton[actionButton='true'] { background: #edf4f8; color: #285d77; border-color: #dce8f0; font-weight: normal; }"
        "QPushButton[actionButton='true']:hover { background: #dcecf4; border-color: #85b5c8; }"
        "QPushButton[actionButton='true']:pressed { background: #c9e2ef; }"
        "QPushButton#logoutButton { background: #f0f2f5; color: #657486; border-color: #dfe5eb; }"
        "QFrame#bankCard { background: qlineargradient(x1:0,y1:0,x2:1,y2:1,stop:0 #173e65,stop:1 #277f8f); border-radius: 14px; }"
        "QLabel#statusLabel { border-radius: 9px; padding: 9px 12px; }"
        "QTableWidget { background: white; alternate-background-color: #f3f8fb; gridline-color: #e3edf3; border: 1px solid #dae7ef; border-radius: 7px; selection-background-color: #d8eaf4; selection-color: #214b64; }"
        "QHeaderView::section { background: #e8f1f6; color: #34586e; border: none; padding: 9px; font-weight: bold; }");

    clock_panel_ = new QFrame(this);
    clock_panel_->setObjectName("clockPanel");
    QVBoxLayout* clock_layout = new QVBoxLayout(clock_panel_);
    clock_layout->setContentsMargins(18, 10, 18, 12);
    clock_layout->setSpacing(2);
    clock_label_ = new QLabel(clock_panel_);
    clock_label_->setObjectName("clockLabel");
    clock_label_->setAlignment(Qt::AlignCenter);
    clock_label_->setStyleSheet("color: white; font-size: 30px; font-weight: bold;");
    clock_layout->addWidget(clock_label_);
    date_label_ = new QLabel(clock_panel_);
    date_label_->setObjectName("dateLabel");
    date_label_->setAlignment(Qt::AlignCenter);
    date_label_->setStyleSheet("color: #dbe8f0; font-size: 13px;");
    clock_layout->addWidget(date_label_);
    QTimer* clock = new QTimer(this);
    auto update_clock = [this]() {
        QDateTime now = QDateTime::currentDateTime();
        clock_label_->setText(now.toString("HH:mm"));
        date_label_->setText(now.toString("yyyy.MM.dd"));
    };
    connect(clock, &QTimer::timeout, this, update_clock);
    clock->start(1000);
    update_clock();

    panel_ = new QFrame(this);
    panel_->setObjectName("contentPanel");
    QGraphicsDropShadowEffect* shadow = new QGraphicsDropShadowEffect(panel_);
    shadow->setBlurRadius(32);
    shadow->setOffset(0, 10);
    shadow->setColor(QColor(10, 30, 50, 65));
    panel_->setGraphicsEffect(shadow);
    QVBoxLayout* layout = new QVBoxLayout(panel_);
    layout->setContentsMargins(26, 24, 26, 22);
    layout->setSpacing(12);
    QVBoxLayout* heading = new QVBoxLayout;
    heading->setSpacing(4);
    QLabel* title = new QLabel("ATM 银行模拟系统", panel_);
    title->setStyleSheet("font-size: 25px; font-weight: bold; color: #193f5d;");
    heading->addWidget(title);
    subtitle_label_ = new QLabel(panel_);
    subtitle_label_->hide();
    subtitle_label_->setStyleSheet("color: #6b8293; font-size: 13px;");
    heading->addWidget(subtitle_label_);
    layout->addLayout(heading);

    pages_ = new QStackedWidget(panel_);
    pages_->setObjectName("pages");
    pages_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Ignored);
    build_login_page();
    build_account_page();
    layout->addWidget(pages_, 1);

    status_label_ = new QLabel(panel_);
    status_label_->setObjectName("statusLabel");
    status_label_->setWordWrap(true);
    status_label_->setMinimumHeight(56);
    layout->addWidget(status_label_);
    show_message("请输入卡号和密码。", true);
    position_content();
}

void ATMWindow::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    if (!background_.isNull()) {
        // 保持照片比例，在绘制时裁切，原图片文件不变。
        double scale = qMax(double(width()) / background_.width(), double(height()) / background_.height());
        double source_width = width() / scale;
        double source_height = height() / scale;
        QRectF source((background_.width() - source_width) * 0.6,
                      (background_.height() - source_height) * 0.3, source_width, source_height);
        painter.drawPixmap(QRectF(rect()), background_, source);
    } else {
        painter.fillRect(rect(), QColor("#386b88"));
    }
    QLinearGradient shade(0, 0, 0, height());
    shade.setColorAt(0, QColor(13, 32, 50, 30));
    shade.setColorAt(0.5, QColor(13, 32, 50, 8));
    shade.setColorAt(1, QColor(13, 32, 50, 80));
    painter.fillRect(rect(), shade);
}

void ATMWindow::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    // 构造期间 resize() 可能触发事件，子控件建立后才安排它们的位置。
    if (panel_) { position_content(); }
}

void ATMWindow::position_content() {
    ui_scale_ = qBound(1.0, qMin(width() / 1080.0, height() / 720.0), 2.5);
    scale_widgets(this, ui_scale_, false);
    QGraphicsDropShadowEffect* shadow = static_cast<QGraphicsDropShadowEffect*>(panel_->graphicsEffect());
    shadow->setBlurRadius(32 * ui_scale_);
    shadow->setOffset(0, 10 * ui_scale_);
    bool logged_in = atm_.is_logged_in();
    int panel_width = qMin(qRound((logged_in ? 660 : 450) * ui_scale_), width() - qRound(48 * ui_scale_));
    int panel_height = qRound((logged_in ? 540 : 480) * ui_scale_);
    int x = (width() - panel_width) / 2;
    if (!logged_in && width() >= 1000) { x = qRound(72 * ui_scale_); }
    panel_->setGeometry(x, (height() - panel_height) / 2, panel_width, panel_height);
    clock_panel_->setVisible(width() / ui_scale_ >= 1040);
    int clock_width = qRound(180 * ui_scale_);
    int clock_height = qRound(92 * ui_scale_);
    int margin = qRound(24 * ui_scale_);
    clock_panel_->setGeometry(width() - clock_width - margin, margin, clock_width, clock_height);
    clock_panel_->raise();
    update();
}

void ATMWindow::build_login_page() {
    QWidget* page = new QWidget;
    page->setObjectName("loginPage");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    layout->addStretch();
    layout->addWidget(new QLabel("银行卡号", page));
    card_input_ = new QLineEdit(page);
    card_input_->setObjectName("cardInput");
    card_input_->setMaxLength(16);
    card_input_->setPlaceholderText("请输入 16 位银行卡号");
    layout->addWidget(card_input_);
    layout->addWidget(new QLabel("登录密码", page));
    password_input_ = password_field("passwordInput", page);
    layout->addWidget(password_input_);
    layout->addSpacing(6);
    QPushButton* button = new QPushButton("登录账户", page);
    button->setObjectName("loginButton");
    button->setMinimumHeight(46);
    button->setCursor(Qt::PointingHandCursor);
    layout->addWidget(button);
    layout->addStretch();
    connect(button, &QPushButton::clicked, this, &ATMWindow::login);
    connect(password_input_, &QLineEdit::returnPressed, this, &ATMWindow::login);
    connect(card_input_, &QLineEdit::returnPressed, password_input_, [this]() { password_input_->setFocus(); });
    pages_->addWidget(page);
}

void ATMWindow::build_account_page() {
    QWidget* page = new QWidget;
    page->setObjectName("accountPage");
    QVBoxLayout* layout = new QVBoxLayout(page);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    QFrame* bank_card = new QFrame(page);
    bank_card->setObjectName("bankCard");
    bank_card->setMinimumHeight(130);
    QVBoxLayout* card_layout = new QVBoxLayout(bank_card);
    card_layout->setContentsMargins(20, 14, 20, 14);
    card_layout->setSpacing(5);
    QLabel* caption = new QLabel("模拟银行卡  /  当前余额", bank_card);
    caption->setStyleSheet("font-size: 12px; color: #c6e3ec;");
    card_layout->addWidget(caption);
    balance_label_ = new QLabel(bank_card);
    balance_label_->setObjectName("balanceLabel");
    balance_label_->setStyleSheet("font-size: 33px; font-weight: bold; color: white;");
    card_layout->addWidget(balance_label_);
    card_label_ = new QLabel(bank_card);
    card_label_->setObjectName("cardLabel");
    card_label_->setStyleSheet("font-size: 13px; color: #d8edf2;");
    card_layout->addWidget(card_label_);
    layout->addWidget(bank_card);
    QLabel* operations = new QLabel("快捷操作", page);
    operations->setStyleSheet("font-size: 13px; color: #698294;");
    layout->addWidget(operations);
    QGridLayout* grid = new QGridLayout;
    grid->setSpacing(10);
    const QString names[] = {"查询余额", "存款", "取款", "修改密码", "交易记录与曲线", "退卡"};
    const QString ids[] = {"balanceButton", "depositButton", "withdrawButton", "passwordButton", "historyButton", "logoutButton"};
    for (int i = 0; i < 6; ++i) {
        QPushButton* button = new QPushButton(names[i], page);
        button->setObjectName(ids[i]);
        button->setProperty("actionButton", true);
        button->setMinimumHeight(48);
        button->setCursor(Qt::PointingHandCursor);
        button->setIcon(action_icon(i));
        button->setIconSize(QSize(22, 22));
        grid->addWidget(button, i / 2, i % 2);
        if (i == 0) {
            connect(button, &QPushButton::clicked, this, [this]() {
                refresh_account();
                show_message("当前余额：" + QString::fromStdString(format_money(atm_.current_card()->get_balance())) + " 元", true);
            });
        } else if (i == 1 || i == 2) {
            connect(button, &QPushButton::clicked, this, [this, i]() { amount_dialog(i == 1); });
        } else if (i == 3) {
            connect(button, &QPushButton::clicked, this, &ATMWindow::password_dialog);
        } else if (i == 4) {
            connect(button, &QPushButton::clicked, this, &ATMWindow::history_dialog);
        } else {
            connect(button, &QPushButton::clicked, this, &ATMWindow::logout);
        }
    }
    layout->addLayout(grid);
    pages_->addWidget(page);
}

void ATMWindow::show_message(const QString& text, bool success) {
    status_label_->setText((success ? "✓  " : "!  ") + text);
    QString style = success ? "color: #2d6b65; background: #eaf4f1;" : "color: #a24d43; background: #f9eeeb;";
    status_label_->setProperty("baseStyle", style);
    status_label_->setStyleSheet(style);
}

void ATMWindow::login() {
    bool ok = atm_.login(card_input_->text().trimmed().toStdString(), password_input_->text().toStdString());
    password_input_->clear();
    show_message(QString::fromStdString(atm_.message()), ok);
    if (ok) {
        refresh_account();
        pages_->setCurrentIndex(1);
        subtitle_label_->setText("账户操作 · 请选择需要办理的业务。");
        subtitle_label_->show();
        position_content();
    } else {
        password_input_->setFocus();
    }
}

void ATMWindow::refresh_account() {
    const BankCard* card = atm_.current_card();
    if (card) {
        card_label_->setText("银行卡号：" + QString::fromStdString(card->get_number()));
        balance_label_->setText("￥ " + QString::fromStdString(format_money(card->get_balance())));
    }
}

void ATMWindow::amount_dialog(bool is_deposit) {
    QDialog dialog(this);
    dialog.setObjectName("amountDialog");
    dialog.setWindowTitle(is_deposit ? "存款" : "取款");
    dialog.setMinimumWidth(420);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 24);
    layout->setSpacing(16);
    QLabel* title = new QLabel(is_deposit ? "办理存款" : "办理取款", &dialog);
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #204963;");
    layout->addWidget(title);
    layout->addWidget(new QLabel("当前余额：" + QString::fromStdString(format_money(atm_.current_card()->get_balance())) + " 元", &dialog));
    layout->addWidget(new QLabel(is_deposit ? "存款金额（元，最多两位小数）" : "取款金额（元，100 元的整数倍）", &dialog));
    QLineEdit* amount = new QLineEdit(&dialog);
    amount->setObjectName("amountInput");
    amount->setPlaceholderText(is_deposit ? "例如：200.50" : "例如：100");
    layout->addWidget(amount);
    dialog_buttons(dialog, layout);
    scale_widgets(&dialog, ui_scale_);
    if (dialog.exec() != QDialog::Accepted) { return; }
    long long cents = 0;
    if (!parse_amount(amount->text().trimmed().toStdString(), cents)) {
        show_message("请输入有效的正金额，最多两位小数。", false);
        return;
    }
    bool ok = is_deposit ? atm_.deposit(cents) : atm_.withdraw(cents);
    refresh_account();
    QString feedback = QString::fromStdString(atm_.message()) + "  "
        + (ok ? "本次" : "申请") + (is_deposit ? "存款 " : "取款 ")
        + QString::fromStdString(format_money(cents)) + " 元\n当前余额："
        + QString::fromStdString(format_money(atm_.current_card()->get_balance())) + " 元";
    show_message(feedback, ok);
}

void ATMWindow::password_dialog() {
    QDialog dialog(this);
    dialog.setObjectName("passwordDialog");
    dialog.setWindowTitle("修改密码");
    dialog.setMinimumWidth(430);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(26, 24, 26, 24);
    layout->setSpacing(16);
    QLabel* title = new QLabel("修改登录密码", &dialog);
    title->setStyleSheet("font-size: 22px; font-weight: bold; color: #204963;");
    layout->addWidget(title);
    QFormLayout* form = new QFormLayout;
    form->setVerticalSpacing(12);
    QLineEdit* old_password = password_field("oldPasswordInput", &dialog);
    QLineEdit* new_password = password_field("newPasswordInput", &dialog);
    QLineEdit* confirm = password_field("confirmPasswordInput", &dialog);
    form->addRow("原密码", old_password);
    form->addRow("新密码", new_password);
    form->addRow("再次输入", confirm);
    layout->addLayout(form);
    dialog_buttons(dialog, layout);
    scale_widgets(&dialog, ui_scale_);
    if (dialog.exec() != QDialog::Accepted) { return; }
    if (new_password->text() != confirm->text()) {
        show_message("两次新密码不一致。", false);
        return;
    }
    bool ok = atm_.change_password(old_password->text().toStdString(), new_password->text().toStdString());
    show_message(QString::fromStdString(atm_.message()), ok);
}

void ATMWindow::history_dialog() {
    QDialog dialog(this);
    dialog.setObjectName("historyDialog");
    dialog.setWindowTitle("交易记录与余额曲线");
    dialog.resize(860, 650);
    QVBoxLayout* layout = new QVBoxLayout(&dialog);
    layout->setContentsMargins(24, 20, 24, 20);
    layout->setSpacing(12);
    QLabel* title = new QLabel("交易记录与余额曲线", &dialog);
    title->setStyleSheet("font-size: 23px; font-weight: bold; color: #204963;");
    layout->addWidget(title);
    QLabel* heading = new QLabel("银行卡号：" + QString::fromStdString(atm_.current_card()->get_number()), &dialog);
    heading->setStyleSheet("color: #6b8293;");
    layout->addWidget(heading);
    BalanceChart* chart = new BalanceChart(atm_.balance_history(), &dialog);
    layout->addWidget(chart, 1);
    std::vector<Transaction> records = atm_.current_records();
    QLabel* summary = new QLabel(records.empty() ? "暂无交易记录，可先进行一笔存款或取款。"
                                : QString("共 %1 笔交易，最近的交易显示在最上方。").arg(records.size()), &dialog);
    summary->setStyleSheet("color: #6b8293; font-size: 13px;");
    layout->addWidget(summary);
    QTableWidget* table = new QTableWidget(static_cast<int>(records.size()), 4, &dialog);
    table->setObjectName("recordsTable");
    table->setHorizontalHeaderLabels({"时间", "类型", "金额（元）", "余额（元）"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->verticalHeader()->hide();
    table->verticalHeader()->setDefaultSectionSize(36);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setAlternatingRowColors(true);
    for (std::size_t i = 0; i < records.size(); ++i) {
        const Transaction& record = records[records.size() - 1 - i];
        QString values[] = {QString::fromStdString(record.time), record.type == "deposit" ? "存款" : "取款",
                            QString::fromStdString(format_money(record.amount)), QString::fromStdString(format_money(record.balance))};
        for (int column = 0; column < 4; ++column) {
            QTableWidgetItem* item = new QTableWidgetItem(values[column]);
            item->setTextAlignment(Qt::AlignCenter);
            table->setItem(static_cast<int>(i), column, item);
        }
    }
    layout->addWidget(table, 1);
    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    buttons->button(QDialogButtonBox::Close)->setText("关闭");
    buttons->button(QDialogButtonBox::Close)->setObjectName("closeHistoryButton");
    buttons->button(QDialogButtonBox::Close)->setIcon(QIcon());
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttons);
    scale_widgets(&dialog, ui_scale_);
    chart->set_ui_scale(ui_scale_);
    table->verticalHeader()->setDefaultSectionSize(qRound(36 * ui_scale_));
    dialog.resize(qRound(860 * ui_scale_), qRound(650 * ui_scale_));
    dialog.exec();
}

void ATMWindow::logout() {
    atm_.logout();
    password_input_->clear();
    card_input_->clear();
    card_label_->clear();
    balance_label_->clear();
    pages_->setCurrentIndex(0);
    subtitle_label_->clear();
    subtitle_label_->hide();
    position_content();
    card_input_->setFocus();
    show_message(QString::fromStdString(atm_.message()), true);
}
