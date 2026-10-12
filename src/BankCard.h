#ifndef BANK_CARD_H
#define BANK_CARD_H

#include <string>

class BankCard {
public:
    BankCard(const std::string& number, const std::string& password,
             long long balance, long long initial_balance);
    const std::string& get_number() const;
    const std::string& get_password() const;
    long long get_balance() const;
    long long get_initial_balance() const;
    bool check_password(const std::string& input) const;
    bool deposit(long long amount);
    bool withdraw(long long amount);
    bool change_password(const std::string& old_password, const std::string& new_password);

private:
    std::string number_;
    std::string password_;
    long long balance_;
    long long initial_balance_;
};

#endif
