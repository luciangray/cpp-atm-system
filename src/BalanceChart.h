#ifndef BALANCE_CHART_H
#define BALANCE_CHART_H

#include <QWidget>
#include <vector>

class BalanceChart : public QWidget {
public:
    explicit BalanceChart(const std::vector<long long>& balances, QWidget* parent = nullptr);
    void set_ui_scale(double scale);

protected:
    void paintEvent(QPaintEvent* event) override;

private:
    std::vector<long long> balances_;
    double ui_scale_ = 1.0;
};

#endif
