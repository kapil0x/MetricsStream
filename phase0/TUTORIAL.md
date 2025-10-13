# Craft #0: Monitoring Platform PoC - Tutorial

**Build Time:** 2-3 hours
**Difficulty:** Beginner
**Prerequisites:** C++17, basic networking knowledge

## Learning Objectives

By the end of this craft, you'll understand:
- How distributed system components connect and communicate
- Producer-consumer pattern with thread-safe queues
- HTTP API design for metrics ingestion and querying
- Time-series data storage patterns
- Rule-based alerting architecture

## What You're Building

A complete monitoring platform with 5 components in a single executable:

```
Client → Ingestion API → Queue → Storage → Query API
                                    ↓
                              Alerting Engine
```

**Important:** This is a *proof of concept*, not production code. It's intentionally simple to show architecture first, optimization later (Crafts #1-5).

## Step 1: Understand the Architecture (15 minutes)

### The 5 Components

1. **Ingestion API** (lines 415-587 in phase0_poc.cpp)
   - HTTP POST /metrics endpoint
   - Single-threaded request handling (blocking)
   - Simple JSON parsing with string operations
   - Immediately pushes to queue

2. **In-Memory Queue** (lines 67-95)
   - `std::queue<Metric>` wrapped with `std::mutex`
   - Thread-safe push/pop operations
   - Unbounded (no backpressure)
   - Lost on crash (no persistence)

3. **Storage Consumer** (lines 99-152)
   - Background thread polling the queue
   - Blocking file writes (no buffering)
   - JSON Lines format (one metric per line)
   - Append-only writes

4. **Query API** (lines 156-234)
   - HTTP GET /query endpoint
   - Full file scan (no indexes)
   - In-memory filtering by name and time range
   - Returns JSON array

5. **Alerting Engine** (lines 238-332)
   - Background thread checking rules periodically
   - Calculates averages over time windows
   - Compares against thresholds
   - Logs alerts to console

### Data Flow

```
1. curl POST /metrics '{"name":"cpu_usage","value":85}'
2. IngestionServer parses JSON → Metric object
3. Push to g_metric_queue (thread-safe)
4. StorageConsumer polls queue
5. Write to phase0_metrics.jsonl
6. QueryEngine reads file on demand
7. AlertingEngine evaluates rules every 10s
8. Alert printed if threshold exceeded
```

**Question to think about:** What happens if ingestion is faster than storage can write?

## Step 2: Build and Run (10 minutes)

### Clone and Build

```bash
cd phase0
./build.sh
```

This creates `build/phase0_poc` executable.

### Start the Server

```bash
cd build
./phase0_poc
```

You should see:
```
[Storage] Consumer started, writing to phase0_metrics.jsonl
[Alerting] Added rule: cpu_usage > 80 (window: 60s)
[Alerting] Engine started, checking every 10 seconds
[Ingestion] Server started on port 8080
```

**All 5 components are now running in separate threads!**

## Step 3: Test Ingestion → Storage Flow (15 minutes)

### Send Metrics

Open a new terminal:

```bash
# Ingest CPU metric
curl -X POST http://localhost:8080/metrics \
     -H "Content-Type: application/json" \
     -d '{"name":"cpu_usage","value":85}'
# Response: {"status":"accepted"}

# Ingest memory metric
curl -X POST http://localhost:8080/metrics \
     -H "Content-Type: application/json" \
     -d '{"name":"memory_usage","value":75}'
```

### Verify Storage

```bash
cat build/phase0_metrics.jsonl
```

You should see:
```json
{"name":"cpu_usage","value":85,"timestamp":1696789234567}
{"name":"memory_usage","value":75,"timestamp":1696789235123}
```

**Key observation:** Metrics flow from ingestion → queue → storage asynchronously. The HTTP response returns immediately (202 Accepted) before storage completes.

### Experiment: Fast Ingestion

```bash
# Send 100 metrics rapidly
for i in {1..100}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d "{\"name\":\"test\",\"value\":$i}" &
done
wait
```

**What do you notice about performance?** This is the bottleneck that Craft #1 fixes with threading.

## Step 4: Test Query API (15 minutes)

### Query by Name and Time Range

```bash
# Get current timestamp (milliseconds)
NOW=$(date +%s)000
START=$((NOW - 300000))  # 5 minutes ago

# Query CPU usage
curl "http://localhost:8080/query?name=cpu_usage&start=$START&end=$NOW"
```

Response:
```json
[
  {"name":"cpu_usage","value":85,"timestamp":1696789234567}
]
```

### Query Different Metrics

```bash
# Query memory usage
curl "http://localhost:8080/query?name=memory_usage&start=$START&end=$NOW"

# Query all metrics (no name filter)
curl "http://localhost:8080/query?name=all&start=0&end=9999999999999"
```

**Key observation:** Query reads the *entire file* every time. No indexes, no caching. This is inefficient but simple.

## Step 5: Test Alerting (20 minutes)

### Trigger an Alert

The server has 3 default rules:
```
1. cpu_usage > 80 (avg over 60s)
2. memory_usage > 90 (avg over 60s)
3. error_rate > 5 (avg over 30s)
```

Let's trigger the CPU alert:

```bash
# Send high CPU values
for i in {1..10}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d '{"name":"cpu_usage","value":90}' > /dev/null
  sleep 1
done
```

**Wait 10 seconds** (alert check interval), then check server logs:

```
🚨 [ALERT] 2025-09-17 13:45:23
   Metric: cpu_usage
   Condition: avg(60s) > 80.0
   Current: 90.0 (from 10 samples)
```

**Question:** Why do we need to wait for the alert? What's the trade-off between check frequency and system load?

### Add Custom Alert Rule

Modify `main()` in phase0_poc.cpp (lines 635-637):

```cpp
// Add your custom rule
alerting.add_rule(AlertRule("request_latency", 200.0, ">", 30));
```

Rebuild and test:

```bash
./build.sh
cd build && ./phase0_poc

# In another terminal, trigger it
for i in {1..5}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d '{"name":"request_latency","value":250}' > /dev/null
done
```

## Step 6: Understand the Code (40 minutes)

### Exercise: Add a New Metric Type

Currently, all metrics are numbers. Let's add support for **status metrics** (UP/DOWN).

**Task:** Modify the code to:
1. Accept `{"name":"service_status","value":"UP"}` (string value)
2. Store it in the same file format
3. Query returns string values
4. Alert rule: if status == "DOWN" for 30s → ALERT

**Hints:**
- Modify `Metric` struct to support `std::string value_str`
- Update JSON parsing in `parse_ingestion_json()`
- Update storage serialization in `to_json()`
- Add string comparison in `AlertRule`

**Don't worry about making it perfect!** The goal is understanding data flow, not production code.

### Exercise: Measure Query Performance

**Task:** Measure how query performance degrades as file grows.

```bash
# Ingest 1000 metrics
for i in {1..1000}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d "{\"name\":\"load_test\",\"value\":$i}" > /dev/null
done

# Time query
time curl "http://localhost:8080/query?name=load_test&start=0&end=9999999999999"
```

**Question:** What happens at 10,000 metrics? 100,000? This motivates indexes in Craft #3.

### Exercise: Test Queue Backpressure

**Task:** Send metrics faster than storage can write.

```bash
# Send 10,000 metrics rapidly (no delay)
for i in {1..10000}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d "{\"name\":\"flood\",\"value\":$i}" > /dev/null &
done
wait

# Check queue size
curl http://localhost:8080/health
# Response: {"status":"healthy","queue_size":5243}
```

**Observation:** Queue grows unbounded! In production, you need backpressure (reject requests when queue is full). Craft #2 adds this.

## Step 7: Optimization Planning (30 minutes)

Now that you've built and tested the PoC, let's identify bottlenecks:

### Bottleneck #1: Single-threaded Ingestion

**Problem:** Handles one request at a time (blocking).

**Measurement:**
```bash
# Load test with 50 concurrent clients
time for i in {1..50}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d '{"name":"test","value":1}' > /dev/null &
done
wait
```

**Expected:** ~5-10 seconds for 50 requests = 5-10 RPS

**Craft #1 solution:** Thread pool (1 thread per request) → 2,253 RPS (450x faster!)

### Bottleneck #2: Blocking File I/O

**Problem:** Every write blocks the storage thread.

**Measurement:** Check queue buildup under load (health endpoint).

**Craft #1 solution:** Async I/O with producer-consumer pipeline.

### Bottleneck #3: Full File Scan for Queries

**Problem:** O(n) complexity for every query.

**Craft #3 solution:** Inverted indexes + LSM tree storage → O(log n).

### Bottleneck #4: Polling-based Alerting

**Problem:** 10-second delay between alert condition and detection.

**Craft #5 solution:** Event-driven evaluation (push-based) → <1 second latency.

## Step 8: Run the Full Demo (10 minutes)

```bash
# In terminal 1: Start server
cd phase0/build && ./phase0_poc

# In terminal 2: Run demo
cd phase0 && ./demo.sh
```

The demo script:
1. Checks server health
2. Ingests sample metrics (CPU, memory, errors)
3. Queries metrics by name
4. Waits for alerts to trigger

Watch the server logs for alerts!

## Common Issues

### Issue: Port 8080 already in use

```bash
# Find and kill process
lsof -ti:8080 | xargs kill -9

# Or use different port
./phase0_poc 8081
```

### Issue: Query returns empty array

**Cause:** Time range doesn't include metrics.

**Fix:** Use start=0, end=9999999999999 to query all time.

### Issue: Alert never triggers

**Causes:**
1. Not enough data points in time window
2. Metric name doesn't match rule exactly
3. Need to wait for check interval (10s)

**Debug:** Add logging in `evaluate_rule()` to see query results.

## What You've Learned

✅ **Architecture:** How 5 components connect in a monitoring system
✅ **Concurrency:** Producer-consumer pattern with thread-safe queues
✅ **API Design:** RESTful endpoints for ingestion and queries
✅ **Storage:** JSON Lines format for time-series data
✅ **Alerting:** Rule-based threshold evaluation

✅ **Bottlenecks:** Single-threaded ingestion, blocking I/O, linear scans
✅ **Trade-offs:** Simplicity vs performance, polling vs event-driven

## Next Steps

### Ready for Craft #1?

You've validated the architecture. Now optimize ingestion:

**Craft #1: Metrics Ingestion System**
- Add thread pool (1 thread per request)
- Implement rate limiting per client
- Optimize JSON parsing (O(n²) → O(n))
- Add async I/O pipeline
- **Result:** 200 RPS → 2,253 RPS (10x improvement)

**Time investment:** 8-12 hours
**Link:** [Craft #1 Tutorial](../craft1.html)

### Or Explore More

- Modify alert rules (change thresholds, time windows)
- Add more metric types (histograms, summaries)
- Implement query aggregations (sum, min, max)
- Add a simple web UI (HTML + JavaScript)

## Summary

**Time spent:** 2-3 hours
**Lines of code:** 600 (single file)
**Throughput:** ~50-100 RPS

**Key insight:** Simple implementations reveal architecture and bottlenecks. Optimization comes *after* validation, not before.

You now understand how a complete monitoring platform works. Each component will become production-grade in Crafts #1-5, but the architecture stays the same.

**Great work!** Move on to Craft #1 when you're ready to optimize.

---

**Questions?** Open an issue on GitHub or check the [main documentation](../README.md).
