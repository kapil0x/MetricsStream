# MetricsStream Exercises
## Hands-On Learning Activities and Practice Problems

This document provides structured exercises for each chapter, including variations, challenges, and extension projects.

---

## Exercise Categories

- 🟢 **Beginner** - Foundation building
- 🟡 **Intermediate** - Core concepts
- 🔴 **Advanced** - Deep technical mastery
- 🟣 **Expert** - Research-level challenges

---

## Chapter 1: HTTP Server Basics

### Exercise 1.1: Basic Socket Server 🟢
**Difficulty**: Easy
**Time**: 2-3 hours

**Task**: Implement a minimal TCP server that accepts connections and echoes received data.

**Requirements**:
```cpp
// Accept connection
// Read data into buffer
// Echo back to client
// Close connection
```

**Test**:
```bash
nc localhost 8080
# Type: Hello
# Receive: Hello
```

**Learning**: Socket lifecycle, read/write syscalls

---

### Exercise 1.2: HTTP Header Parser 🟡
**Difficulty**: Medium
**Time**: 3-4 hours

**Task**: Parse HTTP headers into a map structure.

**Input**:
```
GET /path HTTP/1.1\r\n
Host: localhost\r\n
Content-Type: application/json\r\n
\r\n
```

**Output**:
```cpp
map<string, string> headers = {
    {"Host", "localhost"},
    {"Content-Type", "application/json"}
};
```

**Edge Cases**:
- Header with colon in value: `Authorization: Bearer: xyz`
- Multi-word headers: `User-Agent: Mozilla/5.0`
- Missing final `\r\n\r\n`

**Learning**: String parsing, edge case handling

---

### Exercise 1.3: HTTP Response Builder 🟢
**Difficulty**: Easy
**Time**: 1-2 hours

**Task**: Build valid HTTP response strings.

**Template**:
```cpp
string build_response(int status, string body) {
    // Return: "HTTP/1.1 <status> <status_text>\r\n"
    //         "Content-Length: <len>\r\n"
    //         "\r\n"
    //         "<body>"
}
```

**Test Cases**:
```cpp
build_response(200, "OK") →
  "HTTP/1.1 200 OK\r\nContent-Length: 2\r\n\r\nOK"

build_response(404, "Not Found") →
  "HTTP/1.1 404 Not Found\r\nContent-Length: 9\r\n\r\nNot Found"
```

**Learning**: HTTP protocol format

---

## Chapter 2: Request Routing

### Exercise 2.1: Path Matching 🟡
**Difficulty**: Medium
**Time**: 2-3 hours

**Task**: Implement wildcard path matching.

**Examples**:
```cpp
match("/users/*", "/users/123")      → true
match("/api/v1/*", "/api/v1/metrics") → true
match("/exact", "/exact")            → true
match("/exact", "/exact/")           → false
```

**Extension**: Add path parameters
```cpp
match("/users/:id", "/users/123") → {"id": "123"}
```

**Learning**: Pattern matching, parameter extraction

---

### Exercise 2.2: Method-Based Routing 🟢
**Difficulty**: Easy
**Time**: 2 hours

**Task**: Route same path to different handlers by method.

**Test**:
```bash
curl -X GET http://localhost:8080/data    # → get_handler()
curl -X POST http://localhost:8080/data   # → post_handler()
curl -X DELETE http://localhost:8080/data # → delete_handler()
```

**Learning**: Method dispatch, handler registration

---

### Exercise 2.3: Middleware Chain 🔴
**Difficulty**: Advanced
**Time**: 4-6 hours

**Task**: Implement middleware pipeline.

**Design**:
```cpp
using Middleware = function<HttpResponse(HttpRequest, NextHandler)>;

class Router {
    vector<Middleware> middlewares_;

    void use(Middleware mw) {
        middlewares_.push_back(mw);
    }
};
```

**Example Middlewares**:
- Logging: Log request details
- Auth: Check authorization header
- Timing: Measure request duration

**Learning**: Middleware pattern, function composition

---

## Chapter 3: Metrics Validation

### Exercise 3.1: Custom Validators 🟡
**Difficulty**: Medium
**Time**: 3 hours

**Task**: Implement domain-specific validators.

**Validators**:
```cpp
// 1. Range validator
ValidateRange(double min, double max)
  → Reject values outside [min, max]

// 2. Regex name validator
ValidateName(regex pattern)
  → Reject names not matching pattern

// 3. Tag validator
ValidateTags(set<string> required_tags)
  → Reject metrics missing required tags
```

**Test**:
```cpp
auto validator = ValidateRange(0, 100);
validator.check({name: "cpu", value: 75})   → OK
validator.check({name: "cpu", value: 150})  → ERROR
```

**Learning**: Validation patterns, composable validators

---

### Exercise 3.2: Batch Sanitization 🟢
**Difficulty**: Easy
**Time**: 2 hours

**Task**: Sanitize invalid metrics instead of rejecting batch.

**Behavior**:
```cpp
Input:  [{valid}, {invalid}, {valid}]
Output: [{valid}, {valid}]  // Invalid removed
```

**Extension**: Add sanitization report
```cpp
{
  "accepted": 2,
  "rejected": 1,
  "errors": ["Metric at index 1: invalid value"]
}
```

**Learning**: Data sanitization, error reporting

---

### Exercise 3.3: Schema Validation 🔴
**Difficulty**: Advanced
**Time**: 5-7 hours

**Task**: Implement JSON schema validation.

**Schema**:
```json
{
  "type": "object",
  "required": ["metrics"],
  "properties": {
    "metrics": {
      "type": "array",
      "items": {
        "type": "object",
        "required": ["name", "value"],
        "properties": {
          "name": {"type": "string", "maxLength": 255},
          "value": {"type": "number"}
        }
      }
    }
  }
}
```

**Learning**: Schema validation, recursive validation

---

## Chapter 4: Rate Limiting

### Exercise 4.1: Token Bucket 🔴
**Difficulty**: Advanced
**Time**: 6-8 hours

**Task**: Implement token bucket algorithm.

**Algorithm**:
```cpp
class TokenBucket {
    double tokens;
    double capacity;
    double refill_rate;  // tokens per second
    time_point last_refill;

    bool try_consume(double amount) {
        // Refill based on elapsed time
        // Try to consume tokens
        // Return true if successful
    }
};
```

**Properties**:
- Allow bursts up to capacity
- Refill at constant rate
- Per-client isolation

**Test**:
```cpp
TokenBucket bucket(10, 5);  // capacity=10, rate=5/sec

// Burst: 10 requests succeed immediately
// Then: 5 requests/sec allowed
```

**Learning**: Token bucket algorithm, burst handling

---

### Exercise 4.2: Leaky Bucket 🔴
**Difficulty**: Advanced
**Time**: 6-8 hours

**Task**: Implement leaky bucket algorithm.

**Algorithm**:
```cpp
class LeakyBucket {
    queue<time_point> bucket;
    size_t capacity;
    duration leak_rate;

    bool try_add() {
        // Leak old requests
        // Check if space available
        // Add request if space
    }
};
```

**Difference from Token Bucket**:
- Smooths traffic (no bursts)
- FIFO queue
- Constant leak rate

**Learning**: Rate limiting algorithms, queueing theory

---

### Exercise 4.3: Distributed Rate Limiting 🟣
**Difficulty**: Expert
**Time**: 2-3 weeks

**Task**: Implement distributed rate limiting with Redis.

**Architecture**:
```
Service Instance 1 ──┐
Service Instance 2 ──┼──→ Redis (shared state)
Service Instance 3 ──┘
```

**Redis Commands**:
```redis
# Increment counter
INCR client:123:count
EXPIRE client:123:count 1

# Check limit
GET client:123:count
```

**Challenges**:
- Network latency
- Redis failures (fallback to local)
- Clock skew between instances
- Sliding window with Redis

**Learning**: Distributed systems, external dependencies

---

### Exercise 4.4: Adaptive Rate Limiting 🟣
**Difficulty**: Expert
**Time**: 2-3 weeks

**Task**: Implement rate limiting that adapts to system load.

**Algorithm**:
```cpp
class AdaptiveRateLimiter {
    double base_limit;
    double current_limit;

    void adjust_based_on_metrics() {
        if (cpu_usage > 80%) {
            current_limit *= 0.9;  // Reduce limit
        } else if (cpu_usage < 50%) {
            current_limit *= 1.1;  // Increase limit
        }
    }
};
```

**Metrics to Consider**:
- CPU usage
- Memory pressure
- Queue depth
- Response latency

**Learning**: Adaptive algorithms, feedback control

---

## Chapter 5-6: JSON Parsing

### Exercise 5.1: Performance Comparison 🟡
**Difficulty**: Medium
**Time**: 4-6 hours

**Task**: Compare naive vs optimized parser performance.

**Benchmarks**:
```cpp
// Measure:
// 1. Parse time per metric
// 2. Memory allocations
// 3. CPU cache misses

for (int size : {10, 100, 1000, 10000}) {
    auto batch = generate_batch(size);

    time_naive = benchmark(parse_json_naive, batch);
    time_optimized = benchmark(parse_json_optimized, batch);

    cout << "Size " << size << ": "
         << "Naive=" << time_naive << "μs, "
         << "Optimized=" << time_optimized << "μs\n";
}
```

**Tools**:
```bash
# Memory allocations
valgrind --tool=massif ./benchmark

# Cache misses
perf stat -e cache-misses ./benchmark
```

**Learning**: Performance measurement, profiling

---

### Exercise 5.2: SIMD JSON Parsing 🟣
**Difficulty**: Expert
**Time**: 3-4 weeks

**Task**: Implement SIMD-accelerated JSON parser.

**Approach**:
```cpp
#include <immintrin.h>  // AVX2

// Find quotes in 32 bytes at once
__m256i chunk = _mm256_loadu_si256((__m256i*)json);
__m256i quotes = _mm256_set1_epi8('"');
__m256i matches = _mm256_cmpeq_epi8(chunk, quotes);
int mask = _mm256_movemask_epi8(matches);
// Positions of quotes in mask bits
```

**Or use simdjson library**:
```cpp
#include "simdjson.h"

simdjson::dom::parser parser;
auto json = parser.parse(input);
```

**Learning**: SIMD programming, vectorization

---

### Exercise 5.3: Streaming Parser 🔴
**Difficulty**: Advanced
**Time**: 1-2 weeks

**Task**: Parse JSON without loading entire string.

**Design**:
```cpp
class StreamingJSONParser {
    istream& input;

    Metric next_metric() {
        // Read character by character
        // Build metric incrementally
        // Return when complete
    }
};
```

**Benefits**:
- Constant memory usage
- Handle huge payloads
- Start processing early

**Learning**: Streaming algorithms, incremental parsing

---

## Chapter 7-8: File I/O

### Exercise 7.1: WAL Implementation 🔴
**Difficulty**: Advanced
**Time**: 1-2 weeks

**Task**: Implement Write-Ahead Log for durability.

**Design**:
```cpp
class WAL {
    ofstream log_file;
    uint64_t sequence_number;

    void append(const Metric& metric) {
        // Write: <seq><metric>\n
        // Fsync periodically
    }

    void replay() {
        // Read log on startup
        // Restore state
    }
};
```

**Features**:
- Sequential writes (fast)
- Fsync for durability
- Replay on crash recovery
- Log compaction

**Learning**: WAL pattern, crash recovery

---

### Exercise 7.2: Memory-Mapped I/O 🔴
**Difficulty**: Advanced
**Time**: 1 week

**Task**: Use mmap() for file I/O.

**Implementation**:
```cpp
#include <sys/mman.h>

void* addr = mmap(NULL, file_size,
                  PROT_READ | PROT_WRITE,
                  MAP_SHARED, fd, 0);

// Write directly to memory
memcpy(addr + offset, data, size);

// OS handles actual I/O
msync(addr, size, MS_ASYNC);
```

**Benefits**:
- Zero-copy
- OS page cache
- No read/write syscalls

**Learning**: Memory-mapped files, zero-copy I/O

---

### Exercise 7.3: io_uring Integration 🟣
**Difficulty**: Expert
**Time**: 2-3 weeks

**Task**: Replace blocking I/O with io_uring (Linux 5.1+).

**Implementation**:
```cpp
#include <liburing.h>

struct io_uring ring;
io_uring_queue_init(QUEUE_DEPTH, &ring, 0);

// Submit write
struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
io_uring_prep_write(sqe, fd, buffer, len, offset);
io_uring_submit(&ring);

// Poll completion
struct io_uring_cqe* cqe;
io_uring_wait_cqe(&ring, &cqe);
```

**Benefits**:
- Truly async I/O
- Batch submissions
- Kernel bypass

**Learning**: Modern Linux I/O, io_uring

---

## Chapter 9: Mutex Contention

### Exercise 9.1: Lock Profiling 🟡
**Difficulty**: Medium
**Time**: 4-6 hours

**Task**: Profile lock contention in rate limiter.

**Tools**:
```bash
# Perf for lock contention
perf record -e sched:sched_stat_blocked \
  -g -- ./metricstream_server

./build/load_test 8080 100 10

perf report --stdio
```

**Metrics to Measure**:
- Lock hold time
- Wait time per lock
- Contention hotspots
- Fairness (starvation?)

**Visualization**:
```bash
# Flamegraph of contention
perf script | stackcollapse-perf.pl | flamegraph.pl > locks.svg
```

**Learning**: Lock profiling, performance analysis

---

### Exercise 9.2: Read-Write Lock Optimization 🔴
**Difficulty**: Advanced
**Time**: 1 week

**Task**: Use shared_mutex for read-heavy workloads.

**Design**:
```cpp
class RateLimiter {
    std::shared_mutex mutex_;

    bool allow_request(const string& client_id) {
        std::unique_lock lock(mutex_);  // Exclusive
        // Modify client_requests_
    }

    void get_stats() {
        std::shared_lock lock(mutex_);  // Shared
        // Read client_requests_ (multiple readers OK)
    }
};
```

**Benchmark**:
```cpp
// Ratio: 90% reads, 10% writes
// Compare: mutex vs shared_mutex
```

**Learning**: Reader-writer locks, lock granularity

---

### Exercise 9.3: Lock-Free HashMap 🟣
**Difficulty**: Expert
**Time**: 3-4 weeks

**Task**: Implement lock-free concurrent hash map.

**Approach**: Use split-ordered lists (from "The Art of Multiprocessor Programming")

**Challenges**:
- CAS-based insertion
- Memory reclamation
- ABA problem
- Resizing

**Or use library**:
```cpp
#include <tbb/concurrent_hash_map.h>
```

**Learning**: Lock-free data structures, hazard pointers

---

## Chapter 10: Lock-Free Programming

### Exercise 10.1: SPSC Queue 🔴
**Difficulty**: Advanced
**Time**: 1 week

**Task**: Implement lock-free single-producer single-consumer queue.

**Design**:
```cpp
template<typename T, size_t N>
class SPSCQueue {
    std::array<T, N> buffer;
    std::atomic<size_t> read_idx{0};
    std::atomic<size_t> write_idx{0};

    bool try_push(T value) {
        size_t write = write_idx.load(relaxed);
        size_t next = (write + 1) % N;

        if (next == read_idx.load(acquire)) {
            return false;  // Full
        }

        buffer[write] = value;
        write_idx.store(next, release);
        return true;
    }
};
```

**Memory Ordering**:
- `acquire` on read: See all writes
- `release` on write: Publish write
- `relaxed` for indices (within same thread)

**Learning**: SPSC queue, memory ordering

---

### Exercise 10.2: MPSC Queue 🟣
**Difficulty**: Expert
**Time**: 2-3 weeks

**Task**: Multi-producer single-consumer queue.

**Approach**: Dmitry Vyukov's intrusive MPSC queue

**Key Challenge**: Multiple producers need CAS
```cpp
bool try_push(T* node) {
    node->next.store(nullptr, relaxed);

    T* prev = tail.exchange(node, acq_rel);
    prev->next.store(node, release);
}
```

**Learning**: Lock-free queues, CAS operations

---

### Exercise 10.3: ABA Problem Demo 🔴
**Difficulty**: Advanced
**Time**: 3-5 days

**Task**: Demonstrate and solve ABA problem.

**Problem**:
```cpp
// Thread 1:
Node* old_head = head.load();
// ... do work ...
head.compare_exchange_strong(old_head, new_head);
// CAS succeeds but head was A→B→A (changed!)
```

**Solution 1**: Tagged pointers
```cpp
struct TaggedPtr {
    Node* ptr;
    uint64_t tag;  // Version number
};

std::atomic<TaggedPtr> head;
```

**Solution 2**: Hazard pointers (defer deletion)

**Learning**: ABA problem, versioning, hazard pointers

---

## Chapter 11: Thread Pool

### Exercise 11.1: Work Stealing 🔴
**Difficulty**: Advanced
**Time**: 1-2 weeks

**Task**: Implement work-stealing thread pool.

**Design**:
```cpp
class WorkStealingPool {
    vector<deque<Task>> per_thread_queues;

    void worker_loop(int thread_id) {
        while (true) {
            // Try own queue first
            Task task = queues[thread_id].pop_front();

            if (!task) {
                // Steal from random thread
                int victim = random() % num_threads;
                task = queues[victim].pop_back();
            }

            if (task) execute(task);
        }
    }
};
```

**Benefits**:
- Load balancing
- Cache locality
- Reduced contention

**Learning**: Work stealing, load balancing

---

### Exercise 11.2: Priority Queue 🟡
**Difficulty**: Medium
**Time**: 4-6 hours

**Task**: Add priority levels to thread pool.

**Design**:
```cpp
enum Priority { HIGH, NORMAL, LOW };

class PriorityThreadPool {
    priority_queue<Task> tasks;

    void enqueue(Task task, Priority p) {
        task.priority = p;
        tasks.push(task);
    }

    Task get_next() {
        return tasks.top();  // Highest priority
    }
};
```

**Test**:
```cpp
pool.enqueue(slow_task, LOW);
pool.enqueue(urgent_task, HIGH);
// urgent_task executes first
```

**Learning**: Priority queues, task scheduling

---

### Exercise 11.3: Async/Await with Futures 🔴
**Difficulty**: Advanced
**Time**: 1-2 weeks

**Task**: Add future-based async API.

**Design**:
```cpp
template<typename R>
future<R> async(function<R()> task) {
    auto promise = make_shared<std::promise<R>>();
    auto future = promise->get_future();

    thread_pool.enqueue([promise, task]() {
        try {
            R result = task();
            promise->set_value(result);
        } catch (...) {
            promise->set_exception(current_exception());
        }
    });

    return future;
}
```

**Usage**:
```cpp
future<int> f = async([]() { return compute(); });
int result = f.get();  // Wait for completion
```

**Learning**: Futures, promises, async patterns

---

## Capstone Projects

### Project 1: Metrics Query Engine 🟣
**Duration**: 4-6 weeks

**Task**: Build query engine for stored metrics.

**Features**:
- PromQL-like query language
- Time-range queries
- Aggregations (sum, avg, max, min, percentile)
- Grouping by tags
- Query optimization

**Example**:
```
rate(http_requests_total[5m])
avg by (status) (http_duration_seconds)
```

**Learning**: Query engines, time-series databases

---

### Project 2: Distributed Metrics System 🟣
**Duration**: 8-12 weeks

**Task**: Scale to multiple nodes.

**Architecture**:
```
Load Balancer
     |
     v
[Ingestion Node 1] [Ingestion Node 2] [Ingestion Node 3]
     |                  |                  |
     v                  v                  v
           [Distributed Storage: ClickHouse]
                        |
                        v
              [Query Service Cluster]
```

**Challenges**:
- Consistent hashing for sharding
- Replication for fault tolerance
- Distributed queries
- Cross-datacenter replication

**Learning**: Distributed systems, sharding, replication

---

### Project 3: Real-Time Alerting 🔴
**Duration**: 3-4 weeks

**Task**: Build alerting system on metrics.

**Features**:
- Alert rules (threshold, anomaly detection)
- Alert routing (email, Slack, PagerDuty)
- Deduplication and grouping
- Silencing and acknowledgement

**Example Alert**:
```yaml
alert: HighCPU
expr: cpu_usage > 80
for: 5m
labels:
  severity: warning
annotations:
  summary: CPU usage above 80% for 5 minutes
```

**Learning**: Event processing, notification systems

---

### Project 4: Observability Dashboard 🔴
**Duration**: 3-4 weeks

**Task**: Build web UI for metrics visualization.

**Tech Stack**:
- Backend: MetricsStream query API
- Frontend: React + D3.js/Chart.js
- WebSocket for real-time updates

**Features**:
- Time-series charts
- Heatmaps for latency distribution
- Custom dashboards
- Alert visualization

**Learning**: Full-stack development, data visualization

---

## Practice Problem Sets

### Problem Set 1: Concurrency Bugs 🔴
**Fix these intentionally buggy implementations:**

**Bug 1**: Data race
```cpp
class Counter {
    int count = 0;

    void increment() {
        count++;  // RACE!
    }
};
```

**Bug 2**: Deadlock
```cpp
void transfer(Account& from, Account& to, int amount) {
    lock(from.mutex);
    lock(to.mutex);  // DEADLOCK if reversed elsewhere
    from.balance -= amount;
    to.balance += amount;
    unlock(to.mutex);
    unlock(from.mutex);
}
```

**Bug 3**: Use-after-free
```cpp
void async_read(int fd) {
    char buffer[1024];
    thread([fd, &buffer]() {  // buffer is stack var!
        read(fd, buffer, 1024);
    }).detach();
}  // buffer destroyed, thread still running
```

**Learning**: Concurrency bug patterns, debugging

---

### Problem Set 2: Performance Optimization 🔴
**Optimize these slow implementations:**

**Slow 1**: String concatenation
```cpp
string build_json(vector<Metric> metrics) {
    string result = "{";
    for (auto& m : metrics) {
        result += "\"" + m.name + "\":" + to_string(m.value) + ",";
    }
    result += "}";
    return result;
}
```

**Slow 2**: Unnecessary copies
```cpp
void process(vector<int> data) {
    vector<int> copy = data;  // Deep copy
    for (int x : copy) {
        // ...
    }
}
```

**Learning**: Performance optimization, profiling

---

## Additional Resources

### Books
- "C++ Concurrency in Action" - Anthony Williams
- "Systems Performance" - Brendan Gregg
- "The Art of Multiprocessor Programming" - Herlihy & Shavit

### Online Courses
- CMU 15-445: Database Systems
- MIT 6.824: Distributed Systems
- Stanford CS140: Operating Systems

### Tools to Master
- perf, valgrind, gdb
- strace, ltrace
- flamegraph, perf-tools
- Thread Sanitizer (tsan)

---

## Submission Guidelines

For each exercise:
1. **Implementation** - Working code
2. **Tests** - Demonstrate correctness
3. **Benchmarks** - Measure performance
4. **Write-up** - Document learnings

**Template**:
```markdown
# Exercise X.Y: [Name]

## Implementation
[Code or link to code]

## Testing
[Test results, edge cases covered]

## Performance
[Benchmarks, before/after measurements]

## Learnings
[What you learned, challenges faced]
```

---

Start with exercises matching your skill level, then progressively tackle harder challenges. Each exercise builds expertise toward principal engineer-level mastery!
