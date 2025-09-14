#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <thread>
#include <chrono>

void send_http_request(const std::string& host, int port, const std::string& request) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);
    
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to connect to server" << std::endl;
        close(sock);
        return;
    }
    
    send(sock, request.c_str(), request.length(), 0);
    
    char buffer[1024] = {0};
    ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received > 0) {
        std::cout << "Response:\n" << buffer << std::endl;
    }
    
    close(sock);
}

std::string create_metrics_request(const std::string& client_id = "test_client") {
    std::string json_body = R"({
        "metrics": [
            {
                "name": "cpu_usage",
                "value": 75.5,
                "type": "gauge",
                "tags": {"host": "server1", "region": "us-west"}
            },
            {
                "name": "memory_usage",
                "value": 1024000000,
                "type": "gauge",
                "tags": {"host": "server1"}
            },
            {
                "name": "requests_total",
                "value": 12345,
                "type": "counter"
            }
        ]
    })";
    
    std::string request = "POST /metrics HTTP/1.1\r\n";
    request += "Host: localhost\r\n";
    request += "Content-Type: application/json\r\n";
    request += "Authorization: " + client_id + "\r\n";
    request += "Content-Length: " + std::to_string(json_body.length()) + "\r\n";
    request += "\r\n";
    request += json_body;
    
    return request;
}

std::string create_health_request() {
    return "GET /health HTTP/1.1\r\n"
           "Host: localhost\r\n"
           "\r\n";
}

std::string create_stats_request() {
    return "GET /metrics HTTP/1.1\r\n"
           "Host: localhost\r\n"
           "\r\n";
}

void test_rate_limiting(const std::string& host, int port) {
    std::cout << "\n=== Testing Rate Limiting ===" << std::endl;
    
    // Send requests rapidly to trigger rate limiting
    for (int i = 0; i < 5; ++i) {
        std::cout << "Sending request " << (i + 1) << "..." << std::endl;
        send_http_request(host, port, create_metrics_request("rate_test_client"));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void test_multiple_clients(const std::string& host, int port) {
    std::cout << "\n=== Testing Multiple Clients ===" << std::endl;
    
    std::vector<std::thread> threads;
    
    for (int i = 0; i < 3; ++i) {
        threads.emplace_back([host, port, i]() {
            std::string client_id = "client_" + std::to_string(i);
            std::cout << "Client " << client_id << " sending request..." << std::endl;
            send_http_request(host, port, create_metrics_request(client_id));
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;
    
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    
    std::cout << "MetricStream Test Client" << std::endl;
    std::cout << "Testing server at " << host << ":" << port << std::endl;
    
    // Test health check
    std::cout << "\n=== Testing Health Check ===" << std::endl;
    send_http_request(host, port, create_health_request());
    
    // Test basic metrics submission
    std::cout << "\n=== Testing Metrics Submission ===" << std::endl;
    send_http_request(host, port, create_metrics_request());
    
    // Test statistics endpoint
    std::cout << "\n=== Testing Statistics ===" << std::endl;
    send_http_request(host, port, create_stats_request());
    
    // Test rate limiting
    test_rate_limiting(host, port);
    
    // Test multiple clients
    test_multiple_clients(host, port);
    
    // Final statistics
    std::cout << "\n=== Final Statistics ===" << std::endl;
    send_http_request(host, port, create_stats_request());
    
    std::cout << "\nTest completed! Check metrics.jsonl file for stored data." << std::endl;
    
    return 0;
}