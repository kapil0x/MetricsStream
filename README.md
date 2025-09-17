# MetricStream

A learning-focused metrics collection system built in C++ to demonstrate distributed systems concepts. **Think Datadog, New Relic, or Prometheus** - but built from scratch to teach the underlying engineering patterns.

This repository shows how companies like **Netflix, Uber, and Airbnb** handle millions of metrics per second through hands-on implementation of real-world distributed systems patterns.

## 🎯 What We're Building

**A production-style metrics platform** similar to industry leaders:

| Real-World Example | What They Handle | Our Learning Implementation |
|---|---|---|
| **Netflix** | 2.5 billion metrics/day | Sliding window rate limiting + ring buffers |
| **Uber** | 100M+ events/second | Thread pools → Async I/O → Horizontal scaling |
| **Datadog** | Custom time-series DB | File storage → ClickHouse integration |
| **Prometheus** | Pull-based monitoring | HTTP API with JSON metrics format |

## 🎯 Learning Goals

This project demonstrates the core patterns behind these systems:
- **HTTP Server Implementation** - Raw socket programming (like nginx internals)
- **Rate Limiting Algorithms** - Sliding window (used by Twitter, GitHub APIs)
- **Concurrent Programming** - Thread safety patterns (Go/Rust-style concurrency)
- **System Design** - Layered architecture (microservices at scale)
- **JSON Processing** - Custom parser (zero-dependency approach)
- **Performance Engineering** - 200 RPS → 10K RPS scaling journey

## 📋 Architecture

For detailed architecture documentation, see: **[CLAUDE.md](CLAUDE.md)**

**High-level data flow:**
```
Client Apps → HTTP API → Rate Limiter → JSON Parser → Storage → Query Engine
```

## Quick Start

### Build and Run
```bash
git clone https://github.com/kapil0x/Metrics.git
cd Metrics
mkdir build && cd build
cmake .. && make
./metricstream_server
```

### Test the System
```bash
# In another terminal
./test_client

# Check stored metrics
cat metrics.jsonl
```

## Current Implementation

**What's Built (Production Patterns):**
- **HTTP Server** - Raw socket implementation (like nginx/Apache internals)
- **Rate Limiting** - Sliding window algorithm (used by Twitter/GitHub APIs) 
- **JSON Processing** - Custom parser (zero dependencies, like embedded systems)
- **Storage Layer** - JSON Lines format (similar to log aggregators)
- **Monitoring** - Statistics endpoints (like Prometheus /metrics)

**Real-World Comparisons:**
- **Current**: ~200 RPS (small startup scale)
- **Target**: 10K RPS (mid-size company scale - like Reddit, Slack)
- **Industry**: 100K+ RPS (Netflix, Uber, Google scale)

**Why This Matters:**
- **Startups** often begin with simple metrics collection (our MVP)
- **Scale-ups** need rate limiting and concurrent processing (Phase 2)
- **Enterprises** require distributed systems and horizontal scaling (Phase 3+)

## API Examples

### Submit Metrics
```bash
curl -X POST http://localhost:8080/metrics \
  -H "Content-Type: application/json" \
  -H "Authorization: my_client_id" \
  -d '{
    "metrics": [
      {
        "name": "cpu_usage",
        "value": 75.5,
        "type": "gauge",
        "tags": {"host": "server1"}
      }
    ]
  }'
```

### Check Statistics
```bash
curl http://localhost:8080/metrics
# Returns: {"metrics_received":27,"batches_processed":9,"validation_errors":0,"rate_limited_requests":0}
```

## Architecture Patterns Demonstrated

### 1. Sequential Request Processing
- Single-threaded HTTP server
- Simple but limited throughput
- Easy to understand and debug
- Clear upgrade path to concurrent processing

### 2. Sliding Window Rate Limiting
```cpp
// Remove old timestamps (older than 1 second)
while (!client_queue.empty()) {
    auto oldest = client_queue.front();
    if (now - oldest >= 1_second) {
        client_queue.pop_front();
    } else break;
}

// Check if under limit
if (client_queue.size() < max_requests_) {
    client_queue.push_back(now);
    return true; // Allow request
}
```

### 3. Ring Buffer Metrics Collection
```cpp
struct ClientMetrics {
    std::array<MetricEvent, 1000> ring_buffer;
    std::atomic<size_t> write_index{0};
    std::atomic<size_t> read_index{0};
};

// Lockless write
size_t idx = write_index.load();
ring_buffer[idx % 1000] = event;
write_index.store(idx + 1);
```

### 4. Custom JSON Parser
- No external dependencies
- Demonstrates string parsing techniques
- Handles nested objects and arrays
- Error handling and validation

## Scaling to 10K RPS

**Current State**: 200 RPS (sequential processing)  
**Target**: 10,000 RPS

### Scaling Options:

**Option A: Thread Pool (Single Machine)**
```cpp
ThreadPool pool(100);  // 100 worker threads
while (running_) {
    int client = accept(server_fd, ...);
    pool.submit([client]() {
        process_request(client);  // Each thread handles ~100 RPS
    });
}
```
- **Capacity**: 100 threads × 100 RPS = 10K RPS ✅
- **Hardware**: 8-16 cores, 16GB RAM
- **Pros**: Single machine, easier deployment
- **Cons**: Thread overhead, context switching

**Option B: Async I/O (Single Machine)**
```cpp
int epoll_fd = epoll_create1(0);
while (running_) {
    int events = epoll_wait(epoll_fd, events_buf, MAX_EVENTS, -1);
    for (int i = 0; i < events; ++i) {
        handle_event_non_blocking(events_buf[i]);
    }
}
```
- **Capacity**: Single thread handling 10K+ connections ✅
- **Hardware**: 4-8 cores, 8GB RAM
- **Pros**: Lower memory, highest performance
- **Cons**: Complex programming model

**Option C: Horizontal Scaling (Multiple Machines)**
```
Load Balancer → [Server1: 2K RPS] 
              → [Server2: 2K RPS]
              → [Server3: 2K RPS] 
              → [Server4: 2K RPS]
              → [Server5: 2K RPS]
              = 10K RPS total
```
- **Capacity**: 5 machines × 2K RPS each = 10K RPS ✅
- **Hardware**: 5 modest servers (4 cores, 8GB each)
- **Pros**: Fault tolerance, easier scaling
- **Cons**: Network complexity, data consistency

### Recommended Learning Path:
1. **Start with Thread Pool** (easiest to implement)
2. **Try Async I/O** (best performance)
3. **Experiment with Distribution** (real-world scaling)

## Learning Path

### Phase 1: Current MVP ✅
- Basic HTTP server
- Rate limiting
- JSON parsing
- File storage

### Phase 2: Concurrency (Target: 10K RPS)
- **Option A**: Thread pool (100 threads) - Single machine
- **Option B**: Async I/O with epoll/kqueue - Single machine  
- **Option C**: Load balancer + multiple servers - Distributed
- Connection pooling and keep-alive
- Performance benchmarking and profiling

### Phase 3: Streaming (Netflix/Uber Scale)
- **Message Queue Integration** (Kafka - like LinkedIn's architecture)
- **Stream Processing** (real-time aggregation - like Twitter's infrastructure) 
- **Event-driven Architecture** (microservices communication)
- **Data Partitioning** (sharding strategies used by Facebook)

### Phase 4: Storage (Datadog/Prometheus Scale)
- **Time-series Database** (ClickHouse - used by Cloudflare, Uber)
- **Data Retention Policies** (automated cleanup - like AWS CloudWatch)
- **Query Optimization** (indexing strategies - like Elasticsearch)
- **Compression** (storage efficiency - like InfluxDB)

### Phase 5: Distribution (Google/Amazon Scale)
- **Horizontal Auto-scaling** (Kubernetes-style orchestration)
- **Load Balancing** (consistent hashing - like Cassandra)
- **Service Discovery** (health checks - like Consul/etcd)
- **Cross-region Replication** (global availability - like AWS)

## Key Files

```
src/
├── http_server.cpp          # Raw HTTP implementation
├── ingestion_service.cpp    # Business logic and JSON parsing
└── main.cpp                 # Server startup

include/
├── http_server.h           # HTTP server interface
├── ingestion_service.h     # Service definitions
└── metric.h                # Data structures

test_client.cpp             # Load testing and validation
CMakeLists.txt              # Build configuration
```

## Development

### Adding Features
1. Fork the repository
2. Create feature branch
3. Implement with tests
4. Update documentation
5. Submit pull request

### Testing Rate Limiting
```cpp
// Test with multiple clients
for (int i = 0; i < 1000; ++i) {
    send_request("client_" + std::to_string(i));
}
```

### Debugging
- Server logs to stdout
- Metrics stored in `metrics.jsonl`
- Statistics available at `/metrics` endpoint

## Common Issues

**Server won't start:**
- Check port 8080 is available: `lsof -i :8080`
- Try different port: `./metricstream_server 8081`

**Low throughput:**
- Current implementation is sequential (by design)
- Expected ~200 RPS for learning purposes
- See Phase 2 for concurrency improvements

**Build errors:**
- Requires C++17 or later
- Check CMake version >= 3.16

## Contributing

This is a learning project! Contributions welcome:
- Implement concurrent processing
- Add more metric types
- Improve JSON parser
- Add benchmarking tools
- Write tutorials

## Resources

- [System Design Primer](https://github.com/donnemartin/system-design-primer)
- [C++ Concurrency in Action](https://www.manning.com/books/c-plus-plus-concurrency-in-action)
- [Designing Data-Intensive Applications](https://dataintensive.net/)
- [High Performance Browser Networking](https://hpbn.co/)