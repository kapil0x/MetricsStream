# Craft #2: Distributed Message Queue

**Status:** 🔜 Coming Soon

**Goal:** Build a distributed message queue (like Kafka or Pulsar) that buffers metrics between ingestion and storage.

## What You'll Build

A production-grade message queue with:
- **Partitioning** for parallel processing and scalability
- **Write-ahead log (WAL)** for durability
- **Consumer groups** for load distribution
- **Offset management** for reliable consumption
- **Replication** with leader election (Raft consensus)

## Systems Design Focus

- Partitioning trade-offs: parallelism vs ordering guarantees
- Message ordering: per-partition vs global ordering
- Delivery semantics: at-least-once vs exactly-once
- Leader election: Raft consensus protocol
- Consumer coordination: group membership and rebalancing

## Target Performance

- **Throughput:** 1M+ messages/sec
- **Latency:** <10ms p99
- **Scalability:** Horizontal scaling with partitions
- **Durability:** Survive broker failures with replication

## Service API Contract

See [API.md](API.md) for the complete API specification.

**Producer API:**
```
POST /produce
Body: {"topic": "metrics.inbound", "key": "client_123", "value": {...}}
Response: {"partition": 3, "offset": 12847}
```

**Consumer API:**
```
GET /consume?partition=3&offset=12847&count=100
Response: {"messages": [...], "next_offset": 12947}
```

**Consumer Group API:**
```
POST /subscribe
Body: {"group": "storage-workers", "topics": ["metrics.inbound"]}
Response: {"assigned_partitions": [3, 7, 11]}
```

## Architecture Overview

```
Producers (Craft #1: Ingestion)
    ↓
┌──────────────────────────────────────┐
│  Message Queue Cluster (3 brokers)  │
│                                      │
│  Topic: metrics.inbound             │
│  ├─ Partition 0 (Leader: B1, R: B2) │
│  ├─ Partition 1 (Leader: B2, R: B3) │
│  └─ Partition 2 (Leader: B3, R: B1) │
└──────────────────────────────────────┘
    ↓
Consumer Groups (Craft #3: Storage)
```

## Implementation Plan

### Phase 1: Single-Node Queue
- In-memory queue with persistence
- Write-ahead log (append-only file)
- Produce and consume APIs

### Phase 2: Partitioning
- Multiple partitions for parallelism
- Key-based routing (hash partitioning)
- Independent offset tracking per partition

### Phase 3: Consumer Groups
- Group membership management
- Partition assignment and rebalancing
- Offset commit and tracking

### Phase 4: Replication
- Leader-follower replication per partition
- Raft consensus for leader election
- Automatic failover on broker failure

### Phase 5: Optimization
- Batch writes to WAL
- Zero-copy message transfer
- Compression support

## Key Learnings

- **Partitioning enables parallelism** but requires careful key design
- **Ordering guarantees** are per-partition, not global
- **Replication** trades write latency for durability
- **Consumer groups** enable horizontal scaling of consumers
- **WAL** is the foundation of durability in distributed systems

## Prerequisites

- Complete [Craft #1: Metrics Ingestion](../src/)
- Understand producer-consumer patterns from [Craft #0](../phase0/)
- Familiarity with consensus algorithms (Raft) helpful but not required

## Next Steps

1. **[→ Read API Specification](API.md)**
2. **[→ Review Design Decisions](DESIGN.md)**
3. **[→ Start Implementation Guide](TUTORIAL.md)** (Coming soon)

## Service Decomposition for AI Agents

This craft can be built by 3 parallel agents:

**Agent 1: WAL & Persistence**
- Implement write-ahead log
- Partition storage management
- Offset indexing

**Agent 2: Network & APIs**
- HTTP server for produce/consume
- Request routing to partitions
- Client connection management

**Agent 3: Coordination**
- Consumer group management
- Partition assignment algorithm
- Offset commit tracking

**Integration point:** All agents implement interfaces defined in `API.md` and `DESIGN.md`.

---

**This is Craft #2 of Systems Craft.** Once complete, you'll understand how Kafka, Pulsar, and RabbitMQ work internally.
