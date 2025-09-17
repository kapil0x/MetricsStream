# MetricStream

A learning-focused metrics collection system built in C++ to demonstrate distributed systems concepts. This repository teaches key patterns like rate limiting, concurrent processing, and system design through hands-on implementation.

## 🎯 Learning Goals

This project demonstrates:
- **HTTP Server Implementation** - Raw socket programming and request parsing
- **Rate Limiting Algorithms** - Sliding window with ring buffer metrics
- **Concurrent Programming** - Thread safety, atomic operations, lockless patterns
- **System Design** - Layered architecture and scaling considerations
- **JSON Processing** - Custom parser implementation without external dependencies
- **File I/O** - Efficient metric storage and retrieval

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

**What's Built:**
- HTTP server with basic routing (`/metrics`, `/health`, `/stats`)
- Sliding window rate limiter (per-client tracking)
- JSON parser for metric data (name, value, type, tags)
- File storage in JSON Lines format
- Test client for validation

**Performance:**
- ~200 requests/second (sequential processing)
- Each request can contain multiple metrics in batches
- Single-threaded MVP design for simplicity

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

## Learning Path

### Phase 1: Current MVP ✅
- Basic HTTP server
- Rate limiting
- JSON parsing
- File storage

### Phase 2: Concurrency
- Thread-per-request model
- Thread pools
- Connection management
- Performance testing

### Phase 3: Streaming
- Message queue integration (Kafka)
- Stream processing
- Event-driven architecture
- Data partitioning

### Phase 4: Storage
- Time-series database (ClickHouse)
- Data retention policies
- Query optimization
- Indexing strategies

### Phase 5: Distribution
- Horizontal scaling
- Load balancing
- Service discovery
- Monitoring and observability

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