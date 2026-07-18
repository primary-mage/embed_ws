#pragma once

#include <string>
#include <vector>

namespace basic_cpp {

/** 获取内核版本字符串 */
std::string getKernelVersion();

/** 获取主机名 */
std::string getHostname();

/** 获取 CPU 型号 */
std::string getCpuModel();

/** 获取当前进程 PID、PPID、UID（返回格式化字符串） */
std::string getProcessInfo();

/** 列出目录下所有文件名 */
std::vector<std::string> listDirectory(const std::string& path);

/** 读取文件全部内容 */
std::string readFile(const std::string& filePath);

/** 写字符串到文件，返回是否成功 */
bool writeFile(const std::string& filePath, const std::string& content);

/** 按分隔符切分字符串 */
std::vector<std::string> splitString(const std::string& str, char delimiter);

} // namespace basic_cpp
