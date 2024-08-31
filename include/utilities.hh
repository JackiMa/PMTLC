// utilities.hh
#ifndef UTILITIES_H
#define UTILITIES_H
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

// 辅助函数，用于将任意类型转换为字符串
template<typename T>
std::string to_string(const T& value) {
    std::ostringstream oss;
    oss << value;
    return oss.str();
}

// 格式化函数实现
template<typename... Args>
std::string f(const std::string& format, Args... args) {
    std::vector<std::string> arguments{to_string(args)...};
    std::string result;
    std::size_t lastPos = 0;
    std::size_t pos = 0;

    std::vector<std::string>::size_type argIndex = 0; // 修改类型
    while ((pos = format.find("{}", lastPos)) != std::string::npos) {
        result.append(format, lastPos, pos - lastPos); // 添加前一个{}之前的文本
        if (argIndex < arguments.size()) {
            result += arguments[argIndex++]; // 添加参数
        }
        lastPos = pos + 2; // 更新位置，跳过{}
    }
    result += format.substr(lastPos); // 添加剩余文本
    return result;
}

enum LogLevel {
    DEBUG,
    INFO,
    ERROR
};


void myPrint(LogLevel level, const std::string& message);

extern std::string My_msg;
extern LogLevel lv;

// int main() {
//     int age = 30;
//     double height = 5.9;
//     std::string name = "Alice";

//     std::string message = f("Name: ++{}, Age: {}, Height: {}", name, age, height);
//     myPrint(DEBUG, mg);

//     return 0;
// }


#endif // UTILITIES_H