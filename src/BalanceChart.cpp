#include "BalanceChart.h"
#include "Money.h"
#include <QPainter>
#include <QPainterPath>
#include <algorithm>

BalanceChart::BalanceChart(const std::vector<long long>& balances, QWidget* parent)
    : QWidget(parent), balances_(balances) {
    setObjectName("balanceChart");
    setMinimumSize(560, 230);
}

void BalanceChart::set_ui_scale(double scale) {
    ui_scale_ = scale;
    setMinimumSize(qRound(560 * scale), qRound(230 * scale));
    update();
}

void BalanceChart::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.scale(ui_scale_, ui_scale_);
    QFont text_font("Noto Sans CJK SC");
    text_font.setPixelSize(14);
    painter.setFont(text_font);
    double chart_width = width() / ui_scale_;
    double chart_height = height() / ui_scale_;
    QRectF chart_rect(0, 0, chart_width, chart_height);
    painter.setPen(QPen(QColor("#d9e7ef"), 1));
    painter.setBrush(Qt::white);
    painter.drawRoundedRect(chart_rect.adjusted(1, 1, -1, -1), 10, 10);
    if (balances_.empty()) {
        painter.drawText(chart_rect, Qt::AlignCenter, "暂无余额数据");
        return;
    }
    QRectF plot(100, 25, chart_width - 125, chart_height - 80);
    long long minimum = *std::min_element(balances_.begin(), balances_.end());
    long long maximum = *std::max_element(balances_.begin(), balances_.end());
    // 水平曲线或仅有初始余额时，也保留可见的纵轴范围。
    double padding = std::max(10000.0, (maximum - minimum) * 0.15);
    double low = std::max(0.0, minimum - padding);
    double high = maximum + padding;
    painter.setPen(QColor("#cdd8e4"));
    for (int i = 0; i <= 4; ++i) {
        double y = plot.bottom() - plot.height() * i / 4;
        painter.drawLine(QPointF(plot.left(), y), QPointF(plot.right(), y));
        long long value = static_cast<long long>(low + (high - low) * i / 4);
        painter.setPen(QColor("#526174"));
        painter.drawText(QRectF(0, y - 12, 90, 24), Qt::AlignRight | Qt::AlignVCenter,
                         QString::fromStdString(format_money(value)));
        painter.setPen(QColor("#cdd8e4"));
    }
    painter.setPen(QColor("#526174"));
    painter.drawText(QRectF(5, 0, 90, 22), Qt::AlignRight, "余额（元）");
    painter.drawText(QRectF(plot.left(), plot.bottom() + 7, 100, 22), Qt::AlignLeft, "初始余额");
    if (balances_.size() > 1) {
        painter.drawText(QRectF(plot.right() - 160, plot.bottom() + 7, 160, 22), Qt::AlignRight,
                         QString("第 %1 笔交易").arg(balances_.size() - 1));
    }
    painter.drawText(QRectF(plot.left(), chart_height - 26, plot.width(), 22), Qt::AlignCenter,
                     "按交易顺序显示余额变化");
    QPainterPath line;
    std::vector<QPointF> points;
    for (std::size_t i = 0; i < balances_.size(); ++i) {
        double x = balances_.size() == 1 ? plot.center().x()
            : plot.left() + plot.width() * i / (balances_.size() - 1);
        double y = plot.bottom() - plot.height() * (balances_[i] - low) / (high - low);
        points.emplace_back(x, y);
        if (i == 0) { line.moveTo(x, y); }
        else { line.lineTo(x, y); }
    }
    if (points.size() > 1) {
        QPainterPath area = line;
        area.lineTo(points.back().x(), plot.bottom());
        area.lineTo(points.front().x(), plot.bottom());
        area.closeSubpath();
        QLinearGradient fill(plot.topLeft(), plot.bottomLeft());
        fill.setColorAt(0, QColor(39, 125, 149, 45));
        fill.setColorAt(1, QColor(39, 125, 149, 0));
        painter.fillPath(area, fill);
    }
    painter.setPen(QPen(QColor("#277d95"), 2.5));
    painter.drawPath(line);
    painter.setBrush(QColor("#277d95"));
    for (const QPointF& point : points) {
        painter.drawEllipse(point, 3.5, 3.5);
    }
}
