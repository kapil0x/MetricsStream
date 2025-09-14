# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

MetricStream is a high-performance metrics collection, processing, and querying system designed for real-time metric data at scale. The system follows a layered architecture with event-driven components optimized for time-series data processing.

## Architecture

The system implements a multi-layer architecture:
- **Ingestion Layer**: HTTP/gRPC APIs for metric submission with validation and rate limiting
- **Message Queue**: Apache Kafka for durable message buffering and partitioning
- **Stream Processing**: Real-time aggregation and anomaly detection using Kafka Streams
- **Storage Layer**: ClickHouse for time-series data, PostgreSQL for metadata, Redis for caching
- **Query Layer**: GraphQL/REST APIs for metric queries and dashboard integration

## Core Components Structure

```
src/
├── ingestion/          # Metrics collection APIs and validation
│   ├── adapters/       # Protocol-specific adapters (HTTP, gRPC)
│   ├── validators/     # Data validation and normalization
│   └── rate_limiter/   # Ingestion rate control
├── processing/         # Stream processing pipeline
│   ├── aggregators/    # Real-time metric aggregation
│   ├── transformers/   # Data transformation logic
│   └── rules_engine/   # Processing rules and anomaly detection
├── storage/           # Storage abstractions and interfaces
│   ├── timeseries/    # Time-series operations (ClickHouse)
│   ├── metadata/      # Metadata management (PostgreSQL)
│   └── retention/     # Data retention policies
├── query/             # Query processing engine
│   ├── parser/        # Query language parser
│   ├── optimizer/     # Query optimization
│   └── executor/      # Query execution
├── api/               # External API layer
│   ├── grpc/          # gRPC service definitions
│   ├── http/          # REST endpoints
│   └── websocket/     # Real-time streaming
└── common/            # Shared utilities and libraries
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

### Development Environment
```bash
# Start local dependencies (Kafka, ClickHouse, PostgreSQL, Redis)
docker-compose up -d

# Build and run specific component
cd src/ingestion
make ingestion_service
./ingestion_service --port=8001

# Stop dependencies
docker-compose down
```

## Data Flow and Integration

Metrics flow through the system as follows:
1. **Ingestion**: Clients submit metrics via HTTP/gRPC → validation and rate limiting
2. **Queuing**: Validated metrics published to Kafka topics partitioned by source/type
3. **Processing**: Kafka Streams applications perform real-time aggregation and enrichment
4. **Storage**: Processed metrics stored in ClickHouse with metadata in PostgreSQL
5. **Querying**: Query engine optimizes and executes requests against stored data

## Key Design Decisions

- **Time-Series First**: All data modeling optimized for time-series workloads
- **Event-Driven**: Components communicate via Kafka for loose coupling
- **Horizontal Scaling**: Architecture supports sharding and clustering from day one
- **Multi-Protocol Support**: Both HTTP/REST and gRPC for different client needs
- **Hybrid Processing**: Real-time for alerting, batch for complex analytics

## Performance Targets

- **MVP**: 10K metrics/second ingestion, <100ms query latency
- **Scale Phase**: 100K metrics/second with horizontal scaling
- **Enterprise**: 1M+ metrics/second with multi-tenant support

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

## Storage Considerations

### Time-Series Data (ClickHouse)
- Partitioned by time and metric type for optimal query performance
- Compression ratio typically 10:1
- Retention policies: raw (7d), minute aggregates (30d), hour aggregates (1y)

### Metadata (PostgreSQL)
- Service configurations, user management, alert rules
- ACID compliance for critical configuration data

### Caching (Redis)
- Hot metrics data for sub-second query response
- Rate limiting counters and circuit breaker state
- Query result caching with TTL-based invalidation

## Security Model

- TLS 1.3 for all external communications
- API key-based authentication with rate limiting per client
- Input validation and sanitization at ingestion layer
- PII detection and masking for compliance
- Configurable data retention for GDPR compliance