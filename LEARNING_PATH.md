# MetricsStream Learning Path

Recommended progression through MetricsStream's optimization journey for maximum educational value.

## Overview

This learning path guides you through building a high-performance metrics ingestion service from first principles. Each lesson builds on the previous, introducing new bottlenecks and teaching systematic optimization.

**Total Duration**: 40-60 hours (10-15 hours per week for 4-6 weeks)

**Prerequisites**:
- C++11 or later experience
- Basic understanding of threads and mutexes
- Familiarity with HTTP and JSON
- Comfort with Linux/Unix command line

**Learning Outcomes**:
- Performance profiling and bottleneck identification
- Concurrent programming patterns
- Lock-free data structures
- Memory ordering semantics
- Systems programming best practices

---

## Phase 0: Understanding the Problem Space (Week 1, Days 1-2)

### Lesson 0.1: What is a Metrics Platform?

**Concepts**: Observability, time-series data, metrics vs logs vs traces

**Activities**:
- Read `README.md` and `CLAUDE.md`
- Explore the final implementation (main branch)
- Run the server and send test requests
- Examine `metrics.jsonl` output format

**Starter Code**: Main branch (fully implemented)

**Time**: 2-3 hours

**Deliverables**:
- [ ] Successfully build and run the server
- [ ] Send metrics via curl
- [ ] Understand the data flow diagram

---

### Lesson 0.2: Baseline Implementation

**Concepts**: HTTP from sockets, synchronous I/O, naive JSON parsing

**Activities**:
- Read baseline implementation (to be created in `phase-1-baseline` branch)
- Understand sequential request processing
- Run load test: `./load_test 8080 10 10`
- Measure baseline performance

**Starter Code**: Branch `phase-1-baseline` (to be created)

**Key Files to Study**:
- `src/http_server.cpp`: Socket handling
- `src/ingestion_service.cpp`: Request handling
- `load_test.cpp`: Performance testing tool

**Time**: 3-4 hours

**Deliverables**:
- [ ] Understand every line of baseline code
- [ ] Document baseline performance metrics
- [ ] Identify potential bottlenecks

**Expected Performance**: ~50-100 RPS, 100% success at low concurrency

---

## Phase 1: Concurrency Basics (Week 1, Days 3-5)

### Lesson 1.1: Thread-Per-Request Model

**Concepts**: Threading, std::thread, concurrent request handling

**Problem**: Sequential processing limits throughput

**Exercise**: Modify baseline to spawn thread per request

**Code Location**: `src/http_server.cpp:75-86`

**Current Implementation** (reference):
```cpp
int client_socket = accept(server_fd, ...);
// Spawn thread for handling
std::thread([this, client_socket]() {
    handle_client(client_socket);
}).detach();
```

**Your Task**:
- Replace sequential handling with thread-per-request
- Ensure proper socket cleanup
- Handle thread creation failures

**Testing**:
```bash
./load_test 8080 20 10  # 20 concurrent clients
```

**Time**: 4-5 hours

**Deliverables**:
- [ ] Working thread-per-request implementation
- [ ] Performance comparison with baseline
- [ ] Analysis: where is the new bottleneck?

**Expected Results**:
- 20 clients: 81% → 88% success rate
- Latency: ~5-10ms

**Learning Insight**: Threading improves concurrency but introduces new challenges

---

### Lesson 1.2: Thread Safety and Mutexes

**Concepts**: Race conditions, mutex, lock_guard, data races

**Problem**: Shared state (file handle, counters) needs protection

**Exercise**: Add mutex protection for shared resources

**Code Location**: `src/ingestion_service.cpp:100-102`, `676`

**Current Implementation** (reference):
```cpp
std::mutex file_mutex_;

void store_metrics_to_file(const MetricBatch& batch) {
    std::lock_guard<std::mutex> lock(file_mutex_);
    // Write to file safely
}
```

**Your Task**:
- Identify all shared mutable state
- Add appropriate mutexes
- Use RAII (lock_guard) for exception safety
- Test under concurrent load

**Testing**:
```bash
./load_test 8080 50 10  # Higher concurrency
# Check metrics.jsonl for corruption
```

**Time**: 3-4 hours

**Deliverables**:
- [ ] Thread-safe implementation
- [ ] No data corruption under load
- [ ] Understanding of when mutexes are needed

**Learning Insight**: Correctness comes first; performance second

---

## Phase 2: Asynchronous I/O (Week 2)

### Lesson 2.1: Producer-Consumer Pattern

**Concepts**: Work queues, condition variables, background threads

**Problem**: File I/O blocks request handlers

**Exercise**: Implement background writer thread

**Code Location**: `src/ingestion_service.cpp:729-761`

**Current Implementation** (reference):
```cpp
std::queue<MetricBatch> write_queue_;
std::mutex queue_mutex_;
std::condition_variable queue_cv_;
std::thread writer_thread_;
```

**Your Task**:
1. Create write queue and synchronization primitives
2. Implement `async_writer_loop()` function
3. Modify `handle_metrics_post()` to enqueue instead of write directly
4. Handle graceful shutdown

**Testing**:
```bash
./load_test 8080 50 10
# Observe: requests return faster, writing happens in background
```

**Time**: 6-8 hours

**Deliverables**:
- [ ] Working producer-consumer implementation
- [ ] No lost metrics on shutdown
- [ ] Performance improvement measurement

**Expected Results**:
- 50 clients: 59% → 66% success rate
- Latency: Reduced by 30-50%

**Learning Insight**: Decouple I/O from request handling

---

### Lesson 2.2: Condition Variables and Synchronization

**Concepts**: Wait/notify, spurious wakeups, predicate loops

**Exercise**: Deep dive into condition variable usage

**Study**: The wait predicate in `async_writer_loop()`

```cpp
queue_cv_.wait(lock, [this] {
    return !write_queue_.empty() || !writer_running_;
});
```

**Your Tasks**:
- [ ] Explain why predicate is needed
- [ ] Demonstrate spurious wakeup handling
- [ ] Implement timeout-based wake (e.g., flush every 5 seconds)

**Time**: 3-4 hours

**Learning Insight**: Condition variables are more than just sleep/wake

---

## Phase 3: Algorithm Optimization (Week 3)

### Lesson 3.1: Profiling and Bottleneck Identification

**Concepts**: Profiling, hot paths, algorithmic complexity

**Problem**: JSON parsing becomes bottleneck at high throughput

**Exercise**: Profile the application

**Tools**:
- `perf` or `gprof`
- Manual timing with `std::chrono`
- Load testing at various concurrency levels

**Your Task**:
1. Add timing instrumentation to `parse_json_metrics()`
2. Run load test: `./load_test 8080 100 10`
3. Identify the hottest function
4. Analyze algorithmic complexity

**Testing**:
```bash
# Profile with perf
perf record -g ./metricstream_server &
./load_test 8080 100 10
perf report
```

**Time**: 4-5 hours

**Deliverables**:
- [ ] Profile data showing JSON parsing bottleneck
- [ ] Analysis of current parser's complexity (O(n²))
- [ ] Design for optimized parser

**Learning Insight**: Measure before optimizing

---

### Lesson 3.2: Optimized JSON Parsing

**Concepts**: State machines, single-pass algorithms, memory allocation

**Problem**: Multiple `find()` and `substr()` calls are O(n²)

**Exercise**: Implement single-pass state machine parser

**Code Location**: `src/ingestion_service.cpp:340-522`

**Current Implementation** (reference - state machine approach):
```cpp
enum class ParseState {
    LOOKING_FOR_METRICS,
    IN_METRICS_ARRAY,
    IN_METRIC_OBJECT,
    PARSING_FIELD_NAME,
    // ...
};

while (i < len && state != ParseState::DONE) {
    char c = json_body[i];
    switch (state) {
        case ParseState::IN_METRIC_OBJECT:
            // Handle character based on state
            break;
        // ...
    }
}
```

**Your Task**:
1. Design state machine (draw diagram)
2. Implement single-pass character iteration
3. Pre-allocate string buffers
4. Use `strtod()` for numeric parsing
5. Compare performance with naive parser

**Testing**:
```bash
./performance_test.sh
# Compare with Phase 2 results
```

**Time**: 8-10 hours

**Deliverables**:
- [ ] Working optimized parser
- [ ] Performance comparison (should see 50-100% improvement)
- [ ] State machine diagram

**Expected Results**:
- 50 clients: 64.8% success, 2.25ms latency
- 100 clients: 80.2% success, 2.73ms latency

**Learning Insight**: Algorithm matters more than low-level optimization

---

## Phase 4: Advanced Locking (Week 4, Days 1-3)

### Lesson 4.1: Rate Limiting Implementation

**Concepts**: Sliding window, time-based algorithms, per-client state

**Exercise**: Implement sliding window rate limiter

**Code Location**: `src/ingestion_service.cpp:30-104`

**Problem**: Need to limit requests per client per second

**Your Task**:
1. Design data structure: `std::deque<timestamp>` per client
2. Implement `allow_request(client_id)`:
   - Remove timestamps older than 1 second
   - Check if under limit
   - Add current timestamp if allowed
3. Add mutex protection (start with global mutex)

**Testing**:
```bash
# Test rate limiting works
for i in {1..20}; do
  curl -H "Authorization: client1" http://localhost:8080/metrics -d '{...}'
done
# Should see 429 responses after limit
```

**Time**: 4-5 hours

**Deliverables**:
- [ ] Working rate limiter
- [ ] Per-client limits enforced
- [ ] 429 status codes when limit exceeded

**Learning Insight**: Time-based algorithms require careful state management

---

### Lesson 4.2: The Deadlock Problem

**Concepts**: Deadlock, lock ordering, mutex granularity

**Problem**: Need to iterate all clients in `flush_metrics()` while `allow_request()` holds client locks

**Exercise**: Attempt naive implementation and encounter deadlock

**Your Task**:
1. Implement `flush_metrics()` that iterates all clients
2. Try to acquire locks in arbitrary order
3. Run concurrent load test + flush
4. Observe deadlock (program hangs)
5. Analyze lock ordering problem

**Time**: 3-4 hours

**Deliverables**:
- [ ] Demonstration of deadlock scenario
- [ ] Analysis of why deadlock occurs
- [ ] List of potential solutions

**Learning Insight**: Lock ordering is critical

---

### Lesson 4.3: Hash-Based Mutex Pool

**Concepts**: Lock striping, hash functions, prime numbers

**Solution Approach**: Fixed mutex pool with hash-based selection

**Code Location**: `src/ingestion_service.cpp:23-28`

**Current Implementation** (reference):
```cpp
static constexpr size_t MUTEX_POOL_SIZE = 10007;  // Prime
std::array<std::mutex, MUTEX_POOL_SIZE> client_mutex_pool_;

std::mutex& get_client_mutex(const std::string& client_id) {
    size_t hash = std::hash<std::string>{}(client_id);
    return client_mutex_pool_[hash % MUTEX_POOL_SIZE];
}
```

**Your Task**:
1. Implement fixed mutex pool
2. Hash client_id to select mutex
3. Use prime number for pool size (explain why)
4. Update `allow_request()` to use pool
5. Simplify `flush_metrics()` (no longer needs complex lock ordering)

**Testing**:
```bash
./load_test 8080 100 10
# Should see improved performance and no deadlocks
```

**Time**: 5-6 hours

**Deliverables**:
- [ ] Working hash-based mutex pool
- [ ] No deadlocks under load
- [ ] Performance analysis vs. global mutex

**Learning Insight**: Lock granularity is a tradeoff

---

## Phase 5: Lock-Free Programming (Week 4-5)

### Lesson 5.1: Memory Ordering Fundamentals

**Concepts**: Memory barriers, acquire/release semantics, visibility

**Prerequisites**: Strong understanding of concurrency

**Study Materials**:
- `docs/ring_buffer_implementation.md`
- C++ memory ordering reference
- "C++ Concurrency in Action" Chapter 5

**Exercise**: Memory ordering quiz and demonstrations

**Your Task**:
1. Read about memory ordering models
2. Understand the difference between:
   - `memory_order_relaxed`
   - `memory_order_acquire`
   - `memory_order_release`
   - `memory_order_seq_cst`
3. Write small examples demonstrating each

**Time**: 6-8 hours

**Deliverables**:
- [ ] Written explanations of each memory order
- [ ] Code examples demonstrating visibility guarantees

**Learning Insight**: Lock-free programming requires deep understanding

**⚠️ Difficulty Warning**: This is advanced material. Take your time.

---

### Lesson 5.2: Lock-Free Ring Buffer

**Concepts**: Ring buffers, atomic operations, single-writer/single-reader

**Problem**: Mutex in metrics collection hot path

**Exercise**: Implement lock-free metrics collection

**Code Location**: `src/ingestion_service.cpp:92-103` (write), `132-149` (read)

**Current Implementation** (reference):
```cpp
struct ClientMetrics {
    std::array<MetricEvent, 1000> ring_buffer;
    std::atomic<size_t> write_index{0};
    std::atomic<size_t> read_index{0};
};

// Writer (allow_request)
size_t idx = write_index.load(std::memory_order_relaxed);
ring_buffer[idx % BUFFER_SIZE] = event;
write_index.store(idx + 1, std::memory_order_release);

// Reader (flush_metrics)
size_t read_idx = read_index.load(std::memory_order_acquire);
size_t write_idx = write_index.load(std::memory_order_acquire);
for (size_t i = read_idx; i < write_idx; ++i) {
    process(ring_buffer[i % BUFFER_SIZE]);
}
read_index.store(write_idx, std::memory_order_release);
```

**Your Task**:
1. Implement `ClientMetrics` structure
2. Update `allow_request()` to write to ring buffer (lock-free)
3. Update `flush_metrics()` to read from ring buffer (lock-free)
4. Ensure proper memory ordering
5. Handle buffer overflow (wrap around)

**Testing**:
```bash
# Stress test with high concurrency
./load_test 8080 200 100
# Verify no metrics are lost or corrupted
```

**Time**: 10-12 hours

**Deliverables**:
- [ ] Working lock-free implementation
- [ ] Correctness verification
- [ ] Performance comparison with mutex version
- [ ] Explanation of memory ordering choices

**Expected Results**:
- Eliminated mutex from hot path
- Improved scalability at high concurrency

**Learning Insight**: Lock-free is powerful but requires careful design

---

## Phase 6: Resource Management (Week 6)

### Lesson 6.1: Thread Pool Implementation

**Concepts**: Worker threads, task queues, backpressure

**Problem**: Thread creation overhead at 150+ concurrent clients

**Exercise**: Replace thread-per-request with thread pool

**Code Location**: `include/thread_pool.h`, `src/http_server.cpp:87-115`

**Current Implementation** (reference):
```cpp
class ThreadPool {
public:
    ThreadPool(size_t num_threads, size_t max_queue_size);
    bool enqueue(std::function<void()> task);  // false if queue full
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::condition_variable condition_;
    std::atomic<bool> stop_;
};
```

**Your Task**:
1. Implement `ThreadPool` class
2. Create fixed number of worker threads (default 16)
3. Implement task queue with bounded size
4. Return `false` when queue is full (backpressure)
5. Update `HttpServer` to use thread pool
6. Return 503 when queue is full

**Testing**:
```bash
./load_test 8080 200 10
# Should handle more clients without degradation
```

**Time**: 8-10 hours

**Deliverables**:
- [ ] Working thread pool
- [ ] Graceful shutdown
- [ ] Backpressure handling (503 responses)
- [ ] Performance comparison with thread-per-request

**Learning Insight**: Resource pooling is essential for production systems

---

### Lesson 6.2: Load Shedding and Graceful Degradation

**Concepts**: Backpressure, circuit breakers, graceful degradation

**Exercise**: Analyze how the system handles overload

**Your Task**:
1. Test with extreme load: `./load_test 8080 500 20`
2. Observe 503 responses when queue is full
3. Analyze system behavior under overload
4. Implement monitoring for queue depth
5. Add circuit breaker pattern (optional advanced exercise)

**Time**: 4-5 hours

**Deliverables**:
- [ ] Load testing results across different concurrency levels
- [ ] Analysis of system behavior at capacity
- [ ] Recommendations for production deployment

**Learning Insight**: Systems must handle overload gracefully

---

## Phase 7: Production Readiness (Optional)

### Lesson 7.1: Performance Monitoring

**Concepts**: Observability, metrics, logging

**Exercise**: Implement TODO #1 from main.cpp

**Code Location**: `src/main.cpp:36`

**Your Task**:
1. Track active connection count
2. Monitor request processing time distribution (p50, p95, p99)
3. Track queue depths
4. Monitor memory usage
5. Log periodic statistics

**Time**: 6-8 hours

**Deliverables**:
- [ ] Real-time monitoring output
- [ ] Performance dashboard (text-based)
- [ ] Alerts for anomalies

---

### Lesson 7.2: Integration and Deployment

**Concepts**: System integration, configuration, deployment

**Exercise**: Prepare for production deployment

**Your Task**:
1. Add configuration file support
2. Implement signal handling for graceful shutdown
3. Add structured logging
4. Create systemd service file
5. Write deployment documentation

**Time**: 8-10 hours

**Deliverables**:
- [ ] Production-ready configuration
- [ ] Deployment guide
- [ ] Monitoring and alerting setup

---

## Capstone Project

### Option 1: Time-Series Storage Integration

Integrate with ClickHouse or TimescaleDB for persistent storage.

**Time**: 20-30 hours

---

### Option 2: Query API

Implement query endpoint for retrieving and aggregating metrics.

**Time**: 15-20 hours

---

### Option 3: Distributed Architecture

Design and implement sharding for horizontal scaling.

**Time**: 30-40 hours

---

## Assessment and Progression

### Knowledge Checks

After each phase, you should be able to:

**Phase 1**:
- Explain thread-per-request model
- Identify race conditions
- Use mutexes correctly

**Phase 2**:
- Implement producer-consumer pattern
- Use condition variables
- Decouple I/O from processing

**Phase 3**:
- Profile applications
- Identify algorithmic bottlenecks
- Optimize based on measurements

**Phase 4**:
- Recognize deadlock scenarios
- Design lock ordering strategies
- Implement lock striping

**Phase 5**:
- Understand memory ordering
- Implement lock-free data structures
- Reason about concurrency without locks

**Phase 6**:
- Design resource pooling
- Implement backpressure
- Build production-ready systems

---

## Resources

### Documentation
- `CLAUDE.md`: Project overview and methodology
- `TODOS.md`: Complete TODO analysis
- `COMMIT_JOURNEY.md`: Implementation history
- `docs/ring_buffer_implementation.md`: Lock-free techniques

### Performance Testing
- `load_test.cpp`: Load testing tool
- `performance_test.sh`: Systematic benchmarking
- `performance_results.txt`: Historical results

### External References
- "C++ Concurrency in Action" by Anthony Williams
- "Systems Performance" by Brendan Gregg
- Linux perf documentation
- C++ memory model reference

---

## Tips for Success

1. **Measure Everything**: Always profile before optimizing
2. **Start Simple**: Get correctness first, performance second
3. **Test Concurrently**: Race conditions only appear under load
4. **Read the Docs**: Study existing documentation in `docs/`
5. **Compare Results**: Validate each phase against expected performance
6. **Ask Questions**: Use GitHub issues for questions and discussions

---

**Total Estimated Time**: 40-60 hours core material + 15-30 hours capstone

**Generated**: 2025-10-04
**Repository State**: commit 9dd194d
