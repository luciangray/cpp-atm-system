#ifndef ATM_H
#define ATM_H

#include "BankCard.h"
#include "Transaction.h"
#include <string>
#include <vector>

class ATM {
public:
    explicit ATM(const std::string& data_file);
    bool load();
    bool login(const std::string& number, const std::string& password);
    void logout();
    bool is_logged_in() const;
    const BankCard* current_card() const;
    bool deposit(long long amount);
    bool withdraw(long long amount);
    bool change_password(const std::string& old_password, const std::string& new_password);
    std::vector<Transaction> current_records() const;
    std::vector<long long> balance_history() const;
    const std::string& message() const;

private:
    std::vector<BankCard> cards_;
    std::vector<Transaction> records_;
    int current_index_ = -1; // -1 表示没有登录
    bool loaded_ = false;
    std::string data_file_;
    std::string message_;
    bool save();
    bool check_login();
    bool transact(long long amount, bool is_deposit);
    void create_demo_accounts();
};

#endif
