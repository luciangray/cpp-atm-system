#include "BankCard.h"
#include "Money.h"

BankCard::BankCard(const std::string& number, const std::string& password,
                   long long balance, long long initial_balance)
    : number_(number), password_(password), balance_(balance), initial_balance_(initial_balance) {}

const std::string& BankCard::get_number() const { return number_; }
const std::string& BankCard::get_password() const { return password_; }
long long BankCard::get_balance() const { return balance_; }
long long BankCard::get_initial_balance() const { return initial_balance_; }
bool BankCard::check_password(const std::string& input) const { return password_ == input; }

bool BankCard::deposit(long long amount) {
    if (amount <= 0 || amount > MAX_BALANCE - balance_) {
        return false;
    }
    balance_ += amount;
    return true;
}

bool BankCard::withdraw(long long amount) {
    if (amount <= 0 || amount % 10000 != 0 || amount > balance_) {
        return false;
    }
    balance_ -= amount;
    return true;
}

bool BankCard::change_password(const std::string& old_password, const std::string& new_password) {
    if (!check_password(old_password) || !valid_password(new_password)) {
        return false;
    }
    password_ = new_password;
    return true;
}
