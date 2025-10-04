#pragma once

#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <atomic>

namespace metricstream {

// Phase 6: Thread Pool for eliminating thread creation overhead
// Replaces thread-per-request model with fixed worker pool
class ThreadPool {
public:
    // Constructor: Create pool with specified number of workers
    // Default: 16 workers (good for 8-core system with hyperthreading)
    explicit ThreadPool(size_t num_threads = 16, size_t max_queue_size = 10000);

    // Destructor: Clean shutdown, waits for all tasks to complete
    ~ThreadPool();

    // Enqueue a task for execution
    // Returns true if enqueued, false if queue is full (backpressure)
    bool enqueue(std::function<void()> task);

    // Get current queue depth (for monitoring)
    size_t queue_size() const;

    // Get number of worker threads
    size_t worker_count() const { return workers_.size(); }

private:
    // Worker threads
    std::vector<std::thread> workers_;

    // Task queue
    std::queue<std::function<void()>> tasks_;
    size_t max_queue_size_;

    // Synchronization
    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;

    // Shutdown flag
    std::atomic<bool> stop_;

    // Worker function - runs in each thread
    void worker_loop();
};

} // namespace metricstream
