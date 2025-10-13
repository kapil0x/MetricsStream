# Craft #3: Time-Series Storage Engine

**Status:** 🔜 Coming Soon

**Goal:** Build a time-series database (like InfluxDB or TimescaleDB) optimized for storing and querying metrics.

## What You'll Build

A production-grade storage engine with:
- **LSM tree architecture** for write-optimized storage
- **Gorilla compression** for time-series data (10:1 ratio)
- **Tag-based indexing** for fast filtering
- **Time-range partitioning** for efficient queries
- **Compaction** for space reclamation

## Systems Design Focus

- Storage engine trade-offs: write vs read optimization
- Compression algorithms specific to time-series data
- Indexing strategies for different query patterns
- Data layout for sequential and random access
- LSM tree compaction strategies

## Target Performance

- **Write throughput:** 1M+ data points/sec
- **Compression:** 10:1 ratio (8 bytes per data point)
- **Query latency:** <1 second for 24-hour time range
- **Storage:** Efficient for high-cardinality metrics

## Service API Contract

See [API.md](API.md) for the complete API specification.

**Write API:**
```
POST /write
Body: {
  "measurement": "cpu_usage",
  "tags": {"host": "server01", "region": "us-west"},
  "fields": {"value": 75.5},
  "timestamp": 1696789234567
}
```

**Query API:**
```
POST /query
Body: {
  "measurement": "cpu_usage",
  "start": 1696789230000,
  "end": 1696875630000,
  "filters": {"host": "server01"}
}
Response: {"data_points": [...]}
```

## Architecture Overview

```
Message Queue (Craft #2)
    ↓
┌────────────────────────────────────┐
│  Storage Engine                    │
│                                    │
│  MemTable (in-memory, sorted)     │
│      ↓                             │
│  WAL (write-ahead log, durability) │
│      ↓                             │
│  SSTables (immutable, on-disk)    │
│  ├─ L0: Recent data (uncompressed)│
│  ├─ L1: Compacted (compressed)    │
│  └─ L2: Historical (downsampled)  │
│                                    │
│  Indexes (tag → SSTable)          │
└────────────────────────────────────┘
    ↓
Query Engine (Craft #4)
```

## Implementation Plan

### Phase 1: Basic Storage
- In-memory MemTable with sorted insert
- Write-ahead log for crash recovery
- Flush to disk as SSTables

### Phase 2: LSM Tree
- Multi-level SSTables (L0, L1, L2)
- Background compaction thread
- Merge-sort during compaction

### Phase 3: Compression
- Delta encoding for timestamps
- XOR encoding for float values (Gorilla)
- Run-length encoding for repeated values

### Phase 4: Indexing
- Inverted index for tags
- Bloom filters for existence checks
- Time-range index for efficient scans

### Phase 5: Optimization
- Batch writes for throughput
- Memory-mapped files for reads
- Block cache for hot data

## Key Learnings

- **LSM trees** optimize for write-heavy workloads
- **Gorilla compression** exploits patterns in time-series data
- **Indexing** is critical for query performance
- **Compaction** trades CPU for storage space
- **Data layout** matters for query efficiency

## Prerequisites

- Complete [Craft #2: Message Queue](../craft2/)
- Understand storage from [Craft #0](../phase0/)
- Familiarity with compression algorithms helpful

## Next Steps

1. **[→ Read API Specification](API.md)**
2. **[→ Review Design Decisions](DESIGN.md)**
3. **[→ Start Implementation Guide](TUTORIAL.md)** (Coming soon)

## Service Decomposition for AI Agents

This craft can be built by 3 parallel agents:

**Agent 1: Storage Engine**
- MemTable and WAL implementation
- SSTable format and writing
- LSM tree structure and compaction

**Agent 2: Compression**
- Gorilla algorithm (delta + XOR)
- Encoding/decoding routines
- Compression benchmarking

**Agent 3: Indexing**
- Tag inverted index
- Bloom filters
- Query optimization using indexes

**Integration point:** All agents implement interfaces defined in `API.md` and `DESIGN.md`.

---

**This is Craft #3 of Systems Craft.** Once complete, you'll understand how InfluxDB, TimescaleDB, and other time-series databases work internally.
