//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_TOOL_H
#define FRAME_TOOL_H

#include <string>
#include <vector>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

inline void spilt_string(string &input, const string &split_symbol,vector<string> &output) {
    size_t pos = 0;
    size_t start = 0;
    while ((pos = input.find(split_symbol, start)) != string::npos) {
        output.push_back(input.substr(start, pos - start));
        start = pos + split_symbol.size();
    }
    output.push_back(input.substr(start));
}

// 检查并创建文件
inline bool ensureFileExists(string filepath) {
    // 先检查文件是否存在
    if (access(filepath.c_str(), F_OK) != -1) {
        return true; // 文件已存在
    }
    // 文件不存在，创建文件
    int fd = open(filepath.c_str(), O_CREAT | O_WRONLY, 0666);
    if (fd == -1) {
        return false;
    }
    close(fd);
    return true;
}


#endif //FRAME_TOOL_H