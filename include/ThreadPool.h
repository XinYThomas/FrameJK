//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_THREADPOOL_H
#define FRAME_THREADPOOL_H

#include <memory>
#include <utility>
#include <TaskQueue.h>
#include "Logger.h"


// 线程池
class ThreadPool {
public:
    ThreadPool(size_t num_threads,
        std::shared_ptr<PriorityTaskQueue> queue,
        shared_ptr<TransCtx> ctx)
        : queue_(std::move(queue))
        , stopped_(false)
        , ctx_(std::move(ctx)){
        for (size_t i = 0; i < num_threads; ++i) {
            workers_.emplace_back([this, i]() {
                this->workerThread(i);
            });
        }
    }

    ~ThreadPool() {
        stop();
    }

    void stop() {
        stopped_ = true;
        queue_->stop();

        for (auto& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    size_t getActiveThreads() const {
        return active_threads_.load();
    }

    uint64_t getCompletedTasks() const {
        return completed_tasks_.load();
    }

private:
    void workerThread(size_t thread_id) {
        LOG(DEBUG) << "Worker thread " << thread_id << " started (PID: "
                  << getpid() << ")";

        while (!stopped_) {
            auto task = queue_->pop(1000); // 1秒超时

            if (!task) continue;

            active_threads_++;

            auto start = std::chrono::steady_clock::now();
            task->execute(ctx_);
            auto end = std::chrono::steady_clock::now();

            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
                end - start).count();

            LOG(DEBUG) << "Thread " << thread_id << " completed task "
                      << task->getTaskId() << " (priority: "
                      << static_cast<int>(task->getPriority())
                      << ", duration: " << duration << "ms)" ;

            active_threads_--;
            completed_tasks_++;
        }

        LOG(DEBUG) << "Worker thread " << thread_id << " stopped";
    }

    std::shared_ptr<TransCtx> ctx_;
    std::vector<std::thread> workers_;
    std::shared_ptr<PriorityTaskQueue> queue_;
    std::atomic<bool> stopped_;
    std::atomic<size_t> active_threads_{0};
    std::atomic<uint64_t> completed_tasks_{0};
};

#endif //FRAME_THREADPOOL_H