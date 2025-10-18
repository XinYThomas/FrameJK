//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_THREADPOOL_H
#define FRAME_THREADPOOL_H

#include <functional>
#include <TransCtx.h>
#include <chrono>
#include <atomic>

using namespace std;

// 优先级
enum class Priority {
    CRITICAL,
    HIGH,
    NORMAL,
    LOW
};

// 任务状态
enum class TaskState {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    TIMEOUT
};

// 任务基类
class Task {
public:
    using TaskFunc = std::function<void(TransCtx&)>;

    Task(TaskFunc func, Priority priority)
        : func_(move(func))
        , priority_(priority)
        , submit_time_(chrono::steady_clock::now())
        , state_(TaskState::PENDING)
        , task_id_(next_task_id_++) {}

    void execute(TransCtx& ctx) {
        state_ = TaskState::RUNNING;
        try {
            func_(ctx);
            state_ = TaskState::COMPLETED;
        } catch (std::exception& e) {

        }
    }

private:
    TaskFunc func_;
    Priority priority_;
    TaskState state_;
    chrono::steady_clock::time_point submit_time_;
    uint64_t task_id_;
    static atomic<uint64_t> next_task_id_;
};



#endif //FRAME_THREADPOOL_H