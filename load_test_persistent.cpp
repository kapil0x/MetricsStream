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

// Persistent connection load test - reuses sockets to eliminate connection overhead
// This tests the actual server capacity, not TCP handshake performance

class LoadTestStats {
public:
    std::atomic<int> total_requests{0};
    std::atomic<int> successful_requests{0};
    std::atomic<int> failed_requests{0};
    std::atomic<long> total_latency_us{0};  // Changed to microseconds for precision
    std::chrono::steady_clock::time_point start_time;
    std::mutex print_mutex;

    void record_request(bool success, long latency_us) {
        total_requests++;
        if (success) {
            successful_requests++;
        } else {
            failed_requests++;
        }
        total_latency_us += latency_us;
    }

    void print_stats() {
        std::lock_guard<std::mutex> lock(print_mutex);
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);

        double requests_per_second = duration.count() > 0 ?
            static_cast<double>(total_requests) / (duration.count() / 1000.0) : 0;
        double avg_latency_us = total_requests > 0 ?
            static_cast<double>(total_latency_us) / total_requests : 0;
        double success_rate = total_requests > 0 ?
            static_cast<double>(successful_requests) / total_requests * 100 : 0;

        std::cout << "\n=== Persistent Connection Load Test ===" << std::endl;
        std::cout << "Duration: " << duration.count() << " ms" << std::endl;
        std::cout << "Total Requests: " << total_requests << std::endl;
        std::cout << "Successful: " << successful_requests << std::endl;
        std::cout << "Failed: " << failed_requests << std::endl;
        std::cout << "Success Rate: " << std::fixed << std::setprecision(2) << success_rate << "%" << std::endl;
        std::cout << "Actual RPS: " << std::fixed << std::setprecision(2) << requests_per_second << std::endl;
        std::cout << "Avg Latency: " << std::fixed << std::setprecision(2) << avg_latency_us << " μs" << std::endl;
        std::cout << "========================================" << std::endl;
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
        request += "Connection: keep-alive\r\n";  // Request persistent connection
        request += "Content-Length: " + std::to_string(json_body.length()) + "\r\n";
        request += "\r\n";
        request += json_body;

        return request;
    }
};

// Persistent connection client - creates socket once, reuses for all requests
void persistent_client_worker(const std::string& host, int port, int client_id,
                              int requests_per_client, int request_interval_us,
                              LoadTestStats& stats, MetricGenerator& generator) {
    std::string client_name = "persistent_client_" + std::to_string(client_id);

    // Create socket ONCE
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Client " << client_id << ": Failed to create socket" << std::endl;
        for (int i = 0; i < requests_per_client; ++i) {
            stats.record_request(false, 0);
        }
        return;
    }

    // Set socket timeout
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));

    // Connect ONCE
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Client " << client_id << ": Failed to connect" << std::endl;
        close(sock);
        for (int i = 0; i < requests_per_client; ++i) {
            stats.record_request(false, 0);
        }
        return;
    }

    // Reuse connection for ALL requests
    for (int i = 0; i < requests_per_client; ++i) {
        auto start_time = std::chrono::steady_clock::now();

        std::string request = generator.generate_metrics(client_name);

        bool success = true;
        if (send(sock, request.c_str(), request.length(), 0) < 0) {
            // Send failed - connection might be closed
            success = false;
            // Record failure and exit early
            auto end_time = std::chrono::steady_clock::now();
            auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
            stats.record_request(success, latency_us);
            // Record remaining requests as failures
            for (int j = i + 1; j < requests_per_client; ++j) {
                stats.record_request(false, 0);
            }
            break;
        } else {
            // Read response
            char buffer[2048] = {0};
            ssize_t bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes_received <= 0) {
                // Connection closed by server or error
                success = false;
                // Record failure and exit early
                auto end_time = std::chrono::steady_clock::now();
                auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
                stats.record_request(success, latency_us);
                // Record remaining requests as failures
                for (int j = i + 1; j < requests_per_client; ++j) {
                    stats.record_request(false, 0);
                }
                break;
            }
        }

        auto end_time = std::chrono::steady_clock::now();
        auto latency_us = std::chrono::duration_cast<std::chrono::microseconds>(end_time - start_time).count();
        stats.record_request(success, latency_us);

        if (request_interval_us > 0) {
            std::this_thread::sleep_for(std::chrono::microseconds(request_interval_us));
        }
    }

    // Close connection after all requests
    close(sock);
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
    int request_interval_us = 100000;  // 100ms = 100000μs default

    if (argc > 1) port = std::stoi(argv[1]);
    if (argc > 2) num_clients = std::stoi(argv[2]);
    if (argc > 3) requests_per_client = std::stoi(argv[3]);
    if (argc > 4) request_interval_us = std::stoi(argv[4]);

    int target_rps = num_clients * (1000000 / request_interval_us);

    std::cout << "MetricStream Persistent Connection Load Test" << std::endl;
    std::cout << "=============================================" << std::endl;
    std::cout << "Target: " << host << ":" << port << std::endl;
    std::cout << "Clients: " << num_clients << " (persistent connections)" << std::endl;
    std::cout << "Requests per client: " << requests_per_client << std::endl;
    std::cout << "Interval between requests: " << request_interval_us << "μs" << std::endl;
    std::cout << "Expected total requests: " << (num_clients * requests_per_client) << std::endl;
    std::cout << "Target RPS: " << target_rps << std::endl;
    std::cout << "Expected duration: ~" << (requests_per_client * request_interval_us / 1000000) << " seconds" << std::endl;
    std::cout << "\nStarting load test..." << std::endl;

    LoadTestStats stats;
    MetricGenerator generator;
    stats.start_time = std::chrono::steady_clock::now();

    std::vector<std::thread> client_threads;

    // Start progress monitor
    int expected_duration = requests_per_client * request_interval_us / 1000000;
    std::thread progress_thread(print_progress, std::ref(stats), expected_duration + 5);

    // Launch persistent connection clients
    for (int i = 0; i < num_clients; ++i) {
        client_threads.emplace_back(persistent_client_worker, host, port, i, requests_per_client,
                                   request_interval_us, std::ref(stats), std::ref(generator));
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
