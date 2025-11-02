//
// Created by Yu Xin on 2025/10/19.
//

#ifndef FRAME_TASKQUEUE_H
#define FRAME_TASKQUEUE_H

#include <functional>
#include <atomic>
#include <chrono>
#include <mutex>
#include <utility>
#include <condition_variable>
#include "Logger.h"
#include "TransCtx.h"

using namespace std;

// 任务队列
enum class Priority {
    CRITICAL = 4,
    HIGH = 3,
    NORMAL = 2,
    LOW = 1
};

// 任务状态
enum class TaskState {
    PENDING,
    RUNNING,
    COMPLETED,
    FAILED,
    TIMEOUT
};

class Task {
public:
    using TaskFunc_Ctx = std::function<void(shared_ptr<TransCtx>)>;

    explicit Task(TaskFunc_Ctx func, Priority priority = Priority::HIGH) :
        func_(std::move(func)),
        priority_(priority),
        state_(TaskState::PENDING),
        submit_time_(std::chrono::steady_clock::now()),
        task_id_(next_task_id_++) {}

    ~Task() = default;

    [[nodiscard]] Priority getPriority() const { return priority_; }
    [[nodiscard]] uint64_t getTaskId() const { return task_id_; }
    [[nodiscard]] TaskState getState() const { return state_; }

    void execute(shared_ptr<TransCtx> ctx) {
        state_ = TaskState::RUNNING;
        try {
            func_(std::move(ctx));
            state_ = TaskState::COMPLETED;
        } catch (const std::exception& e) {
            LOG(ERROR) << "Task " << task_id_ << " failed: " << e.what();
            state_ = TaskState::FAILED;
        }
    }

    // 计算带老化的优先级分数
    [[nodiscard]] double getScore() const {
        auto now = std::chrono::steady_clock::now();
        auto wait_time = std::chrono::duration_cast<std::chrono::seconds>(now - submit_time_).count();

        // 基础分数
        double base_score = static_cast<double>(priority_) * 1000000;

        // 老化因子：优先级越低，老化越快
        double aging_factor = 0;
        switch (priority_) {
            case Priority::CRITICAL: aging_factor = 0; break;      // 不老化
            case Priority::HIGH: aging_factor = static_cast<double>(wait_time); break;   // 每秒+1
            case Priority::NORMAL: aging_factor = static_cast<double>(wait_time) * 2; break; // 每秒+2
            case Priority::LOW: aging_factor = static_cast<double>(wait_time) * 5; break;    // 每秒+5
        }

        // 加上微秒时间戳确保FIFO（同优先级）
        auto micros = std::chrono::duration_cast<std::chrono::microseconds>(
            submit_time_.time_since_epoch()).count();

        return base_score + aging_factor - static_cast<double>(micros % 1000000) * 0.000001;
    }

    [[nodiscard]] bool isTimeout(int timeout_ms) const {
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - submit_time_).count();
        return elapsed > timeout_ms;
    }
private:
    TaskFunc_Ctx func_{};
    Priority priority_;
    TaskState state_;
    std::chrono::steady_clock::time_point submit_time_;
    uint64_t task_id_;
    static std::atomic<uint64_t> next_task_id_;
};
std::atomic<uint64_t> Task::next_task_id_{0};

// 优先级队列比较器
struct TaskComparator {
    bool operator()(const std::shared_ptr<Task>& a, const std::shared_ptr<Task>& b) const {
        // 大顶堆
        return a->getScore() < b->getScore();
    }
};

class PriorityTaskQueue {
public:
    explicit PriorityTaskQueue(size_t max_size = 1000)
        : max_size_(max_size)
        , stopped_(false) {
    }

    ~PriorityTaskQueue() {
        stop();
    }

    // 推送任务
    bool push(const std::shared_ptr<Task>& task) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (stopped_) return false;

        // 检查队列是否已满
        if (queue_.size() >= max_size_) {
            // 如果不是高优先级，拒绝
            if (task->getPriority() < Priority::HIGH) {
                return false;
            }
            // 高优先级且队列已满
            if (static_cast<double>(queue_.size()) >=
                static_cast<double>(max_size_) * 1.2) {
                return false;
            }
        }

        queue_.push(task);
        cv_.notify_one();
        return true;
    }

    // 弹出任务（阻塞）
    std::shared_ptr<Task> pop(int timeout_ms = -1) {
        std::unique_lock<std::mutex> lock(mutex_);

        while (queue_.empty() && !stopped_) {
            if (timeout_ms > 0) {
                if (cv_.wait_for(lock, std::chrono::milliseconds(timeout_ms))
                    == std::cv_status::timeout) {
                    return nullptr;
                }
            } else {
                cv_.wait(lock);
            }
        }

        if (stopped_ && queue_.empty()) {
            return nullptr;
        }

        auto task = queue_.top();
        queue_.pop();

        // 检查任务是否超时
        int task_timeout = getTimeoutForPriority(task->getPriority());
        if (task->isTimeout(task_timeout)) {
            // 超时任务直接丢弃，递归获取下一个
            return pop(timeout_ms);
        }

        return task;
    }

    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

    void stop() {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            stopped_ = true;
        }
        cv_.notify_all();
    }

    // 获取各优先级统计信息
    std::unordered_map<Priority, size_t> getStats() const {
        std::lock_guard<std::mutex> lock(mutex_);
        std::unordered_map<Priority, size_t> stats;
        // 维护计数器
        stats[Priority::CRITICAL] = 0;
        stats[Priority::HIGH] = 0;
        stats[Priority::NORMAL] = 0;
        stats[Priority::LOW] = 0;
        return stats;
    }

private:
    static int getTimeoutForPriority(Priority priority) {
        switch (priority) {
            case Priority::CRITICAL: return 2000;  // 2秒
            case Priority::HIGH: return 1000;      // 1秒
            case Priority::NORMAL: return 500;     // 500ms
            case Priority::LOW: return 200;        // 200ms
            default: return 1000;
        }
    }

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::priority_queue<std::shared_ptr<Task>,
                       std::vector<std::shared_ptr<Task>>,
                       TaskComparator> queue_;
    size_t max_size_;
    bool stopped_;
};

#endif //FRAME_TASKQUEUE_H