/*
 * Simple Benchmark Tool for Phase 0 PoC
 *
 * Measures ingestion performance with configurable concurrency.
 * Outputs: throughput (RPS), latency percentiles (p50, p90, p99)
 *
 * Usage: ./simple_benchmark <host> <port> <num_clients> <requests_per_client>
 * Example: ./simple_benchmark localhost 8080 10 100
 */

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <mutex>
#include <sstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

struct BenchmarkResult {
    int requests_sent;
    int requests_succeeded;
    std::vector<double> latencies_ms;  // Per-request latency
};

std::mutex results_mutex;
std::vector<double> all_latencies;
std::atomic<int> total_requests_sent(0);
std::atomic<int> total_requests_succeeded(0);

// Send a single HTTP POST request
bool send_metric_request(const std::string& host, int port, int value, double& latency_ms) {
    auto start = std::chrono::high_resolution_clock::now();

    // Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return false;
    }

    // Connect
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return false;
    }

    // Build HTTP request
    std::ostringstream body;
    body << "{\"name\":\"benchmark\",\"value\":" << value << "}";
    std::string body_str = body.str();

    std::ostringstream request;
    request << "POST /metrics HTTP/1.1\r\n";
    request << "Host: " << host << "\r\n";
    request << "Content-Type: application/json\r\n";
    request << "Content-Length: " << body_str.length() << "\r\n";
    request << "\r\n";
    request << body_str;

    std::string request_str = request.str();

    // Send request
    if (send(sock, request_str.c_str(), request_str.length(), 0) < 0) {
        close(sock);
        return false;
    }

    // Receive response (just check for success, don't parse fully)
    char buffer[1024];
    ssize_t bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);

    close(sock);

    auto end = std::chrono::high_resolution_clock::now();
    latency_ms = std::chrono::duration<double, std::milli>(end - start).count();

    // Check if successful (look for "202" or "200" in response)
    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::string response(buffer);
        return (response.find("202") != std::string::npos ||
                response.find("200") != std::string::npos);
    }

    return false;
}

// Client thread function
void client_worker(const std::string& host, int port, int requests_per_client) {
    for (int i = 0; i < requests_per_client; i++) {
        total_requests_sent++;

        double latency_ms;
        bool success = send_metric_request(host, port, i, latency_ms);

        if (success) {
            total_requests_succeeded++;

            // Record latency
            std::lock_guard<std::mutex> lock(results_mutex);
            all_latencies.push_back(latency_ms);
        }
    }
}

// Calculate percentile
double calculate_percentile(std::vector<double>& sorted_data, double percentile) {
    if (sorted_data.empty()) return 0.0;

    size_t index = static_cast<size_t>(std::ceil(percentile / 100.0 * sorted_data.size())) - 1;
    index = std::min(index, sorted_data.size() - 1);

    return sorted_data[index];
}

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <host> <port> <num_clients> <requests_per_client>" << std::endl;
        std::cerr << "Example: " << argv[0] << " localhost 8080 10 100" << std::endl;
        return 1;
    }

    std::string host = argv[1];
    int port = std::stoi(argv[2]);
    int num_clients = std::stoi(argv[3]);
    int requests_per_client = std::stoi(argv[4]);

    std::cout << "==========================================" << std::endl;
    std::cout << "Phase 0 Simple Benchmark" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  Target: " << host << ":" << port << std::endl;
    std::cout << "  Clients: " << num_clients << std::endl;
    std::cout << "  Requests per client: " << requests_per_client << std::endl;
    std::cout << "  Total requests: " << (num_clients * requests_per_client) << std::endl;
    std::cout << std::endl;

    std::cout << "Running benchmark..." << std::endl;

    // Start timer
    auto start_time = std::chrono::high_resolution_clock::now();

    // Spawn client threads
    std::vector<std::thread> threads;
    for (int i = 0; i < num_clients; i++) {
        threads.emplace_back(client_worker, host, port, requests_per_client);
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Stop timer
    auto end_time = std::chrono::high_resolution_clock::now();
    double duration_sec = std::chrono::duration<double>(end_time - start_time).count();

    // Calculate statistics
    int total_requests = num_clients * requests_per_client;
    int succeeded = total_requests_succeeded.load();
    int failed = total_requests - succeeded;
    double success_rate = (succeeded * 100.0) / total_requests;
    double rps = succeeded / duration_sec;

    // Sort latencies for percentile calculation
    std::sort(all_latencies.begin(), all_latencies.end());

    double avg_latency = 0.0;
    if (!all_latencies.empty()) {
        for (double lat : all_latencies) {
            avg_latency += lat;
        }
        avg_latency /= all_latencies.size();
    }

    double p50 = calculate_percentile(all_latencies, 50);
    double p90 = calculate_percentile(all_latencies, 90);
    double p99 = calculate_percentile(all_latencies, 99);

    // Print results
    std::cout << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << "Results" << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << std::endl;

    std::cout << "Throughput:" << std::endl;
    std::cout << "  Duration: " << duration_sec << "s" << std::endl;
    std::cout << "  Requests sent: " << total_requests << std::endl;
    std::cout << "  Successful: " << succeeded << std::endl;
    std::cout << "  Failed: " << failed << std::endl;
    std::cout << "  Success rate: " << success_rate << "%" << std::endl;
    std::cout << "  Throughput: " << rps << " RPS" << std::endl;
    std::cout << std::endl;

    std::cout << "Latency:" << std::endl;
    std::cout << "  Average: " << avg_latency << "ms" << std::endl;
    std::cout << "  p50: " << p50 << "ms" << std::endl;
    std::cout << "  p90: " << p90 << "ms" << std::endl;
    std::cout << "  p99: " << p99 << "ms" << std::endl;
    std::cout << std::endl;

    // Analysis
    std::cout << "Analysis:" << std::endl;
    if (rps < 50) {
        std::cout << "  ⚠️  Low throughput detected (<50 RPS)" << std::endl;
        std::cout << "     → Bottleneck: Single-threaded ingestion" << std::endl;
        std::cout << "     → Fix in Craft #1: Thread pool + async I/O" << std::endl;
    } else if (rps < 200) {
        std::cout << "  ⚠️  Moderate throughput (50-200 RPS)" << std::endl;
        std::cout << "     → Still limited by single-threaded design" << std::endl;
    } else {
        std::cout << "  ✅ Good throughput (>200 RPS)" << std::endl;
    }

    if (success_rate < 95.0) {
        std::cout << "  ⚠️  High failure rate (" << (100 - success_rate) << "%)" << std::endl;
        std::cout << "     → Possible causes: Connection limits, timeouts, server overload" << std::endl;
    }

    if (p99 > 100) {
        std::cout << "  ⚠️  High tail latency (p99 = " << p99 << "ms)" << std::endl;
        std::cout << "     → Queue backpressure or slow file writes" << std::endl;
    }

    std::cout << std::endl;
    std::cout << "Next steps:" << std::endl;
    std::cout << "  • Try different client counts: 1, 10, 50, 100" << std::endl;
    std::cout << "  • Compare with Craft #1 optimized version" << std::endl;
    std::cout << "  • Expected improvement: 10x throughput (200 → 2,253 RPS)" << std::endl;
    std::cout << std::endl;

    return 0;
}
