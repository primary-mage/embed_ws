#include "utils.hpp"

#include <dirent.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

namespace basic_cpp {

std::string getKernelVersion() {
    std::ifstream file("/proc/version");
    if (!file) return "无法读取 /proc/version";

    std::string line;
    std::getline(file, line);
    return line;
}

std::string getHostname() {
    char name[256] = {};
    gethostname(name, sizeof(name));
    return std::string(name);
}

std::string getCpuModel() {
    std::ifstream file("/proc/cpuinfo");
    if (!file) return "无法读取 /proc/cpuinfo";

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("model name") != std::string::npos) {
            auto pos = line.find(':');
            if (pos != std::string::npos) {
                return line.substr(pos + 2); // 跳过 ": "
            }
        }
    }
    return "未找到 CPU 型号";
}

std::string getProcessInfo() {
    std::ostringstream oss;
    oss << "PID:  " << getpid()  << '\n'
        << "PPID: " << getppid() << '\n'
        << "UID:  " << getuid();
    return oss.str();
}

std::vector<std::string> listDirectory(const std::string& path) {
    std::vector<std::string> result;
    DIR* dir = opendir(path.c_str());
    if (!dir) return result;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        result.push_back(entry->d_name);
    }
    closedir(dir);
    return result;
}

std::string readFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file) return "无法打开文件: " + filePath;

    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

bool writeFile(const std::string& filePath, const std::string& content) {
    std::ofstream file(filePath);
    if (!file) return false;

    file << content;
    return true;
}

std::vector<std::string> splitString(const std::string& str, char delimiter) {
    std::vector<std::string> result;
    std::istringstream ss(str);
    std::string token;
    while (std::getline(ss, token, delimiter)) {
        result.push_back(token);
    }
    return result;
}

} // namespace basic_cpp
