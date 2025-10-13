# CLAUDE.md

Guidance for Claude Code when working with this codebase.

## Project Overview

**Systems Craft: Build a Complete Monitoring Platform**

A hands-on tutorial series for building a production-grade monitoring platform from scratch. Students build real infrastructure through first principles, starting with a working proof-of-concept (Phase 0), then optimizing each component (Crafts #1-5).

**Learning Path:**
- **Phase 0 (Craft #0):** Complete monitoring PoC in 2-3 hours - validate architecture
- **Craft #1:** Metrics Ingestion optimization (200 → 2,253 RPS through 7 phases)
- **Craft #2:** Distributed Message Queue (Kafka-like, coming soon)
- **Craft #3:** Time-Series Storage Engine (InfluxDB-like, coming soon)
- **Craft #4:** Query & Aggregation Engine (PromQL-like, coming soon)
- **Craft #5:** Alerting & Notification System (PagerDuty-like, coming soon)

Students learn by doing - building each optimization themselves, measuring performance, and understanding trade-offs.

## Approach

This is a space to focus on craft - the tough engineering problems that push learning. With Claude Code, we can skip boilerplate and dive into the interesting parts:

- Performance bottleneck identification and resolution
- Concurrent programming patterns and race condition analysis  
- Memory layout and allocation optimization
- Systematic measurement and optimization methodology

The goal is deep understanding through building and measuring real systems.

## Current Implementation

**Phase 0: Complete Monitoring PoC** ✅ (phase0/)
A working end-to-end system in 600 lines:
- Ingestion API (single-threaded HTTP server)
- In-memory queue (thread-safe buffer)
- Storage consumer (background file writer)
- Query API (HTTP GET with filtering)
- Alerting engine (rule evaluation)

Performance: ~50-100 RPS (intentionally simple to show bottlenecks)

**Craft #1: Metrics Ingestion** ✅ (src/)
Optimized from first principles through 7 phases:
- HTTP server from raw sockets
- Thread pool for concurrent requests
- Sliding window rate limiting
- Custom JSON parser (O(n), zero dependencies)
- Async file I/O with producer-consumer pattern
- Lock-free ring buffers
- HTTP Keep-Alive optimization

Performance: 2,253 RPS sustained, 100% reliability, p50 = 0.25ms

**Future Crafts (Crafts #2-5):**
- Craft #2: Distributed message queue (Kafka-like)
- Craft #3: Time-series storage engine (InfluxDB-like)
- Craft #4: Query & aggregation engine (PromQL-like)
- Craft #5: Alerting & notification system (PagerDuty-like)

## Code Structure

```
phase0/                      # Phase 0: Complete PoC
├── phase0_poc.cpp          # All 5 components in one file
├── simple_benchmark.cpp    # C++ load testing tool
├── build.sh, demo.sh       # Build and demo scripts
├── QUICKSTART.md           # 5-minute quick start
├── README.md               # Comprehensive guide
├── TUTORIAL.md             # 2-3 hour tutorial
└── EXERCISES.md            # 10 hands-on exercises

src/                         # Craft #1: Optimized Ingestion
├── main.cpp                # Entry point
├── http_server.cpp         # Socket handling
├── ingestion_service.cpp   # Core logic, rate limiting, JSON parsing
└── metric.h                # Data structures

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

## Skills Checklist

When working on this project with superpowers/skills active, **ALWAYS** use these skills:

### Mandatory Skills (Must Use)

| Task | Skill Path | When to Use |
|------|-----------|-------------|
| **Creating worktrees** | `/skills/collaboration/using-git-worktrees/SKILL.md` | Before creating ANY git worktree - ensures .gitignore setup, proper directory structure, and baseline verification |
| **Starting new features** | `/skills/collaboration/brainstorming/SKILL.md` | Before writing ANY code for new features - refines requirements and prevents scope creep |
| **Debugging issues** | `/skills/debugging/systematic-debugging/SKILL.md` | When investigating bugs - ensures root cause analysis before fixes |
| **Writing code with tests** | `/skills/testing/test-driven-development/SKILL.md` | When implementing new functionality - write test first, watch it fail, then implement |

### Workflow Discipline

**Before ANY task:**
1. Run `find-skills [keyword]` to check for relevant skills
2. If relevant skill exists → Read it with Read tool
3. Announce: "I've read [Skill Name] skill and I'm using it to [purpose]"
4. Follow the skill exactly (especially checklists - use TodoWrite for each item)

**Skills exist and you didn't use them = failed task.**

### Worktree Configuration

- **Preferred directory**: `.worktrees/` (hidden, project-local)
- **Must be in .gitignore**: YES - the using-git-worktrees skill will verify and add if missing
- **Never create worktrees manually** - always use the skill or `/worktree` slash command

## Current Focus

Phase 4: Hash-based per-client mutex optimization to eliminate double mutex bottleneck in rate limiting. See TODO(human) in `flush_metrics()` for contribution opportunity.
- Our goal is to build a  realtime montoting system as well a learning system for engineers to learn how we build the system phase by phase. They should be able to start from the begining and move from phase 0 to final system. They would get opportunity to run labs for each phase/chapters, test it, performance test it.