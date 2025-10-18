//
// Created by Yu Xin on 2025/10/18.
//

#ifndef FRAME_HTTPREQUEST_H
#define FRAME_HTTPREQUEST_H

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include <TransCtx.h>


using namespace std;

// HTTP 方法枚举
enum class HttpMethod {
    GET,
    POST,
    PUT,
    DELETE,
    PATCH,
    HEAD,
    OPTIONS
};

// HTTP 响应状态
enum class HttpStatus {
    SUCCESS,
    FAILED,
    TIMEOUT,
    NETWORK_ERROR,
    CANCELLED
};

struct HttpResponse {
    int statusCode;                          // HTTP状态码
    string statusMessage;                    // 状态消息
    unordered_map<string, string> headers;   // 响应头
    string body;                             // 响应体
    HttpStatus status;                       // 请求状态
    string errorMessage;                     // 错误信息

    HttpResponse()
        : statusCode(0)
        , status(HttpStatus::FAILED) {}

    bool isSuccess() const {
        return status == HttpStatus::SUCCESS && statusCode >= 200 && statusCode < 300;
    }
};

// 回调函数类型定义
// 参数1: TransCtx引用 - 传输上下文
// 参数2: HttpResponse引用 - HTTP响应
// 参数3: bool - 是否完成（用于进度回调）
using HttpCallback = std::function<void(TransCtx&, const HttpResponse&, bool)>;

// 进度回调类型
using ProgressCallback = std::function<void(TransCtx&, size_t, size_t)>;

// HTTP请求配置
struct HttpRequestConfig {
    string url;                         // 请求URL
    HttpMethod method;                       // HTTP方法
    unordered_map<string, string> headers; // 请求头
    string body;                        // 请求体
    int timeout;                             // 超时时间（秒）
    int maxRetries;                          // 最大重试次数
    bool followRedirects;                    // 是否跟随重定向

    HttpRequestConfig()
        : method(HttpMethod::GET)
        , timeout(30)
        , maxRetries(0)
        , followRedirects(true) {}
};

class HttpRequest {
public:
    HttpRequest();
    ~HttpRequest();

    // 发送异步请求（带完成回调）
    void sendAsync(const HttpRequestConfig& config,
                   HttpCallback callback);

    // 发送异步请求（带完成回调和进度回调）
    void sendAsync(const HttpRequestConfig& config,
                   HttpCallback completionCallback,
                   ProgressCallback progressCallback);

    // 发送同步请求
    HttpResponse sendSync(const HttpRequestConfig& config);

    // 取消请求
    void cancel(const std::string& requestId);

    // 取消所有请求
    void cancelAll();

    // 设置全局请求头
    void setGlobalHeader(const std::string& key, const std::string& value);

    // 移除全局请求头
    void removeGlobalHeader(const std::string& key);

    // 设置全局超时时间
    void setGlobalTimeout(int seconds);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

    // 执行HTTP请求的核心逻辑
    void executeRequest(TransCtx& ctx,
                       const HttpRequestConfig& config,
                       HttpResponse& response);

    // 处理重试逻辑
    bool shouldRetry(const TransCtx& ctx,
                    const HttpRequestConfig& config,
                    const HttpResponse& response);

    // 网络传输进度更新
    void updateProgress(TransCtx& ctx,
                       size_t transferred,
                       size_t total,
                       ProgressCallback callback);
};


#endif //FRAME_HTTPREQUEST_H