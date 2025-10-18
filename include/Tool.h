//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_TOOL_H
#define FRAME_TOOL_H

#include <string>
#include <vector>

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

#endif //FRAME_TOOL_H