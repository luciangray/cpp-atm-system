#include "ATM.h"
#include "Money.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

#define CHECK(condition) do { if (!(condition)) { \
    throw std::runtime_error(std::string("检查失败：") + #condition + "，行 " + std::to_string(__LINE__)); \
} } while (false)

int main() {
    auto unique = std::chrono::steady_clock::now().time_since_epoch().count();
    std::filesystem::path directory = std::filesystem::temp_directory_path() /
        ("atm-core-test-" + std::to_string(unique));
    std::filesystem::create_directories(directory);
    try {
        long long cents = 0;
        CHECK(parse_amount("1", cents) && cents == 100);
        CHECK(parse_amount("200.50", cents) && cents == 20050);
        CHECK(parse_amount("0.01", cents) && cents == 1);
        CHECK(parse_amount("5.1", cents) && cents == 510);
        CHECK(parse_amount("0005.10", cents) && cents == 510);
        CHECK(parse_amount("1000000000", cents) && cents == MAX_BALANCE);
        for (const std::string text : {"", "0", "0.00", "-1", "+1", "abc", "1e3", "1.001",
                                       ".50", "1.", "1.2.3", "1 0", "1000000000.01", "999999999999999999999999"}) {
            CHECK(!parse_amount(text, cents));
        }
        CHECK(format_money(1) == "0.01");
        CHECK(format_money(20050) == "200.50");
        CHECK(valid_password("012345"));
        CHECK(!valid_password("12345"));
        CHECK(!valid_password("1234567"));
        CHECK(!valid_password("12a456"));
        std::cout << "PASS 金额解析、显示、密码格式\n";

        BankCard card("6222020000000001", "123456", 100000, 100000);
        CHECK(!card.deposit(0));
        CHECK(!card.deposit(-100));
        CHECK(card.deposit(1));
        CHECK(card.get_balance() == 100001);
        CHECK(!card.withdraw(5000));
        CHECK(!card.withdraw(200000));
        CHECK(card.withdraw(100000));
        CHECK(card.get_balance() == 1);
        CHECK(!card.change_password("000000", "111111"));
        CHECK(card.change_password("123456", "012345"));
        CHECK(card.check_password("012345"));
        std::cout << "PASS 银行卡基本行为\n";

        std::string path = (directory / "accounts.txt").string();
        ATM atm(path);
        CHECK(!atm.login("6222020000000001", "123456"));
        CHECK(atm.load());
        CHECK(!atm.deposit(100));
        CHECK(!atm.withdraw(10000));
        CHECK(!atm.change_password("123456", "111111"));
        CHECK(atm.current_card() == nullptr);
        CHECK(atm.current_records().empty());
        CHECK(atm.balance_history().empty());
        CHECK(!atm.login("6222020000000999", "123456"));
        CHECK(!atm.login("6222020000000001", "000000"));
        CHECK(atm.login("6222020000000001", "123456"));
        CHECK(!atm.login("6222020000000002", "234567"));
        CHECK(atm.current_card()->get_balance() == 100000);
        CHECK(atm.balance_history() == std::vector<long long>({100000}));
        CHECK(atm.deposit(20050));
        CHECK(atm.withdraw(10000));
        CHECK(atm.current_card()->get_balance() == 110050);
        CHECK(atm.current_records().size() == 2);
        CHECK(atm.balance_history() == std::vector<long long>({100000, 120050, 110050}));
        CHECK(!atm.deposit(-1));
        CHECK(!atm.deposit(MAX_BALANCE));
        CHECK(!atm.withdraw(5000));
        CHECK(!atm.withdraw(200000));
        CHECK(atm.current_records().size() == 2);
        CHECK(atm.current_card()->get_balance() == 110050);
        std::cout << "PASS 登录、存取款、失败操作不改变余额和记录\n";

        CHECK(!atm.change_password("000000", "111111"));
        CHECK(!atm.change_password("123456", "abc123"));
        CHECK(atm.change_password("123456", "012345"));
        CHECK(atm.current_records().size() == 2);
        atm.logout();
        CHECK(!atm.is_logged_in());
        CHECK(!atm.login("6222020000000001", "123456"));
        CHECK(atm.login("6222020000000001", "012345"));
        atm.logout();
        CHECK(atm.login("6222020000000002", "234567"));
        CHECK(atm.current_card()->get_balance() == 250000);
        CHECK(atm.current_records().empty());
        CHECK(atm.deposit(10000));
        atm.logout();
        CHECK(atm.login("6222020000000003", "345678"));
        CHECK(atm.withdraw(50000));
        CHECK(atm.current_card()->get_balance() == 0);
        CHECK(!atm.withdraw(10000));
        CHECK(atm.deposit(1));
        std::cout << "PASS 改密、退卡、多账户隔离、余额为零\n";

        ATM restarted(path);
        CHECK(restarted.load());
        CHECK(!restarted.login("6222020000000001", "123456"));
        CHECK(restarted.login("6222020000000001", "012345"));
        CHECK(restarted.current_card()->get_balance() == 110050);
        CHECK(restarted.balance_history() == std::vector<long long>({100000, 120050, 110050}));
        CHECK(!restarted.current_records()[0].time.empty());
        restarted.logout();
        CHECK(restarted.login("6222020000000002", "234567"));
        CHECK(restarted.current_card()->get_balance() == 260000);
        CHECK(restarted.current_records().size() == 1);
        restarted.logout();
        CHECK(restarted.login("6222020000000003", "345678"));
        CHECK(restarted.current_card()->get_balance() == 1);
        std::cout << "PASS 程序重启后恢复余额、密码和交易历史\n";

        // 用同名目录模拟无法创建临时文件，检查失败后内存与原文件保持一致。
        CHECK(atm.load());
        CHECK(atm.login("6222020000000001", "012345"));
        std::filesystem::create_directory(path + ".tmp");
        CHECK(!atm.deposit(10000));
        CHECK(!atm.withdraw(10000));
        CHECK(!atm.change_password("012345", "111111"));
        CHECK(atm.current_card()->get_balance() == 110050);
        CHECK(atm.current_records().size() == 2);
        CHECK(atm.current_card()->check_password("012345"));
        ATM unchanged(path);
        CHECK(unchanged.load());
        CHECK(unchanged.login("6222020000000001", "012345"));
        CHECK(unchanged.current_card()->get_balance() == 110050);
        std::cout << "PASS 文件保存失败时明确失败并恢复本次操作\n";

        for (const std::string contents : {
                "", "wrong header", "ATM_DATA 1\n",
                "ATM_DATA 1\nCARD 6222020000000001 123456 -1 100000\n",
                "ATM_DATA 1\nCARD 6222020000000001 123456 100000 100000\nCARD 6222020000000001 123456 100000 100000\n",
                "ATM_DATA 1\nCARD 6222020000000001 123456 100000 100000\nRECORD 6222020000000002 \"2026-10-12 12:00:00\" deposit 10000 110000\n"}) {
            std::string bad_path = (directory / "bad.txt").string();
            std::ofstream out(bad_path);
            out << contents;
            out.close();
            ATM bad(bad_path);
            CHECK(!bad.load());
            CHECK(!bad.login("6222020000000001", "123456"));
        }
        std::cout << "PASS 数据文件错误时停止加载，不覆盖原文件\n";
        std::filesystem::remove_all(directory);
        std::cout << "全部核心测试通过。\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << "\n测试数据：" << directory << '\n';
        return 1;
    }
}
