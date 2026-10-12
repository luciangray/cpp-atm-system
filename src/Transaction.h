#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

struct Transaction {
    std::string card_number;
    std::string time;
    std::string type; // deposit 或 withdraw
    long long amount;
    long long balance;
};

#endif
