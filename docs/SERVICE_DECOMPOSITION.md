# Service Decomposition Guide

**Context:** This guide teaches how to decompose Systems Craft into independent services that can be built by separate teams or AI agents working in parallel.

## Overview

In production, the complete monitoring platform consists of 5 independent services, each with clear API contracts:

```
┌────────────────┐
│  Craft #1:     │  HTTP POST /metrics
│  Ingestion     │────────────┐
└────────────────┘            │
                              ↓
                    ┌────────────────┐
                    │  Craft #2:     │  /produce, /consume
                    │  Message Queue │────────────┐
                    └────────────────┘            │
                                                  ↓
                                        ┌────────────────┐
                                        │  Craft #3:     │  /write, /query
                                        │  Storage       │─────────┐
                                        └────────────────┘         │
                                                                   ↓
┌────────────────┐                            ┌────────────────────────┐
│  Craft #5:     │  /evaluate                 │  Craft #4:             │
│  Alerting      │←───────────────────────────│  Query Engine          │
└────────────────┘                            └────────────────────────┘
```

## Service Boundaries

### Service 1: Ingestion (Craft #1)

**Responsibility:** Accept metrics from clients via HTTP.

**API Contract:**
```
POST /metrics
Headers: Authorization: <client_id>
Body: {
  "metrics": [
    {"name": "cpu_usage", "value": 75.5, "type": "gauge"}
  ]
}
Response: 202 Accepted
```

**Dependencies:**
- Downstream: Message Queue (POST /produce)

**Team Skills:** HTTP server, rate limiting, concurrency

**AI Agent Task:** "Implement high-throughput HTTP ingestion service with per-client rate limiting (10 req/sec). Must achieve 2,000+ RPS with 100 concurrent clients. API contract: POST /metrics → return 202, forward to queue."

---

### Service 2: Message Queue (Craft #2)

**Responsibility:** Durable buffering between ingestion and storage.

**API Contract:**
```
POST /produce
Body: {
  "topic": "metrics.inbound",
  "key": "client_123",
  "value": <metric_json>
}
Response: {"partition": 3, "offset": 12847}

GET /consume?partition=3&offset=12847&count=100
Response: {
  "messages": [...],
  "next_offset": 12947
}
```

**Dependencies:**
- Upstream: Ingestion (producer)
- Downstream: Storage (consumer)

**Team Skills:** Distributed systems, consensus (Raft), persistence

**AI Agent Task:** "Implement distributed message queue with 12 partitions, write-ahead log, and Raft-based replication. API: /produce and /consume endpoints. Target: 1M+ messages/sec, <10ms latency."

---

### Service 3: Time-Series Storage (Craft #3)

**Responsibility:** Store and retrieve metrics efficiently.

**API Contract:**
```
POST /write
Body: {
  "measurement": "cpu_usage",
  "tags": {"host": "server01"},
  "fields": {"value": 75.5},
  "timestamp": 1696789234567
}

POST /query
Body: {
  "measurement": "cpu_usage",
  "start": 1696789230000,
  "end": 1696875630000,
  "filters": {"host": "server01"}
}
Response: {"data_points": [...]}
```

**Dependencies:**
- Upstream: Message Queue (consumer from "metrics.inbound")
- Downstream: Query Engine (read access)

**Team Skills:** Storage engines, compression, indexing

**AI Agent Task:** "Implement LSM tree storage with Gorilla compression for time-series data. API: /write and /query endpoints. Target: 1M+ writes/sec, 10:1 compression, <1s queries for 24h range."

---

### Service 4: Query Engine (Craft #4)

**Responsibility:** Execute queries and aggregations on stored data.

**API Contract:**
```
POST /query
Body: {
  "query": "SELECT avg(cpu_usage) WHERE host='server01' AND time > now() - 1h GROUP BY time(5m)"
}
Response: {
  "series": [
    {"time": 1696789500000, "avg": 75.5},
    ...
  ]
}
```

**Dependencies:**
- Upstream: Storage (read /query)
- Downstream: Alerting (query execution for rules)

**Team Skills:** Query parsing, execution planning, parallel processing

**AI Agent Task:** "Implement SQL-like query engine with parser, planner, and parallel executor. Support avg/sum/percentile aggregations. API: POST /query with SQL-like syntax. Target: <1s for 100M+ data points."

---

### Service 5: Alerting (Craft #5)

**Responsibility:** Evaluate rules and send notifications.

**API Contract:**
```
POST /rules
Body: {
  "name": "high_cpu_alert",
  "query": "avg(cpu_usage) > 80",
  "duration": "5m",
  "channels": ["slack", "email"]
}

GET /alerts
Response: {
  "alerts": [
    {"rule": "high_cpu_alert", "state": "firing", "value": 85.5, ...}
  ]
}
```

**Dependencies:**
- Upstream: Query Engine (execute alert queries)
- Downstream: Notification services (Slack, email, PagerDuty)

**Team Skills:** Event processing, state machines, notification APIs

**AI Agent Task:** "Implement alert evaluation engine with state machine (pending→firing→resolved). Support Slack/email/webhook notifications. API: /rules and /alerts endpoints. Target: 10K+ rules/sec, <1 min detection latency."

---

## Parallel Development Patterns

### Pattern 1: API-First Development

1. **Define API contracts FIRST** (in API.md files)
2. Each team/agent implements their service independently
3. Use mocks/stubs for dependencies during development
4. Integration testing once all services complete

**Example:**
```
Week 1: Define all 5 API contracts
Week 2: Agent 1 builds Ingestion (mocks Queue)
        Agent 2 builds Queue (mocks Storage)
        Agent 3 builds Storage (standalone tests)
Week 3: Integration testing, fix contract mismatches
```

### Pattern 2: Vertical Slicing

Build end-to-end for a single metric type first:

```
Sprint 1: All 5 services handle "cpu_usage" metric only
Sprint 2: Expand to handle all metric types
Sprint 3: Add advanced features (compression, caching, etc.)
```

**Benefits:** Early integration, faster feedback loop

### Pattern 3: Interface-Based Contracts

Define Go/C++ interfaces for service boundaries:

```cpp
// queue_interface.h
class IMessageQueue {
public:
    virtual ProduceResult produce(const Message& msg) = 0;
    virtual ConsumeResult consume(int partition, int offset, int count) = 0;
};

// Ingestion service uses IMessageQueue, doesn't care about implementation
// Queue team implements IMessageQueue independently
```

### Pattern 4: Contract Testing

Each service publishes contract tests:

```bash
# Queue service provides contract tests
craft2/tests/producer_contract_test.cpp  # Tests expected by Ingestion
craft2/tests/consumer_contract_test.cpp  # Tests expected by Storage

# Ingestion runs Queue's producer contract tests against mocks
# Storage runs Queue's consumer contract tests against mocks
```

---

## AI Agent Coordination Examples

### Example 1: Building Craft #2 with 3 Agents

**Your role (Tech Lead):**
1. Define API contract in `craft2/API.md`
2. Define interface in `craft2/queue_interface.h`
3. Spawn 3 agents with clear tasks

**Agent 1: WAL & Persistence**
```
Task: Implement write-ahead log for durability.
- File: craft2/wal.cpp, craft2/wal.h
- Interface: WAL::append(), WAL::read()
- Tests: craft2/tests/wal_test.cpp
- No network code, pure storage logic
```

**Agent 2: Partitioning & Routing**
```
Task: Implement partition management and key-based routing.
- File: craft2/partition.cpp, craft2/partition.h
- Interface: PartitionManager::get_partition(key), Partition::append()
- Tests: craft2/tests/partition_test.cpp
- Uses WAL interface (mock for unit tests)
```

**Agent 3: HTTP Server**
```
Task: Implement /produce and /consume HTTP endpoints.
- File: craft2/http_server.cpp
- Routes: POST /produce → PartitionManager::get_partition()
- Tests: craft2/tests/http_test.cpp
- Uses PartitionManager interface (mock for unit tests)
```

**Integration (You):**
1. Review each agent's code
2. Wire up components: `main.cpp` creates WAL → PartitionManager → HTTPServer
3. Run integration tests
4. Measure performance, iterate

---

### Example 2: Optimizing Multiple Crafts in Parallel

**Scenario:** Craft #1-3 are complete, now optimize independently.

**Agent 1: Optimize Ingestion (Craft #1)**
```
Task: Reduce p99 latency from 0.65ms to <0.3ms.
- Profile ingestion_service.cpp, identify bottlenecks
- Implement lock-free rate limiting with sharding
- Measure before/after, document in performance_results.txt
- NO changes to HTTP API contract
```

**Agent 2: Optimize Storage (Craft #3)**
```
Task: Improve compression ratio from 8:1 to 10:1.
- Implement dictionary compression for tag values
- Benchmark on 1M metrics from production dataset
- Document in craft3/COMPRESSION_RESULTS.md
- NO changes to /write or /query API
```

**Agent 3: Add Query Caching (Craft #4)**
```
Task: Cache query results for 60 seconds.
- Implement LRU cache with 1GB limit
- Cache key: hash(query + time_range)
- Measure cache hit rate on dashboard queries
- NO changes to POST /query API
```

**Key:** All optimizations are independent because API contracts are stable.

---

## Best Practices

### 1. Define Contracts Early
- Write API specs before implementation
- Include error cases and edge conditions
- Version APIs (e.g., `/v1/metrics`)

### 2. Use Feature Flags
- Enable gradual rollout of new features
- Test integrations without blocking development
- Example: `ENABLE_COMPRESSION=1 ./metricstream_server`

### 3. Monitor Integration Points
- Log all cross-service calls
- Measure latency at service boundaries
- Alert on contract violations (unexpected responses)

### 4. Design for Failure
- Each service must handle downstream failures gracefully
- Example: Ingestion queues locally if Message Queue is down
- Example: Query Engine returns cached results if Storage is slow

### 5. Document Dependencies
- Maintain dependency graph
- Update when adding new service interactions
- Review during architecture changes

---

## Summary

**Key Principles:**

1. **API contracts enable parallelism** - define first, implement independently
2. **Each service has clear boundaries** - single responsibility, well-defined interfaces
3. **Integration is separate from implementation** - test components in isolation first
4. **AI agents (or teams) can work in parallel** - with clear contracts and interfaces
5. **Coordination is a critical skill** - decomposition, review, integration

**This is how distributed systems are built at scale—whether by human teams or AI agents.**

---

**Next Steps:**
1. Review API contracts for each craft (craft*/API.md - coming soon)
2. Choose a craft to implement
3. Practice decomposing it into sub-tasks for parallel agents
4. Coordinate implementation, integration, and testing

**Welcome to distributed systems engineering.**
