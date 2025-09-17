# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MetricStream is an educational platform for learning performance optimization, concurrency, system design, and scalable architecture patterns. Built as a metrics collection system, it serves as a hands-on laboratory for exploring async programming, load testing, and distributed systems concepts without the complexity of production monitoring platforms.

## Principal Engineer Learning Objectives

This project is designed for deep technical mastery and principal engineer-level system design skills:

### **Core System Design Mastery**
- **Performance Engineering**: CPU profiling, memory optimization, I/O bottleneck analysis
- **Concurrency Deep Dive**: Lock-free programming, memory models, async runtime design
- **Distributed Systems**: Consensus algorithms, CAP theorem trade-offs, failure mode analysis
- **Architecture Evolution**: Monolith → microservices → event-driven → serverless patterns

### **Principal Engineer Debugging Skills**
- **Linux Performance Analysis**: perf, strace, tcpdump, eBPF for production debugging
- **Memory Profiling**: Valgrind, AddressSanitizer, heap analysis, memory leak detection
- **Network Debugging**: Wireshark packet analysis, TCP tuning, connection pooling
- **Distributed Tracing**: Correlating failures across service boundaries

### **Production Engineering Excellence**
- **Observability Design**: Metrics design patterns, alerting strategies, SLI/SLO implementation
- **Capacity Planning**: Load modeling, hardware sizing, cost optimization
- **Failure Engineering**: Chaos testing, circuit breakers, graceful degradation
- **Performance Tuning**: Kernel parameters, GC tuning, database optimization

### **Advanced Technical Leadership**
- **Architecture Decision Records**: Document trade-offs and reasoning
- **Code Review Excellence**: Performance implications, security vulnerabilities
- **Technical Mentoring**: Teaching through code examples and system design

The system serves as a hands-on laboratory for mastering these principal engineer competencies.

## Architecture

**Current Implementation (First-Principles Approach):**
Started with the simplest possible approach and systematically optimized:

- **HTTP Server**: Raw socket implementation with concurrent request handling
- **Rate Limiting**: Sliding window algorithm with lockless metrics collection
- **JSON Processing**: Custom parser for zero-dependency operation
- **Storage**: JSON Lines file format with asynchronous batch writer
- **Load Testing**: Multi-client load generator with realistic metric patterns

**Architecture Evolution Journey:**
1. **MVP**: Single-threaded HTTP → synchronous file I/O
2. **Phase 1**: Added threading for concurrent request processing  
3. **Phase 2**: Async I/O with producer-consumer pattern for file writes
4. **Phase 3**: JSON parsing optimization (current focus)
5. **Future**: Message queues, distributed storage, horizontal scaling

## Current Implementation Structure

```
src/
├── main.cpp                    # Server startup and signal handling
├── http_server.cpp            # Raw socket HTTP implementation
├── ingestion_service.cpp      # Business logic, JSON parsing, async I/O
└── metric.h                   # Core data structures

include/
├── http_server.h             # HTTP server interface  
├── ingestion_service.h       # Service definitions and async writer
└── metric.h                  # Metric types and data structures

Performance Testing:
├── load_test.cpp             # Multi-client load generator
├── performance_test.sh       # Systematic RPS testing (100→2500)
└── monitor_load_test.sh      # System monitoring during tests

Build:
├── CMakeLists.txt           # Build configuration
└── build/                   # Compiled binaries and metrics data
```

## Development Commands

### Build System
The project uses CMake for building:
```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### Testing
```bash
# Run all tests
make test

# Run specific test suite
ctest -R "ingestion_tests"

# Run with coverage
make coverage
```

### Running the Server
```bash
# Start the metrics server
./build/metricstream_server

# Test with single request
curl -X POST http://localhost:8080/metrics \
  -H "Content-Type: application/json" \
  -H "Authorization: test_client" \
  -d '{"metrics":[{"name":"cpu_usage","value":75.5,"type":"gauge"}]}'

# Run load tests
./build/load_test 8080 50 10 100  # 50 clients, 10 req/client, 100ms interval

# Monitor performance
./performance_test.sh  # Systematic 100→2500 RPS testing
```

## Current Data Flow

**Current Implementation (File-Based):**
1. **HTTP Request**: Client sends JSON metrics via POST to `/metrics`
2. **Rate Limiting**: Sliding window algorithm checks per-client request limits
3. **JSON Parsing**: Custom parser extracts metrics without external dependencies
4. **Validation**: Metric names, values, and types validated
5. **Async Queuing**: Metrics queued for background writer (no blocking)
6. **File Storage**: Background thread writes JSON Lines to `metrics.jsonl`
7. **Response**: Fast HTTP response while file I/O happens asynchronously

**Key Performance Optimizations:**
- **Threading**: Each HTTP request processed concurrently
- **Producer-Consumer**: Request threads queue, writer thread processes
- **Zero-Copy**: Minimal string allocations where possible
- **Lock-Free Metrics**: Rate limiter uses ring buffer for stats

## Key Design Decisions

**Current Phase (First-Principles):**
- **File-First Approach**: Start simple before distributed complexity  
- **Zero Dependencies**: Custom JSON parser, raw socket HTTP server
- **Performance Focused**: Each optimization measured with concrete metrics
- **Educational**: Document bottlenecks and solutions for learning
- **Concurrent Design**: Threading → Async I/O → Memory optimization progression

**Future Evolution:**
- **Distributed Storage**: Multiple files → Database → Distributed storage
- **Message Queues**: Direct file writes → Kafka → Stream processing  
- **Horizontal Scaling**: Single server → Load balancer

## Principal Engineer Learning Progression

### **Phase 1: Sequential → Concurrent Processing** ✅
- **Problem**: Single-threaded server, connection queue overflow
- **Solution**: Threading per request for concurrent handling
- **Result**: 20 clients: 81% → 88% success rate (+7% improvement)
- **Learning**: Connection handling vs request processing bottlenecks

### **Phase 2: Sync → Async I/O** ✅  
- **Problem**: File mutex serialization, threads waiting for I/O
- **Solution**: Producer-consumer pattern with background writer
- **Result**: 50 clients: 59% → 66% success rate, reduced latency 10%
- **Learning**: I/O serialization creates convoy effects under load

### **Phase 3: JSON Parsing Optimization** 🔄 (Current)
- **Problem**: String operations create memory pressure at high concurrency
- **Current**: 100 clients: 35% success rate, 3.2ms latency
- **Target**: Optimize string allocations, consider JSON library
- **Learning**: CPU-intensive operations become bottlenecks after I/O optimization

### **Phase 4: Memory & Threading Limits** (Next)
- **Target**: Identify thread creation overhead, memory allocation patterns
- **Skills**: Memory profiling, thread pool optimization
- **Tools**: Valgrind, memory sanitizers, thread analysis
- **Learning**: Resource limits vs algorithmic optimization

### **Phase 5: Distributed Architecture** (Future)
- **Target**: 10K+ req/sec with horizontal scaling
- **Skills**: Message queues, load balancing, distributed storage
- **Tools**: Kafka, distributed tracing, chaos engineering  
- **Learning**: CAP theorem trade-offs, consistency patterns

Each phase includes hands-on debugging scenarios and architecture decision documentation.

## Debugging and Observability

The system implements distributed tracing with correlation IDs:
- All requests tagged with X-Correlation-ID header
- Jaeger tracing across all components
- Structured logging with correlation context
- Prometheus metrics for all services
- Grafana dashboards for system monitoring

## Component Testing Strategy

- **Unit Tests (70%)**: Fast, isolated testing with mocked dependencies
- **Integration Tests (25%)**: Component interaction testing with real databases
- **End-to-End Tests (5%)**: Full workflow validation in production-like environment

Each component maintains >90% test coverage with automated CI pipeline validation.

## Current Storage Implementation

### JSON Lines File Format
```bash
# Each metric stored as one JSON object per line in metrics.jsonl
{"timestamp":"2025-09-17T13:45:23.123Z","name":"cpu_usage","value":75.5,"type":"gauge","tags":{"host":"server1"}}
{"timestamp":"2025-09-17T13:45:23.124Z","name":"memory_usage","value":4500000000,"type":"gauge","tags":{"host":"server1"}}
```

**Design Benefits:**
- **Append-only**: Fast writes, no file locking complexity
- **Human readable**: Easy debugging and analysis  
- **Streaming friendly**: Can process line by line
- **Simple recovery**: Corrupted lines don't affect others

**Current Limitations:**
- **Single file**: Becomes bottleneck at very high volume
- **No indexing**: Linear scan for queries
- **No compression**: Storage efficiency could improve

**Future Evolution:**
- **Multiple files**: Partition by time/client for parallel writes
- **Database storage**: Time-series DB when file system limits reached
- **Compression**: LZ4/Snappy for storage efficiency

## Current Security Implementation

**Basic Security (Current):**
- **Rate Limiting**: Per-client sliding window to prevent abuse
- **Input Validation**: JSON parsing with bounds checking  
- **Authorization Header**: Simple client identification via `Authorization` header
- **Resource Limits**: Connection limits, request size limits

**Missing (Future Improvements):**
- **TLS/HTTPS**: Currently HTTP only for development simplicity
- **Authentication**: Real auth vs simple header-based identification  
- **Input Sanitization**: More robust validation and attack prevention
- **Monitoring**: Security event logging and alerting

**Educational Focus:**
- Learn rate limiting algorithms first
- Add TLS when moving to production deployment
- Implement proper auth when adding user management
- Security complexity grows with system maturity