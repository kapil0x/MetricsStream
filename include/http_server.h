#pragma once

#include <string>
#include <functional>
#include <memory>
#include <thread>
#include <atomic>
#include <unordered_map>
#include "thread_pool.h"

namespace metricstream {

struct HttpRequest {
    std::string method;
    std::string path;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
};

struct HttpResponse {
    int status_code = 200;
    std::string body;
    std::unordered_map<std::string, std::string> headers;
    
    void set_json_content() {
        headers["Content-Type"] = "application/json";
    }
};

using HttpHandler = std::function<HttpResponse(const HttpRequest&)>;

class HttpServer {
public:
    HttpServer(int port, size_t thread_pool_size = 16);
    ~HttpServer();

    void add_handler(const std::string& path, const std::string& method, HttpHandler handler);
    void start();
    void stop();

private:
    int port_;
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> server_thread_;
    std::unique_ptr<ThreadPool> thread_pool_;  // Phase 6: Thread pool

    void run_server();
    HttpResponse handle_request(const HttpRequest& request);
    HttpRequest parse_request(const std::string& raw_request);
    std::string format_response(const HttpResponse& response);

    std::unordered_map<std::string, std::unordered_map<std::string, HttpHandler>> handlers_;
};

} // namespace metricstream