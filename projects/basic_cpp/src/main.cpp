#include "utils.hpp"

#include <iostream>

using namespace basic_cpp;

static void printUsage() {
    std::cout << "用法:\n"
              << "  main proc                  进程信息\n"
              << "  main sys                   内核版本 + 主机名\n"
              << "  main cpu                   CPU 型号\n"
              << "  main list <path>           列出目录\n"
              << "  main read <file>           读取文件\n"
              << "  main write <file> <内容>   写入文件\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        printUsage();
        return 1;
    }

    std::string cmd = argv[1];

    if (cmd == "proc") {
        std::cout << getProcessInfo() << '\n';
    }
    else if (cmd == "sys") {
        std::cout << "内核: " << getKernelVersion() << '\n'
                  << "主机: " << getHostname() << '\n';
    }
    else if (cmd == "cpu") {
        std::cout << "CPU: " << getCpuModel() << '\n';
    }
    else if (cmd == "list") {
        if (argc < 3) {
            std::cerr << "用法: main list <路径>\n";
            return 1;
        }
        auto files = listDirectory(argv[2]);
        for (const auto& f : files) {
            std::cout << f << '\n';
        }
    }
    else if (cmd == "read") {
        if (argc < 3) {
            std::cerr << "用法: main read <文件>\n";
            return 1;
        }
        std::cout << readFile(argv[2]);
    }
    else if (cmd == "write") {
        if (argc < 4) {
            std::cerr << "用法: main write <文件> <内容>\n";
            return 1;
        }
        bool ok = writeFile(argv[2], argv[3]);
        std::cout << (ok ? "写入成功\n" : "写入失败\n");
    }
    else {
        printUsage();
        return 1;
    }

    return 0;
}
