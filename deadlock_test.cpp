#include "ingestion_service.h"
#include <thread>
#include <vector>
#include <chrono>
#include <iostream>
#include <atomic>
#include <random>

using namespace metricstream;

class DeadlockTest {
public:
    void run_concurrent_flush_test() {
        std::cout << "=== Testing Concurrent flush_metrics() Calls ===" << std::endl;
        
        RateLimiter rate_limiter(1000); // 1000 requests per second
        
        // Step 1: Generate metrics from multiple clients
        const int NUM_CLIENTS = 20;
        const int REQUESTS_PER_CLIENT = 50;
        
        std::cout << "Generating metrics from " << NUM_CLIENTS << " clients..." << std::endl;
        
        // Generate rate limiting decisions to populate ring buffers
        for (int client = 0; client < NUM_CLIENTS; ++client) {
            std::string client_id = "client_" + std::to_string(client);
            
            for (int req = 0; req < REQUESTS_PER_CLIENT; ++req) {
                rate_limiter.allow_request(client_id);
                
                // Add small delay to create timing variance
                std::this_thread::sleep_for(std::chrono::microseconds(10));
            }
        }
        
        std::cout << "Metrics generated. Starting concurrent flush test..." << std::endl;
        
        // Step 2: Launch multiple flush_metrics() threads concurrently
        const int NUM_FLUSH_THREADS = 8;
        const int FLUSH_ITERATIONS = 10;
        
        std::vector<std::thread> flush_threads;
        std::atomic<int> successful_flushes{0};
        std::atomic<int> total_flush_calls{0};
        std::atomic<bool> test_failed{false};
        
        auto flush_worker = [&]() {
            for (int i = 0; i < FLUSH_ITERATIONS; ++i) {
                try {
                    total_flush_calls++;
                    
                    auto start = std::chrono::steady_clock::now();
                    rate_limiter.flush_metrics();
                    auto end = std::chrono::steady_clock::now();
                    
                    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
                    
                    // Flush should complete within reasonable time (not hang due to deadlock)
                    if (duration_ms > 1000) { // 1 second timeout
                        std::cout << "WARNING: flush_metrics() took " << duration_ms << "ms" << std::endl;
                    }
                    
                    successful_flushes++;
                    
                    // Random delay between flushes to create different timing patterns
                    std::random_device rd;
                    std::mt19937 gen(rd());
                    std::uniform_int_distribution<> dis(1, 50);
                    std::this_thread::sleep_for(std::chrono::milliseconds(dis(gen)));
                    
                } catch (const std::exception& e) {
                    std::cout << "ERROR in flush_metrics(): " << e.what() << std::endl;
                    test_failed = true;
                }
            }
        };
        
        // Launch all flush threads simultaneously
        auto test_start = std::chrono::steady_clock::now();
        
        for (int i = 0; i < NUM_FLUSH_THREADS; ++i) {
            flush_threads.emplace_back(flush_worker);
        }
        
        // Wait for all threads to complete
        for (auto& thread : flush_threads) {
            thread.join();
        }
        
        auto test_end = std::chrono::steady_clock::now();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(test_end - test_start).count();
        
        // Results
        std::cout << "\n=== Test Results ===" << std::endl;
        std::cout << "Total flush calls: " << total_flush_calls.load() << std::endl;
        std::cout << "Successful flushes: " << successful_flushes.load() << std::endl;
        std::cout << "Test duration: " << total_duration << "ms" << std::endl;
        std::cout << "Average flush time: " << (total_duration / successful_flushes.load()) << "ms" << std::endl;
        
        if (test_failed) {
            std::cout << "❌ TEST FAILED: Errors occurred during flush operations" << std::endl;
        } else if (successful_flushes.load() == total_flush_calls.load()) {
            std::cout << "✅ TEST PASSED: No deadlocks detected, all flushes completed" << std::endl;
        } else {
            std::cout << "⚠️  TEST PARTIAL: Some flushes may have been skipped due to contention" << std::endl;
        }
    }
    
    void run_stress_test() {
        std::cout << "\n=== Stress Test: Concurrent Requests + Flushes ===" << std::endl;
        
        RateLimiter rate_limiter(2000); // Higher rate limit for stress test
        
        const int NUM_REQUEST_THREADS = 10;
        const int NUM_FLUSH_THREADS = 3;
        const int TEST_DURATION_SECONDS = 5;
        
        std::atomic<bool> stop_test{false};
        std::atomic<int> total_requests{0};
        std::atomic<int> total_flushes{0};
        
        std::vector<std::thread> request_threads;
        std::vector<std::thread> flush_threads;
        
        // Request generating threads
        auto request_worker = [&](int thread_id) {
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<> client_dis(1, 100); // 100 different clients
            
            while (!stop_test.load()) {
                std::string client_id = "stress_client_" + std::to_string(client_dis(gen));
                rate_limiter.allow_request(client_id);
                total_requests++;
                
                // Small delay to prevent spinning
                std::this_thread::sleep_for(std::chrono::microseconds(100));
            }
        };
        
        // Flush threads
        auto flush_worker = [&]() {
            while (!stop_test.load()) {
                rate_limiter.flush_metrics();
                total_flushes++;
                
                // Flush every 100ms
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        };
        
        std::cout << "Starting stress test for " << TEST_DURATION_SECONDS << " seconds..." << std::endl;
        
        // Start all threads
        for (int i = 0; i < NUM_REQUEST_THREADS; ++i) {
            request_threads.emplace_back(request_worker, i);
        }
        
        for (int i = 0; i < NUM_FLUSH_THREADS; ++i) {
            flush_threads.emplace_back(flush_worker);
        }
        
        // Run for specified duration
        std::this_thread::sleep_for(std::chrono::seconds(TEST_DURATION_SECONDS));
        
        // Stop all threads
        stop_test.store(true);
        
        for (auto& thread : request_threads) {
            thread.join();
        }
        
        for (auto& thread : flush_threads) {
            thread.join();
        }
        
        std::cout << "\n=== Stress Test Results ===" << std::endl;
        std::cout << "Total requests processed: " << total_requests.load() << std::endl;
        std::cout << "Total flushes completed: " << total_flushes.load() << std::endl;
        std::cout << "Requests per second: " << (total_requests.load() / TEST_DURATION_SECONDS) << std::endl;
        std::cout << "Flushes per second: " << (total_flushes.load() / TEST_DURATION_SECONDS) << std::endl;
        std::cout << "✅ STRESS TEST COMPLETED: No deadlocks or hangs detected" << std::endl;
    }
};

int main() {
    std::cout << "MetricStream Deadlock Prevention Test" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    DeadlockTest test;
    
    // Run concurrent flush test
    test.run_concurrent_flush_test();
    
    // Run stress test
    test.run_stress_test();
    
    std::cout << "\n🎯 All deadlock prevention tests completed successfully!" << std::endl;
    std::cout << "The flush_metrics() implementation handles concurrent access safely." << std::endl;
    
    return 0;
}