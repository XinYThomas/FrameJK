//
// Created by Yu Xin on 2025/11/2.
//

#ifndef FRAME_COMMENT_CTRL_H
#define FRAME_COMMENT_CTRL_H

#include <string>
#include <filesystem>
#include <fstream>
#include <utility>
#include <Logger.h>

using namespace std;

class CommentCtrl{
public:
    CommentCtrl(CommentCtrl &other) = delete;
    void operator=(const CommentCtrl &) = delete;
    static CommentCtrl *GetInstance(string pid_path) {
        return nullptr;
    }
protected:
    explicit CommentCtrl(string pid_path) : pid_path_(std::move(pid_path)){

    }
    ~CommentCtrl();
private:
    [[nodiscard]] bool fileExists() const {
        return std::filesystem::exists(pid_path_);
    }

    void savePid() const {
        if(!fileExists()) {
            LOG(DEBUG) << "pid path is no exist";
            throw runtime_error("pid path is no exist");
        }
        std::ofstream pidFile(pid_path_);
        pidFile << getpid();
        pidFile.close();
    }

    [[nodiscard]] pid_t loadPid() const {
        if(!fileExists()) {
            LOG(DEBUG) << "pid path is no exist";
            throw runtime_error("pid path is no exist");
        }
        std::ifstream pidFile(pid_path_);
        pid_t pid;
        pidFile >> pid;
        pidFile.close();
        return pid;
    }
    // 程序控制
    // 启动
    bool start() {
        // 检查 pid 文件是否存在
        if (fileExists()) {
            LOG(INFO) << "Process is already running";
            return false;
        }
        return true;
    }

    // 停止
    void stop() {

    }
    // 帮助
    void help() {

    }

    string pid_path_;
    static CommentCtrl* comment_ctrl_;
};

#endif //FRAME_COMMENT_CTRL_H