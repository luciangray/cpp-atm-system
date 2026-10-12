"""通过真实控制台进程验证菜单、输入错误和重启恢复，不使用演示账户文件。"""
import pathlib
import os
import pty
import select
import signal
import subprocess
import sys
import tempfile
import time


def run(binary, data_file, inputs):
    result = subprocess.run(
        [binary, "--data", str(data_file)], input=inputs,
        text=True, encoding="utf-8", capture_output=True, timeout=10,
        cwd=data_file.parent,
    )
    assert result.returncode == 0, result.stderr
    assert "\x1b" not in result.stdout, "重定向输出不应包含清屏控制序列"
    return result.stdout


def test_terminal(binary, data_file):
    """使用伪终端检查刷新后的当前页面，而不是只检查累积输出。"""
    master, slave = pty.openpty()
    process = subprocess.Popen(
        [binary, "--data", str(data_file)], stdin=slave, stdout=slave, stderr=slave,
        env=dict(os.environ, TERM="xterm-256color"),
    )
    os.close(slave)
    buffer = b""

    def expect(text):
        nonlocal buffer
        marker = text.encode("utf-8")
        deadline = time.monotonic() + 5
        while marker not in buffer:
            remaining = deadline - time.monotonic()
            assert remaining > 0, (text, buffer.decode("utf-8", errors="replace"))
            ready, _, _ = select.select([master], [], [], remaining)
            assert ready, text
            buffer += os.read(master, 4096)
        end = buffer.index(marker) + len(marker)
        output, buffer = buffer[:end], buffer[end:]
        return output.decode("utf-8")

    def send(text):
        os.write(master, text.encode("utf-8"))

    clear = "\x1b[2J\x1b[H"
    try:
        initial = expect("请选择：")
        assert "\x1b[?1049h" in initial and clear in initial
        send("1\n6222020000000001\n123456\n")
        screen = expect("请选择：").rsplit(clear, 1)[-1]
        assert "当前卡号：6222020000000001" in screen
        assert "操作结果：登录成功。" in screen
        assert "密码：" not in screen

        send("1\n")
        output = expect("请选择：")
        assert output.count(clear) == 1
        assert "操作结果：当前余额：1000.00 元" in output

        send("2\n200.50\n")
        output = expect("请选择：")
        assert output.count(clear) == 2
        screen = output.rsplit(clear, 1)[-1]
        assert "操作结果：存款成功。" in screen
        assert "本次存款：200.50 元" in screen and "当前余额：1200.50 元" in screen

        send("3\n2000\n")
        screen = expect("请选择：").rsplit(clear, 1)[-1]
        assert "余额不足" in screen
        assert "申请取款：2000.00 元" in screen and "当前余额：1200.50 元" in screen
        send("3\n100\n")
        screen = expect("请选择：").rsplit(clear, 1)[-1]
        assert "本次取款：100.00 元" in screen and "当前余额：1100.50 元" in screen

        send("5\n")
        records = expect("翻页选择（回车返回主菜单）：")
        assert clear in records and "200.50 元" in records and "1200.50 元" in records
        assert "1. 查询余额" not in records
        assert process.poll() is None
        send("\n")
        assert "操作结果：已返回主菜单。" in expect("请选择：")

        for amount in ["1.01", "2.02", "3.03", "4.04", "5.05", "6.06"]:
            send("2\n" + amount + "\n")
            assert "本次存款：" + amount + " 元" in expect("请选择：")
        send("5\n")
        first = expect("翻页选择（回车返回主菜单）：").rsplit(clear, 1)[-1]
        assert "第 1 / 2 页，共 8 笔记录" in first
        assert "3.03 元" in first and "4.04 元" not in first
        assert first.count("  余额：") == 5
        send("1\n")
        assert "已经是第一页" in expect("翻页选择（回车返回主菜单）：")
        send("2\n")
        second = expect("翻页选择（回车返回主菜单）：").rsplit(clear, 1)[-1]
        assert "第 2 / 2 页" in second
        assert "4.04 元" in second and "6.06 元" in second
        assert "  存款  200.50 元" not in second and second.count("  余额：") == 3
        send("2\n")
        assert "已经是最后一页" in expect("翻页选择（回车返回主菜单）：")
        send("bad\n")
        assert "请输入 1 或 2" in expect("翻页选择（回车返回主菜单）：")
        send("1\n")
        assert "第 1 / 2 页" in expect("翻页选择（回车返回主菜单）：")
        send("\n")
        assert "已返回主菜单" in expect("请选择：")

        send("6\n")
        screen = expect("请选择：").rsplit(clear, 1)[-1]
        assert "已退卡" in screen and "当前卡号" not in screen
        send("0\n")
        assert "\x1b[?1049l" in expect("程序已退出。")
        assert process.wait(timeout=5) == 0
        print("PASS 备用屏幕进入与正常退出、具体交易结果、每页五笔记录、翻页边界与返回")
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=5)
        os.close(master)


def test_terminal_exit(binary, data_file):
    for termination in ["eof", signal.SIGINT, signal.SIGTERM]:
        master, slave = pty.openpty()
        process = subprocess.Popen(
            [binary, "--data", str(data_file)], stdin=slave, stdout=slave, stderr=slave,
            env=dict(os.environ, TERM="xterm-256color"),
        )
        os.close(slave)
        output = b""
        try:
            deadline = time.monotonic() + 5
            while "请选择：".encode() not in output:
                remaining = deadline - time.monotonic()
                assert remaining > 0
                ready, _, _ = select.select([master], [], [], remaining)
                assert ready
                output += os.read(master, 4096)
            if termination == "eof":
                os.write(master, b"\x04")
            else:
                process.send_signal(termination)
            code = process.wait(timeout=5)
            while select.select([master], [], [], 0)[0]:
                try:
                    block = os.read(master, 4096)
                except OSError:
                    break
                if not block:
                    break
                output += block
            assert b"\x1b[?1049h" in output and b"\x1b[?1049l" in output
            assert code == (0 if termination == "eof" else 128 + termination)
        finally:
            if process.poll() is None:
                process.terminate()
                process.wait(timeout=5)
            os.close(master)
    print("PASS Ctrl+D、SIGINT（Ctrl+C）、SIGTERM 退出时恢复主屏幕")


def main():
    binary = str(pathlib.Path(sys.argv[1]).resolve())
    with tempfile.TemporaryDirectory(prefix="atm-console-test-") as directory:
        data_file = pathlib.Path(directory) / "accounts.txt"
        inputs = "\n".join([
            "bad", "1", "6222020000000001", "000000",
            "1", "6222020000000001", "123456", "1",
            "2", "abc", "2", "200.50", "3", "50", "3", "2000",
            "3", "100", "5", "", "4", "123456", "111111", "222222",
            "4", "000000", "012345", "012345",
            "4", "123456", "012345", "012345", "6", "0", "",
        ])
        output = run(binary, data_file, inputs)
        for message in ["有效菜单", "卡号或密码错误", "1000.00 元", "有效的正金额",
                        "存款成功", "100 元的整数倍", "余额不足", "取款成功",
                        "1100.50 元", "两次新密码不一致", "原密码错误",
                        "密码修改成功", "已退卡", "程序已退出"]:
            assert message in output, (message, output)
        print("PASS 控制台菜单和所有基础操作、错误输入")

        output = run(binary, data_file, "1\n6222020000000001\n123456\n1\n6222020000000001\n012345\n1\n5\n\n6\n1\n6222020000000002\n234567\n1\n5\n\n0\n")
        for message in ["卡号或密码错误", "1100.50 元", "200.50 元", "2500.00 元", "暂无交易记录"]:
            assert message in output, (message, output)
        print("PASS 新进程恢复密码、余额和交易记录，其他账户数据独立")

        for inputs in ["", "1\n", "1\n6222020000000001\n",
                       "1\n6222020000000001\n012345\n2\n",
                       "1\n6222020000000001\n012345\n5\n"]:
            assert "程序已退出" in run(binary, data_file, inputs)
        invalid = subprocess.run([binary, "--wrong"], capture_output=True, text=True)
        assert invalid.returncode == 1
        print("PASS 输入结束时正常退出、无效启动参数")
        test_terminal(binary, pathlib.Path(directory) / "terminal-accounts.txt")
        test_terminal_exit(binary, pathlib.Path(directory) / "exit-accounts.txt")


if __name__ == "__main__":
    main()
