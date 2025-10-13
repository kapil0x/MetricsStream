# Phase 0: Hands-On Exercises

These exercises help you deeply understand the Phase 0 architecture by modifying and extending it. Each exercise builds your intuition for distributed systems design.

---

## Exercise 1: Add Request Logging (Beginner, 20 minutes)

**Goal:** Track every incoming request with timestamp and response time.

### Task

Add logging to `IngestionServer::handle_client()` that outputs:
```
[2025-09-17 13:45:23] POST /metrics - 202 Accepted - 3ms
[2025-09-17 13:45:24] GET /query?name=cpu_usage - 200 OK - 15ms
```

### Learning Objectives
- Understand request lifecycle
- Measure end-to-end latency
- See how logging impacts performance

### Implementation Hints

```cpp
// At start of handle_client()
auto start_time = std::chrono::high_resolution_clock::now();

// After sending response
auto end_time = std::chrono::high_resolution_clock::now();
auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
    end_time - start_time).count();

// Format and log
std::cout << "[" << formatted_timestamp() << "] "
          << method << " " << url << " - "
          << status_code << " - " << duration << "ms" << std::endl;
```

### Test It

```bash
# Rebuild
./build.sh && cd build && ./phase0_poc &

# Send requests and watch logs
curl -X POST http://localhost:8080/metrics -d '{"name":"test","value":1}'
```

**Expected output:**
```
[2025-09-17 13:45:23] POST /metrics - 202 Accepted - 2ms
```

### Questions to Answer

1. What's the average latency for POST /metrics?
2. What's the average latency for GET /query?
3. Why is query slower than ingestion?

---

## Exercise 2: Add Queue Depth Monitoring (Beginner, 25 minutes)

**Goal:** Track queue depth over time to detect backpressure.

### Task

Modify `main()` to log queue statistics every 5 seconds:

```
[Queue Stats] Depth: 127 | Peak: 453 | Total Processed: 2,341
```

### Learning Objectives
- Understand backpressure in producer-consumer systems
- Learn when queues grow (ingestion > storage)
- See how queue depth correlates with load

### Implementation Hints

Add a statistics thread in `main()`:

```cpp
// Global stats
std::atomic<size_t> peak_queue_depth(0);
std::atomic<size_t> total_processed(0);

// In StorageConsumer::worker(), after processing:
total_processed++;

// In main(), add monitoring thread:
std::thread monitor([&]() {
    while (true) {
        size_t current = g_metric_queue.size();
        if (current > peak_queue_depth) {
            peak_queue_depth = current;
        }

        std::cout << "[Queue Stats] Depth: " << current
                  << " | Peak: " << peak_queue_depth
                  << " | Total Processed: " << total_processed
                  << std::endl;

        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
});
monitor.detach();
```

### Test It

```bash
# Send burst of metrics
for i in {1..1000}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d "{\"name\":\"burst\",\"value\":$i}" > /dev/null &
done
wait

# Watch queue stats in server logs
```

### Questions to Answer

1. How high does the queue grow during burst load?
2. How long does it take for the queue to drain?
3. What happens if you send 10,000 metrics instead of 1,000?

---

## Exercise 3: Implement Query Filtering by Value Range (Intermediate, 30 minutes)

**Goal:** Add support for querying metrics by value range.

### Task

Support queries like:
```bash
curl "http://localhost:8080/query?name=cpu_usage&min_value=80&max_value=100"
```

Returns only metrics where `80 <= value <= 100`.

### Learning Objectives
- Understand query parameter parsing
- Learn filtering patterns
- See trade-offs between features and complexity

### Implementation Guide

1. **Parse new query parameters** in `handle_client()`:

```cpp
auto params = parse_query_params(url);
double min_value = params.count("min_value") ?
    std::stod(params["min_value"]) : -INFINITY;
double max_value = params.count("max_value") ?
    std::stod(params["max_value"]) : INFINITY;
```

2. **Add filtering to QueryEngine::query()**:

```cpp
if (metric.name == name &&
    metric.timestamp >= start_ts &&
    metric.timestamp <= end_ts &&
    metric.value >= min_value &&          // NEW
    metric.value <= max_value) {          // NEW
    results.push_back(metric);
}
```

3. **Update function signature**:

```cpp
std::vector<Metric> query(const std::string& name,
                         long long start_ts, long long end_ts,
                         double min_value = -INFINITY,
                         double max_value = INFINITY)
```

### Test It

```bash
# Ingest varied CPU values
for val in 50 65 75 85 95; do
  curl -s -X POST http://localhost:8080/metrics \
       -d "{\"name\":\"cpu_usage\",\"value\":$val}" > /dev/null
done

# Query high CPU only (>= 80)
curl "http://localhost:8080/query?name=cpu_usage&min_value=80&start=0&end=9999999999999"
```

**Expected:** Only values 85 and 95 returned.

---

## Exercise 4: Add Batched Metrics Ingestion (Intermediate, 45 minutes)

**Goal:** Support ingesting multiple metrics in one HTTP request.

### Task

Accept JSON like:
```json
{
  "metrics": [
    {"name":"cpu_usage","value":85},
    {"name":"memory_usage","value":75},
    {"name":"disk_io","value":120}
  ]
}
```

All metrics pushed to queue from single request.

### Learning Objectives
- Understand batching for throughput
- Learn JSON array parsing
- See how batching reduces HTTP overhead

### Implementation Guide

1. **Add batch parsing** in `handle_client()`:

```cpp
// Check if body contains "metrics" array
if (body.find("\"metrics\":[") != std::string::npos) {
    // Parse array (simple: split by "},")
    size_t pos = body.find("[");
    size_t end = body.find("]");
    std::string metrics_str = body.substr(pos + 1, end - pos - 1);

    // TODO(human): Parse each metric object and push to queue
    // Hint: Split by "},", parse each one like single metric

} else {
    // Single metric (existing code)
}
```

2. **TODO(human) section for you to implement:**

Your task: Implement the batch parsing logic that:
- Splits the metrics array by `},`
- Parses each metric (name, value)
- Pushes each to the queue
- Returns count in response: `{"status":"accepted","count":3}`

**Guidance:** You can reuse `parse_ingestion_json()` for each metric. Consider edge cases like empty arrays and malformed JSON.

### Test It

```bash
curl -X POST http://localhost:8080/metrics \
     -H "Content-Type: application/json" \
     -d '{
       "metrics": [
         {"name":"cpu_usage","value":85},
         {"name":"memory_usage","value":75},
         {"name":"disk_io","value":120}
       ]
     }'
```

**Expected:** `{"status":"accepted","count":3}`

### Performance Test

```bash
# Benchmark: Single requests vs batching
time for i in {1..100}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d "{\"name\":\"test\",\"value\":$i}" > /dev/null
done

# vs

time curl -s -X POST http://localhost:8080/metrics \
     -d "{\"metrics\":[$(for i in {1..100}; do
       echo -n "{\"name\":\"test\",\"value\":$i}";
       [ $i -lt 100 ] && echo -n ",";
     done)]}" > /dev/null
```

**Question:** How much faster is batching?

---

## Exercise 5: Implement Alert Deduplication (Intermediate, 40 minutes)

**Goal:** Prevent alert spam by deduplicating repeated alerts.

### Task

Only alert once when condition is met, not every 10 seconds while condition persists.

**Current behavior:**
```
[ALERT] cpu_usage > 80 (avg: 85)
[ALERT] cpu_usage > 80 (avg: 86)  # Repeated!
[ALERT] cpu_usage > 80 (avg: 87)  # Repeated!
```

**Desired behavior:**
```
[ALERT] cpu_usage > 80 (avg: 85)
[ALERT RESOLVED] cpu_usage returned to normal (avg: 75)
```

### Learning Objectives
- Understand alert state management
- Learn deduplication patterns
- See how state complicates systems

### Implementation Guide

1. **Add alert state tracking**:

```cpp
class AlertingEngine {
private:
    // Track which alerts are currently firing
    std::set<std::string> active_alerts_;
    std::mutex state_mutex_;

    bool is_active(const std::string& alert_key) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return active_alerts_.count(alert_key) > 0;
    }

    void set_active(const std::string& alert_key, bool active) {
        std::lock_guard<std::mutex> lock(state_mutex_);
        if (active) {
            active_alerts_.insert(alert_key);
        } else {
            active_alerts_.erase(alert_key);
        }
    }
};
```

2. **Modify evaluate_rule()**:

```cpp
void evaluate_rule(const AlertRule& rule) {
    // ... existing code to calculate avg ...

    std::string alert_key = rule.metric_name + "_" + rule.condition +
                           std::to_string(rule.threshold);
    bool currently_active = is_active(alert_key);
    bool should_fire = /* condition check */;

    if (should_fire && !currently_active) {
        // New alert!
        std::cout << "🚨 [ALERT] ..." << std::endl;
        set_active(alert_key, true);
    } else if (!should_fire && currently_active) {
        // Resolved!
        std::cout << "✅ [ALERT RESOLVED] ..." << std::endl;
        set_active(alert_key, false);
    }
    // else: no change, stay quiet
}
```

### Test It

```bash
# Trigger alert
for i in {1..5}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d '{"name":"cpu_usage","value":95}' > /dev/null
  sleep 1
done

# Wait 20 seconds - should only see ONE alert

# Resolve it
for i in {1..5}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d '{"name":"cpu_usage","value":60}' > /dev/null
  sleep 1
done

# Wait 20 seconds - should see RESOLVED message
```

---

## Exercise 6: Add Percentile Queries (Advanced, 60 minutes)

**Goal:** Calculate p50, p90, p99 percentiles for query results.

### Task

Support queries like:
```bash
curl "http://localhost:8080/query?name=cpu_usage&percentile=99&start=0&end=999999"
```

Returns: `{"p99":95.5}`

### Learning Objectives
- Understand percentile calculations
- Learn aggregation patterns
- See trade-offs between accuracy and performance

### Implementation Guide

1. **Add percentile calculation helper**:

```cpp
double calculate_percentile(std::vector<double>& values, double p) {
    if (values.empty()) return 0.0;

    std::sort(values.begin(), values.end());
    size_t index = static_cast<size_t>(std::ceil(p / 100.0 * values.size())) - 1;
    index = std::min(index, values.size() - 1);

    return values[index];
}
```

2. **Add percentile endpoint** in `handle_client()`:

```cpp
else if (method == "GET" && url.find("/percentile") == 0) {
    auto params = parse_query_params(url);
    std::string name = params["name"];
    double p = params.count("percentile") ? std::stod(params["percentile"]) : 50;

    // Get all values
    std::vector<Metric> metrics = query_engine->query(name, start_ts, end_ts);
    std::vector<double> values;
    for (const auto& m : metrics) {
        values.push_back(m.value);
    }

    // Calculate
    double result = calculate_percentile(values, p);

    // Response
    std::ostringstream json;
    json << "{\"p" << p << "\":" << result << ",\"count\":" << values.size() << "}";
    // ... send response ...
}
```

### Test It

```bash
# Ingest varied latencies
for val in 10 12 15 18 20 25 30 35 45 60 80 100 150 200 500; do
  curl -s -X POST http://localhost:8080/metrics \
       -d "{\"name\":\"latency\",\"value\":$val}" > /dev/null
done

# Query percentiles
curl "http://localhost:8080/percentile?name=latency&percentile=50&start=0&end=999999"
# Expected: {"p50":30,"count":15}

curl "http://localhost:8080/percentile?name=latency&percentile=99&start=0&end=999999"
# Expected: {"p99":500,"count":15}
```

### Questions to Answer

1. Why does p99 differ from average?
2. When would you use p99 vs average for alerts?
3. What's the performance cost of sorting for percentiles?

---

## Exercise 7: Implement Exponential Backoff for Failed Writes (Advanced, 45 minutes)

**Goal:** Retry failed storage writes with exponential backoff.

### Task

If file write fails, retry with delays: 100ms, 200ms, 400ms, 800ms, then drop.

### Learning Objectives
- Understand retry patterns
- Learn backoff strategies
- See reliability vs throughput trade-offs

### Implementation Guide

```cpp
bool write_with_retry(const Metric& metric, const std::string& filename) {
    int max_retries = 4;
    int delay_ms = 100;

    for (int attempt = 0; attempt < max_retries; attempt++) {
        std::ofstream file(filename, std::ios::app);
        if (file.is_open()) {
            file << metric.to_json() << "\n";
            file.close();

            if (attempt > 0) {
                std::cout << "[Storage] Write succeeded after "
                         << attempt << " retries" << std::endl;
            }
            return true;
        }

        // Failed, backoff
        std::cerr << "[Storage] Write failed, retry in "
                 << delay_ms << "ms" << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        delay_ms *= 2;  // Exponential backoff
    }

    std::cerr << "[Storage] Dropped metric after "
             << max_retries << " retries" << std::endl;
    return false;
}
```

### Test It

```bash
# Simulate file system issues
chmod 000 build/phase0_metrics.jsonl  # Make file unwritable

# Send metrics - watch retry logs
curl -X POST http://localhost:8080/metrics -d '{"name":"test","value":1}'

# Restore permissions
chmod 644 build/phase0_metrics.jsonl
```

**Expected logs:**
```
[Storage] Write failed, retry in 100ms
[Storage] Write failed, retry in 200ms
[Storage] Write succeeded after 2 retries
```

---

## Exercise 8: Build a Simple Load Tester (Advanced, 60 minutes)

**Goal:** Create a custom load testing tool to measure throughput and latency.

### Task

Write `load_test.cpp` that:
1. Spawns N concurrent client threads
2. Each sends M metrics
3. Measures success rate, throughput (RPS), and latency percentiles

### Learning Objectives
- Understand load testing methodology
- Learn concurrency patterns
- See how to measure system performance

### Implementation

● **Learn by Doing**

**Context:** I've set up the Phase 0 PoC with all components working. Now we need a load testing tool to measure its performance characteristics and identify bottlenecks. This will help students understand the impact of optimization in Craft #1.

**Your Task:** In `phase0/load_test.cpp`, implement a multi-threaded load tester. Look for TODO(human). The program should accept command-line arguments (host, port, num_clients, requests_per_client), spawn client threads that send metrics concurrently, and report statistics (total requests, success rate, throughput, avg/p50/p99 latency).

**Guidance:** Consider using std::thread for concurrency, std::chrono for timing, and collect all results in a thread-safe way (mutex-protected vector or atomic counters). Each client should connect via TCP socket and send HTTP POST requests. Calculate statistics after all threads complete. The goal is to stress-test the single-threaded ingestion server to expose its bottleneck.

---

## Exercise 9: Add Metric Expiration (Advanced, 50 minutes)

**Goal:** Automatically delete metrics older than N days.

### Task

Add a background thread that periodically:
1. Reads metrics file
2. Filters out old metrics (timestamp < now - N days)
3. Rewrites file with only recent data

### Learning Objectives
- Understand data retention policies
- Learn file compaction patterns
- See trade-offs between storage and history

### Hints

```cpp
void compact_metrics_file(const std::string& filename, int retention_days) {
    long long cutoff_ts = get_current_timestamp() -
                         (retention_days * 24 * 60 * 60 * 1000LL);

    // Read all metrics
    std::vector<Metric> recent_metrics;
    // ... parse file, keep only metrics with timestamp >= cutoff_ts ...

    // Rewrite file
    std::ofstream file(filename, std::ios::trunc);
    for (const auto& m : recent_metrics) {
        file << m.to_json() << "\n";
    }
}
```

---

## Exercise 10: Create a Web Dashboard (Advanced, 90 minutes)

**Goal:** Build a simple HTML dashboard that displays metrics in real-time.

### Task

Create `dashboard.html` that:
- Fetches metrics via `/query` API
- Displays as line charts (use Chart.js)
- Updates every 5 seconds
- Shows current alert status

### Learning Objectives
- Understand web-based monitoring UIs
- Learn AJAX polling patterns
- See end-to-end system integration

### Starter Code

```html
<!DOCTYPE html>
<html>
<head>
    <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
</head>
<body>
    <h1>Phase 0 Dashboard</h1>
    <canvas id="cpuChart"></canvas>

    <script>
    async function fetchMetrics() {
        const now = Date.now();
        const start = now - (5 * 60 * 1000); // Last 5 minutes

        const response = await fetch(
            `http://localhost:8080/query?name=cpu_usage&start=${start}&end=${now}`
        );
        const metrics = await response.json();

        // TODO: Update chart with metrics
    }

    setInterval(fetchMetrics, 5000);
    </script>
</body>
</html>
```

---

## Summary

These exercises progressively build your understanding:

**Beginner (1-2):** Observability and monitoring
**Intermediate (3-5):** Feature development and state management
**Advanced (6-10):** Performance, reliability, and full-stack integration

**Recommended order:**
1. Start with 1-2 (quick wins, see the system in action)
2. Do 3-5 (understand trade-offs, add features)
3. Tackle 6-10 (deep systems knowledge)

**Time investment:**
- All exercises: ~8-10 hours
- Selective (1,2,3,5,8): ~3 hours for core understanding

After completing exercises, you'll have intuition for distributed systems design that goes far beyond Phase 0!

---

**Next:** Move to [Craft #1](../craft1.html) to optimize ingestion 10x (200 → 2,253 RPS)
