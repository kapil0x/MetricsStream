#include "thread_pool.h"
#include <iostream>

namespace metricstream {

ThreadPool::ThreadPool(size_t num_threads, size_t max_queue_size)
    : max_queue_size_(max_queue_size), stop_(false) {

    // Pre-create all worker threads
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }

    std::cerr << "[ThreadPool] Started with " << num_threads
              << " workers, max queue size: " << max_queue_size << std::endl;
}

ThreadPool::~ThreadPool() {
    // Signal shutdown
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        stop_.store(true);
    }

    // Wake up all workers
    condition_.notify_all();

    // Wait for all workers to finish
    for (std::thread& worker : workers_) {
        if (worker.joinable()) {
            worker.join();
        }
    }

    std::cerr << "[ThreadPool] Shutdown complete" << std::endl;
}

bool ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        // Check if queue is full (backpressure)
        if (tasks_.size() >= max_queue_size_) {
            return false; // Reject task, queue is full
        }

        // Check if shutting down
        if (stop_.load()) {
            return false;
        }

        tasks_.push(std::move(task));
    }

    // Notify one worker that a task is available
    condition_.notify_one();
    return true;
}

size_t ThreadPool::queue_size() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return tasks_.size();
}

void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            // Wait for a task or shutdown signal
            condition_.wait(lock, [this] {
                return stop_.load() || !tasks_.empty();
            });

            // If stopping and no tasks left, exit
            if (stop_.load() && tasks_.empty()) {
                return;
            }

            // Get task from queue
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        // Execute task outside the lock
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "[ThreadPool] Task threw exception: "
                          << e.what() << std::endl;
            } catch (...) {
                std::cerr << "[ThreadPool] Task threw unknown exception"
                          << std::endl;
            }
        }
    }
}

} // namespace metricstream
