# MetricsStream Learning Path
## Structured Journey from Fundamentals to Principal Engineer Level

This document provides a comprehensive learning path through MetricsStream, organized into 15 chapters with clear progression from basics to advanced systems programming.

---

## Learning Philosophy

**Build. Measure. Optimize. Repeat.**

Every chapter follows this cycle:
1. **Understand the problem** - What bottleneck exists?
2. **Implement a solution** - Write code to solve it
3. **Measure the impact** - Use load_test to verify
4. **Document learnings** - Record insights

**Key Principle**: Never optimize without measuring first!

---

## Prerequisites

### Required Knowledge
- C++ basics (classes, pointers, references)
- Understanding of compilation and linking
- Basic command-line proficiency

### Recommended Background
- Operating systems concepts (processes, threads)
- Network programming basics (HTTP, TCP)
- Data structures and algorithms

### Tools Needed
```bash
# Install on Ubuntu/Debian
sudo apt install build-essential cmake git perf valgrind

# Verify installation
g++ --version
cmake --version
```

---

## Learning Paths by Goal

### Path A: Full Stack Developer → Systems Engineer
**Duration**: 8-12 weeks
**Focus**: Fundamentals + Core optimizations

Chapters: 1 → 2 → 3 → 4 → 5 → 6 → 8 → 13

### Path B: Backend Engineer → Performance Specialist
**Duration**: 6-8 weeks
**Focus**: Performance and concurrency

Chapters: 1 → 4 → 6 → 8 → 9 → 10 → 11 → 13

### Path C: Systems Programmer → Principal Engineer
**Duration**: 12-16 weeks
**Focus**: Complete mastery

All chapters in order: 1 → 15

---

## Chapter Breakdown

---

## 📚 Chapter 1: HTTP Server Basics
**Duration**: 3-5 days
**Difficulty**: ⭐⭐☆☆☆

### Learning Objectives
- Understand raw socket programming
- Implement TCP server from scratch
- Parse HTTP protocol manually
- Handle basic request/response cycle

### TODOs in This Chapter
- TODO #1: Socket creation and binding
- TODO #2: Accept loop implementation
- TODO #3: HTTP request parser

### Prerequisites
- C++ basics
- Understanding of client-server model
- Basic networking concepts

### Implementation Guide

**Step 1: Create Socket** (2 hours)
```cpp
// src/http_server.cpp:48-73
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
struct sockaddr_in address;
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY;
address.sin_port = htons(port);
bind(server_fd, ...);
listen(server_fd, 10);
```

**Step 2: Accept Connections** (3 hours)
```cpp
while (running) {
    int client_socket = accept(server_fd, ...);
    // For now: handle in same thread (blocking)
    handle_request(client_socket);
    close(client_socket);
}
```

**Step 3: Parse HTTP** (4 hours)
```cpp
// Read from socket
char buffer[4096];
read(client_socket, buffer, sizeof(buffer));

// Parse: "GET /path HTTP/1.1\r\n"
// Extract method, path, headers, body
```

### Testing
```bash
mkdir build && cd build
cmake .. && make

# Terminal 1
./metricstream_server

# Terminal 2
curl http://localhost:8080/health
```

### Success Criteria
- ✅ Server accepts connections
- ✅ Parses GET/POST requests
- ✅ Returns valid HTTP responses
- ✅ Handles 1 request at a time

### Learning Outcomes
- Socket API mastery (socket, bind, listen, accept)
- HTTP protocol understanding
- Buffer management
- Error handling patterns

**Next Chapter**: Request routing →

---

## 📚 Chapter 2: Request Routing & Handlers
**Duration**: 2-3 days
**Difficulty**: ⭐⭐☆☆☆

### Learning Objectives
- Implement handler registration system
- Route requests by path and method
- Return appropriate HTTP status codes
- Use std::function for callbacks

### TODOs in This Chapter
- TODO #4: Handler registration
- TODO #5: Request dispatching

### Implementation Guide

**Step 1: Handler Storage** (2 hours)
```cpp
std::unordered_map<
    std::string,  // path
    std::unordered_map<
        std::string,  // method
        HttpHandler   // function<HttpResponse(HttpRequest)>
    >
> handlers_;
```

**Step 2: Registration** (1 hour)
```cpp
void add_handler(string path, string method, HttpHandler handler) {
    handlers_[path][method] = std::move(handler);
}
```

**Step 3: Dispatching** (3 hours)
```cpp
HttpResponse handle_request(const HttpRequest& req) {
    auto path_it = handlers_.find(req.path);
    if (path_it == handlers_.end()) {
        return error_404();
    }

    auto method_it = path_it->second.find(req.method);
    if (method_it == path_it->second.end()) {
        return error_405();
    }

    return method_it->second(req);  // Call handler
}
```

### Testing
```bash
# Test different endpoints
curl http://localhost:8080/health          # 200 OK
curl http://localhost:8080/metrics         # 200 OK
curl http://localhost:8080/nonexistent     # 404 Not Found
curl -X DELETE http://localhost:8080/health # 405 Method Not Allowed
```

### Success Criteria
- ✅ Multiple endpoints supported
- ✅ Method-specific routing works
- ✅ Correct status codes returned
- ✅ Lambda handlers work

### Learning Outcomes
- Nested map data structures
- std::function usage
- HTTP status code semantics
- Move semantics for handlers

**Next Chapter**: Metrics validation →

---

## 📚 Chapter 3: Metrics Validation
**Duration**: 2-3 days
**Difficulty**: ⭐⭐☆☆☆

### Learning Objectives
- Input validation patterns
- Error message construction
- Edge case handling
- Validation result structs

### TODOs in This Chapter
- TODO #6: Single metric validation
- TODO #7: Batch validation

### Implementation Guide

**Step 1: Validation Result** (1 hour)
```cpp
struct ValidationResult {
    bool valid;
    string error_message;
};
```

**Step 2: Metric Validation** (2 hours)
```cpp
ValidationResult validate_metric(const Metric& m) {
    if (m.name.empty()) {
        return {false, "Metric name cannot be empty"};
    }
    if (m.name.length() > 255) {
        return {false, "Metric name too long"};
    }
    if (std::isnan(m.value) || std::isinf(m.value)) {
        return {false, "Value must be finite"};
    }
    return {true, ""};
}
```

**Step 3: Batch Validation** (3 hours)
```cpp
ValidationResult validate_batch(const MetricBatch& batch) {
    if (batch.empty()) {
        return {false, "Batch cannot be empty"};
    }
    if (batch.size() > 1000) {
        return {false, "Batch too large"};
    }

    for (const auto& metric : batch.metrics) {
        auto result = validate_metric(metric);
        if (!result.valid) {
            return result;  // First error
        }
    }

    return {true, ""};
}
```

### Testing
```bash
# Valid metric
curl -X POST http://localhost:8080/metrics \
  -d '{"metrics":[{"name":"cpu","value":75.5}]}'

# Invalid: empty name
curl -X POST http://localhost:8080/metrics \
  -d '{"metrics":[{"name":"","value":75.5}]}'

# Invalid: NaN value
curl -X POST http://localhost:8080/metrics \
  -d '{"metrics":[{"name":"cpu","value":NaN}]}'
```

### Success Criteria
- ✅ Valid metrics accepted
- ✅ Invalid metrics rejected with clear errors
- ✅ Edge cases handled (NaN, Inf, empty)
- ✅ Batch size limits enforced

### Learning Outcomes
- Input sanitization
- Defensive programming
- Error message design
- Edge case enumeration

**Next Chapter**: Rate limiting →

---

## 📚 Chapter 4: Rate Limiting Fundamentals
**Duration**: 4-6 days
**Difficulty**: ⭐⭐⭐⭐☆

### Learning Objectives
- Implement sliding window algorithm
- Per-client rate limiting
- Thread-safe data structures
- Mutex usage patterns

### TODOs in This Chapter
- TODO #8: Sliding window rate limiting ⭐ (PRIMARY EXERCISE)

### Background Reading
- Token bucket vs sliding window
- Time-based eviction
- Deque for sliding window

### Implementation Guide

**Step 1: Data Structure** (2 hours)
```cpp
class RateLimiter {
    size_t max_requests_;
    std::mutex mutex_;
    std::unordered_map<
        std::string,  // client_id
        std::deque<time_point>  // request timestamps
    > client_requests_;
};
```

**Step 2: Cleanup Old Timestamps** (3 hours)
```cpp
auto now = steady_clock::now();
auto& queue = client_requests_[client_id];

// Remove timestamps older than 1 second
while (!queue.empty()) {
    auto oldest = queue.front();
    auto age = duration_cast<seconds>(now - oldest);

    if (age.count() >= 1) {
        queue.pop_front();
    } else {
        break;  // Rest are newer
    }
}
```

**Step 3: Allow/Deny Decision** (2 hours)
```cpp
if (queue.size() < max_requests_) {
    queue.push_back(now);
    return true;  // Allowed
} else {
    return false;  // Rate limited
}
```

**Step 4: Thread Safety** (3 hours)
```cpp
bool allow_request(const string& client_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    // Critical section:
    // 1. Cleanup old timestamps
    // 2. Check limit
    // 3. Add new timestamp

    return decision;
}
```

### Testing
```bash
# Test rate limit (10 req/sec)
for i in {1..15}; do
    curl -H "Authorization: client1" \
         http://localhost:8080/metrics \
         -d '{"metrics":[{"name":"test","value":1}]}'
    echo "Request $i"
done

# Expect: First 10 succeed, next 5 fail with 429
```

### Load Testing
```bash
# 50 clients, 10 requests each
./build/load_test 8080 50 10

# Verify rate limiting works across clients
```

### Success Criteria
- ✅ Per-client limits enforced
- ✅ Sliding window works correctly
- ✅ Thread-safe under concurrent load
- ✅ Returns 429 when limited

### Common Pitfalls
- ❌ Not removing old timestamps (memory leak)
- ❌ Using system_clock instead of steady_clock
- ❌ Forgetting mutex (race conditions)
- ❌ Evicting too aggressively

### Learning Outcomes
- Sliding window algorithm
- Time-based eviction
- Mutex-based synchronization
- Client isolation

**Next Chapter**: JSON parsing (naive) →

---

## 📚 Chapter 5: JSON Parsing - Naive Implementation
**Duration**: 2-3 days
**Difficulty**: ⭐⭐☆☆☆

### Learning Objectives
- Parse JSON manually (no libraries)
- String manipulation techniques
- Field extraction
- Understand O(n²) complexity

### TODOs in This Chapter
- TODO #9: Naive JSON parser

### Implementation Guide

**Step 1: Find Metrics Array** (2 hours)
```cpp
MetricBatch parse_json_metrics(const string& json) {
    size_t metrics_pos = json.find("\"metrics\"");
    size_t array_start = json.find("[", metrics_pos);
    size_t array_end = json.find("]", array_start);

    string metrics_array = json.substr(
        array_start + 1,
        array_end - array_start - 1
    );
}
```

**Step 2: Extract Objects** (3 hours)
```cpp
size_t pos = 0;
while (pos < metrics_array.length()) {
    size_t obj_start = metrics_array.find("{", pos);
    size_t obj_end = metrics_array.find("}", obj_start);

    string metric_obj = metrics_array.substr(
        obj_start,
        obj_end - obj_start + 1
    );

    Metric m = parse_single_metric(metric_obj);
    batch.add_metric(m);

    pos = obj_end + 1;
}
```

**Step 3: Extract Fields** (3 hours)
```cpp
string extract_string_field(const string& json, const string& field) {
    size_t field_pos = json.find("\"" + field + "\"");
    size_t colon = json.find(":", field_pos);
    size_t quote_start = json.find("\"", colon);
    size_t quote_end = json.find("\"", quote_start + 1);

    return json.substr(
        quote_start + 1,
        quote_end - quote_start - 1
    );
}
```

### Testing
```bash
# Simple metric
curl -X POST http://localhost:8080/metrics \
  -d '{"metrics":[{"name":"cpu_usage","value":75.5,"type":"gauge"}]}'

# Multiple metrics
curl -X POST http://localhost:8080/metrics \
  -d '{"metrics":[{"name":"cpu","value":75},{"name":"mem","value":80}]}'
```

### Performance Baseline
```bash
./build/load_test 8080 50 10
# Record: Success rate, latency
# This will be SLOW (O(n²))
```

### Success Criteria
- ✅ Parses valid JSON correctly
- ✅ Extracts all fields
- ✅ Handles multiple metrics
- ✅ Baseline performance measured

### Complexity Analysis
```
find("metrics")      -> O(n)
find("[")           -> O(n)
Loop: find("{")     -> O(n) * m = O(n*m)
  find("name")      -> O(n)
  find("value")     -> O(n)
  ...
Total: O(n²) or worse
```

### Learning Outcomes
- String searching algorithms
- Complexity analysis
- Baseline establishment
- Why naive approaches fail at scale

**Next Chapter**: JSON optimization →

---

## 📚 Chapter 6: JSON Parsing Optimization (Phase 3)
**Duration**: 5-7 days
**Difficulty**: ⭐⭐⭐⭐⭐

### Learning Objectives
- State machine design
- Single-pass algorithms
- Memory allocation optimization
- Performance profiling

### TODOs in This Chapter
- TODO #10: Single-pass JSON parser ⭐ (MAJOR OPTIMIZATION)

### Background Reading
- Finite state machines
- String allocation costs
- strtod() vs stod()

### Implementation Guide

**Step 1: Define States** (2 hours)
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

**Step 2: Pre-Allocate Buffers** (1 hour)
```cpp
// Avoid allocations in loop
string current_field;
current_field.reserve(32);

string current_value;
current_value.reserve(128);

string metric_name;
metric_name.reserve(64);
```

**Step 3: Single-Pass Iteration** (6 hours)
```cpp
ParseState state = LOOKING_FOR_METRICS;
size_t i = 0;

while (i < json.length()) {
    char c = json[i];

    switch (state) {
        case LOOKING_FOR_METRICS:
            if (c == '"') {
                if (parse_string(current_field) &&
                    current_field == "metrics") {
                    // Found it! Move to array
                    state = IN_METRICS_ARRAY;
                }
            }
            break;

        case IN_METRICS_ARRAY:
            if (c == '{') {
                state = IN_METRIC_OBJECT;
            }
            break;

        // ... more states
    }
}
```

**Step 4: Direct Numeric Parsing** (2 hours)
```cpp
auto parse_number = [&]() -> double {
    size_t start = i;
    while (isdigit(json[i]) || json[i] == '.' || json[i] == '-') {
        i++;
    }

    const char* start_ptr = json.data() + start;
    const char* end_ptr = json.data() + i;

    return std::strtod(start_ptr, &end_ptr);  // Fast!
};
```

### Optimization Checklist
- ✅ No string::find() in loops
- ✅ Single character iteration
- ✅ Pre-allocated buffers
- ✅ Direct strtod() for numbers
- ✅ Minimal string allocations

### Performance Testing
```bash
# Before optimization
./build/load_test 8080 100 10
# Record: ~60% success, high latency

# After optimization
./build/load_test 8080 100 10
# Target: 80%+ success, lower latency
```

### Performance Analysis
```bash
# Profile CPU usage
perf record -g ./metricstream_server
# In another terminal:
./build/load_test 8080 100 10
# Stop server

perf report --stdio | grep parse
```

### Success Criteria
- ✅ 100 clients: 80%+ success rate
- ✅ Latency < 3ms average
- ✅ No string::find() in hotpath
- ✅ Memory allocations minimized

### Complexity Analysis
```
Before: O(n²) - multiple find() loops
After:  O(n)  - single character iteration

Performance gain: 2-3x throughput
```

### Learning Outcomes
- State machine design patterns
- Algorithm complexity in practice
- Memory allocation impact
- Performance profiling techniques

**Next Chapter**: File I/O (sync) →

---

## 📚 Chapter 7: File I/O - Synchronous Version
**Duration**: 2-3 days
**Difficulty**: ⭐⭐☆☆☆

### Learning Objectives
- File handling in C++
- JSON Lines format
- Thread-safe file writes
- Identify I/O bottleneck

### TODOs in This Chapter
- TODO #11: Synchronous file writer

### Implementation Guide

**Step 1: Open File** (1 hour)
```cpp
std::ofstream metrics_file_;
metrics_file_.open("metrics.jsonl", std::ios::app);
```

**Step 2: Format JSON Line** (3 hours)
```cpp
void store_metrics_to_file(const MetricBatch& batch) {
    for (const auto& metric : batch.metrics) {
        // Timestamp
        auto ms = duration_cast<milliseconds>(
            metric.timestamp.time_since_epoch()
        );

        // Write JSON line
        metrics_file_
            << "{\"timestamp\":" << ms.count()
            << ",\"name\":\"" << metric.name << "\""
            << ",\"value\":" << metric.value
            << "}\n";
    }

    metrics_file_.flush();  // BLOCKS HERE!
}
```

**Step 3: Thread Safety** (2 hours)
```cpp
std::mutex file_mutex_;

void store_metrics_to_file(const MetricBatch& batch) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    // File write (synchronized)
}
```

### Testing
```bash
# Send metrics
curl -X POST http://localhost:8080/metrics \
  -d '{"metrics":[{"name":"test","value":123}]}'

# Verify file
cat metrics.jsonl
# Should see: {"timestamp":...,"name":"test","value":123}
```

### Performance Analysis
```bash
# Measure impact of sync I/O
./build/load_test 8080 50 10

# Expected: Lower success rate due to blocking
```

### Success Criteria
- ✅ Metrics written to file
- ✅ JSON Lines format correct
- ✅ Thread-safe writes
- ✅ Bottleneck identified

### Bottleneck Discovery
```
Request thread:
  1. Parse JSON      ✅ Fast
  2. Validate        ✅ Fast
  3. Write to file   ❌ BLOCKS! (disk I/O)
  4. Send response   ⏸️ Delayed

Problem: HTTP thread blocked on disk I/O
Solution: Chapter 8 (async I/O)
```

### Learning Outcomes
- File I/O basics
- JSON Lines format
- I/O bottleneck identification
- Motivation for async I/O

**Next Chapter**: Async I/O →

---

## 📚 Chapter 8: Async I/O (Phase 2)
**Duration**: 5-7 days
**Difficulty**: ⭐⭐⭐⭐☆

### Learning Objectives
- Producer-consumer pattern
- Condition variables
- Background threads
- Queue-based decoupling

### TODOs in This Chapter
- TODO #12: Async write queue ⭐
- TODO #13: Background writer thread ⭐

### Background Reading
- Producer-consumer problem
- std::condition_variable usage
- Thread synchronization

### Implementation Guide

**Step 1: Queue Infrastructure** (2 hours)
```cpp
std::queue<MetricBatch> write_queue_;
std::mutex queue_mutex_;
std::condition_variable queue_cv_;
std::atomic<bool> writer_running_{true};
std::thread writer_thread_;
```

**Step 2: Enqueue (Producer)** (2 hours)
```cpp
void queue_metrics_for_async_write(const MetricBatch& batch) {
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        write_queue_.push(batch);
    }
    queue_cv_.notify_one();  // Wake up writer
}
```

**Step 3: Writer Loop (Consumer)** (6 hours)
```cpp
void async_writer_loop() {
    while (writer_running_) {
        std::unique_lock<std::mutex> lock(queue_mutex_);

        // Wait for batches
        queue_cv_.wait(lock, [this] {
            return !write_queue_.empty() || !writer_running_;
        });

        // Process all pending batches
        while (!write_queue_.empty() && writer_running_) {
            MetricBatch batch = write_queue_.front();
            write_queue_.pop();

            // Release lock before I/O!
            lock.unlock();

            store_metrics_to_file(batch);

            // Reacquire for next iteration
            lock.lock();
        }
    }
}
```

**Step 4: Graceful Shutdown** (2 hours)
```cpp
~IngestionService() {
    writer_running_ = false;
    queue_cv_.notify_all();

    if (writer_thread_.joinable()) {
        writer_thread_.join();  // Wait for pending writes
    }
}
```

### Integration
```cpp
HttpResponse handle_metrics_post(...) {
    // Parse and validate
    MetricBatch batch = parse_json(...);

    // Enqueue (non-blocking!)
    queue_metrics_for_async_write(batch);

    // Respond immediately
    return success_response();
}
```

### Testing
```bash
# High request rate
./build/load_test 8080 100 10

# Verify file writes complete
wc -l metrics.jsonl
```

### Performance Testing
```bash
# Before (sync I/O):
./build/load_test 8080 50 10
# ~60% success

# After (async I/O):
./build/load_test 8080 50 10
# Target: 66%+ success
```

### Success Criteria
- ✅ HTTP threads not blocked
- ✅ File writes happen in background
- ✅ No data loss on shutdown
- ✅ Performance improved

### Debugging Tips
```bash
# Check queue depth
gdb ./metricstream_server
(gdb) break queue_metrics_for_async_write
(gdb) run
# Inspect write_queue_.size()
```

### Learning Outcomes
- Producer-consumer pattern mastery
- Condition variable usage
- Lock/unlock patterns
- Graceful shutdown techniques

**Performance Gain**: 59% → 66% success at 50 clients

**Next Chapter**: Mutex contention →

---

## 📚 Chapter 9: Mutex Contention (Phase 4)
**Duration**: 5-7 days
**Difficulty**: ⭐⭐⭐⭐☆

### Learning Objectives
- Identify lock contention
- Hash-based lock sharding
- Deadlock prevention
- Lock granularity tradeoffs

### TODOs in This Chapter
- TODO #14: Per-client mutex (discovers deadlock)
- TODO #15: Hash-based mutex pool ⭐

### Background Reading
- Lock contention analysis
- Deadlock conditions
- Lock ordering
- Hash-based sharding

### Part A: Discover the Deadlock (TODO #14)

**Implementation** (3 hours)
```cpp
// NAIVE APPROACH (will deadlock!)
struct ClientState {
    std::deque<time_point> requests;
    std::mutex mutex;  // Per-client lock
};

std::unordered_map<string, ClientState> clients_;

bool allow_request(const string& client_id) {
    auto& state = clients_[client_id];
    std::lock_guard lock(state.mutex);  // Lock this client

    // Rate limiting logic
}
```

**The Deadlock** (2 hours to discover)
```cpp
void flush_metrics() {
    for (auto& [client_id, state] : clients_) {
        std::lock_guard lock(state.mutex);  // DEADLOCK!
        // If allow_request() is running, we wait
        // If we're iterating, allow_request() waits
    }
}
```

**Deadlock Conditions**:
1. Thread A: In allow_request(), holds client1 mutex
2. Thread B: In flush_metrics(), locks client2 mutex
3. Thread A: Tries to lock client2 (blocks)
4. Thread B: Tries to lock client1 (blocks)
5. **DEADLOCK!**

### Testing for Deadlock
```bash
# Run deadlock detector
./build/deadlock_test

# Symptoms:
# - System hangs
# - No progress
# - High CPU in one thread
```

### Part B: Fix with Hash-Based Pool (TODO #15)

**Solution** (4 hours)
```cpp
// Use pool of mutexes, not per-client
static constexpr size_t MUTEX_POOL_SIZE = 10007;  // Prime!
std::array<std::mutex, MUTEX_POOL_SIZE> mutex_pool_;

std::mutex& get_client_mutex(const string& client_id) {
    size_t hash = std::hash<string>{}(client_id);
    size_t index = hash % MUTEX_POOL_SIZE;
    return mutex_pool_[index];
}

bool allow_request(const string& client_id) {
    std::lock_guard lock(get_client_mutex(client_id));
    // Multiple clients may share mutex (OK!)
}

void flush_metrics() {
    // No locking needed for iteration!
    // Metrics are lock-free (Chapter 10)
}
```

**Why This Works**:
- No per-client mutex ownership
- Hash collision is acceptable
- Flush doesn't need client mutex
- Deadlock impossible

### Performance Analysis
```bash
# Profile lock contention
perf record -e sched:sched_stat_blocked ./metricstream_server

./build/load_test 8080 100 10

perf report --stdio | grep mutex
```

### Success Criteria
- ✅ No deadlocks under load
- ✅ Reduced lock contention
- ✅ Multiple clients progress concurrently
- ✅ flush_metrics() works safely

### Learning Outcomes
- Deadlock detection and prevention
- Hash-based lock sharding
- Lock granularity tradeoffs
- Prime numbers for distribution

**Next Chapter**: Lock-free programming →

---

## 📚 Chapter 10: Lock-Free Programming (Phase 5)
**Duration**: 7-10 days
**Difficulty**: ⭐⭐⭐⭐⭐ (HARDEST CHAPTER)

### Learning Objectives
- Atomic operations
- Memory ordering semantics
- Lock-free data structures
- Ring buffer implementation

### TODOs in This Chapter
- TODO #16: Lock-free ring buffer ⭐
- TODO #17: Lock-free metrics write ⭐
- TODO #18: Lock-free metrics read ⭐

### Background Reading (CRITICAL)
- C++ memory model
- std::atomic operations
- memory_order_acquire/release
- ABA problem
- Ring buffer patterns

### Recommended Resources
```
1. "C++ Concurrency in Action" - Anthony Williams (Chapter 5)
2. Jeff Preshing's blog: preshing.com/archives
3. CPPReference: std::memory_order
```

### Part A: Ring Buffer Structure (TODO #16)

**Design** (3 hours)
```cpp
struct ClientMetrics {
    static constexpr size_t BUFFER_SIZE = 1000;

    // Fixed-size array
    std::array<MetricEvent, BUFFER_SIZE> ring_buffer;

    // Atomic indices
    std::atomic<size_t> write_index{0};
    std::atomic<size_t> read_index{0};
};
```

**Key Properties**:
- Single writer (allow_request thread)
- Single reader (flush_metrics thread)
- Fixed size (no allocation)
- Wraparound with modulo

### Part B: Lock-Free Write (TODO #17)

**Implementation** (4 hours)
```cpp
bool allow_request(const string& client_id) {
    // ... rate limiting with mutex

    // LOCK-FREE metrics write
    auto& metrics = client_metrics_[client_id];

    // Load current write index
    size_t idx = metrics.write_index.load(
        std::memory_order_relaxed  // No ordering needed
    );

    // Write to ring buffer
    metrics.ring_buffer[idx % BUFFER_SIZE] = MetricEvent{now, allowed};

    // Publish write with release semantics
    metrics.write_index.store(
        idx + 1,
        std::memory_order_release  // Ensure write visible
    );

    return allowed;
}
```

**Memory Ordering Explained**:
- `relaxed` on load: Just getting a number, no ordering
- `release` on store: Ensures buffer write happens-before index update

### Part C: Lock-Free Read (TODO #18)

**Implementation** (5 hours)
```cpp
void flush_metrics() {
    // Get snapshot of clients
    std::vector<string> client_ids;
    {
        static std::mutex list_mutex;
        std::lock_guard lock(list_mutex);
        for (const auto& [id, _] : client_metrics_) {
            client_ids.push_back(id);
        }
    }

    // Process each client (lock-free!)
    for (const auto& client_id : client_ids) {
        auto& metrics = client_metrics_[client_id];

        // Load indices with acquire
        size_t read_idx = metrics.read_index.load(
            std::memory_order_acquire
        );
        size_t write_idx = metrics.write_index.load(
            std::memory_order_acquire
        );

        // Process events
        for (size_t i = read_idx; i < write_idx; ++i) {
            const auto& event = metrics.ring_buffer[i % BUFFER_SIZE];
            send_to_monitoring(client_id, event);
        }

        // Update read index
        metrics.read_index.store(
            write_idx,
            std::memory_order_release
        );
    }
}
```

**Memory Ordering Explained**:
- `acquire` on read_index: See all writes before it
- `acquire` on write_index: See all buffer writes
- `release` on read_index update: Publish consumption

### Memory Ordering Quiz
**Question**: Why release/acquire?
**Answer**: Ensures buffer writes visible before index update

**Question**: What if we used relaxed for everything?
**Answer**: Race condition - might read uninitialized data

**Question**: What if buffer overflows?
**Answer**: Wrap around (% BUFFER_SIZE), old data lost

### Testing
```bash
# Stress test lock-free code
./build/load_test 8080 200 10

# Run thread sanitizer
export TSAN_OPTIONS="second_deadlock_stack=1"
clang++ -fsanitize=thread ...
./metricstream_server
```

### Debugging Lock-Free Code
```bash
# Use helgrind for data races
valgrind --tool=helgrind ./metricstream_server

# Use DRD detector
valgrind --tool=drd ./metricstream_server
```

### Success Criteria
- ✅ No locks in metrics path
- ✅ No data races (pass tsan)
- ✅ Correct under high concurrency
- ✅ Performance improved

### Common Pitfalls
- ❌ Using relaxed everywhere (data races)
- ❌ Forgetting modulo (buffer overflow)
- ❌ Wrong memory ordering
- ❌ ABA problem (not applicable here)

### Learning Outcomes
- Atomic operations mastery
- Memory ordering understanding
- Lock-free algorithm design
- Ring buffer implementation

**Performance Gain**: Eliminates lock in critical path

**Next Chapter**: Thread pool →

---

## 📚 Chapter 11: Thread Pool (Phase 6)
**Duration**: 5-7 days
**Difficulty**: ⭐⭐⭐⭐☆

### Learning Objectives
- Thread pool pattern
- Worker thread management
- Bounded queues
- Backpressure handling

### TODOs in This Chapter
- TODO #19: Thread pool implementation ⭐
- TODO #20: Task enqueuing with backpressure ⭐
- TODO #21: Worker loop ⭐
- TODO #22: HTTP server integration ⭐

### Background Reading
- Thread pool patterns
- Work stealing queues
- Backpressure design

### Part A: Thread Pool Core (TODO #19)

**Structure** (3 hours)
```cpp
class ThreadPool {
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex queue_mutex_;
    std::condition_variable condition_;
    std::atomic<bool> stop_{false};
    size_t max_queue_size_;

    void worker_loop();
};
```

**Initialization** (2 hours)
```cpp
ThreadPool(size_t num_threads, size_t max_queue)
    : max_queue_size_(max_queue) {

    // Pre-create workers
    workers_.reserve(num_threads);
    for (size_t i = 0; i < num_threads; ++i) {
        workers_.emplace_back(&ThreadPool::worker_loop, this);
    }
}
```

### Part B: Task Enqueuing (TODO #20)

**Implementation** (3 hours)
```cpp
bool enqueue(std::function<void()> task) {
    {
        std::lock_guard lock(queue_mutex_);

        // Backpressure: reject if full
        if (tasks_.size() >= max_queue_size_) {
            return false;
        }

        if (stop_.load()) {
            return false;
        }

        tasks_.push(std::move(task));
    }

    condition_.notify_one();
    return true;
}
```

### Part C: Worker Loop (TODO #21)

**Implementation** (4 hours)
```cpp
void worker_loop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock lock(queue_mutex_);

            // Wait for task or shutdown
            condition_.wait(lock, [this] {
                return stop_.load() || !tasks_.empty();
            });

            // Exit if stopping and no tasks
            if (stop_.load() && tasks_.empty()) {
                return;
            }

            // Get task
            if (!tasks_.empty()) {
                task = std::move(tasks_.front());
                tasks_.pop();
            }
        }

        // Execute outside lock
        if (task) {
            try {
                task();
            } catch (const std::exception& e) {
                std::cerr << "Task exception: " << e.what() << '\n';
            }
        }
    }
}
```

### Part D: HTTP Integration (TODO #22)

**Replace thread-per-request** (3 hours)
```cpp
// Before:
void run_server() {
    int client_socket = accept(...);
    std::thread([this, client_socket]() {
        handle_request(client_socket);
    }).detach();
}

// After:
ThreadPool thread_pool_(16);  // In constructor

void run_server() {
    int client_socket = accept(...);

    bool enqueued = thread_pool_.enqueue([this, client_socket]() {
        handle_request(client_socket);
    });

    if (!enqueued) {
        // Backpressure response
        const char* msg = "HTTP/1.1 503 Service Unavailable\r\n"
                         "Content-Length: 21\r\n\r\n"
                         "Server overloaded\r\n";
        write(client_socket, msg, strlen(msg));
        close(client_socket);
    }
}
```

### Configuration

**Choosing Thread Count** (1 hour)
```cpp
// CPU-bound: num_cores
// I/O-bound: num_cores * 2
// Mixed:     num_cores + sqrt(num_cores)

// For metrics ingestion (I/O-bound):
size_t thread_count = std::thread::hardware_concurrency() * 2;
// Or fixed: 16 threads (good for 8-core system)
```

### Testing
```bash
# Test different thread counts
for threads in 4 8 16 32; do
    # Modify thread pool size
    ./metricstream_server --threads=$threads &

    ./build/load_test 8080 100 10

    pkill metricstream_server
done

# Compare results
```

### Performance Analysis
```bash
# Before (thread-per-request):
# - Thread creation overhead
# - Context switch storm
# - Memory for stacks

# After (thread pool):
# - Fixed thread count
# - No creation overhead
# - Better cache locality
```

### Success Criteria
- ✅ No thread creation per request
- ✅ Bounded resource usage
- ✅ Backpressure (503) when overloaded
- ✅ Graceful shutdown works

### Learning Outcomes
- Thread pool pattern mastery
- Backpressure handling
- Worker thread management
- Resource bounding

**Performance Gain**: Eliminates thread creation overhead

**Next Chapter**: Advanced rate limiting →

---

## 📚 Chapters 12-15: Advanced Topics
**Duration**: 4-8 weeks
**Difficulty**: ⭐⭐⭐⭐⭐

*See TODOS.md for detailed exercises in:*
- Chapter 12: Advanced Rate Limiting (Token bucket, distributed)
- Chapter 13: Performance Measurement (Latency tracking, profiling)
- Chapter 14: Testing & Load Generation (Unit tests, deadlock detection)
- Chapter 15: Production Readiness (Graceful shutdown, config, logging)

---

## Progress Tracking

### Completion Checklist

**Foundation (Weeks 1-2)**
- [ ] Chapter 1: HTTP Server
- [ ] Chapter 2: Routing
- [ ] Chapter 3: Validation

**Core Features (Weeks 3-5)**
- [ ] Chapter 4: Rate Limiting
- [ ] Chapter 5: JSON Parsing (naive)
- [ ] Chapter 6: JSON Optimization
- [ ] Chapter 7: File I/O (sync)
- [ ] Chapter 8: Async I/O

**Optimizations (Weeks 6-8)**
- [ ] Chapter 9: Mutex Contention
- [ ] Chapter 10: Lock-Free (hardest!)
- [ ] Chapter 11: Thread Pool

**Advanced (Weeks 9-12)**
- [ ] Chapter 12: Advanced Rate Limiting
- [ ] Chapter 13: Performance Measurement
- [ ] Chapter 14: Testing
- [ ] Chapter 15: Production Readiness

---

## Next Steps

1. **Choose your learning path** (A, B, or C above)
2. **Set up development environment**
3. **Start with Chapter 1** (or your comfort level)
4. **Follow the cycle**: Implement → Test → Measure → Document
5. **Join discussions** in GitHub issues

Welcome to the path of deep systems mastery! 🚀
