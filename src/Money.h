#ifndef MONEY_H
#define MONEY_H

#include <string>

// 金额统一以分为单位，显示和输入时才转换成元。
const long long MAX_BALANCE = 100000000000LL;
bool parse_amount(const std::string& text, long long& cents);
std::string format_money(long long cents);
bool valid_password(const std::string& password);

#endif
