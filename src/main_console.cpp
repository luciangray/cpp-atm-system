#include "ATM.h"
#include "Money.h"
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

namespace {
volatile std::sig_atomic_t alternate_screen_active = 0;

void restore_screen() {
    if (alternate_screen_active) {
        std::cout << "\033[?1049l" << std::flush;
        alternate_screen_active = 0;
    }
}

// Ctrl+C 退出时也要切回原终端；信号处理函数只写固定字符，不使用 cout。
void handle_interrupt(int signal) {
    if (alternate_screen_active) {
        const char sequence[] = "\033[?1049l";
        ssize_t written = write(STDOUT_FILENO, sequence, sizeof(sequence) - 1);
        (void)written;
    }
    std::_Exit(128 + signal);
}

void start_screen(bool interactive) {
    if (interactive) {
        alternate_screen_active = 1;
        std::signal(SIGINT, handle_interrupt);
        std::signal(SIGTERM, handle_interrupt);
        // 备用屏幕独立于命令行历史，退出时恢复启动前的画面。
        std::cout << "\033[?1049h" << std::flush;
    }
}

// VS Code / WSL 终端支持这两个控制序列：清屏、将光标移到左上角。
// 输出重定向到文件或管道时不清屏，方便查看日志和自动测试。
void clear_screen(bool interactive) {
    if (interactive) {
        std::cout << "\033[2J\033[H";
    }
}

void show_header(const ATM& atm, const std::string& page, bool interactive) {
    clear_screen(interactive);
    std::cout << "============== ATM 银行模拟系统 ==============\n"
              << page << "\n\n";
    if (atm.is_logged_in()) {
        std::cout << "当前卡号：" << atm.current_card()->get_number() << "\n\n";
    }
}

void show_menu(const ATM& atm, const std::string& result, bool interactive) {
    show_header(atm, atm.is_logged_in() ? "账户操作" : "欢迎使用，请登录银行卡", interactive);
    if (atm.is_logged_in()) {
        std::cout << "  1. 查询余额          2. 存款\n"
                  << "  3. 取款              4. 修改密码\n"
                  << "  5. 交易记录          6. 退卡\n"
                  << "  0. 退出程序\n";
    } else {
        std::cout << "  1. 登录\n"
                  << "  0. 退出程序\n";
    }
    std::cout << "\n----------------------------------------------\n"
              << "操作结果：" << result << "\n请选择：";
}

bool show_records(const ATM& atm, bool interactive) {
    std::vector<Transaction> records = atm.current_records();
    const std::size_t page_size = 5;
    std::size_t page_count = records.empty() ? 1 : (records.size() + page_size - 1) / page_size;
    std::size_t page = 0;
    std::string notice;
    while (true) {
        show_header(atm, "交易记录（按发生顺序）", interactive);
        std::cout << "第 " << page + 1 << " / " << page_count << " 页，共 "
                  << records.size() << " 笔记录\n\n";
        if (records.empty()) {
            std::cout << "暂无交易记录。\n";
        }
        for (std::size_t i = page * page_size;
             i < records.size() && i < (page + 1) * page_size; ++i) {
            const Transaction& record = records[i];
            std::cout << record.time << "  " << (record.type == "deposit" ? "存款" : "取款")
                      << "  " << format_money(record.amount) << " 元  余额："
                      << format_money(record.balance) << " 元\n";
        }
        std::cout << "\n----------------------------------------------\n";
        if (!notice.empty()) { std::cout << notice << '\n'; }
        std::cout << "1. 上一页  2. 下一页\n翻页选择（回车返回主菜单）：";
        std::string choice;
        if (!std::getline(std::cin, choice)) { return false; }
        if (choice.empty()) { return true; }
        notice.clear();
        if (choice == "1") {
            if (page > 0) { --page; }
            else { notice = "已经是第一页。"; }
        } else if (choice == "2") {
            if (page + 1 < page_count) { ++page; }
            else { notice = "已经是最后一页。"; }
        } else {
            notice = "请输入 1 或 2 翻页，按回车返回主菜单。";
        }
    }
}
}

int main(int argc, char* argv[]) {
    std::string data_file = ATM_DEFAULT_DATA_FILE;
    if (argc == 3 && std::string(argv[1]) == "--data") {
        data_file = argv[2];
    } else if (argc != 1) {
        std::cerr << "用法：atm_console [--data 数据文件路径]\n";
        return 1;
    }
    ATM atm(data_file);
    if (!atm.load()) {
        std::cerr << atm.message() << '\n';
        return 1;
    }
    const char* term = std::getenv("TERM");
    bool interactive = isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)
                       && (!term || std::string(term) != "dumb");
    start_screen(interactive);
    std::string result = "请输入菜单编号。";
    std::string choice;
    while (true) {
        show_menu(atm, result, interactive);
        if (!std::getline(std::cin, choice) || choice == "0") {
            break;
        }
        if (!atm.is_logged_in()) {
            if (choice != "1") {
                result = "请选择有效菜单项。";
                continue;
            }
            show_header(atm, "银行卡登录", interactive);
            std::string number, password;
            std::cout << "卡号：";
            if (!std::getline(std::cin, number)) { break; }
            std::cout << "密码：";
            if (!std::getline(std::cin, password)) { break; }
            atm.login(number, password);
            result = atm.message();
            continue;
        }
        if (choice == "1") {
            result = "当前余额：" + format_money(atm.current_card()->get_balance()) + " 元";
        } else if (choice == "2" || choice == "3") {
            show_header(atm, choice == "2" ? "存款" : "取款", interactive);
            std::string input;
            std::cout << (choice == "2" ? "存款金额（元，最多两位小数）：" : "取款金额（100 元整数倍）：");
            if (!std::getline(std::cin, input)) { break; }
            long long amount = 0;
            if (!parse_amount(input, amount)) {
                result = "请输入有效的正金额，最多两位小数。";
                continue;
            }
            bool ok = choice == "2" ? atm.deposit(amount) : atm.withdraw(amount);
            result = atm.message();
            result += std::string("\n") + (ok ? "本次" : "申请")
                      + (choice == "2" ? "存款：" : "取款：") + format_money(amount) + " 元";
            result += "\n当前余额：" + format_money(atm.current_card()->get_balance()) + " 元";
        } else if (choice == "4") {
            show_header(atm, "修改密码", interactive);
            std::string old_password, new_password, confirm;
            std::cout << "原密码：";
            if (!std::getline(std::cin, old_password)) { break; }
            std::cout << "新密码（6 位数字）：";
            if (!std::getline(std::cin, new_password)) { break; }
            std::cout << "再次输入新密码：";
            if (!std::getline(std::cin, confirm)) { break; }
            if (new_password != confirm) {
                result = "两次新密码不一致。";
            } else {
                atm.change_password(old_password, new_password);
                result = atm.message();
            }
        } else if (choice == "5") {
            if (!show_records(atm, interactive)) { break; }
            result = "已返回主菜单。";
        } else if (choice == "6") {
            atm.logout();
            result = atm.message();
        } else {
            result = "请选择有效菜单项。";
        }
    }
    atm.logout();
    restore_screen();
    std::cout << "程序已退出。\n";
    return 0;
}
