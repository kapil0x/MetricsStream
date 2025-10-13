# Phase 0: Monitoring Platform Proof of Concept

**Goal:** Build a complete end-to-end monitoring platform in 2-3 hours to understand how distributed systems components connect.

## What You'll Build

A working monitoring platform with all 5 components:

1. **Ingestion API** - Accept metrics via HTTP POST
2. **In-Memory Queue** - Thread-safe buffer between ingestion and storage
3. **Storage Consumer** - Write metrics to file
4. **Query API** - Read and filter metrics via HTTP GET
5. **Alerting Engine** - Evaluate rules and trigger alerts

## Architecture

```
┌─────────────┐
│   Clients   │ (send metrics)
└──────┬──────┘
       │ POST /metrics
       ▼
┌─────────────────────┐
│ Component 1:        │
│ Ingestion API       │ (HTTP server, single-threaded)
└──────┬──────────────┘
       │ push
       ▼
┌─────────────────────┐
│ Component 2:        │
│ In-Memory Queue     │ (std::queue + mutex)
└──────┬──────────────┘
       │ pop
       ▼
┌─────────────────────┐
│ Component 3:        │
│ Storage Consumer    │ (background thread → file)
└──────┬──────────────┘
       │ write
       ▼
   metrics.jsonl
       │
       ├──────────────────┐
       ▼                  ▼
┌─────────────┐    ┌─────────────┐
│ Component 4:│    │ Component 5:│
│ Query API   │    │ Alerting    │
└─────────────┘    └─────────────┘
  GET /query         (checks rules)
```

## Quick Start

### 1. Build

```bash
cd phase0
./build.sh
```

### 2. Run the Server

```bash
cd build
./phase0_poc
```

You should see:
```
=======================================
Phase 0: Monitoring Platform PoC
=======================================

[Storage] Consumer started, writing to phase0_metrics.jsonl
[Alerting] Added rule: cpu_usage > 80.0 (window: 60s)
[Alerting] Added rule: memory_usage > 90.0 (window: 60s)
[Alerting] Added rule: error_rate > 5.0 (window: 30s)
[Alerting] Engine started, checking every 10 seconds

[Ingestion] Server started on port 8080
[Ingestion] Endpoints:
  POST /metrics - Ingest metric
  GET  /query?name=<name>&start=<ts>&end=<ts> - Query metrics
  GET  /health - Health check
```

### 3. Run the Demo (in another terminal)

```bash
cd phase0
./demo.sh
```

This will:
- Ingest sample metrics (CPU, memory, error rate)
- Query metrics by name and time range
- Trigger alerts when thresholds are exceeded
- Show the complete end-to-end flow

## Manual Testing

### Ingest Metrics

```bash
# Ingest CPU usage
curl -X POST http://localhost:8080/metrics \
     -H "Content-Type: application/json" \
     -d '{"name":"cpu_usage","value":85}'

# Ingest memory usage
curl -X POST http://localhost:8080/metrics \
     -H "Content-Type: application/json" \
     -d '{"name":"memory_usage","value":75}'

# Ingest custom metric
curl -X POST http://localhost:8080/metrics \
     -H "Content-Type: application/json" \
     -d '{"name":"request_latency","value":125.5}'
```

### Query Metrics

```bash
# Get current timestamp (milliseconds)
NOW=$(date +%s)000
START=$((NOW - 300000))  # 5 minutes ago

# Query CPU usage for last 5 minutes
curl "http://localhost:8080/query?name=cpu_usage&start=$START&end=$NOW"

# Query memory usage
curl "http://localhost:8080/query?name=memory_usage&start=$START&end=$NOW"
```

### Health Check

```bash
curl http://localhost:8080/health
# Returns: {"status":"healthy","queue_size":0}
```

## Understanding the Code

### Component 1: Ingestion API (lines 415-587)

**Key Design Decisions:**
- **Single-threaded:** Handles one request at a time (blocking)
- **Simple HTTP parsing:** String manipulation, no dependencies
- **Immediate queuing:** Push metric to queue and return 202 Accepted

**Why it's slow:**
- `accept()` blocks until request arrives
- Each request processed sequentially
- No connection pooling or keep-alive

**What Craft #1 adds:**
- Thread pool for concurrent requests
- Rate limiting per client
- Connection keep-alive
- **Result:** 200 RPS → 2,253 RPS

### Component 2: In-Memory Queue (lines 67-95)

**Key Design Decisions:**
- **Thread-safe:** `std::mutex` protects queue operations
- **Unbounded:** No size limit (can grow indefinitely)
- **Non-blocking push:** Producer never waits

**Why it's simple:**
- In-memory only (lost on crash)
- No backpressure mechanism
- Single process (not distributed)

**What Craft #2 adds:**
- Persistent write-ahead log
- Partitioning for parallelism
- Replication for durability
- **Result:** Kafka-like distributed queue

### Component 3: Storage Consumer (lines 99-152)

**Key Design Decisions:**
- **Background thread:** Polls queue and writes to file
- **Append-only:** Simple file I/O, no indexing
- **JSON Lines format:** One metric per line, human-readable

**Why it's simple:**
- Blocking file writes (no async I/O)
- No batching or buffering
- Linear scan for queries (no indexes)

**What Craft #3 adds:**
- LSM tree storage engine
- Time-series compression (Gorilla)
- Tag-based indexing
- **Result:** InfluxDB-like time-series database

### Component 4: Query API (lines 156-234)

**Key Design Decisions:**
- **Full file scan:** Reads entire file for every query
- **In-memory filtering:** Load all data, then filter
- **Simple JSON parsing:** String manipulation

**Why it's slow:**
- No indexes (O(n) scan)
- No query planning or optimization
- Single-threaded execution

**What Craft #4 adds:**
- Inverted indexes for fast lookups
- Parallel query execution
- Aggregation functions (sum, avg, percentiles)
- **Result:** Query 100M+ data points in <1 second

### Component 5: Alerting Engine (lines 238-332)

**Key Design Decisions:**
- **Polling model:** Check rules every 10 seconds
- **Aggregation-based:** Evaluate avg() over time window
- **Console alerts:** Print to stdout (no notifications)

**Why it's simple:**
- No alert state tracking
- No deduplication or grouping
- No notification channels (email, Slack, PagerDuty)

**What Craft #5 adds:**
- Event-driven evaluation (real-time)
- Alert lifecycle management
- Multi-channel notifications
- **Result:** <1 min detection latency, 99.99% delivery

## Data Flow Example

Let's trace a metric through the system:

```
1. Client sends: POST /metrics {"name":"cpu_usage","value":85}
   → IngestionServer::handle_client() parses request

2. Ingestion creates Metric object with timestamp
   → Metric("cpu_usage", 85.0, 1696789234567)

3. Push to queue: g_metric_queue.push(metric)
   → std::queue with mutex protection

4. Storage consumer pops from queue
   → StorageConsumer::worker() calls try_pop()

5. Write to file: metrics.jsonl
   → {"name":"cpu_usage","value":85,"timestamp":1696789234567}

6. Query reads file and filters
   → QueryEngine::query("cpu_usage", start, end)

7. Alerting evaluates rules
   → AlertingEngine::evaluate_rule() checks avg(cpu_usage) > 80

8. Alert triggered!
   → Print: 🚨 [ALERT] cpu_usage avg(60s) > 80.0 (current: 85.0)
```

## Key Learnings

### 1. Component Boundaries
- Each component has clear inputs/outputs
- Queue decouples producers from consumers
- APIs define contracts between components

### 2. Simplicity First
- Single-threaded ingestion is easy to understand
- Simple file storage validates architecture
- Optimization comes later (Crafts #1-5)

### 3. End-to-End Thinking
- Metrics flow through entire pipeline
- Each component depends on previous ones
- Complete system > optimized parts

### 4. Measurement Matters
- Health endpoint shows queue depth
- Alerts show system is working
- Bottlenecks become obvious (single-threaded ingestion)

## Performance Characteristics

**Expected throughput:**
- ~50-100 requests/second (single-threaded)
- Latency: 10-20ms per request
- Storage: Limited by disk I/O speed

**Bottlenecks (by design):**
1. Single-threaded ingestion (biggest bottleneck)
2. Blocking file writes
3. Full file scan for queries
4. Polling-based alerting

**Why these bottlenecks exist:**
- Educational - students see them immediately
- Motivate optimization in Crafts #1-5
- Simple code is easier to understand first

## Common Issues

### Port already in use
```bash
# Kill existing process
lsof -ti:8080 | xargs kill -9

# Or use different port
./phase0_poc 8081
```

### File permission errors
```bash
# Make sure you have write permissions
chmod 644 phase0_metrics.jsonl
```

### Alert not triggering
- Wait at least 10 seconds (check interval)
- Ensure enough data points in time window
- Check metric names match exactly

## Next Steps

### Immediate experiments:
1. Send 100 metrics rapidly - see ingestion bottleneck
2. Query with different time ranges - see linear scan slowness
3. Trigger alerts by sending high values - see evaluation delay

### Move to Craft #1:
Once you understand the architecture, optimize ingestion:
- Add threading per request
- Implement rate limiting
- Add async I/O pipeline
- **Result:** 200 RPS → 2,253 RPS (10x improvement)

### Future Crafts:
- **Craft #2:** Replace in-memory queue with distributed message queue
- **Craft #3:** Replace file with time-series database
- **Craft #4:** Add parallel query execution and aggregations
- **Craft #5:** Add real-time alerting with notifications

## Files Generated

```
phase0/
├── phase0_poc.cpp           # Main implementation (all components)
├── CMakeLists.txt           # Build configuration
├── build.sh                 # Build script
├── demo.sh                  # Demo script
├── README.md                # This file
├── build/
│   └── phase0_poc           # Compiled binary
└── phase0_metrics.jsonl     # Stored metrics (generated at runtime)
```

## Summary

**What you built:**
- ✅ Complete monitoring platform (5 components)
- ✅ HTTP API for ingestion and queries
- ✅ Thread-safe queue for buffering
- ✅ Persistent storage in JSON Lines format
- ✅ Alert engine with threshold evaluation

**What you learned:**
- ✅ How distributed systems components connect
- ✅ Producer-consumer pattern with queues
- ✅ HTTP API design for metrics ingestion
- ✅ Time-series data storage and querying
- ✅ Rule-based alerting architecture

**Time invested:** 2-3 hours

**Next craft:** Optimize ingestion (8-12 hours → 10x performance)

---

**Questions or issues?** Open an issue on GitHub or check the main documentation.
