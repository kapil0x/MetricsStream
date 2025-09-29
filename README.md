# MetricStream

A comprehensive metrics platform built from first principles - like Prometheus or DataDog, but designed for learning the underlying engineering. Currently focused on the ingestion and rate limiting subsystem.

## What's Here

**Current Component: Metrics Ingestion Service**
- HTTP server from raw sockets
- Rate limiting with sliding windows  
- Custom JSON parser (zero dependencies)
- Concurrent request processing
- Performance measurement and optimization

Currently handles ~200 RPS. Working toward 10K RPS through systematic optimization.

**Planned Components:**
- Time-series storage engine
- Query processing and aggregation
- Alerting and notification system
- Visualization and dashboards
- Distributed architecture for horizontal scaling

## Data Flow

```
HTTP Request → Rate Limiting → JSON Parsing → File Storage
```

## Build

```bash
mkdir build && cd build
cmake .. && make
./metricstream_server
```

```bash
# Test it
./test_client
```

## Current Status

- Sequential request processing
- File-based storage (JSON Lines)
- Per-client rate limiting
- ~200 RPS throughput

Optimization journey documented in commits.

## API

```bash
# Send metrics
curl -X POST http://localhost:8080/metrics \
  -H "Content-Type: application/json" \
  -H "Authorization: client_id" \
  -d '{"metrics":[{"name":"cpu_usage","value":75.5,"type":"gauge"}]}'

# Check stats  
curl http://localhost:8080/metrics
```

## Key Implementation Details

**Rate Limiting**: Sliding window per client
```cpp
while (!client_queue.empty() && now - client_queue.front() >= 1s) {
    client_queue.pop_front();
}
if (client_queue.size() < max_requests_) {
    client_queue.push_back(now);
    return true;
}
```

**Metrics Collection**: Lock-free ring buffer
```cpp
size_t idx = write_index.load();
ring_buffer[idx % BUFFER_SIZE] = event;
write_index.store(idx + 1);
```

## Performance Optimization Journey

**Phase 1**: Threading per request (20 clients: 81% → 88%)
**Phase 2**: Async I/O (50 clients: 59% → 66%)
**Phase 3**: JSON parsing optimization (100 clients: 80.2%)
**Phase 4**: Hash-based per-client locking (current)

Each phase targets a specific bottleneck. Measurements guide the next optimization.

## Load Testing

```bash
./build/load_test 8080 50 10    # 50 clients, 10 requests each
./performance_test.sh           # Systematic RPS testing
```

Results stored in `performance_results.txt`.

## Structure

```
src/
├── http_server.cpp        # Socket handling
├── ingestion_service.cpp  # Rate limiting, JSON parsing  
└── main.cpp              # Entry point

load_test.cpp             # Performance testing
```

## Notes

- Requires C++17
- Check `metrics.jsonl` for stored data
- Server logs to stdout
- Current bottleneck: mutex contention (Phase 4 target)

## Contributing

Open issue: `flush_metrics()` needs safe multi-client iteration without deadlocks. See TODO(human) in `src/ingestion_service.cpp`.