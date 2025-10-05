# Principal Engineer Learning Path
## Deep Technical Mastery Through MetricStream

This document outlines a structured path for developing principal engineer-level skills through hands-on system building and debugging.

**Core Philosophy**: Design the complete system architecture first, then implement one component deeply. This demonstrates both breadth (system design thinking) and depth (implementation mastery).

**MetricStream Approach**:
- **Designed**: Full production platform (Kafka, Flink, InfluxDB, Grafana) with capacity planning
- **Built**: Phase 1 Ingestion Service optimized to 2,253 RPS with 100% success rate
- **Documented**: Architecture Decision Records (ADRs), capacity planning, cost analysis

---

## Phase 0: System Design Foundations
**Before writing any code, design the complete production system.**

### Staff Engineer Deliverables

#### **1. Architecture Design Document**
Design the full platform even before implementing Phase 1:

```
Target System: Production metrics platform for 10K monitored servers

Agents (10K servers)
    ↓ (HTTP/JSON, 10 metrics/sec per server = 100K writes/sec total)
Ingestion Service (10 servers @ 10K RPS each)
    ↓ (Kafka partitioning by client_id)
Apache Kafka (3 brokers, 12 partitions)
    ↓ (Stream processing with 1-second tumbling windows)
Apache Flink (12 workers, CEP for alerts)
    ↓ (Batch writes to time-series DB)
InfluxDB Cluster (3 nodes, 30-day retention)
    ↓ (Pre-aggregated queries)
Grafana Dashboards (visualization layer)
```

**Key system design decisions**:
- Why Kafka over RabbitMQ (throughput: 1M msg/sec vs 50K msg/sec)
- Why InfluxDB over TimescaleDB (purpose-built for time-series vs PostgreSQL extension)
- Partitioning strategy (hash by client_id for even load distribution)
- Retention policies (30 days raw, 1 year downsampled to 1-hour resolution)

See [`ARCHITECTURE.md`](ARCHITECTURE.md) for complete design.

#### **2. Architecture Decision Records (ADRs)**
Document every major technology choice with systematic analysis:

**ADR Template**:
```markdown
# ADR-XXX: [Decision Title]

## Status
Accepted | Proposed | Deprecated

## Context
What problem are we solving? What constraints exist?

## Decision
What did we choose and why?

## Alternatives Considered
| Option | Pros | Cons | Score |
|--------|------|------|-------|
| Option A | ... | ... | 8/10 |
| Option B | ... | ... | 6/10 |

## Consequences
What are the implications (positive and negative)?

## Implementation Notes
How will this be built? What are the risks?
```

**Example ADRs from MetricStream**:
- ADR-001: Build HTTP from raw sockets (learning goal, not production pragmatism)
- ADR-002: Lock-free ring buffer vs mutex (wait-free metrics collection)
- ADR-005: InfluxDB vs TimescaleDB (detailed scoring across 8 dimensions)

See [`DESIGN_DECISIONS.md`](DESIGN_DECISIONS.md) for all ADRs.

#### **3. Capacity Planning with Real Costs**
Calculate actual infrastructure costs and scaling characteristics:

**Capacity Planning Framework**:
```
1. Current Performance Measurement
   - Measured RPS: 2,253 (Phase 1, single server)
   - Resource usage: 45% CPU, 150MB memory
   - Bottleneck identification: CPU-bound on JSON parsing

2. Single Server Capacity Limits
   - Projected max RPS: ~10K (at 90% CPU utilization)
   - Memory capacity: ~500 concurrent connections
   - Network bandwidth: 1 Gbps NIC = 125 MB/sec

3. Horizontal Scaling Math
   - Target: 100K writes/sec from 10K servers
   - Required ingestion servers: 100K / 10K = 10 servers
   - Cost per server: c6i.2xlarge @ $0.34/hour = $2,448/year
   - Total ingestion cost: 10 × $2,448 = $24,480/year

4. Full Infrastructure Cost Breakdown
   - Ingestion: 10 × c6i.2xlarge = $730/month
   - Kafka: 3 × m5.xlarge = $1,260/month
   - Flink: 12 × c6i.xlarge = $1,752/month
   - InfluxDB: 3 × i3.2xlarge = $1,872/month
   - Total: $6,070/month baseline
   - Optimized: $924/month (Spot instances, reserved capacity)

5. Cost Per Metric Calculation
   - 100K writes/sec × 86,400 sec/day = 8.64B metrics/day
   - Monthly cost: $924 / (8.64B × 30) = $0.000000023 per metric
   - Comparison: 96% cheaper than Datadog ($150K/month for 10K hosts)
```

See [`CAPACITY_PLANNING.md`](CAPACITY_PLANNING.md) for detailed analysis.

### **Learning Objectives**
- Design complete production systems before writing code
- Create Architecture Decision Records with alternatives analysis
- Perform capacity planning with real AWS/GCP pricing
- Demonstrate staff-level breadth (system design) and depth (implementation)

### **Why This Matters for Staff Engineer Roles**
**Hiring managers evaluate**:
- Can you design systems beyond what's currently built?
- Do you document decisions systematically (ADRs)?
- Can you estimate costs and make build-vs-buy decisions?
- Do you think about scaling before it's a problem?

**This approach demonstrates**:
- **Breadth**: Designed Kafka, Flink, InfluxDB integration (not implemented yet)
- **Depth**: Implemented Phase 1 from raw sockets, optimized to 2,253 RPS
- **Decision-making**: 5 detailed ADRs with scoring matrices
- **Business acumen**: $924/month cost (96% cheaper than Datadog)

**What differentiates staff from senior**:
- Senior: "I built a high-performance HTTP server"
- Staff: "I designed a scalable metrics platform (10K servers, $924/month AWS cost), implemented Phase 1 deeply (2,253 RPS, 0.25ms latency), and documented all architectural decisions with ADRs"

---

## Phase 1: Performance Engineering Deep Dive
**Current State**: Phase 7 complete - **2,253 RPS sustained, 100% success rate, 0.25ms latency**

**Optimization Journey** (each phase targeted a measured bottleneck):
- Phase 1: Threading per request → 88% success at 20 clients
- Phase 2: Async I/O → 66% success at 50 clients
- Phase 3: JSON parsing O(n²)→O(n) → 80.2% success at 100 clients
- Phase 4: Hash-based mutex pool → Reduced contention
- Phase 5: Thread pool architecture → 100% success, 0.65ms latency
- Phase 6: Lock-free ring buffer → Eliminated metrics overhead
- **Phase 7: HTTP Keep-Alive + listen backlog 1024 → 100% success, 0.25ms latency**

See [`docs/phase7_keep_alive_results.md`](phase7_keep_alive_results.md) for detailed phase-by-phase analysis.

### Core Skills Development

#### **CPU Profiling Mastery**
```bash
# Profile MetricStream under load
perf record -g ./metricstream_server
./load_test 8080 100 50 10  # Generate load
perf report --stdio > cpu_profile.txt

# Analyze hotspots
perf annotate <function_name>
perf top -p $(pgrep metricstream_server)
```

#### **Memory Analysis**
```bash
# Memory leak detection
valgrind --tool=memcheck --leak-check=full ./metricstream_server

# Cache performance analysis
perf stat -e cache-misses,cache-references ./load_test

# Heap profiling
valgrind --tool=massif ./metricstream_server
ms_print massif.out.12345
```

#### **System Call Analysis**
```bash
# Trace system calls under load
strace -c -p $(pgrep metricstream_server)
strace -e trace=network -p $(pgrep metricstream_server)

# I/O latency analysis
iotop -p $(pgrep metricstream_server)
iostat -x 1
```

### **Learning Objectives**
- Identify why thread-per-request models fail at scale
- Understand memory allocation patterns and heap fragmentation
- Analyze I/O bottlenecks and context switching overhead
- Master Linux performance tools for production debugging

### **Principal Engineer Questions to Answer**
- Why does success rate drop from 99.25% to 91.4% at 12.5x load?
- Which system resources become bottlenecks first (CPU, memory, I/O)?
- How do thread context switches impact latency distribution?
- What kernel parameters affect network performance?

---

## Phase 2: Concurrency Architecture Mastery
**Target**: 2K req/sec with 99.5% success rate

### Advanced Concurrency Patterns

#### **Lock-Free Programming**
```cpp
// Replace mutex-based rate limiter with lock-free implementation
class LockFreeRateLimiter {
    std::atomic<uint64_t> token_bucket;
    std::atomic<uint64_t> last_refill;

public:
    bool try_acquire() {
        // CAS-based token bucket algorithm
        uint64_t current_time = get_time_ns();
        uint64_t tokens = token_bucket.load();

        // Refill tokens based on time elapsed
        uint64_t last_time = last_refill.load();
        if (current_time > last_time) {
            uint64_t new_tokens = calculate_refill(current_time - last_time);
            token_bucket.compare_exchange_weak(tokens, tokens + new_tokens);
            last_refill.compare_exchange_weak(last_time, current_time);
        }

        // Atomic decrement if tokens available
        return token_bucket.fetch_sub(1) > 0;
    }
};
```

#### **Async I/O Implementation**
```cpp
// Replace blocking file I/O with async writes
class AsyncMetricWriter {
    std::queue<MetricBatch> write_queue;
    std::condition_variable cv;
    std::thread writer_thread;

public:
    void write_async(const MetricBatch& batch) {
        {
            std::lock_guard<std::mutex> lock(queue_mutex);
            write_queue.push(batch);
        }
        cv.notify_one();  // Non-blocking return
    }

private:
    void writer_loop() {
        // Background thread handles all I/O
        while (running) {
            std::unique_lock<std::mutex> lock(queue_mutex);
            cv.wait(lock, [this]{ return !write_queue.empty() || !running; });

            // Batch multiple writes for efficiency
            std::vector<MetricBatch> batches;
            while (!write_queue.empty()) {
                batches.push_back(write_queue.front());
                write_queue.pop();
            }

            lock.unlock();
            write_batches_to_disk(batches);  // Blocking I/O off critical path
        }
    }
};
```

### **Debugging Concurrency Issues**
```bash
# Thread contention analysis
perf lock record -p $(pgrep metricstream_server)
perf lock report

# Race condition detection
clang++ -fsanitize=thread -g -o metricstream_server src/*.cpp
./metricstream_server  # ThreadSanitizer will catch races

# CPU cache analysis
perf c2c record ./load_test
perf c2c report
```

### **Learning Objectives**
- Implement wait-free data structures for high-contention scenarios
- Design async I/O pipelines that decouple request handling from persistence
- Master memory ordering and atomic operations
- Debug race conditions and deadlocks in production systems

---

## Phase 3: Distributed Systems Engineering
**Target**: 10K req/sec with 99.9% success rate

### Core Distributed Systems Concepts

#### **Message Queue Integration**
```cpp
// Event-driven architecture with Kafka
class KafkaMetricProducer {
    rd_kafka_t* producer;

public:
    void send_metric_async(const Metric& metric) {
        std::string key = metric.tags["host"];  // Partition by host
        std::string json = serialize_metric(metric);

        rd_kafka_produce(
            topic,
            RD_KAFKA_PARTITION_UA,  // Automatic partitioning
            RD_KAFKA_MSG_F_COPY,
            json.data(), json.size(),
            key.data(), key.size(),
            nullptr  // Async, no callback needed
        );
    }
};
```

#### **Partitioning and Sharding**
```cpp
// Consistent hashing for horizontal scaling
class MetricShardRouter {
    std::vector<ShardInfo> shards;
    std::hash<std::string> hasher;

public:
    size_t route_metric(const Metric& metric) {
        // Route by metric name + host for even distribution
        std::string key = metric.name + ":" + metric.tags.at("host");
        size_t hash = hasher(key);
        return hash % shards.size();
    }

    void add_shard(const ShardInfo& shard) {
        // Implement consistent hashing ring for smooth rebalancing
        shards.push_back(shard);
        rebalance_ring();
    }
};
```

#### **Failure Mode Analysis**
```cpp
// Circuit breaker pattern for resilience
class CircuitBreaker {
    enum State { CLOSED, OPEN, HALF_OPEN };

    std::atomic<State> state{CLOSED};
    std::atomic<size_t> failure_count{0};
    std::atomic<uint64_t> last_failure_time{0};

public:
    bool allow_request() {
        State current_state = state.load();

        switch (current_state) {
            case CLOSED:
                return true;
            case OPEN:
                if (should_attempt_reset()) {
                    state.store(HALF_OPEN);
                    return true;
                }
                return false;
            case HALF_OPEN:
                return true;  // Allow single test request
        }
    }

    void record_success() {
        failure_count.store(0);
        state.store(CLOSED);
    }

    void record_failure() {
        size_t failures = failure_count.fetch_add(1);
        if (failures >= FAILURE_THRESHOLD) {
            state.store(OPEN);
            last_failure_time.store(get_time_ms());
        }
    }
};
```

### **Distributed Debugging**
```bash
# Network latency analysis
tcpdump -i any -w network_capture.pcap host metricstream-server
wireshark network_capture.pcap

# Distributed tracing
# Implement correlation IDs across service boundaries
curl -H "X-Correlation-ID: req-123" http://metricstream:8080/metrics

# Kafka monitoring
kafka-consumer-groups.sh --bootstrap-server localhost:9092 --describe --group metricstream-consumers
```

### **Learning Objectives**
- Implement event-driven architectures with message queues
- Design partitioning strategies for horizontal scaling
- Master distributed tracing and correlation across services
- Engineer fault tolerance with circuit breakers and bulkheads

---

## Phase 4: Production Engineering Excellence
**Target**: 100K+ req/sec with 99.99% reliability

### Advanced Production Skills

#### **Capacity Planning**
```cpp
// Performance modeling and capacity estimation
class CapacityPlanner {
    struct ResourceMetrics {
        double cpu_usage_percent;
        size_t memory_usage_bytes;
        size_t network_bandwidth_mbps;
        size_t disk_iops;
    };

public:
    size_t estimate_max_throughput(const ResourceMetrics& current,
                                   size_t current_rps) {
        // Find limiting resource
        double cpu_utilization = current.cpu_usage_percent / 100.0;
        double memory_pressure = current.memory_usage_bytes / max_memory;

        // Model throughput based on bottleneck resource
        if (cpu_utilization > 0.8) {
            return current_rps / cpu_utilization * 0.8;  // Leave 20% headroom
        }

        // Continue analysis for other resources...
    }
};
```

#### **Chaos Engineering**
```cpp
// Failure injection for testing resilience
class ChaosMonkey {
public:
    void inject_network_partition() {
        // Simulate network split between services
        system("iptables -A OUTPUT -d kafka-broker -j DROP");

        // Verify system behavior during partition
        std::this_thread::sleep_for(std::chrono::seconds(30));

        // Restore connectivity
        system("iptables -D OUTPUT -d kafka-broker -j DROP");
    }

    void inject_memory_pressure() {
        // Allocate large chunks to trigger OOM behavior
        std::vector<std::unique_ptr<char[]>> memory_pressure;
        for (int i = 0; i < 1000; ++i) {
            memory_pressure.push_back(std::make_unique<char[]>(10 * 1024 * 1024));
        }
    }
};
```

### **eBPF for Deep System Observability**
```c
// eBPF program to trace MetricStream system calls
#include <linux/bpf.h>
#include <bpf/bpf_helpers.h>

SEC("tracepoint/syscalls/sys_enter_write")
int trace_write_syscalls(struct trace_event_raw_sys_enter* ctx) {
    pid_t pid = bpf_get_current_pid_tgid() >> 32;

    // Filter for MetricStream process
    if (pid == metricstream_pid) {
        // Log write system call details
        bpf_printk("MetricStream write: fd=%d, size=%d\\n",
                   ctx->args[0], ctx->args[2]);
    }

    return 0;
}
```

### **Production Debugging Commands**
```bash
# eBPF-based monitoring
bpftrace -e 'tracepoint:syscalls:sys_enter_write /comm == "metricstream_server"/ { @[args[0]] = count(); }'

# Network performance tuning
echo 'net.core.rmem_max = 134217728' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_congestion_control = bbr' >> /etc/sysctl.conf

# Container resource limits analysis
docker stats metricstream-container
cgroup memory limits and usage analysis
```

### **Learning Objectives**
- Design horizontally scalable architectures
- Implement comprehensive observability and alerting
- Master production debugging with eBPF and advanced Linux tools
- Engineer systems for 99.99% reliability and graceful degradation

---

## Principal Engineer Skills Validation

### **Technical Leadership Competencies**

#### **Architecture Decision Records (ADRs)**
Document every major technical decision with:
- Context and problem statement
- Considered alternatives and trade-offs
- Decision rationale and success metrics
- Implementation plan and rollback strategy

#### **Code Review Excellence**
- Performance implications of design changes
- Security vulnerability identification
- Scalability and maintainability assessment
- Mentoring junior engineers through review feedback

#### **Incident Response Leadership**
- Root cause analysis methodology
- Post-mortem facilitation and action item tracking
- System resilience improvement identification
- Cross-team coordination during outages

### **Measuring Principal Engineer Growth**

**Technical Depth**:
- Can debug production issues using only Linux command-line tools
- Understands performance implications of every architectural decision
- Designs systems that scale predictably with load

**Technical Breadth**:
- Connects performance optimizations to business impact
- Balances technical debt against feature velocity
- Communicates complex technical concepts to non-technical stakeholders

**Technical Leadership**:
- Influences technical decisions across multiple teams
- Mentors other engineers to become technical leaders
- Drives organization-wide technical standards and practices

This learning path transforms MetricStream from a simple metrics system into a comprehensive principal engineer development platform, providing hands-on experience with every aspect of high-scale system design and operation.