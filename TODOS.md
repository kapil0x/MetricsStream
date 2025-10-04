# MetricsStream Learning TODOs
## Complete List of Learning Exercises by Chapter

This document contains all TODO(human) items organized by learning chapter. Each TODO represents a hands-on exercise for building MetricsStream from first principles.

---

## Chapter 1: HTTP Server Basics
**Goal**: Build a working HTTP server from raw sockets

### TODO #1: Socket Creation and Binding
**Location**: `src/http_server.cpp:48-73`
**Status**: ✅ Implemented (Example)
**Difficulty**: Easy

```cpp
// TODO(human): Create TCP socket and bind to port
// Requirements:
// 1. Create socket with AF_INET, SOCK_STREAM
// 2. Set SO_REUSEADDR option
// 3. Bind to INADDR_ANY on specified port
// 4. Handle errors gracefully
```

**Solution Approach**: Use standard POSIX socket APIs
**Learning**: Socket programming fundamentals, error handling

---

### TODO #2: Accept Loop Implementation
**Location**: `src/http_server.cpp:75-116`
**Status**: ✅ Implemented (Example)
**Difficulty**: Medium

```cpp
// TODO(human): Implement accept loop for incoming connections
// Requirements:
// 1. Listen on socket with backlog
// 2. Accept connections in loop
// 3. Handle connection in separate thread (Phase 1)
// 4. Close client socket after handling
```

**Learning**: Connection handling, basic threading model

---

### TODO #3: HTTP Request Parser
**Location**: `src/http_server.cpp:121-161`
**Status**: ✅ Implemented (Example)
**Difficulty**: Medium

```cpp
// TODO(human): Parse raw HTTP request into structured format
// Requirements:
// 1. Extract method and path from request line
// 2. Parse headers (key: value format)
// 3. Extract body after headers
// 4. Handle edge cases (missing newlines, etc.)
```

**Learning**: Protocol parsing, string manipulation

---

## Chapter 2: Request Routing & Handlers
**Goal**: Route requests to appropriate handlers

### TODO #4: Handler Registration
**Location**: `src/http_server.cpp:21-23`
**Status**: ✅ Implemented (Example)
**Difficulty**: Easy

```cpp
// TODO(human): Implement handler registration system
// Requirements:
// 1. Store handlers by path and method
// 2. Use nested map structure: path -> method -> handler
// 3. Support lambda/function handlers
```

**Learning**: Function objects, callbacks, std::function

---

### TODO #5: Request Dispatching
**Location**: `src/http_server.cpp:190-208`
**Status**: ✅ Implemented (Example)
**Difficulty**: Medium

```cpp
// TODO(human): Dispatch request to appropriate handler
// Requirements:
// 1. Look up handler by path
// 2. Match HTTP method
// 3. Return 404 if path not found
// 4. Return 405 if method not allowed
```

**Learning**: Routing logic, HTTP status codes

---

## Chapter 3: Metrics Validation
**Goal**: Validate incoming metric data

### TODO #6: Single Metric Validation
**Location**: `src/ingestion_service.cpp:166-189`
**Status**: ✅ Implemented (Example)
**Difficulty**: Easy

```cpp
// TODO(human): Validate individual metric
// Requirements:
// 1. Check name is non-empty and < 255 chars
// 2. Ensure value is finite (not NaN or Inf)
// 3. Return validation result with error message
```

**Learning**: Input validation, edge case handling

---

### TODO #7: Batch Validation
**Location**: `src/ingestion_service.cpp:191-217`
**Status**: ✅ Implemented (Example)
**Difficulty**: Medium

```cpp
// TODO(human): Validate batch of metrics
// Requirements:
// 1. Check batch is not empty
// 2. Enforce max batch size (1000 metrics)
// 3. Validate each metric in batch
// 4. Return first error encountered
```

**Learning**: Batch processing, validation patterns

---

## Chapter 4: Rate Limiting Fundamentals
**Goal**: Implement per-client rate limiting

### TODO #8: Sliding Window Rate Limiting ⭐
**Location**: `src/ingestion_service.cpp:12-16`
**Status**: 🚧 **ACTIVE TODO - LEARNER EXERCISE**
**Difficulty**: Hard

```cpp
// TODO(human): Implement sliding window rate limiting algorithm
// Requirements:
// 1. Store timestamps per client in deque
// 2. Remove timestamps older than 1 second
// 3. Check if request count < max_requests_per_second
// 4. Add current timestamp if allowed
// 5. Handle concurrent access with mutex
//
// Approaches to explore:
// - Sliding window with deque (implemented in main)
// - Token bucket algorithm
// - Leaky bucket algorithm
```

**Current Implementation**: Sliding window (lines 30-104)
**Learning**: Rate limiting algorithms, time-based data structures

**Exercise Variations**:
1. Implement token bucket as alternative
2. Add burst allowance feature
3. Implement distributed rate limiting (Redis-based)

---

## Chapter 5: JSON Parsing - Naive Implementation
**Goal**: Parse JSON metrics (simple but slow)

### TODO #9: Naive JSON Parser
**Location**: `src/ingestion_service.cpp:524-566`
**Status**: ✅ Implemented (Example - O(n²) version)
**Difficulty**: Medium

```cpp
// TODO(human): Implement simple JSON parser for metrics
// Requirements:
// 1. Find "metrics" array using string::find()
// 2. Extract individual metric objects
// 3. Parse name, value, type fields
// 4. Handle tags object if present
//
// Note: This is the SIMPLE version - it will be slow!
// We'll optimize it in Chapter 6
```

**Learning**: String manipulation, JSON structure

---

## Chapter 6: JSON Parsing Optimization (Phase 3)
**Goal**: Optimize JSON parser from O(n²) to O(n)

### TODO #10: Single-Pass JSON Parser ⭐
**Location**: `src/ingestion_service.cpp:340-522`
**Status**: ✅ Implemented (Example)
**Difficulty**: Hard

```cpp
// TODO(human): Optimize JSON parser to single-pass
// Requirements:
// 1. Use state machine for parsing (enum ParseState)
// 2. Single character iteration (no string::find loops)
// 3. Pre-allocate buffers to reduce allocations
// 4. Use strtod() for direct numeric parsing
// 5. Handle escape sequences in strings
//
// Performance target: 100 clients at 80%+ success rate
```

**Learning**: Algorithm optimization, state machines, memory allocation

**Optimization Steps**:
1. Eliminate multiple `string::find()` calls
2. Single-pass character iteration
3. Reduce string allocations
4. Direct numeric conversion

**Performance Impact**: 59% → 80.2% success rate at 100 clients

---

## Chapter 7: File I/O - Synchronous Version
**Goal**: Store metrics to file (blocking I/O)

### TODO #11: Synchronous File Writer
**Location**: `src/ingestion_service.cpp:671-719`
**Status**: ✅ Implemented (Example - Phase 1 version)
**Difficulty**: Easy

```cpp
// TODO(human): Write metrics to JSONL file
// Requirements:
// 1. Open metrics.jsonl in append mode
// 2. Format metric as JSON line
// 3. Add timestamp in ISO format
// 4. Flush after each batch
// 5. Use mutex for thread safety
//
// Note: This BLOCKS the request thread!
// We'll make it async in Chapter 8
```

**Learning**: File I/O, JSON Lines format, thread safety

**Limitation**: Blocks HTTP thread, reduces throughput

---

## Chapter 8: Async I/O (Phase 2)
**Goal**: Make file I/O non-blocking with producer-consumer pattern

### TODO #12: Async Write Queue ⭐
**Location**: `src/ingestion_service.cpp:729-735`
**Status**: ✅ Implemented (Example)
**Difficulty**: Medium

```cpp
// TODO(human): Implement async write queue
// Requirements:
// 1. Create queue for pending batches
// 2. Protect queue with mutex
// 3. Use condition_variable for notification
// 4. Enqueue batches without blocking
```

**Learning**: Producer-consumer pattern, condition variables

---

### TODO #13: Background Writer Thread ⭐
**Location**: `src/ingestion_service.cpp:737-761`
**Status**: ✅ Implemented (Example)
**Difficulty**: Hard

```cpp
// TODO(human): Implement background writer thread
// Requirements:
// 1. Wait on condition_variable for batches
// 2. Pop batches from queue
// 3. Release lock before file I/O
// 4. Handle shutdown signal gracefully
// 5. Process all remaining batches on shutdown
```

**Learning**: Background threads, graceful shutdown, lock/unlock patterns

**Performance Impact**: 59% → 66% success rate at 50 clients

---

## Chapter 9: Mutex Contention (Phase 4)
**Goal**: Reduce mutex contention in rate limiting

### TODO #14: Per-Client Mutex (Naive)
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Medium

```cpp
// TODO(human): Add per-client mutex to reduce contention
// Requirements:
// 1. Store mutex in client_requests_ map
// 2. Lock only the specific client's mutex
// 3. Avoid global lock bottleneck
//
// Problem: This creates a NEW deadlock in flush_metrics()!
// (You'll discover this when testing)
```

**Learning**: Mutex granularity, deadlock discovery

**Expected Result**: Performance improves but flush_metrics() deadlocks

---

### TODO #15: Hash-Based Mutex Pool ⭐
**Location**: `src/ingestion_service.cpp:22-28`
**Status**: ✅ Implemented (Example)
**Difficulty**: Hard

```cpp
// TODO(human): Fix deadlock with hash-based mutex pool
// Requirements:
// 1. Create array of mutexes (use prime number size: 10007)
// 2. Hash client_id to select mutex
// 3. Multiple clients share same mutex (okay!)
// 4. No need to lock for client lookup in flush_metrics()
//
// Why this works: No per-client mutex ownership = no deadlock
```

**Learning**: Deadlock avoidance, hash-based sharding, lock-free reads

**Performance Impact**: Eliminates flush_metrics() deadlock

---

## Chapter 10: Lock-Free Programming (Phase 5)
**Goal**: Eliminate locks in metrics collection

### TODO #16: Lock-Free Ring Buffer ⭐
**Location**: `include/ingestion_service.h:41-46`
**Status**: ✅ Implemented (Example)
**Difficulty**: Expert

```cpp
// TODO(human): Implement lock-free ring buffer for metrics
// Requirements:
// 1. Use std::atomic for write_index and read_index
// 2. Fixed-size array (BUFFER_SIZE = 1000)
// 3. Single-writer (allow_request) / single-reader (flush_metrics)
// 4. Use memory_order_release for writes
// 5. Use memory_order_acquire for reads
//
// Memory ordering:
// - Release ensures buffer write visible before index update
// - Acquire ensures we see all writes before read_index
```

**Learning**: Atomics, memory ordering, lock-free algorithms

---

### TODO #17: Lock-Free Metrics Write ⭐
**Location**: `src/ingestion_service.cpp:92-103`
**Status**: ✅ Implemented (Example)
**Difficulty**: Hard

```cpp
// TODO(human): Write to ring buffer without locks
// Requirements:
// 1. Load write_index with relaxed ordering
// 2. Write event to ring_buffer[idx % BUFFER_SIZE]
// 3. Store updated write_index with release ordering
// 4. No mutex needed!
```

**Learning**: Lock-free writes, ring buffer indexing

---

### TODO #18: Lock-Free Metrics Read ⭐
**Location**: `src/ingestion_service.cpp:106-151`
**Status**: ✅ Implemented (Example)
**Difficulty**: Hard

```cpp
// TODO(human): Read from ring buffer without locks
// Requirements:
// 1. Load read_index and write_index with acquire ordering
// 2. Process events from read_index to write_index
// 3. Update read_index with release ordering
// 4. Handle wraparound correctly (modulo BUFFER_SIZE)
//
// Challenge: How to safely iterate client_metrics_ map?
// Solution: Take snapshot of client IDs with temporary lock
```

**Learning**: Lock-free reads, safe map iteration

**Performance Impact**: Eliminates lock contention in metrics collection

---

## Chapter 11: Thread Pool (Phase 6)
**Goal**: Replace thread-per-request with thread pool

### TODO #19: Thread Pool Implementation ⭐
**Location**: `src/thread_pool.cpp:6-37`
**Status**: ✅ Implemented (Example)
**Difficulty**: Hard

```cpp
// TODO(human): Implement fixed-size thread pool
// Requirements:
// 1. Pre-create worker threads (default: 16)
// 2. Task queue with mutex protection
// 3. Condition variable for task notification
// 4. Graceful shutdown (wait for tasks to complete)
```

**Learning**: Thread pool pattern, worker threads

---

### TODO #20: Task Enqueuing with Backpressure ⭐
**Location**: `src/thread_pool.cpp:39-59`
**Status**: ✅ Implemented (Example)
**Difficulty**: Medium

```cpp
// TODO(human): Enqueue tasks with backpressure
// Requirements:
// 1. Check queue size before adding task
// 2. Return false if queue is full (reject request)
// 3. Notify one worker thread
// 4. Handle shutdown gracefully
```

**Learning**: Backpressure, bounded queues

---

### TODO #21: Worker Loop ⭐
**Location**: `src/thread_pool.cpp:66-103`
**Status**: ✅ Implemented (Example)
**Difficulty**: Hard

```cpp
// TODO(human): Implement worker thread loop
// Requirements:
// 1. Wait on condition_variable for tasks
// 2. Check shutdown flag and queue state
// 3. Execute task outside the lock
// 4. Handle exceptions gracefully
```

**Learning**: Worker pattern, exception safety

---

### TODO #22: Integrate Thread Pool with HTTP Server ⭐
**Location**: `src/http_server.cpp:87-115`
**Status**: ✅ Implemented (Example)
**Difficulty**: Medium

```cpp
// TODO(human): Replace thread-per-request with thread pool
// Requirements:
// 1. Enqueue request handling to thread pool
// 2. Handle backpressure (return 503 if queue full)
// 3. Close socket after response in worker thread
// 4. No more std::thread creation per request!
```

**Learning**: Integration, backpressure handling

**Performance Impact**: Eliminates thread creation overhead

---

## Chapter 12: Advanced Rate Limiting
**Goal**: Implement alternative rate limiting algorithms

### TODO #23: Token Bucket Algorithm
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Hard

```cpp
// TODO(human): Implement token bucket rate limiter
// Requirements:
// 1. Bucket refills at constant rate
// 2. Allow burst up to bucket capacity
// 3. Deduct tokens on request
// 4. Use atomics for lock-free implementation
//
// Advantages over sliding window:
// - Simpler logic (no deque)
// - Allows controlled bursts
// - Better cache locality
```

**Learning**: Alternative rate limiting, burst handling

---

### TODO #24: Distributed Rate Limiting
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Expert

```cpp
// TODO(human): Implement distributed rate limiting with Redis
// Requirements:
// 1. Store client counts in Redis
// 2. Use INCR with EXPIRE for sliding window
// 3. Handle Redis connection failures
// 4. Local fallback for resilience
//
// Challenge: How to handle network latency?
```

**Learning**: Distributed systems, external dependencies

---

## Chapter 13: Performance Measurement
**Goal**: Measure and understand bottlenecks

### TODO #25: Add Latency Tracking
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Medium

```cpp
// TODO(human): Track request latency percentiles
// Requirements:
// 1. Record start/end time per request
// 2. Calculate p50, p95, p99 latencies
// 3. Use histogram for distribution
// 4. Expose via /metrics endpoint
```

**Learning**: Performance metrics, percentiles

---

### TODO #26: CPU Profiling Integration
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Hard

```cpp
// TODO(human): Add CPU profiling hooks
// Requirements:
// 1. Integrate with perf or gperftools
// 2. Sample call stacks periodically
// 3. Generate flamegraphs
// 4. Identify hot paths
```

**Learning**: Profiling tools, performance analysis

---

## Chapter 14: Testing & Load Generation
**Goal**: Build comprehensive test suite

### TODO #27: Unit Tests for Rate Limiter
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Medium

```cpp
// TODO(human): Write unit tests for rate limiting
// Test cases:
// 1. Allow requests under limit
// 2. Block requests over limit
// 3. Sliding window cleanup
// 4. Multi-client isolation
// 5. Concurrent access safety
```

**Learning**: Unit testing, concurrency testing

---

### TODO #28: Deadlock Detector
**Location**: `deadlock_test.cpp`
**Status**: ✅ Implemented (Example)
**Difficulty**: Hard

```cpp
// TODO(human): Write test to detect flush_metrics() deadlock
// Requirements:
// 1. Simulate high request rate
// 2. Call flush_metrics() concurrently
// 3. Detect if system hangs
// 4. Use timeout for deadlock detection
```

**Learning**: Deadlock testing, timeout patterns

---

### TODO #29: Load Test Framework
**Location**: `load_test.cpp`
**Status**: ✅ Implemented (Example)
**Difficulty**: Medium

```cpp
// TODO(human): Build load testing framework
// Requirements:
// 1. Spawn N concurrent clients
// 2. Send M requests per client
// 3. Track success/failure counts
// 4. Calculate average latency
// 5. Report results in CSV format
```

**Learning**: Load testing, concurrent clients

---

## Chapter 15: Production Readiness
**Goal**: Prepare for production deployment

### TODO #30: Graceful Shutdown
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Medium

```cpp
// TODO(human): Implement graceful shutdown
// Requirements:
// 1. Stop accepting new connections
// 2. Wait for in-flight requests to complete
// 3. Flush all pending writes
// 4. Close file handles cleanly
// 5. Handle SIGTERM/SIGINT
```

**Learning**: Signal handling, graceful shutdown

---

### TODO #31: Configuration Management
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Easy

```cpp
// TODO(human): Add configuration file support
// Requirements:
// 1. Load config from YAML/JSON
// 2. Configure port, rate limit, thread pool size
// 3. Support environment variable overrides
// 4. Validate configuration on startup
```

**Learning**: Configuration patterns, validation

---

### TODO #32: Structured Logging
**Status**: ❌ Not Implemented (Learner Exercise)
**Difficulty**: Medium

```cpp
// TODO(human): Replace cout with structured logging
// Requirements:
// 1. Use spdlog or similar library
// 2. Log levels (DEBUG, INFO, WARN, ERROR)
// 3. JSON structured format
// 4. Log rotation support
// 5. Performance metrics logging
```

**Learning**: Logging best practices, structured data

---

### TODO #33: Health Check Endpoint
**Location**: `src/ingestion_service.cpp:318-323`
**Status**: ✅ Implemented (Example)
**Difficulty**: Easy

```cpp
// TODO(human): Enhance health check endpoint
// Requirements:
// 1. Check file write capability
// 2. Verify thread pool health
// 3. Report queue depth
// 4. Return 503 if unhealthy
```

**Learning**: Health checks, monitoring

---

## Bonus: Advanced Exercises

### TODO #34: Zero-Copy Networking
**Status**: ❌ Not Implemented (Advanced Exercise)
**Difficulty**: Expert

```cpp
// TODO(human): Implement zero-copy with sendfile()
// Requirements:
// 1. Use sendfile() for file responses
// 2. Avoid copying data to userspace
// 3. Measure performance improvement
```

**Learning**: Zero-copy I/O, kernel bypass

---

### TODO #35: SIMD JSON Parsing
**Status**: ❌ Not Implemented (Advanced Exercise)
**Difficulty**: Expert

```cpp
// TODO(human): Use SIMD for JSON parsing
// Requirements:
// 1. Integrate simdjson library
// 2. Vectorized quote/bracket scanning
// 3. Compare performance with custom parser
```

**Learning**: SIMD, vectorization

---

### TODO #36: eBPF Performance Tracing
**Status**: ❌ Not Implemented (Advanced Exercise)
**Difficulty**: Expert

```cpp
// TODO(human): Add eBPF tracing hooks
// Requirements:
// 1. Use bpftrace for latency tracking
// 2. Trace lock contention
// 3. Monitor system call overhead
```

**Learning**: eBPF, kernel tracing

---

## Summary Statistics

**Total TODOs**: 36
**Implemented (Examples)**: 20
**Active Learner Exercises**: 16

**By Difficulty**:
- Easy: 5
- Medium: 12
- Hard: 14
- Expert: 5

**By Phase**:
- Foundation (Ch 1-3): 7 TODOs
- Core Features (Ch 4-8): 9 TODOs
- Optimizations (Ch 9-11): 9 TODOs
- Advanced (Ch 12-15): 11 TODOs

**Recommended Learning Order**:
1. Start with Chapters 1-7 (basics)
2. Implement Chapter 8 (async I/O) - first major optimization
3. Tackle Chapter 10 (lock-free) - hardest but most valuable
4. Add Chapter 11 (thread pool) - final core optimization
5. Choose advanced topics based on interest

---

## Next Steps for Learners

1. **Fork the repository**
2. **Choose a starting TODO** (recommend #8 or #12)
3. **Implement and test** with load_test
4. **Measure performance** improvement
5. **Document learnings** in performance_results.txt
6. **Move to next TODO**

For each TODO, follow the cycle:
```
Measure → Implement → Test → Measure → Document
```

This is the path to principal engineer-level systems mastery!
