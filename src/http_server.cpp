#include "http_server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <cstring>

namespace metricstream {

HttpServer::HttpServer(int port) 
    : port_(port), running_(false) {}

HttpServer::~HttpServer() {
    stop();
}

void HttpServer::add_handler(const std::string& path, const std::string& method, HttpHandler handler) {
    handlers_[path][method] = std::move(handler);
}

void HttpServer::start() {
    if (running_.load()) {
        return;
    }
    
    running_ = true;
    server_thread_ = std::make_unique<std::thread>(&HttpServer::run_server, this);
    std::cout << "HTTP server started on port " << port_ << std::endl;
}

void HttpServer::stop() {
    if (!running_.load()) {
        return;
    }
    
    running_ = false;
    if (server_thread_ && server_thread_->joinable()) {
        server_thread_->join();
    }
    std::cout << "HTTP server stopped" << std::endl;
}

void HttpServer::run_server() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    // Allow socket reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Bind failed on port " << port_ << std::endl;
        close(server_fd);
        return;
    }

    if (listen(server_fd, 10) < 0) {
        std::cerr << "Listen failed" << std::endl;
        close(server_fd);
        return;
    }

    while (running_.load()) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running_.load()) {
                std::cerr << "Accept failed" << std::endl;
            }
            continue;
        }

        // Process request in separate thread for concurrency
        std::thread([this, client_socket]() {
            // Read request
            char buffer[4096] = {0};
            ssize_t bytes_read = read(client_socket, buffer, sizeof(buffer) - 1);
            
            if (bytes_read > 0) {
                std::string request_data(buffer, bytes_read);
                HttpRequest request = parse_request(request_data);
                HttpResponse response = handle_request(request);
                
                std::string response_str = format_response(response);
                write(client_socket, response_str.c_str(), response_str.length());
            }

            close(client_socket);
        }).detach(); // Detach thread to handle request independently
    }

    close(server_fd);
}

HttpRequest HttpServer::parse_request(const std::string& raw_request) {
    HttpRequest request;
    std::istringstream stream(raw_request);
    std::string line;
    
    // Parse request line
    if (std::getline(stream, line)) {
        std::istringstream request_line(line);
        request_line >> request.method >> request.path;
    }
    
    // Parse headers
    while (std::getline(stream, line) && line != "\r" && !line.empty()) {
        size_t colon_pos = line.find(':');
        if (colon_pos != std::string::npos) {
            std::string key = line.substr(0, colon_pos);
            std::string value = line.substr(colon_pos + 2); // Skip ": "
            
            // Remove trailing \r if present
            if (!value.empty() && value.back() == '\r') {
                value.pop_back();
            }
            
            request.headers[key] = value;
        }
    }
    
    // Parse body (if present)
    std::ostringstream body_stream;
    while (std::getline(stream, line)) {
        body_stream << line << "\n";
    }
    request.body = body_stream.str();
    
    // Remove trailing newline if present
    if (!request.body.empty() && request.body.back() == '\n') {
        request.body.pop_back();
    }
    
    return request;
}

std::string HttpServer::format_response(const HttpResponse& response) {
    std::ostringstream stream;
    
    // Status line
    stream << "HTTP/1.1 " << response.status_code << " ";
    switch (response.status_code) {
        case 200: stream << "OK"; break;
        case 400: stream << "Bad Request"; break;
        case 429: stream << "Too Many Requests"; break;
        case 500: stream << "Internal Server Error"; break;
        default: stream << "Unknown"; break;
    }
    stream << "\r\n";
    
    // Headers
    stream << "Content-Length: " << response.body.length() << "\r\n";
    for (const auto& header : response.headers) {
        stream << header.first << ": " << header.second << "\r\n";
    }
    stream << "\r\n";
    
    // Body
    stream << response.body;
    
    return stream.str();
}

HttpResponse HttpServer::handle_request(const HttpRequest& request) {
    auto path_it = handlers_.find(request.path);
    if (path_it == handlers_.end()) {
        HttpResponse response;
        response.status_code = 404;
        response.body = "Not Found";
        return response;
    }
    
    auto method_it = path_it->second.find(request.method);
    if (method_it == path_it->second.end()) {
        HttpResponse response;
        response.status_code = 405;
        response.body = "Method Not Allowed";
        return response;
    }
    
    return method_it->second(request);
}

} // namespace metricstream