# Craft #4: Query & Aggregation Engine

**Status:** 🔜 Coming Soon

**Goal:** Build a query processor (like PromQL or SQL on time-series) for aggregating and analyzing metrics.

## What You'll Build

A production-grade query engine with:
- **Query parser** for SQL-like language
- **Execution planner** with optimization
- **Parallel aggregation** across time ranges
- **Aggregation functions** (sum, avg, percentiles, rate)
- **Result caching** for repeated queries

## Systems Design Focus

- Query language design and trade-offs
- Execution planning and optimization strategies
- Parallel processing for aggregations
- Memory management for large result sets
- Caching strategies for performance

## Target Performance

- **Query latency:** <1 second for 100M+ data points
- **Aggregation:** Compute avg/p99 across 24-hour windows
- **Parallelism:** Utilize all CPU cores for large queries
- **Cache hit rate:** 80%+ for dashboard queries

## Service API Contract

See [API.md](API.md) for the complete API specification.

**Query API:**
```
POST /query
Body: {
  "query": "SELECT avg(cpu_usage) FROM metrics WHERE host='server01' AND time > now() - 1h GROUP BY time(5m)",
  "start": 1696789230000,
  "end": 1696875630000
}
Response: {
  "series": [{
    "time": 1696789500000,
    "avg": 75.5
  }, ...]
}
```

**Supported Aggregations:**
- `avg()`, `sum()`, `min()`, `max()`, `count()`
- `percentile(p)` for p50, p95, p99
- `rate()` for per-second rate calculation
- `stddev()` for standard deviation

## Architecture Overview

```
Storage Engine (Craft #3)
    ↓
┌──────────────────────────────────────┐
│  Query Engine                        │
│                                      │
│  1. Parser (SQL → AST)              │
│      ↓                               │
│  2. Planner (AST → Execution Plan)  │
│      ↓                               │
│  3. Executor (Parallel execution)   │
│     ├─ Worker 1: Range [0-25%]      │
│     ├─ Worker 2: Range [25-50%]     │
│     ├─ Worker 3: Range [50-75%]     │
│     └─ Worker 4: Range [75-100%]    │
│      ↓                               │
│  4. Aggregator (Merge results)      │
│      ↓                               │
│  5. Cache (Store for reuse)         │
└──────────────────────────────────────┘
    ↓
Alerting Engine (Craft #5) / Dashboards
```

## Implementation Plan

### Phase 1: Parser
- Lexer for tokenization
- Parser for SQL-like syntax
- AST representation

### Phase 2: Execution Planner
- Query optimization rules
- Cost-based planning
- Physical plan generation

### Phase 3: Basic Aggregations
- Time-range scan
- Filter pushdown to storage
- Simple aggregations (avg, sum, count)

### Phase 4: Parallel Execution
- Partition time ranges across workers
- Thread pool for parallel processing
- Result merging and ordering

### Phase 5: Advanced Features
- Percentile calculations (P50, P95, P99)
- Rate and derivative functions
- Query result caching

## Key Learnings

- **Query planning** can reduce execution time by orders of magnitude
- **Filter pushdown** to storage layer reduces data transfer
- **Parallel execution** scales with CPU cores
- **Aggregation algorithms** have different memory/CPU trade-offs
- **Caching** is essential for dashboard performance

## Prerequisites

- Complete [Craft #3: Storage Engine](../craft3/)
- Understand querying from [Craft #0](../phase0/)
- Familiarity with SQL helpful but not required

## Next Steps

1. **[→ Read API Specification](API.md)**
2. **[→ Review Design Decisions](DESIGN.md)**
3. **[→ Start Implementation Guide](TUTORIAL.md)** (Coming soon)

## Service Decomposition for AI Agents

This craft can be built by 3 parallel agents:

**Agent 1: Parser & Planner**
- Lexer and parser implementation
- AST construction
- Query optimization rules

**Agent 2: Execution Engine**
- Parallel worker pool
- Time-range partitioning
- Storage interface integration

**Agent 3: Aggregation Functions**
- Statistical aggregations (avg, sum, etc.)
- Percentile algorithms
- Rate calculations

**Integration point:** All agents implement interfaces defined in `API.md` and `DESIGN.md`.

---

**This is Craft #4 of Systems Craft.** Once complete, you'll understand how PromQL, SQL on time-series, and query engines work internally.
