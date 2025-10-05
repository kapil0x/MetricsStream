# MetricStream System Architecture

> Production-grade metrics platform designed for horizontal scale and high availability

## Executive Summary

MetricStream is a distributed metrics collection and monitoring platform designed to handle 10,000+ servers generating 100K+ metrics per second with 99.99% uptime. The system follows event-driven architecture with clear separation between data ingestion, stream processing, storage, and visualization layers.

**Current State:** Phase 1 (Ingestion Service) built and optimized
**Target Capacity:** 100K writes/second across 10K servers
**Estimated Cost:** ~$5,000/month AWS infrastructure

---

## System Overview

### Current Implementation (Phase 1)

```
┌──────────────┐      ┌────────────────────────┐      ┌──────────────┐
│   Clients    │      │  Ingestion Server      │      │  Storage     │
│              ├─────→│                        ├─────→│              │
│ HTTP POST    │      │  • Raw socket HTTP     │      │  metrics.    │
│ JSON metrics │      │  • Rate limiting       │      │  jsonl       │
│              │      │  • JSON parsing        │      │              │
│ 100 clients  │      │  • Thread pool         │      │  Append-only │
└──────────────┘      └────────────────────────┘      └──────────────┘

Measured Performance:
  ├─ Throughput: 2,253 RPS sustained
  ├─ Success Rate: 100% at 100 concurrent clients
  ├─ Latency: 0.25ms (p50), 0.65ms (p99)
  └─ Resource Usage: 45% CPU, 150MB memory
```

### Target Architecture (Phases 2-5)

```
┌─────────────┐   ┌────────────┐   ┌─────────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│   Agents    │   │ Ingestion  │   │   Kafka     │   │  Flink   │   │ InfluxDB │   │ Grafana  │
│  (10K+)     │──→│  Cluster   │──→│  Message    │──→│  Stream  │──→│ Time-    │──→│Dashboard │
│             │   │  (10 svr)  │   │   Queue     │   │Processing│   │ Series   │   │Alerting  │
│ Push        │   │            │   │             │   │          │   │   DB     │   │          │
│ metrics     │   │ - HTTP     │   │ - 3 brokers │   │ - Alerts │   │ - Sharded│   │ - Visual │
│ /10s        │   │ - Rate     │   │ - 12 parts  │   │ - Agg    │   │ - HA     │   │ - Query  │
│             │   │   limit    │   │ - Replicas  │   │ - Enrich │   │ - Retain │   │ - Export │
└─────────────┘   └────────────┘   └─────────────┘   └──────────┘   └──────────┘   └──────────┘
      │                 │                   │                │               │              │
      │                 │                   │                │               │              │
  10K hosts        10K RPS each       100K msg/sec    Real-time         Compressed     Dashboards
  × 10 metrics    = 100K RPS          Partitioned     Processing       Time-series      + Alerts
  / 10 sec        Load balanced       by client_id    < 1s latency     90 days          Sub-second
```

---

## Component Architecture

### 1. Monitoring Agents (Collection Layer)

**Purpose:** Lightweight agents deployed on each monitored server

**Implementation:**
- Language: Go (low overhead, easy deployment)
- Frequency: Collect metrics every 10 seconds
- Push Model: Active push to ingestion endpoints
- Buffering: Local queue with retry on failure

**Metrics Collected:**
```
System Metrics:
  - CPU usage (per core, total)
  - Memory (used, available, cached)
  - Disk I/O (read/write bytes, IOPS)
  - Network (rx/tx bytes, errors)

Application Metrics:
  - Custom metrics via StatsD protocol
  - HTTP endpoint (/metrics) scraping
  - Log-based metric extraction
```

**Failure Handling:**
- Local buffering: 5 minutes of metrics
- Exponential backoff: 1s, 2s, 4s, 8s, 16s
- Circuit breaker: Stop retrying after 5 consecutive failures
- Health endpoint: Self-monitoring agent health

---

### 2. Ingestion Service (Current Implementation)

**Technology:** C++ (built from first principles)

**Architecture:**
```cpp
┌─────────────────────────────────────────────┐
│          Ingestion Server (C++)             │
│                                             │
│  ┌────────────┐  ┌──────────────┐  ┌─────┐ │
│  │ HTTP Server│→ │ Rate Limiter │→ │JSON │ │
│  │  (sockets) │  │ (sliding win)│  │Parse│ │
│  └────────────┘  └──────────────┘  └─────┘ │
│        ↓                  ↓            ↓    │
│  ┌────────────┐  ┌──────────────┐  ┌─────┐ │
│  │Thread Pool │  │ Mutex Pool   │  │Ring │ │
│  │ (16 wrk)   │  │ (64 mutexes) │  │Buf  │ │
│  └────────────┘  └──────────────┘  └─────┘ │
│        ↓                                    │
│  ┌────────────────────────────────┐         │
│  │   JSONL File Writer            │         │
│  │   (async, batched)             │         │
│  └────────────────────────────────┘         │
└─────────────────────────────────────────────┘
```

**Key Design Decisions:**
1. **Raw Sockets:** Custom HTTP implementation for control and learning
2. **Thread Pool:** Fixed 16 workers (eliminates thread creation overhead)
3. **Lock-Free Metrics:** Atomic ring buffers for zero-contention collection
4. **Per-Client Locking:** Hash-based mutex pool for parallel client processing

**Performance Characteristics:**
- Single server capacity: ~10K RPS (theoretical), 2.2K RPS (measured sustained)
- Bottleneck: File I/O becomes limiting factor above 10K RPS
- Horizontal scaling: Load balance 100K RPS across 10 servers

**Migration Path (Phase 2):**
- Replace JSONL file writes with Kafka producer
- Add correlation IDs for distributed tracing
- Implement gRPC alongside HTTP for lower overhead
- Add Prometheus /metrics endpoint for self-monitoring

---

### 3. Message Queue (Kafka)

**Why Kafka over Alternatives:**

| Feature | Kafka | RabbitMQ | Kinesis |
|---------|-------|----------|---------|
| Throughput | Millions/sec | ~50K/sec | Shards × 1K/sec |
| Persistence | Disk-backed | Memory-first | Managed |
| Replay | Full history | Limited | 24h-7d |
| Partitioning | Native | Queues | Shards |
| Cost | Self-hosted | Self-hosted | $$$$ |
| **Decision** | ✅ **CHOSEN** | ❌ Too slow | ❌ Too expensive |

**Configuration:**
```yaml
Cluster:
  - Brokers: 3 (m5.xlarge)
  - Replication Factor: 3 (no data loss)
  - Partitions: 12 (parallelism = 12 Flink workers)

Topic: metrics-raw
  - Retention: 24 hours (replay window)
  - Compression: LZ4 (3-5x reduction)
  - Partitioning Strategy: Hash(client_id) % 12

Performance:
  - Write throughput: 500K msg/sec per broker
  - Total capacity: 1.5M msg/sec (15x headroom)
  - Latency: p99 < 10ms
```

**Partitioning Strategy:**
```python
# Partition by client_id for even distribution
partition = hash(client_id) % num_partitions

Benefits:
  - Load balancing across consumers
  - Preserves ordering per client
  - Enables parallel processing
  - Easy to add partitions for scaling
```

---

### 4. Stream Processing (Apache Flink)

**Why Flink over Alternatives:**

| Feature | Flink | Spark Streaming | Storm |
|---------|-------|-----------------|-------|
| Latency | Milliseconds | Seconds | Milliseconds |
| State Management | Built-in | External | Custom |
| Exactly-once | Native | Micro-batch | At-least-once |
| SQL Support | ✅ | ✅ | ❌ |
| **Decision** | ✅ **CHOSEN** | ❌ Higher latency | ❌ Less mature |

**Processing Pipeline:**
```sql
-- Flink SQL for real-time aggregation
SELECT
    client_id,
    metric_name,
    TUMBLE_START(event_time, INTERVAL '1' MINUTE) as window_start,
    AVG(value) as avg_value,
    MAX(value) as max_value,
    MIN(value) as min_value,
    COUNT(*) as count
FROM metrics_stream
GROUP BY
    client_id,
    metric_name,
    TUMBLE(event_time, INTERVAL '1' MINUTE)
```

**Alert Rules Engine:**
```java
// Flink DataStream API for complex event processing
DataStream<Metric> metrics = env.addSource(kafkaConsumer);

DataStream<Alert> alerts = metrics
    .keyBy(metric -> metric.getClientId())
    .window(TumblingEventTimeWindows.of(Time.minutes(5)))
    .process(new AlertProcessor())
    .filter(alert -> alert.getSeverity() > Threshold.WARNING);

alerts.addSink(alertNotifier);
```

**Alert Conditions:**
- CPU usage > 90% for 5 consecutive minutes
- Memory usage > 95%
- Disk space < 10%
- Custom metric thresholds (user-defined)
- Anomaly detection (ML-based)

---

### 5. Time-Series Database (InfluxDB)

**Why InfluxDB over Alternatives:**

| Feature | InfluxDB | TimescaleDB | Prometheus |
|---------|----------|-------------|------------|
| Write Performance | 500K pts/sec | 100K pts/sec | 1M pts/sec |
| Query Language | InfluxQL | SQL | PromQL |
| Clustering | Enterprise | Built-in | Federation |
| Retention Policies | Native | Manual | Native |
| Compression | 90%+ | 80%+ | 85%+ |
| **Decision** | ✅ **CHOSEN** | ❌ Lower write perf | ❌ Pull model |

**Schema Design:**
```
Measurement: server_metrics
  Tags (indexed):
    - host (server hostname)
    - datacenter (us-east-1, etc)
    - environment (prod, staging)
    - metric_name (cpu_usage, memory_used)

  Fields (not indexed):
    - value (float64)
    - unit (string)

  Timestamp: nanosecond precision
```

**Retention Policies:**
```sql
-- High-resolution data: 7 days
CREATE RETENTION POLICY "7_days"
  ON "metrics"
  DURATION 7d
  REPLICATION 1
  DEFAULT

-- Downsampled data: 90 days (1-minute resolution)
CREATE RETENTION POLICY "90_days"
  ON "metrics"
  DURATION 90d
  REPLICATION 1

-- Long-term aggregates: 1 year (1-hour resolution)
CREATE RETENTION POLICY "1_year"
  ON "metrics"
  DURATION 365d
  REPLICATION 1
```

**Continuous Queries (Downsampling):**
```sql
-- Downsample to 1-minute averages
CREATE CONTINUOUS QUERY "cq_1min"
  ON "metrics"
  BEGIN
    SELECT mean(value) as value
    INTO "90_days"."server_metrics"
    FROM "7_days"."server_metrics"
    GROUP BY time(1m), *
  END
```

**Sharding Strategy:**
```
3 InfluxDB nodes:
  - Shard by time ranges (weekly shards)
  - Each shard replicated 2x
  - Query fan-out across shards
  - Automatic shard migration
```

---

### 6. Visualization Layer (Grafana)

**Dashboards:**
1. **Infrastructure Overview**
   - CPU/Memory/Disk heatmap across all servers
   - Network traffic (ingress/egress)
   - Error rates and latencies

2. **Application Metrics**
   - Custom business metrics
   - Request rates and response times
   - Database query performance

3. **Alert Dashboard**
   - Active alerts
   - Alert history
   - MTTD (Mean Time to Detect)
   - MTTR (Mean Time to Resolve)

**Alerting Channels:**
- Email (high-severity)
- PagerDuty (critical)
- Slack (informational)
- Webhooks (custom integrations)

---

## Data Flow Architecture

### Write Path (Ingestion → Storage)

```
1. Agent pushes metrics
   └─> HTTP POST /metrics
       Payload: {"metrics": [{"name": "cpu", "value": 75.5, ...}]}

2. Ingestion server processes
   ├─> Rate limiting check (per client: 100 req/sec)
   ├─> JSON parsing (O(n) custom parser)
   ├─> Validation (schema, ranges)
   └─> Kafka produce (async, batched)

3. Kafka persists and distributes
   ├─> Write to disk (replicated 3x)
   ├─> Partition by client_id
   └─> Notify consumers

4. Flink consumes and processes
   ├─> Real-time aggregation (1-min windows)
   ├─> Alert evaluation
   └─> Enrichment (add datacenter, tags)

5. InfluxDB stores
   ├─> Time-series optimized storage
   ├─> Compression (90%+ reduction)
   └─> Indexing (tags only)

Latency Budget:
  - Ingestion: < 5ms
  - Kafka: < 10ms
  - Flink: < 100ms
  - InfluxDB write: < 50ms
  Total: < 200ms (well under 1-second SLA)
```

### Read Path (Query → Visualization)

```
1. Grafana issues query
   └─> InfluxQL: SELECT mean(cpu) WHERE time > now() - 1h

2. InfluxDB processes
   ├─> Query planner (choose optimal shard)
   ├─> Parallel scan across time ranges
   ├─> Aggregation in-database
   └─> Return compressed result

3. Grafana renders
   ├─> Data transformation
   ├─> Visualization (graph, heatmap, table)
   └─> Cache (30-second TTL)

Query Latency:
  - Simple query (1 host, 1 metric, 1h): < 100ms
  - Complex query (100 hosts, 10 metrics, 24h): < 2s
  - Dashboard load (20 panels): < 5s
```

---

## Failure Modes & Mitigation

### Ingestion Server Failure
**Scenario:** One of 10 ingestion servers crashes

**Impact:**
- Reduced capacity: 100K → 90K RPS
- Load balancer redirects traffic to healthy servers
- No data loss (agents retry with exponential backoff)

**Mitigation:**
- Auto-scaling: Kubernetes detects failure, spins up replacement
- Health checks: /health endpoint polled every 10s
- Recovery time: < 60s (pod startup + warm-up)

---

### Kafka Broker Failure
**Scenario:** 1 of 3 Kafka brokers becomes unavailable

**Impact:**
- Replication factor = 3, so no data loss
- Performance degradation: higher latency as clients reconnect
- Some partitions temporarily unavailable (leader election)

**Mitigation:**
- Leader election: < 30s for new leader
- Client retries: Automatic reconnection
- Monitoring: Alert on under-replicated partitions
- Recovery: Replace failed broker, data auto-replicates

---

### InfluxDB Node Failure
**Scenario:** 1 of 3 InfluxDB nodes crashes

**Impact:**
- Queries to failed node's shards timeout
- Write capacity reduced (lost 1/3 of cluster)

**Mitigation:**
- Shard replication: Each shard on 2 nodes minimum
- Query retries: Grafana retries failed queries
- Write buffer: Flink buffers writes, replays on recovery
- Recovery: Restore from snapshot + WAL replay

---

### Network Partition (Split Brain)
**Scenario:** Network partition between datacenters

**Impact:**
- Agents in partitioned datacenter can't reach ingestion
- Risk of data loss if partition lasts > agent buffer time (5 min)

**Mitigation:**
- Multi-region deployment: Ingestion servers in each datacenter
- Local caching: Agents buffer up to 5 minutes
- Monitoring: Detect partition via heartbeat loss
- Post-partition: Reconcile data from agent buffers

---

## Scaling Characteristics

### Vertical Scaling Limits (Single Server)

| Resource | Limit | Bottleneck |
|----------|-------|------------|
| CPU | 10K RPS | JSON parsing CPU-bound |
| Memory | 20K clients | Rate limiter state grows linearly |
| Disk I/O | 5K writes/sec | fsync() latency |
| Network | 50K RPS | 1 Gbps NIC saturates at ~10MB/sec |

**Conclusion:** Single ingestion server maxes out around 10K RPS

---

### Horizontal Scaling (Cluster)

**Target:** 100K RPS across 10 ingestion servers

```
Load Balancer (AWS ALB)
        │
        ├──────┬──────┬──────┬──────┬───── (10 ingestion servers)
        │      │      │      │      │
    Server1 Server2 Server3 ... Server10
    10K RPS 10K RPS 10K RPS     10K RPS
        │      │      │      │      │
        └──────┴──────┴──────┴──────┘
                    │
                Kafka Cluster
            (handles 100K msg/sec)
```

**Scaling Strategy:**
1. Stateless ingestion servers (easy to add/remove)
2. Load balancer distributes based on least-connections
3. Auto-scaling: Add server when avg CPU > 70%
4. Kafka partitions = 12 (allows 12 parallel consumers)

**Cost Scaling:**
- 10 ingestion servers × $73/month = $730/month
- Kafka cluster (3 × m5.xlarge) = $1,260/month
- InfluxDB cluster (3 × i3.2xlarge) = $3,050/month
- **Total infrastructure: ~$5,000/month** for 100K RPS

---

## Technology Stack Summary

| Layer | Technology | Justification |
|-------|-----------|---------------|
| **Agents** | Go | Low overhead, easy cross-platform deployment |
| **Ingestion** | C++ (custom) | Learning, performance optimization opportunity |
| **Load Balancer** | AWS ALB | Managed, auto-scaling, health checks |
| **Message Queue** | Kafka | High throughput, durability, replay capability |
| **Stream Processing** | Apache Flink | Low latency, exactly-once, SQL support |
| **Time-Series DB** | InfluxDB | Optimized for metrics, compression, retention |
| **Visualization** | Grafana | Industry standard, plugin ecosystem |
| **Orchestration** | Kubernetes | Container management, auto-scaling, self-healing |

---

## Future Enhancements (Phase 6+)

### Machine Learning Integration
- Anomaly detection using Prophet/ARIMA
- Predictive alerting (alert before failure)
- Capacity forecasting

### Multi-Tenancy
- Per-tenant quotas and isolation
- Custom retention policies per tenant
- RBAC (Role-Based Access Control)

### Global Distribution
- Multi-region active-active
- CRDTs for conflict resolution
- Geo-routing (route to nearest ingestion)

### Cost Optimization
- S3 archival for metrics older than 90 days
- Intelligent tiering (frequently queried → hot storage)
- Spot instances for Flink workers

---

## References

- **Kafka Documentation:** [https://kafka.apache.org/documentation/](https://kafka.apache.org/documentation/)
- **Apache Flink:** [https://flink.apache.org/](https://flink.apache.org/)
- **InfluxDB Best Practices:** [https://docs.influxdata.com/influxdb/](https://docs.influxdata.com/)
- **Grafana Provisioning:** [https://grafana.com/docs/](https://grafana.com/docs/)
- **System Design Interview:** [https://systemdesignschool.io/](https://systemdesignschool.io/)
