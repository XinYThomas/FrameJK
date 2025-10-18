//
// Created by Yu Xin on 2025/10/18.
//
#include <HttpRequest.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>

using namespace std;

// PIMPL实现类
class HttpRequest::Impl {
public:
    unordered_map<string, string> globalHeaders;
    int globalTimeout = 30;
    mutex mutex_;
    unordered_map<string, bool> cancelFlags; // 取消标志
    
    void addCancelFlag(const string& requestId) {
        lock_guard<mutex> lock(mutex_);
        cancelFlags[requestId] = false;
    }
    
    bool isCancelled(const string& requestId) {
        lock_guard<mutex> lock(mutex_);
        auto it = cancelFlags.find(requestId);
        return it != cancelFlags.end() && it->second;
    }
    
    void removeCancelFlag(const string& requestId) {
        lock_guard<mutex> lock(mutex_);
        cancelFlags.erase(requestId);
    }
};

HttpRequest::HttpRequest() : pImpl(std::make_unique<Impl>()) {}

HttpRequest::~HttpRequest() {
    cancelAll();
}

void HttpRequest::sendAsync(const HttpRequestConfig& config,
                            HttpCallback callback) {
    sendAsync(config, callback, nullptr);
}

void HttpRequest::sendAsync(const HttpRequestConfig& config,
                            HttpCallback completionCallback,
                            ProgressCallback progressCallback) {
    // 创建新的线程执行请求
    std::thread([this, config, completionCallback, progressCallback]() {
        TransCtx ctx;
        HttpResponse response;

        pImpl->addCancelFlag(ctx.responseCtx);

        try {
            // 执行请求
            executeRequest(ctx, config, response);

            // 检查是否被取消
            if (pImpl->isCancelled(ctx.requestId)) {
                response.status = HttpStatus::CANCELLED;
                response.errorMessage = "Request was cancelled";
            }

            ctx.endTime = std::chrono::system_clock::now();

            // 调用完成回调
            if (completionCallback) {
                completionCallback(ctx, response, true);
            }
        } catch (const std::exception& e) {
            response.status = HttpStatus::FAILED;
            response.errorMessage = e.what();
            ctx.endTime = std::chrono::system_clock::now();

            if (completionCallback) {
                completionCallback(ctx, response, true);
            }
        }

        pImpl->removeCancelFlag(ctx.requestId);
    }).detach();
}

HttpResponse HttpRequest::sendSync(const HttpRequestConfig& config) {
    TransCtx ctx;
    HttpResponse response;

    pImpl->addCancelFlag(ctx.requestId);

    try {
        executeRequest(ctx, config, response);

        if (pImpl->isCancelled(ctx.requestId)) {
            response.status = HttpStatus::CANCELLED;
            response.errorMessage = "Request was cancelled";
        }
    } catch (const std::exception& e) {
        response.status = HttpStatus::FAILED;
        response.errorMessage = e.what();
    }

    ctx.endTime = std::chrono::system_clock::now();
    pImpl->removeCancelFlag(ctx.requestId);

    return response;
}

void HttpRequest::executeRequest(TransCtx& ctx,
                                 const HttpRequestConfig& config,
                                 HttpResponse& response) {
    // 这里是实际的HTTP请求实现
    // 在实际项目中，你可以使用libcurl、Boost.Beast或其他HTTP库

    // 模拟请求处理
    int timeout = config.timeout > 0 ? config.timeout : pImpl->globalTimeout;
    ctx.totalBytes = 1024; // 模拟数据大小

    for (int retry = 0; retry <= config.maxRetries; ++retry) {
        ctx.retryCount = retry;

        // 检查取消标志
        if (pImpl->isCancelled(ctx.requestId)) {
            break;
        }

        // 模拟网络请求
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 模拟成功响应
        response.statusCode = 200;
        response.statusMessage = "OK";
        response.status = HttpStatus::SUCCESS;
        response.body = "{\"message\": \"Request successful\"}";
        response.headers["Content-Type"] = "application/json";

        ctx.transferredBytes = ctx.totalBytes;

        if (!shouldRetry(ctx, config, response)) {
            break;
        }
    }
}

bool HttpRequest::shouldRetry(const TransCtx& ctx,
                              const HttpRequestConfig& config,
                              const HttpResponse& response) {
    // 如果请求成功，不需要重试
    if (response.isSuccess()) {
        return false;
    }

    // 如果已达到最大重试次数，不再重试
    if (ctx.retryCount >= config.maxRetries) {
        return false;
    }

    // 某些状态码不应该重试（如4xx客户端错误）
    if (response.statusCode >= 400 && response.statusCode < 500) {
        return false;
    }

    return true;
}

void HttpRequest::updateProgress(TransCtx& ctx,
                                 size_t transferred,
                                 size_t total,
                                 ProgressCallback callback) {
    ctx.transferredBytes = transferred;
    ctx.totalBytes = total;

    if (callback) {
        callback(ctx, transferred, total);
    }
}

void HttpRequest::cancel(const std::string& requestId) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    auto it = pImpl->cancelFlags.find(requestId);
    if (it != pImpl->cancelFlags.end()) {
        it->second = true;
    }
}

void HttpRequest::cancelAll() {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    for (auto& pair : pImpl->cancelFlags) {
        pair.second = true;
    }
}

void HttpRequest::setGlobalHeader(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->globalHeaders[key] = value;
}

void HttpRequest::removeGlobalHeader(const std::string& key) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->globalHeaders.erase(key);
}

void HttpRequest::setGlobalTimeout(int seconds) {
    std::lock_guard<std::mutex> lock(pImpl->mutex);
    pImpl->globalTimeout = seconds;
}
