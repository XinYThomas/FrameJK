//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_TRANSCTX_H
#define FRAME_TRANSCTX_H

#include <unordered_map>

using namespace std;

// HTTP 响应结构体
struct ResponseCtx {
    std::string requestId;                           // 请求唯一标识
    std::string downstream;                          // 下游
    std::chrono::system_clock::time_point startTime; // 请求开始时间
    std::chrono::system_clock::time_point endTime;   // 请求结束时间
    size_t totalBytes;                               // 总字节数
    size_t transferredBytes;                         // 已传输字节数
    int retryCount;                                  // 重试次数
    std::string customData;                          // 自定义数据
    void* userData;                                  // 用户自定义指针数据

    ResponseCtx()
        : totalBytes(0)
        , transferredBytes(0)
        , retryCount(0)
        , userData(nullptr) {
        requestId = generateRequestId();
        startTime = std::chrono::system_clock::now();
    }

    // 计算传输进度百分比
    double getProgress() const {
        if (totalBytes == 0) return 0.0;
        return (transferredBytes * 100.0) / totalBytes;
    }

    // 计算传输耗时（毫秒）
    long long getDuration() const {
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            endTime - startTime);
        return duration.count();
    }

private:
    static std::string generateRequestId() {
        static size_t counter = 0;
        return "REQ_" + std::to_string(++counter) + "_" +
               std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
    }
};

// 任务上下文
struct TaskCtx {
    // Todo 补充任务上下文

};

// 测试专用
struct TestCtx {
    int f1;
    int f2;
    int f3;
};

/*
 * 传递上下文
 */
struct TransCtx {
    unordered_map<string, ResponseCtx> responseCtx_;
    TaskCtx taskCtx_;
    TestCtx testCtx_;
};

#endif //FRAME_TRANSCTX_H