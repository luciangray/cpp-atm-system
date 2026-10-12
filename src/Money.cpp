#include "Money.h"
#include <iomanip>
#include <sstream>

bool parse_amount(const std::string& text, long long& cents) {
    if (text.empty()) {
        return false;
    }
    long long yuan = 0;
    int fraction = 0;
    int decimal_digits = 0;
    bool decimal = false;
    bool has_integer_digit = false;
    for (char ch : text) {
        if (ch == '.' && !decimal && has_integer_digit) {
            decimal = true;
        } else if (ch >= '0' && ch <= '9') {
            if (!decimal) {
                yuan = yuan * 10 + ch - '0';
                if (yuan > MAX_BALANCE / 100) {
                    return false;
                }
                has_integer_digit = true;
            } else {
                if (++decimal_digits > 2) {
                    return false;
                }
                fraction = fraction * 10 + ch - '0';
            }
        } else {
            return false;
        }
    }
    if (!has_integer_digit || (decimal && decimal_digits == 0)) {
        return false;
    }
    if (decimal_digits == 1) {
        fraction *= 10;
    }
    long long result = yuan * 100 + fraction;
    if (result <= 0 || result > MAX_BALANCE) {
        return false;
    }
    cents = result;
    return true;
}

std::string format_money(long long cents) {
    std::ostringstream out;
    out << cents / 100 << '.' << std::setw(2) << std::setfill('0') << cents % 100;
    return out.str();
}

bool valid_password(const std::string& password) {
    if (password.size() != 6) {
        return false;
    }
    for (char ch : password) {
        if (ch < '0' || ch > '9') {
            return false;
        }
    }
    return true;
}
