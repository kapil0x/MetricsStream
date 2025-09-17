#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <random>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>
#include <iomanip>

class LoadTestStats {
public:
    std::atomic<int> total_requests{0};
    std::atomic<int> successful_requests{0};
    std::atomic<int> failed_requests{0};
    std::atomic<long> total_latency_ms{0};
    std::chrono::steady_clock::time_point start_time;
    std::mutex print_mutex;

    void record_request(bool success, long latency_ms) {
        total_requests++;
        if (success) {
            successful_requests++;
        } else {
            failed_requests++;
        }
        total_latency_ms += latency_ms;
    }

    void print_stats() {
        std::lock_guard<std::mutex> lock(print_mutex);
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);

        double requests_per_second = duration.count() > 0 ?
            static_cast<double>(total_requests) / duration.count() : 0;
        double avg_latency = total_requests > 0 ?
            static_cast<double>(total_latency_ms) / total_requests : 0;
        double success_rate = total_requests > 0 ?
            static_cast<double>(successful_requests) / total_requests * 100 : 0;

        std::cout << "\n=== Load Test Statistics ===" << std::endl;
        std::cout << "Duration: " << duration.count() << " seconds" << std::endl;
        std::cout << "Total Requests: " << total_requests << std::endl;
        std::cout << "Successful: " << successful_requests << std::endl;
        std::cout << "Failed: " << failed_requests << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(2) << success_rate << "%" << std::endl;
        std::cout << "Requests/sec: " << std::fixed << std::setprecision(2) << requests_per_second << std::endl;
        std::cout << "Avg Latency: " << std::fixed << std::setprecision(2) << avg_latency << " ms" << std::endl;
        std::cout << "=========================" << std::endl;
    }
};

class MetricGenerator {
private:
    std::mt19937 gen;
    std::uniform_real_distribution<> cpu_dist{10.0, 90.0};
    std::uniform_int_distribution<long long> memory_dist{1000000, 8000000000};
    std::uniform_int_distribution<> counter_dist{1, 1000};
    std::vector<std::string> hosts{"web1", "web2", "db1", "db2", "cache1"};
    std::vector<std::string> regions{"us-west", "us-east", "eu-west", "ap-south"};

public:
    MetricGenerator() : gen(std::random_device{}()) {}

    std::string generate_metrics(const std::string& client_id) {
        std::string host = hosts[std::uniform_int_distribution<>(0, hosts.size()-1)(gen)];
        std::string region = regions[std::uniform_int_distribution<>(0, regions.size()-1)(gen)];

        std::string json_body = R"({
            "metrics": [
                {
                    "name": "cpu_usage",
                    "value": )" + std::to_string(cpu_dist(gen)) + R"(,
                    "type": "gauge",
                    "tags": {"host": ")" + host + R"(", "region": ")" + region + R"("}
                },
                {
                    "name": "memory_usage",
                    "value": )" + std::to_string(memory_dist(gen)) + R"(,
                    "type": "gauge",
                    "tags": {"host": ")" + host + R"("}
                },
                {
                    "name": "requests_total",
                    "value": )" + std::to_string(counter_dist(gen)) + R"(,
                    "type": "counter",
                    "tags": {"service": "api", "host": ")" + host + R"("}
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
};

bool send_metric_request(const std::string& host, int port, const std::string& request, LoadTestStats& stats) {
    auto start_time = std::chrono::steady_clock::now();

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        auto end_time = std::chrono::steady_clock::now();
        auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
        stats.record_request(false, latency);
        return false;
    }

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 5;  // 5 second timeout
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    bool success = true;
    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        success = false;
    } else if (send(sock, request.c_str(), request.length(), 0) < 0) {
        success = false;
    } else {
        // Try to read response
        char buffer[1024] = {0};
        ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            success = false;
        }
    }

    close(sock);

    auto end_time = std::chrono::steady_clock::now();
    auto latency = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
    stats.record_request(success, latency);

    return success;
}

void client_worker(const std::string& host, int port, int client_id, int requests_per_client,
                  int request_interval_ms, LoadTestStats& stats, MetricGenerator& generator) {
    std::string client_name = "load_client_" + std::to_string(client_id);

    for (int i = 0; i < requests_per_client; ++i) {
        std::string request = generator.generate_metrics(client_name);
        send_metric_request(host, port, request, stats);

        if (request_interval_ms > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(request_interval_ms));
        }
    }
}

void print_progress(LoadTestStats& stats, int duration_seconds) {
    for (int i = 0; i < duration_seconds; ++i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "Progress: " << (i + 1) << "/" << duration_seconds
                  << " seconds, Requests: " << stats.total_requests
                  << " (Success: " << stats.successful_requests
                  << ", Failed: " << stats.failed_requests << ")" << std::endl;
    }
}

int main(int argc, char* argv[]) {
    std::string host = "127.0.0.1";
    int port = 8080;
    int num_clients = 10;
    int requests_per_client = 100;
    int request_interval_ms = 100;  // 100ms between requests per client

    if (argc > 1) port = std::stoi(argv[1]);
    if (argc > 2) num_clients = std::stoi(argv[2]);
    if (argc > 3) requests_per_client = std::stoi(argv[3]);
    if (argc > 4) request_interval_ms = std::stoi(argv[4]);

    std::cout << "MetricStream Load Test" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "Clients: " << num_clients << std::endl;
    std::cout << "Requests per client: " << requests_per_client << std::endl;
    std::cout << "Interval between requests: " << request_interval_ms << "ms" << std::endl;
    std::cout << "Expected total requests: " << (num_clients * requests_per_client) << std::endl;
    std::cout << "Expected duration: ~" << (requests_per_client * request_interval_ms / 1000) << " seconds" << std::endl;
    std::cout << "\nStarting load test..." << std::endl;

    LoadTestStats stats;
    MetricGenerator generator;
    stats.start_time = std::chrono::steady_clock::now();

    std::vector<std::thread> client_threads;

    // Start progress monitor
    int expected_duration = requests_per_client * request_interval_ms / 1000;
    std::thread progress_thread(print_progress, std::ref(stats), expected_duration + 5);

    // Launch client threads
    for (int i = 0; i < num_clients; ++i) {
        client_threads.emplace_back(client_worker, host, port, i, requests_per_client,
                                   request_interval_ms, std::ref(stats), std::ref(generator));
    }

    // Wait for all clients to complete
    for (auto& thread : client_threads) {
        thread.join();
    }

    // Stop progress monitoring
    progress_thread.detach();

    // Final statistics
    stats.print_stats();

    return 0;
}