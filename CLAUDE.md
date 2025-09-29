# CLAUDE.md

Guidance for Claude Code when working with this codebase.

## Project Overview

MetricStream - A comprehensive metrics platform (like Prometheus/DataDog) built from first principles. Currently implementing the ingestion service component, with focus on performance optimization and concurrent programming. This is one component of a larger distributed metrics system.

## Approach

This is a space to focus on craft - the tough engineering problems that push learning. With Claude Code, we can skip boilerplate and dive into the interesting parts:

- Performance bottleneck identification and resolution
- Concurrent programming patterns and race condition analysis  
- Memory layout and allocation optimization
- Systematic measurement and optimization methodology

The goal is deep understanding through building and measuring real systems.

## Current Implementation

**Component: Metrics Ingestion Service**
Built from first principles, optimized phase by phase:

- HTTP server from raw sockets
- Sliding window rate limiting  
- Custom JSON parser (zero dependencies)
- Async file I/O with producer-consumer pattern
- Load testing with systematic measurement

Each optimization phase targets a specific measured bottleneck.

**Future Components:**
- Time-series storage engine (ClickHouse integration)
- Query processor and aggregation engine
- Alerting and notification service
- Web UI and dashboards
- Distributed coordination and sharding

## Code Structure

```
src/
├── main.cpp                  # Entry point
├── http_server.cpp          # Socket handling
├── ingestion_service.cpp    # Core logic, rate limiting, JSON parsing
└── metric.h                 # Data structures

load_test.cpp               # Performance testing
performance_test.sh         # Systematic load testing
```

## Commands

```bash
# Build
mkdir build && cd build
cmake .. && make

# Run
./metricstream_server

# Test
./load_test 8080 50 10    # 50 clients, 10 requests each
./performance_test.sh     # Systematic load testing
```

## Data Flow

1. HTTP request → rate limiting check
2. JSON parsing → validation  
3. Async queue for file writing
4. Background thread writes to `metrics.jsonl`
5. HTTP response (non-blocking)

Key optimizations: threading per request, producer-consumer I/O, lock-free metrics collection.

## Design Decisions

- Zero dependencies (custom HTTP, JSON parsing)
- File-first storage (simple before distributed)
- Measure everything (performance_results.txt)
- Optimize based on bottlenecks, not assumptions
- Document each optimization phase with concrete metrics

## Optimization Phases

**Phase 1**: Threading per request  
- 20 clients: 81% → 88% success rate

**Phase 2**: Async I/O with producer-consumer  
- 50 clients: 59% → 66% success rate

**Phase 3**: JSON parsing optimization (O(n²) → O(n))  
- 100 clients: 80.2% success rate, 2.73ms latency

**Phase 4**: Hash-based per-client mutex optimization (current)  
- Target: eliminate double mutex bottleneck
- Expected: 95%+ success at 100 clients

Each phase targets the measured bottleneck, not theoretical improvements.

## Performance Methodology

Always measure before optimizing:

```bash
# Baseline
./build/load_test 8080 50 10
./build/load_test 8080 100 10
./build/load_test 8080 150 8

# Document in performance_results.txt
# Phase X: 50 clients → Y% success, Z.Zms latency
```

Template for each optimization:
- **Problem**: Specific bottleneck
- **Solution**: Implementation approach
- **Result**: Before/after measurements
- **Learning**: Key insight gained

Measure first, optimize second.

## Storage

JSON Lines format in `metrics.jsonl`:
```json
{"timestamp":"2025-09-17T13:45:23.123Z","name":"cpu_usage","value":75.5}
```

Benefits: append-only, human readable, streaming friendly
Limitations: single file, no indexing

## Security

Current: rate limiting, input validation, simple auth headers
Missing: TLS, real authentication (development focus for now)

## Current Focus

Phase 4: Hash-based per-client mutex optimization to eliminate double mutex bottleneck in rate limiting. See TODO(human) in `flush_metrics()` for contribution opportunity.