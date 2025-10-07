# Systems Craft → Craft #1: Metrics Ingestion

**A hands-on tutorial for building a metrics ingestion system from scratch.** Learn concurrent programming and performance optimization by building a production-grade metrics ingestion service (like Prometheus or DataDog) through **7 progressive optimization phases** - from sequential baseline to 2,253 RPS with 100% reliability.

> **Systems Craft** is a series of hands-on tutorials teaching systems engineering through building real infrastructure from first principles. This is the first craft.

**What you'll build:**
- HTTP server from raw Berkeley sockets
- Lock-free ring buffers with atomics
- Thread pool architecture
- Custom JSON parser (zero dependencies)
- Sliding window rate limiting
- HTTP Keep-Alive persistent connections

**Performance achieved:** 2,253 RPS sustained, p50 = 0.25ms latency, 100% success rate with 100 concurrent clients.

## Architecture Overview

### What's Built (Phase 1: Ingestion Service)

**Current measured performance:**
- **Throughput**: 2,253 RPS sustained (100% success rate)
- **Latency**: p50 = 0.25ms, p99 = 0.65ms
- **Concurrency**: 100 concurrent clients, HTTP Keep-Alive
- **Resource usage**: 45% CPU, 150MB memory per server

**Implementation from first principles:**
- HTTP server from raw Berkeley sockets (`socket()`, `bind()`, `listen()`, `accept()`)
- Thread pool with 16 worker threads
- Lock-free ring buffer for metrics collection (`std::atomic` with memory ordering)
- Hash-based mutex pool (64 mutexes) for per-client rate limiting
- Custom O(n) JSON parser (zero dependencies, optimized from O(n²) regex)
- Sliding window rate limiting (10 req/sec per client)
- HTTP Keep-Alive persistent connections (60s timeout)

**Current data flow:**
```
HTTP Request → Thread Pool → Rate Limiting → JSON Parsing → JSONL File
```

### What's Designed (Phases 2-5: Production Platform)

**Target capacity:** 100K writes/sec from 10K monitored servers
**Estimated cost:** ~$924/month AWS (optimized from $6,070/month baseline)

**Full system architecture:**
```
Monitoring Agents (10K servers)
         ↓
Ingestion Service (10 servers @ 10K RPS each)
         ↓
Apache Kafka (3 brokers, 12 partitions, client_id hash routing)
         ↓
Apache Flink (stream processing, 12 workers, 1-second tumbling windows)
         ↓
InfluxDB Cluster (3 nodes, 30-day retention, 1-year downsampling)
         ↓
Grafana Dashboards (pre-aggregated queries)
```

**See detailed documentation:**
- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md) - Complete system design with component comparisons
- [`docs/DESIGN_DECISIONS.md`](docs/DESIGN_DECISIONS.md) - Architecture Decision Records (ADRs)
- [`docs/CAPACITY_PLANNING.md`](docs/CAPACITY_PLANNING.md) - Scaling analysis with AWS costs

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

Each phase targets a measured bottleneck with systematic improvements:

| Phase | Optimization | Result @ 100 clients |
|-------|-------------|---------------------|
| **Phase 1** | Threading per request | 20 clients: 81% → 88% success |
| **Phase 2** | Async I/O with producer-consumer | 50 clients: 59% → 66% success |
| **Phase 3** | JSON parsing (O(n²)→O(n)) | 80.2% success, 2.73ms latency |
| **Phase 4** | Hash-based per-client mutex | 95%+ success (reduced contention) |
| **Phase 5** | Thread pool architecture | 100% success, 0.65ms latency |
| **Phase 6** | Lock-free ring buffer | Eliminated metrics collection overhead |
| **Phase 7** | HTTP Keep-Alive | **100% success, 0.25ms latency, 2,253 RPS** |

**Key breakthrough (Phase 7):** Increased listen backlog from 10 → 1024 to handle concurrent connection bursts. Combined with HTTP Keep-Alive, this eliminated TCP handshake overhead (was 1-2ms per request) and achieved 100% success rate at target load.

See [`docs/phase7_keep_alive_results.md`](docs/phase7_keep_alive_results.md) for detailed analysis.

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
- Phase 1 optimization complete (Phase 7: HTTP Keep-Alive)
- See `/docs` for full system architecture and design decisions