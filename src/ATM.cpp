#include "ATM.h"
#include "Money.h"
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>

namespace {
std::string current_time() {
    std::time_t now = std::time(nullptr);
    std::ostringstream out;
    out << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    return out.str();
}
}

ATM::ATM(const std::string& data_file) : data_file_(data_file) {}

void ATM::create_demo_accounts() {
    cards_.emplace_back("6222020000000001", "123456", 100000, 100000);
    cards_.emplace_back("6222020000000002", "234567", 250000, 250000);
    cards_.emplace_back("6222020000000003", "345678", 50000, 50000);
}

bool ATM::load() {
    logout();
    loaded_ = false;
    cards_.clear();
    records_.clear();
    std::error_code error;
    bool exists = std::filesystem::exists(data_file_, error);
    if (error) {
        message_ = "无法访问数据文件，请检查路径。";
        return false;
    }
    if (!exists) {
        create_demo_accounts();
        loaded_ = save();
        return loaded_;
    }
    std::ifstream in(data_file_);
    std::string header;
    int version = 0;
    if (!(in >> header >> version) || header != "ATM_DATA" || version != 1) {
        message_ = "数据文件格式错误，请检查文件或恢复演示数据。";
        return false;
    }
    std::string line;
    std::getline(in, line);
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }
        std::istringstream row(line);
        std::string kind;
        row >> kind;
        if (kind == "CARD") {
            std::string number, password;
            long long balance = 0, initial = 0;
            if (!(row >> number >> password >> balance >> initial) ||
                number.size() != 16 || number.find_first_not_of("0123456789") != std::string::npos ||
                !valid_password(password) || balance < 0 || balance > MAX_BALANCE ||
                initial < 0 || initial > MAX_BALANCE) {
                message_ = "银行卡数据格式错误。";
                return false;
            }
            for (const BankCard& card : cards_) {
                if (card.get_number() == number) {
                    message_ = "数据文件包含重复卡号。";
                    return false;
                }
            }
            cards_.emplace_back(number, password, balance, initial);
        } else if (kind == "RECORD") {
            Transaction record;
            if (!(row >> record.card_number >> std::quoted(record.time) >> record.type >>
                  record.amount >> record.balance) ||
                (record.type != "deposit" && record.type != "withdraw") ||
                record.amount <= 0 || record.amount > MAX_BALANCE ||
                record.balance < 0 || record.balance > MAX_BALANCE) {
                message_ = "交易记录格式错误。";
                return false;
            }
            records_.push_back(record);
        } else {
            message_ = "数据文件包含未知内容。";
            return false;
        }
        std::string extra;
        if (row >> extra) {
            message_ = "数据文件包含多余字段。";
            return false;
        }
    }
    if (in.bad() || cards_.empty()) {
        message_ = "读取数据失败或文件没有账户。";
        return false;
    }
    // 交易记录必须属于已有账户。
    for (const Transaction& record : records_) {
        bool found = false;
        for (const BankCard& card : cards_) {
            if (card.get_number() == record.card_number) {
                found = true;
                break;
            }
        }
        if (!found) {
            message_ = "交易记录对应的账户不存在。";
            return false;
        }
    }
    loaded_ = true;
    message_ = "数据读取成功。";
    return true;
}

bool ATM::save() {
    std::filesystem::path path(data_file_);
    std::error_code error;
    if (!path.parent_path().empty()) {
        std::filesystem::create_directories(path.parent_path(), error);
        if (error) {
            message_ = "无法创建数据目录，本次操作未保存。";
            return false;
        }
    }
    // 先写临时文件，再替换原文件，避免普通写入失败破坏已有数据。
    std::string temp_file = data_file_ + ".tmp";
    std::ofstream out(temp_file);
    out << "ATM_DATA 1\n";
    for (const BankCard& card : cards_) {
        out << "CARD " << card.get_number() << ' ' << card.get_password() << ' '
            << card.get_balance() << ' ' << card.get_initial_balance() << '\n';
    }
    for (const Transaction& record : records_) {
        out << "RECORD " << record.card_number << ' ' << std::quoted(record.time) << ' '
            << record.type << ' ' << record.amount << ' ' << record.balance << '\n';
    }
    out.close();
    if (!out) {
        message_ = "写入数据失败，本次操作未保存。";
        return false;
    }
    std::filesystem::rename(temp_file, path, error);
    if (error) {
        message_ = "保存数据失败，本次操作未生效。";
        return false;
    }
    return true;
}

bool ATM::login(const std::string& number, const std::string& password) {
    if (!loaded_) {
        message_ = "数据未成功加载，无法登录。";
        return false;
    }
    if (is_logged_in()) {
        message_ = "请先退卡，再登录其他账户。";
        return false;
    }
    for (std::size_t i = 0; i < cards_.size(); ++i) {
        if (cards_[i].get_number() == number && cards_[i].check_password(password)) {
            current_index_ = static_cast<int>(i);
            message_ = "登录成功。";
            return true;
        }
    }
    message_ = "卡号或密码错误。";
    return false;
}

void ATM::logout() {
    current_index_ = -1;
    message_ = "已退卡，请取走银行卡。";
}

bool ATM::is_logged_in() const { return current_index_ >= 0; }
const BankCard* ATM::current_card() const {
    return is_logged_in() ? &cards_[current_index_] : nullptr;
}
const std::string& ATM::message() const { return message_; }

bool ATM::check_login() {
    if (!is_logged_in()) {
        message_ = "请先登录。";
        return false;
    }
    return true;
}

bool ATM::transact(long long amount, bool is_deposit) {
    if (!check_login()) {
        return false;
    }
    if (amount <= 0) {
        message_ = "金额必须大于零。";
        return false;
    }
    if (!is_deposit && amount % 10000 != 0) {
        message_ = "取款金额必须是 100 元的整数倍。";
        return false;
    }
    BankCard& card = cards_[current_index_];
    BankCard previous = card;
    bool ok = is_deposit ? card.deposit(amount) : card.withdraw(amount);
    if (!ok) {
        message_ = is_deposit ? "存款后余额超过模拟系统上限。" : "余额不足。";
        return false;
    }
    records_.push_back({card.get_number(), current_time(),
                        is_deposit ? "deposit" : "withdraw", amount, card.get_balance()});
    if (!save()) {
        card = previous;
        records_.pop_back();
        return false;
    }
    message_ = is_deposit ? "存款成功。" : "取款成功。";
    return true;
}

bool ATM::deposit(long long amount) { return transact(amount, true); }
bool ATM::withdraw(long long amount) { return transact(amount, false); }

bool ATM::change_password(const std::string& old_password, const std::string& new_password) {
    if (!check_login()) {
        return false;
    }
    BankCard& card = cards_[current_index_];
    if (!card.check_password(old_password)) {
        message_ = "原密码错误。";
        return false;
    }
    if (!valid_password(new_password)) {
        message_ = "新密码必须为 6 位数字。";
        return false;
    }
    BankCard previous = card;
    card.change_password(old_password, new_password);
    if (!save()) {
        card = previous;
        return false;
    }
    message_ = "密码修改成功。";
    return true;
}

std::vector<Transaction> ATM::current_records() const {
    std::vector<Transaction> result;
    if (is_logged_in()) {
        for (const Transaction& record : records_) {
            if (record.card_number == cards_[current_index_].get_number()) {
                result.push_back(record);
            }
        }
    }
    return result;
}

std::vector<long long> ATM::balance_history() const {
    std::vector<long long> result;
    if (is_logged_in()) {
        result.push_back(cards_[current_index_].get_initial_balance());
        for (const Transaction& record : current_records()) {
            result.push_back(record.balance);
        }
    }
    return result;
}
