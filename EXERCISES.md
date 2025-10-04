# MetricsStream Exercises

Hands-on exercises for each optimization phase, with starter code, hints, and multiple solution approaches.

## How to Use This Guide

Each exercise includes:
- **Difficulty**: ⭐ (Beginner) to ⭐⭐⭐⭐⭐ (Expert)
- **Time Estimate**: Expected completion time
- **Starter Code**: Branch or commit to start from
- **Success Criteria**: How to verify your solution
- **Hints**: Progressive hints if you get stuck
- **Solutions**: Multiple approaches with tradeoffs

---

## Phase 1: Concurrency Basics

### Exercise 1.1: Thread-Per-Request

**Difficulty**: ⭐⭐
**Time**: 3-4 hours
**Starter Branch**: `phase-1-baseline` (to be created)

#### Problem Statement

The current server handles requests sequentially. While one request is being processed, all others must wait. Modify the server to handle multiple requests concurrently using threads.

#### Starter Code

Current sequential implementation:
```cpp
// src/http_server.cpp (baseline)
while (running_.load()) {
    int client_socket = accept(server_fd, ...);
    if (client_socket < 0) continue;

    // Sequential handling - blocks here!
    handle_client_request(client_socket);
}
```

#### Your Task

Modify to spawn a thread for each incoming connection.

#### Requirements

1. Spawn new thread for each accepted connection
2. Handle thread creation failures gracefully
3. Ensure socket is closed even if exception occurs
4. Detach threads (don't join in accept loop)

#### Success Criteria

```bash
# Before: ~100 RPS, 100% success at 10 clients
./load_test 8080 10 10

# After: Improved success rate at 20 clients
./load_test 8080 20 10
# Expected: ~88% success rate
```

#### Hints

<details>
<summary>Hint 1: Thread Creation</summary>

Use `std::thread` with lambda:
```cpp
std::thread([this, client_socket]() {
    // Handle request
}).detach();
```
</details>

<details>
<summary>Hint 2: RAII for Socket Cleanup</summary>

Use try-finally or RAII wrapper to ensure socket is closed:
```cpp
std::thread([this, client_socket]() {
    try {
        handle_client_request(client_socket);
    } catch (...) {
        close(client_socket);
        throw;
    }
    close(client_socket);
}).detach();
```

Or better, use a RAII wrapper:
```cpp
class SocketGuard {
    int fd_;
public:
    SocketGuard(int fd) : fd_(fd) {}
    ~SocketGuard() { if (fd_ >= 0) close(fd_); }
    // ... move constructor, etc.
};
```
</details>

<details>
<summary>Hint 3: Thread Creation Limits</summary>

Thread creation can fail at high concurrency. Consider:
```cpp
try {
    std::thread(...).detach();
} catch (const std::system_error& e) {
    // Log error, close socket, continue
}
```
</details>

#### Solution Approaches

**Approach 1: Simple Detach**
```cpp
std::thread([this, client_socket]() {
    handle_client_request(client_socket);
    close(client_socket);
}).detach();
```

**Pros**: Simple, each request fully independent
**Cons**: No thread reuse, unlimited thread creation

**Approach 2: With Error Handling**
```cpp
try {
    std::thread([this, client_socket]() {
        try {
            handle_client_request(client_socket);
        } catch (const std::exception& e) {
            std::cerr << "Request failed: " << e.what() << "\n";
        }
        close(client_socket);
    }).detach();
} catch (const std::system_error& e) {
    std::cerr << "Thread creation failed: " << e.what() << "\n";
    close(client_socket);
}
```

**Pros**: Production-ready error handling
**Cons**: More complex

#### Discussion Questions

1. What happens when you have 1000 concurrent clients?
2. What is the cost of thread creation?
3. What happens to file descriptors when threads leak?

---

### Exercise 1.2: Thread-Safe File Writing

**Difficulty**: ⭐⭐
**Time**: 2-3 hours
**Prerequisites**: Exercise 1.1

#### Problem Statement

With concurrent request handling, multiple threads try to write to `metrics.jsonl` simultaneously, causing corruption. Add thread-safety to file writes.

#### Starter Code

Current unsafe implementation:
```cpp
void store_metrics_to_file(const MetricBatch& batch) {
    // NO MUTEX! Multiple threads can execute this simultaneously
    for (const auto& metric : batch.metrics) {
        metrics_file_ << /* JSON */ << "\n";
    }
}
```

#### Your Task

Add mutex to protect file writes.

#### Requirements

1. Add `std::mutex` member variable
2. Use RAII (`std::lock_guard`) for exception safety
3. Minimize critical section (don't hold lock during JSON formatting)
4. Verify no corruption in output file

#### Success Criteria

```bash
./load_test 8080 50 10
# Check metrics.jsonl - should be valid JSON Lines
# Each line should parse as valid JSON
```

#### Hints

<details>
<summary>Hint 1: Mutex Declaration</summary>

```cpp
class IngestionService {
private:
    std::mutex file_mutex_;
    std::ofstream metrics_file_;
};
```
</details>

<details>
<summary>Hint 2: RAII Locking</summary>

```cpp
void store_metrics_to_file(const MetricBatch& batch) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    // Critical section - file writes protected
}
```
</details>

<details>
<summary>Hint 3: Minimize Critical Section</summary>

Format JSON string BEFORE acquiring lock:
```cpp
void store_metrics_to_file(const MetricBatch& batch) {
    // Format outside lock
    std::string json_lines = format_batch_as_jsonl(batch);

    // Lock only for actual write
    {
        std::lock_guard<std::mutex> lock(file_mutex_);
        metrics_file_ << json_lines;
        metrics_file_.flush();
    }
}
```
</details>

#### Solution Approaches

**Approach 1: Simple Lock**
```cpp
void store_metrics_to_file(const MetricBatch& batch) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    for (const auto& metric : batch.metrics) {
        metrics_file_ << format_metric_json(metric) << "\n";
    }
    metrics_file_.flush();
}
```

**Approach 2: Optimized (Lock After Formatting)**
```cpp
void store_metrics_to_file(const MetricBatch& batch) {
    std::ostringstream buffer;
    for (const auto& metric : batch.metrics) {
        buffer << format_metric_json(metric) << "\n";
    }

    std::lock_guard<std::mutex> lock(file_mutex_);
    metrics_file_ << buffer.str();
    metrics_file_.flush();
}
```

**Tradeoff**: Approach 2 holds lock for less time but uses more memory.

#### Discussion Questions

1. What happens if you forget `flush()`?
2. Could you use `std::unique_lock` instead? When would you?
3. What's the performance impact of the mutex?

---

## Phase 2: Asynchronous I/O

### Exercise 2.1: Producer-Consumer Queue

**Difficulty**: ⭐⭐⭐
**Time**: 6-8 hours
**Starter Branch**: `phase-2-async-io` (to be created)

#### Problem Statement

File I/O blocks request handler threads, limiting throughput. Implement a background writer thread that asynchronously writes batches to disk.

#### Starter Code

Current synchronous writes in request handler:
```cpp
HttpResponse IngestionService::handle_metrics_post(const HttpRequest& req) {
    // ... parse and validate ...

    store_metrics_to_file(batch);  // BLOCKS HERE!

    return success_response;
}
```

#### Your Task

Create producer-consumer pattern with background writer.

#### Architecture

```
[Request Handler Threads] → [Queue] → [Writer Thread] → [Disk]
      (producers)                      (consumer)
```

#### Requirements

1. Add queue data structure (`std::queue<MetricBatch>`)
2. Add synchronization primitives (mutex, condition variable)
3. Implement background writer thread
4. Modify request handler to enqueue instead of write
5. Handle graceful shutdown (flush queue on exit)

#### Success Criteria

```bash
./load_test 8080 50 10
# Expected: 59% → 66% success rate
# Latency should decrease (no I/O blocking)
```

#### Hints

<details>
<summary>Hint 1: Data Structures</summary>

```cpp
class IngestionService {
private:
    std::queue<MetricBatch> write_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread writer_thread_;
    std::atomic<bool> writer_running_{true};
};
```
</details>

<details>
<summary>Hint 2: Starting Writer Thread</summary>

In constructor:
```cpp
IngestionService::IngestionService(...) {
    writer_thread_ = std::thread(&IngestionService::async_writer_loop, this);
}
```
</details>

<details>
<summary>Hint 3: Writer Loop Structure</summary>

```cpp
void async_writer_loop() {
    while (writer_running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Wait for work or shutdown
        queue_cv_.wait(lock, [this] {
            return !write_queue_.empty() || !writer_running_;
        });

        // Process queue...
    }
}
```
</details>

<details>
<summary>Hint 4: Graceful Shutdown</summary>

In destructor:
```cpp
~IngestionService() {
    writer_running_ = false;
    queue_cv_.notify_all();  // Wake writer
    if (writer_thread_.joinable()) {
        writer_thread_.join();  // Wait for queue to drain
    }
}
```
</details>

#### Step-by-Step Guide

**Step 1**: Add member variables (5 min)
**Step 2**: Implement `queue_metrics_for_async_write()` (15 min)
```cpp
void queue_metrics_for_async_write(const MetricBatch& batch) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        write_queue_.push(batch);
    }
    queue_cv_.notify_one();
}
```

**Step 3**: Implement `async_writer_loop()` (2-3 hours)
**Step 4**: Update `handle_metrics_post()` to enqueue (5 min)
**Step 5**: Handle shutdown in destructor (1 hour)
**Step 6**: Test and debug (1-2 hours)

#### Complete Solution

```cpp
void IngestionService::async_writer_loop() {
    while (writer_running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        queue_cv_.wait(lock, [this] {
            return !write_queue_.empty() || !writer_running_;
        });

        while (!write_queue_.empty() && writer_running_) {
            MetricBatch batch = write_queue_.front();
            write_queue_.pop();

            // Release lock before expensive I/O
            lock.unlock();

            store_metrics_to_file(batch);

            // Reacquire for next iteration
            lock.lock();
        }
    }
}
```

#### Common Pitfalls

1. **Forgetting predicate in wait**: Use lambda, not just `wait(lock)`
2. **Not handling spurious wakeups**: Check condition in loop
3. **Holding lock during I/O**: Release before `store_metrics_to_file()`
4. **Forgetting to notify**: `notify_one()` after enqueue
5. **Not draining queue on shutdown**: Join thread in destructor

#### Discussion Questions

1. What happens if the queue grows unbounded?
2. Should you use `notify_one()` or `notify_all()`?
3. How would you add a bounded queue with backpressure?

---

## Phase 3: Algorithm Optimization

### Exercise 3.1: JSON Parser Optimization

**Difficulty**: ⭐⭐⭐⭐
**Time**: 8-10 hours
**Starter Branch**: `phase-3-json-optimized` (to be created)

#### Problem Statement

Current JSON parser uses multiple `find()` and `substr()` calls, resulting in O(n²) complexity. Implement single-pass state machine parser.

#### Current Implementation Analysis

```cpp
// Naive approach - O(n²)
std::string extract_string_field(const std::string& json, const std::string& field) {
    size_t field_pos = json.find("\"" + field + "\"");  // O(n)
    size_t colon_pos = json.find(":", field_pos);      // O(n)
    size_t quote_start = json.find("\"", colon_pos);   // O(n)
    size_t quote_end = json.find("\"", quote_start + 1); // O(n)
    return json.substr(quote_start + 1, ...);           // O(n) copy
}
// Called multiple times per metric!
```

**Complexity**: O(n²) - multiple passes over input

#### Your Task

Implement single-pass O(n) parser using state machine.

#### Requirements

1. Single character-by-character iteration
2. Pre-allocated buffers (no repeated allocations)
3. State machine for parsing different JSON structures
4. Direct numeric parsing with `strtod()`
5. Performance improvement: 50-100% faster

#### Success Criteria

```bash
./load_test 8080 100 10
# Expected: 80.2% success rate, 2.73ms latency
# Compare with naive parser (should be 50-100% faster)
```

#### State Machine Design

```
[LOOKING_FOR_METRICS] → "metrics" → ':'
                                      ↓
                                    '['
                                      ↓
                           [IN_METRICS_ARRAY] → '{'
                                                  ↓
                                      [IN_METRIC_OBJECT]
                                         ↓           ↓
                                   "name":"..."  "value":123
                                         ↓           ↓
                                     [Repeat]     [Repeat]
                                         ↓           ↓
                                       '}' → Add metric
                                         ↓
                                      Back to ARRAY
```

#### Hints

<details>
<summary>Hint 1: State Enum</summary>

```cpp
enum class ParseState {
    LOOKING_FOR_METRICS,
    IN_METRICS_ARRAY,
    IN_METRIC_OBJECT,
    PARSING_FIELD_NAME,
    PARSING_STRING_VALUE,
    PARSING_NUMBER_VALUE,
    IN_TAGS_OBJECT,
    DONE
};
```
</details>

<details>
<summary>Hint 2: Pre-Allocated Buffers</summary>

```cpp
std::string current_field;
std::string current_value;
current_field.reserve(32);   // Avoid repeated allocations
current_value.reserve(128);
```
</details>

<details>
<summary>Hint 3: Helper Functions</summary>

```cpp
auto skip_whitespace = [&]() {
    while (i < len && std::isspace(json_body[i])) i++;
};

auto parse_string = [&](std::string& result) {
    // Implementation...
    return success;
};

auto parse_number = [&]() -> double {
    // Use strtod() for direct parsing
};
```
</details>

<details>
<summary>Hint 4: State Machine Loop</summary>

```cpp
while (i < len && state != ParseState::DONE) {
    skip_whitespace();
    char c = json_body[i];

    switch (state) {
        case ParseState::IN_METRIC_OBJECT:
            if (c == '"') {
                // Parse field name...
            } else if (c == '}') {
                // End of metric object
                state = ParseState::IN_METRICS_ARRAY;
            }
            break;
        // ... other states
    }
}
```
</details>

#### Step-by-Step Guide

**Step 1**: Design state machine diagram (1 hour)
**Step 2**: Implement state enum and main loop (1 hour)
**Step 3**: Implement helper functions (2 hours)
- `skip_whitespace()`
- `parse_string()`
- `parse_number()`

**Step 4**: Implement each state transition (3-4 hours)
**Step 5**: Test with sample JSON (1 hour)
**Step 6**: Performance testing and optimization (1 hour)

#### Testing Strategy

```cpp
// Unit test each helper
TEST(JsonParser, ParseString) {
    std::string json = "\"hello world\"";
    std::string result;
    size_t i = 0;
    ASSERT_TRUE(parse_string(json, i, result));
    ASSERT_EQ(result, "hello world");
}

// Integration test
TEST(JsonParser, FullMetric) {
    std::string json = R"({"metrics":[{"name":"cpu","value":75.5}]})";
    MetricBatch batch = parse_json_metrics_optimized(json);
    ASSERT_EQ(batch.size(), 1);
    ASSERT_EQ(batch.metrics[0].name, "cpu");
    ASSERT_EQ(batch.metrics[0].value, 75.5);
}
```

#### Performance Comparison

```bash
# Benchmark both parsers
./benchmark_json_parser

# Expected results:
# Naive parser:     ~1000 parses/sec
# Optimized parser: ~2000+ parses/sec
```

#### Discussion Questions

1. Why is a state machine more efficient than multiple `find()` calls?
2. How does pre-allocation help performance?
3. Could you use a JSON library? What are the tradeoffs?

---

## Phase 4: Advanced Locking

### Exercise 4.1: Rate Limiter Implementation

**Difficulty**: ⭐⭐⭐
**Time**: 4-5 hours
**Starter Branch**: `phase-4-mutex-pool` (to be created)

#### Problem Statement

Implement sliding window rate limiting: maximum N requests per client per second.

#### Algorithm

```
For each request:
1. Remove timestamps older than 1 second from client's queue
2. If queue.size() < limit:
     - Add current timestamp
     - Return ALLOW
3. Else:
     - Return DENY (rate limited)
```

#### Your Task

Implement `RateLimiter::allow_request(client_id)`.

#### Requirements

1. Per-client rate limiting
2. Sliding window (not fixed window)
3. Thread-safe
4. Efficient (minimize lock contention)

#### Success Criteria

```bash
# Test rate limiting works
for i in {1..15}; do
  curl -H "Authorization: client1" http://localhost:8080/metrics -d '{...}'
done
# With limit=10, should see 10 successes, 5 × 429 responses
```

#### Hints

<details>
<summary>Hint 1: Data Structure</summary>

```cpp
class RateLimiter {
private:
    size_t max_requests_;
    std::mutex global_mutex_;  // Start simple
    std::unordered_map<std::string,
        std::deque<std::chrono::time_point<std::chrono::steady_clock>>>
        client_requests_;
};
```
</details>

<details>
<summary>Hint 2: Implementation</summary>

```cpp
bool allow_request(const std::string& client_id) {
    auto now = std::chrono::steady_clock::now();

    std::lock_guard<std::mutex> lock(global_mutex_);
    auto& queue = client_requests_[client_id];

    // Remove old timestamps
    while (!queue.empty() &&
           (now - queue.front()) >= std::chrono::seconds(1)) {
        queue.pop_front();
    }

    // Check limit
    if (queue.size() < max_requests_) {
        queue.push_back(now);
        return true;
    }
    return false;
}
```
</details>

#### Discussion Questions

1. Why use `steady_clock` instead of `system_clock`?
2. What's the space complexity per client?
3. How would you implement a token bucket instead?

---

### Exercise 4.2: Solving the Deadlock Problem

**Difficulty**: ⭐⭐⭐⭐
**Time**: 6-8 hours
**Prerequisites**: Exercise 4.1

#### Problem Statement

Need to iterate all clients and flush their metrics to monitoring. With per-client locks, this risks deadlock.

#### Deadlock Scenario

```
Thread 1 (flush_metrics):
  - Acquires lock for client A
  - Tries to acquire lock for client B

Thread 2 (allow_request for B):
  - Acquires lock for client B
  - Thread 1 is waiting...

Thread 3 (allow_request for A):
  - Tries to acquire lock for A
  - Thread 1 holds it...

→ Potential deadlock if lock ordering is wrong!
```

#### Your Task

Implement safe `flush_metrics()` that doesn't deadlock.

#### Solution Approaches

**Approach 1: Lock Ordering** (⭐⭐⭐)
Sort client IDs and acquire locks in order.

**Approach 2: Try-Lock Pattern** (⭐⭐⭐⭐)
Use `try_lock()` and skip if can't acquire.

**Approach 3: Hash-Based Mutex Pool** (⭐⭐⭐)
Fixed pool eliminates ordering problem.

**Approach 4: Lock-Free Ring Buffer** (⭐⭐⭐⭐⭐)
No locks in metrics collection (Phase 5).

#### Exercise: Implement Approach 1 (Lock Ordering)

```cpp
void flush_metrics() {
    // Step 1: Get sorted list of clients
    std::vector<std::pair<size_t, std::string>> client_hashes;
    {
        std::lock_guard<std::mutex> lock(global_mutex_);
        for (const auto& [client_id, _] : client_metrics_) {
            size_t hash = std::hash<std::string>{}(client_id);
            client_hashes.emplace_back(hash, client_id);
        }
    }
    std::sort(client_hashes.begin(), client_hashes.end());

    // Step 2: Acquire locks in sorted order
    std::vector<std::unique_lock<std::mutex>> locks;
    for (const auto& [hash, client_id] : client_hashes) {
        locks.emplace_back(get_client_mutex(client_id));
    }

    // Step 3: Safely iterate and flush
    for (const auto& [hash, client_id] : client_hashes) {
        auto& metrics = client_metrics_[client_id];
        send_to_monitoring(client_id, metrics);
    }
}
```

**Pros**: Guarantees no deadlock
**Cons**: Holds all locks simultaneously (contention)

#### Exercise: Implement Approach 3 (Hash-Based Pool)

```cpp
class RateLimiter {
private:
    static constexpr size_t MUTEX_POOL_SIZE = 10007;  // Prime
    std::array<std::mutex, MUTEX_POOL_SIZE> mutex_pool_;

    std::mutex& get_client_mutex(const std::string& client_id) {
        size_t hash = std::hash<std::string>{}(client_id);
        return mutex_pool_[hash % MUTEX_POOL_SIZE];
    }
};
```

**Pros**: Simple, no complex lock ordering
**Cons**: Hash collisions cause false sharing

#### Discussion Questions

1. Why use a prime number for pool size?
2. What's the tradeoff between pool size and contention?
3. Could two different clients hash to the same mutex?

---

## Phase 5: Lock-Free Programming

### Exercise 5.1: Lock-Free Ring Buffer

**Difficulty**: ⭐⭐⭐⭐⭐
**Time**: 10-12 hours
**Prerequisites**: Strong understanding of concurrency and memory ordering

#### Problem Statement

Eliminate mutex from hot path (metrics collection) using lock-free ring buffer with atomic indices.

#### Architecture

```
Writer (allow_request):               Reader (flush_metrics):
1. Load write_index (relaxed)         1. Load read_index (acquire)
2. Write to ring_buffer[idx]          2. Load write_index (acquire)
3. Store write_index+1 (release)      3. Read ring_buffer[read...write]
                                      4. Store read_index (release)
```

#### Memory Ordering Explained

**`memory_order_relaxed`**: No synchronization, just atomicity
- Use when other operations don't depend on ordering
- Write index load: we'll publish with release anyway

**`memory_order_release`**: All prior writes visible to acquirer
- Write index store: ensures buffer write visible before index update

**`memory_order_acquire`**: See all writes before corresponding release
- Read index load: see all writes from previous flush
- Write index load: see all buffer writes from writer

#### Your Task

Implement lock-free `ClientMetrics` and update `allow_request()` / `flush_metrics()`.

#### Requirements

1. Single-writer / single-reader pattern
2. Proper memory ordering (no data races)
3. Handle buffer overflow (wrap around)
4. Verify correctness under load

#### Step-by-Step Guide

**Step 1**: Define data structure
```cpp
struct MetricEvent {
    std::chrono::time_point<std::chrono::steady_clock> timestamp;
    bool allowed;
};

struct ClientMetrics {
    static constexpr size_t BUFFER_SIZE = 1000;
    std::array<MetricEvent, BUFFER_SIZE> ring_buffer;
    std::atomic<size_t> write_index{0};
    std::atomic<size_t> read_index{0};
};
```

**Step 2**: Update writer (allow_request)
```cpp
bool allow_request(const std::string& client_id) {
    // ... rate limiting logic (with mutex) ...

    // LOCK-FREE metrics collection
    auto& metrics = client_metrics_[client_id];
    size_t write_idx = metrics.write_index.load(std::memory_order_relaxed);

    // Write event
    metrics.ring_buffer[write_idx % ClientMetrics::BUFFER_SIZE] =
        MetricEvent{now, decision};

    // Publish with release semantics
    metrics.write_index.store(write_idx + 1, std::memory_order_release);

    return decision;
}
```

**Step 3**: Update reader (flush_metrics)
```cpp
void flush_metrics() {
    // Snapshot clients (minimal locking)
    std::vector<std::string> client_ids;
    {
        std::lock_guard<std::mutex> lock(client_list_mutex);
        for (const auto& [client_id, _] : client_metrics_) {
            client_ids.push_back(client_id);
        }
    }

    // Process each client lock-free
    for (const auto& client_id : client_ids) {
        auto it = client_metrics_.find(client_id);
        if (it == client_metrics_.end()) continue;

        auto& metrics = it->second;

        // Load indices with acquire semantics
        size_t read_idx = metrics.read_index.load(std::memory_order_acquire);
        size_t write_idx = metrics.write_index.load(std::memory_order_acquire);

        // Process events
        for (size_t i = read_idx; i < write_idx; ++i) {
            const auto& event = metrics.ring_buffer[i % ClientMetrics::BUFFER_SIZE];
            send_to_monitoring(client_id, event.timestamp, event.allowed);
        }

        // Update read index with release
        if (write_idx > read_idx) {
            metrics.read_index.store(write_idx, std::memory_order_release);
        }
    }
}
```

#### Testing for Correctness

```cpp
// Stress test
void stress_test() {
    RateLimiter limiter(1000);

    // Writer threads
    std::vector<std::thread> writers;
    for (int i = 0; i < 100; ++i) {
        writers.emplace_back([&limiter, i]() {
            for (int j = 0; j < 1000; ++j) {
                limiter.allow_request("client" + std::to_string(i));
            }
        });
    }

    // Reader thread
    std::atomic<int> total_events{0};
    std::thread reader([&limiter, &total_events]() {
        for (int i = 0; i < 100; ++i) {
            // Count events in flush
            total_events += count_flushed_events();
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    });

    // Join all
    for (auto& t : writers) t.join();
    reader.join();

    // Verify: total_events should equal 100 * 1000
    ASSERT_EQ(total_events, 100000);
}
```

#### Common Bugs

1. **Wrong memory ordering**: Using `relaxed` everywhere → lost updates
2. **Reading wrong index**: Not loading `write_index` with `acquire`
3. **Buffer overflow**: Not using modulo for ring buffer index
4. **Lost events**: Not checking `read_idx < write_idx` before updating

#### Discussion Questions

1. Why is `release` needed on write_index store?
2. What happens if you use `relaxed` everywhere?
3. How does the ring buffer handle overflow?
4. Could you have multiple readers? What would break?

---

## Phase 6: Resource Management

### Exercise 6.1: Thread Pool Implementation

**Difficulty**: ⭐⭐⭐⭐
**Time**: 8-10 hours
**Starter Branch**: `phase-6-thread-pool` (to be created)

#### Problem Statement

Thread creation overhead limits scalability. Implement fixed thread pool with bounded task queue.

#### Architecture

```
[Accept Loop] → enqueue(task) → [Bounded Queue] → [Worker Threads]
                                     ↓
                              Return false if full
                              (backpressure)
```

#### Your Task

Implement `ThreadPool` class that replaces thread-per-request.

#### Requirements

1. Fixed number of worker threads (default 16)
2. Bounded task queue (default 10,000 tasks)
3. Return `false` if queue is full (backpressure)
4. Graceful shutdown (finish queued tasks)

#### Interface

```cpp
class ThreadPool {
public:
    ThreadPool(size_t num_threads = 16, size_t max_queue_size = 10000);
    ~ThreadPool();

    bool enqueue(std::function<void()> task);
    size_t queue_size() const;

private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    size_t max_queue_size_;

    mutable std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;

    void worker_loop();
};
```

#### Step-by-Step Implementation

**Step 1**: Constructor - Create workers
```cpp
ThreadPool::ThreadPool(size_t num_threads, size_t max_queue_size)
    : max_queue_size_(max_queue_size), stop_(false) {

    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}
```

**Step 2**: Worker loop
```cpp
void ThreadPool::worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(queue_mutex_);

            condition_.wait(lock, [this] {
                return stop_ || !tasks_.empty();
            });

            if (stop_ && tasks_.empty()) {
                return;  // Shutdown
            }

            task = std::move(tasks_.front());
            tasks_.pop();
        }

        task();  // Execute outside lock
    }
}
```

**Step 3**: Enqueue with backpressure
```cpp
bool ThreadPool::enqueue(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        if (tasks_.size() >= max_queue_size_) {
            return false;  // Queue full - backpressure!
        }

        tasks_.push(std::move(task));
    }
    condition_.notify_one();
    return true;
}
```

**Step 4**: Graceful shutdown
```cpp
ThreadPool::~ThreadPool() {
    stop_ = true;
    condition_.notify_all();  // Wake all workers

    for (auto& worker : workers_) {
        if (worker.joinable()) {
            worker.join();  // Wait for completion
        }
    }
}
```

**Step 5**: Integrate with HttpServer
```cpp
// src/http_server.cpp
bool enqueued = thread_pool_->enqueue([this, client_socket]() {
    handle_client_request(client_socket);
    close(client_socket);
});

if (!enqueued) {
    // Queue full - return 503
    const char* response =
        "HTTP/1.1 503 Service Unavailable\r\n"
        "Content-Type: application/json\r\n"
        "\r\n"
        "{\"error\":\"Server overloaded\"}";
    write(client_socket, response, strlen(response));
    close(client_socket);
}
```

#### Testing

```bash
# Test normal load
./load_test 8080 100 10
# Should handle gracefully

# Test overload (trigger backpressure)
./load_test 8080 500 20
# Should see some 503 responses

# Test shutdown
# Start server, send requests, ctrl+C
# Verify: no lost requests, clean shutdown
```

#### Performance Comparison

```
Thread-per-request:
- 150 clients: Thread creation fails
- Resource exhaustion

Thread pool:
- 150 clients: 503 responses when queue full
- Graceful degradation
- Predictable resource usage
```

#### Discussion Questions

1. How did you choose the number of worker threads?
2. What happens if tasks throw exceptions?
3. Should you use `notify_one()` or `notify_all()` in enqueue?
4. How would you add task priorities?

---

## Bonus Exercises

### Bonus 1: Performance Monitoring (TODO #1)

**Difficulty**: ⭐⭐
**Time**: 6-8 hours

Implement real-time performance monitoring for production observability.

#### Requirements

1. Track active connection count
2. Monitor request latency distribution (p50, p95, p99)
3. Track queue depths (write queue, thread pool queue)
4. Monitor memory usage
5. Log statistics every N seconds

#### Starter Code

```cpp
// src/main.cpp:36
while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // TODO(human): Add performance monitoring here
}
```

---

### Bonus 2: Distributed Tracing Integration

**Difficulty**: ⭐⭐⭐⭐
**Time**: 12-15 hours

Integrate OpenTelemetry for distributed tracing.

---

### Bonus 3: Query API

**Difficulty**: ⭐⭐⭐⭐
**Time**: 15-20 hours

Implement HTTP endpoint for querying stored metrics with aggregation.

---

## Assessment Rubric

### Correctness (40%)
- [ ] Passes all unit tests
- [ ] No data races (verified with ThreadSanitizer)
- [ ] No deadlocks under load
- [ ] Proper error handling

### Performance (30%)
- [ ] Meets target benchmarks
- [ ] Efficient resource usage
- [ ] Scales with concurrency

### Code Quality (20%)
- [ ] Clean, readable code
- [ ] Appropriate comments
- [ ] RAII for resource management
- [ ] Const correctness

### Understanding (10%)
- [ ] Can explain design decisions
- [ ] Understands tradeoffs
- [ ] Can identify next bottleneck

---

## Tools and Testing

### Build with Sanitizers

```bash
# Thread sanitizer (detect data races)
cmake -DCMAKE_CXX_FLAGS="-fsanitize=thread" ..
make
./metricstream_server

# Address sanitizer (detect memory errors)
cmake -DCMAKE_CXX_FLAGS="-fsanitize=address" ..
```

### Load Testing

```bash
# Progressive load test
for clients in 10 20 50 100 150 200; do
    ./load_test 8080 $clients 10
    sleep 5
done
```

### Profiling

```bash
# CPU profiling with perf
perf record -g ./metricstream_server &
./load_test 8080 100 10
perf report

# Flamegraph
perf script | stackcollapse-perf.pl | flamegraph.pl > flamegraph.svg
```

---

**Generated**: 2025-10-04
**Repository State**: commit 9dd194d

Happy optimizing! 🚀
