# Phase 0 Video Walkthrough Script

**Target Duration:** 15-20 minutes
**Format:** Screen recording with voiceover
**Audience:** Engineers learning distributed systems

---

## [00:00 - 00:30] Hook & Introduction

**VISUAL:** Title screen with "Phase 0: Building a Complete Monitoring Platform"

**SCRIPT:**
> "In the next 15 minutes, I'll show you how to build a complete monitoring platform from scratch—ingestion, storage, querying, and alerting—all in a single afternoon. This is Phase 0 of Systems Craft, where we validate architecture before optimization. By the end, you'll have a working system that shows you exactly where the bottlenecks are, setting you up to optimize in Craft #1. Let's dive in."

---

## [00:30 - 02:00] Architecture Overview

**VISUAL:** Draw architecture diagram on screen (animated):
```
Clients → Ingestion API → Queue → Storage
                             ↓
                        Alert Engine ← Query API
```

**SCRIPT:**
> "Here's what we're building: a 5-component monitoring platform. First, an ingestion API accepts metrics via HTTP POST. These metrics flow into a thread-safe queue—classic producer-consumer pattern. A storage consumer drains the queue and writes to disk. The query API reads from storage and returns filtered results. Finally, an alerting engine periodically checks rules and triggers alerts. All of this in about 600 lines of C++, deliberately simple so you can see how the pieces fit together."

**CALL-OUT BOXES** (appear as mentioned):
- Component 1: Ingestion API (single-threaded)
- Component 2: In-Memory Queue (std::queue + mutex)
- Component 3: Storage Consumer (background thread)
- Component 4: Query API (full file scan)
- Component 5: Alerting Engine (polling every 10s)

**SCRIPT CONTINUES:**
> "Notice the simplicity: single-threaded ingestion, blocking I/O, no indexes. These are intentional bottlenecks. You'll feel the pain, then fix it in later crafts. That's the learning method—working first, optimized later."

---

## [02:00 - 04:00] Quick Build & Run

**VISUAL:** Terminal, split-screen (left: code editor, right: terminal)

**SCRIPT:**
> "Let's get this running. Clone the repo, navigate to phase0, and run the build script."

**TYPE:**
```bash
cd phase0
./build.sh
```

**VISUAL:** Build output scrolling

**SCRIPT:**
> "CMake generates the build files, make compiles our single source file—phase0_poc.cpp—and we're done. Less than 10 seconds."

**TYPE:**
```bash
cd build
./phase0_poc
```

**VISUAL:** Server startup logs

**SCRIPT:**
> "Look at the startup sequence: storage consumer thread starts, alert rules load, the alerting engine begins checking every 10 seconds, and finally the ingestion server binds to port 8080. All five components are now running in this single process. Different threads, coordinated by the queue."

**HIGHLIGHT:** Each component's startup log as it's mentioned

---

## [04:00 - 06:30] Test: Ingestion → Storage Flow

**VISUAL:** Split terminal (top: server logs, bottom: curl commands)

**SCRIPT:**
> "Let's send our first metric. I'm using curl to POST a JSON payload—CPU usage at 85%."

**TYPE:**
```bash
curl -X POST http://localhost:8080/metrics \
     -H "Content-Type: application/json" \
     -d '{"name":"cpu_usage","value":85}'
```

**VISUAL:** Server responds `{"status":"accepted"}`

**SCRIPT:**
> "202 Accepted. The server immediately returned, but watch what happens in the background..."

**VISUAL:** Highlight server log showing storage write

**SCRIPT:**
> "The metric was pushed to the queue, the storage consumer popped it, and wrote it to phase0_metrics.jsonl. Let's verify."

**TYPE:**
```bash
cat phase0_metrics.jsonl
```

**VISUAL:** File contents showing JSON metric with timestamp

**SCRIPT:**
> "There it is. Name, value, and a Unix timestamp in milliseconds. This is JSON Lines format—one metric per line, human-readable, and easy to parse. Simple but effective for our proof of concept."

**SCRIPT (WHILE TYPING MORE METRICS):**
> "Let me send a few more—memory usage, disk I/O, network latency."

**TYPE (RAPID FIRE):**
```bash
curl -X POST http://localhost:8080/metrics -d '{"name":"memory_usage","value":75}'
curl -X POST http://localhost:8080/metrics -d '{"name":"disk_io","value":120}'
curl -X POST http://localhost:8080/metrics -d '{"name":"network_latency","value":45}'
```

**VISUAL:** File grows with each metric

**SCRIPT:**
> "Each metric flows through the same pipeline: ingestion, queue, storage. Asynchronous, decoupled, and simple."

---

## [06:30 - 09:00] Test: Query API

**VISUAL:** Browser window showing query URL

**SCRIPT:**
> "Now let's query. The API takes a metric name and time range. I'm querying CPU usage from the last 5 minutes."

**TYPE:**
```bash
NOW=$(date +%s)000
START=$((NOW - 300000))
curl "http://localhost:8080/query?name=cpu_usage&start=$START&end=$NOW"
```

**VISUAL:** JSON array response with metrics

**SCRIPT:**
> "And here's our data back. A JSON array of metrics with timestamps. Under the hood, the query engine is reading the entire file—no indexes, just a linear scan—filtering by name and time range, then returning matches. At small scale, this works fine. At 100,000 metrics? You'll feel the slowdown. That's the bottleneck we'll fix in Craft #3 with indexed storage."

**VISUAL:** Open browser DevTools, show Network timing

**SCRIPT:**
> "See the timing? About 15 milliseconds for this query. Not bad for 5 metrics. But this is O(n) complexity—every metric in the file gets scanned. Imagine a million metrics. This is why systems like InfluxDB use LSM trees and inverted indexes."

**TYPE ANOTHER QUERY:**
```bash
curl "http://localhost:8080/query?name=memory_usage&start=$START&end=$NOW"
```

**SCRIPT:**
> "Same process, different metric. Query API is stateless, reads from file every time. No caching. Again, intentionally simple."

---

## [09:00 - 12:00] Test: Alerting Engine

**VISUAL:** Split screen—terminal with alert rules visible

**SCRIPT:**
> "The alerting engine is checking three default rules every 10 seconds. Let's trigger one. The CPU rule fires if the average over 60 seconds exceeds 80%. I'll send a burst of high CPU metrics."

**TYPE:**
```bash
for i in {1..10}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d '{"name":"cpu_usage","value":92}' > /dev/null
  sleep 1
done
```

**VISUAL:** Metrics being sent (show counter: 1/10, 2/10...)

**SCRIPT:**
> "Ten metrics at 92%, spaced one second apart. Now we wait for the alerting engine's next check..."

**VISUAL:** Countdown timer (10, 9, 8...)

**VISUAL:** Alert fires in server logs 🚨

**HIGHLIGHT:** Alert message

**SCRIPT:**
> "There it is! Alert triggered. The engine queried CPU metrics from the last 60 seconds, calculated the average—92%—compared it to the threshold of 80%, and fired the alert. In a production system, this would send emails, Slack messages, or PagerDuty incidents. Here, we just log to console for simplicity."

**VISUAL:** Show alert message details

**SCRIPT:**
> "Notice what the alert tells us: the metric name, the condition, the threshold, and the current value with sample count. This is enough information to investigate. In Craft #5, we'll add deduplication, escalation policies, and multi-channel notifications. But the core pattern—periodic evaluation of rules—is already here."

---

## [12:00 - 14:00] Expose the Bottleneck

**VISUAL:** Terminal showing load test

**SCRIPT:**
> "Now, let's stress test this system. I'm going to send 100 metrics concurrently—100 separate curl processes hitting the server at once. Watch what happens."

**TYPE:**
```bash
time for i in {1..100}; do
  curl -s -X POST http://localhost:8080/metrics \
       -d "{\"name\":\"load_test\",\"value\":$i}" > /dev/null &
done
wait
```

**VISUAL:** Time output showing ~10-15 seconds

**SCRIPT:**
> "10 seconds for 100 requests. That's about 10 requests per second. Why so slow? Because our ingestion server is single-threaded. It handles one request at a time—accept connection, parse HTTP, parse JSON, push to queue, send response, close socket. Repeat. Everything is blocking. The CPU isn't maxed out, but we're stuck waiting on I/O operations. This is the bottleneck."

**VISUAL:** Draw diagram showing single-threaded ingestion with request queue building up

**SCRIPT:**
> "This is exactly what Craft #1 fixes. We'll add a thread pool where each request gets its own thread. The result? 200 RPS becomes 2,253 RPS—over 10x faster. But first, we needed to see the problem."

---

## [14:00 - 15:30] Code Walkthrough

**VISUAL:** Open phase0_poc.cpp in editor, scroll to key sections

**SCRIPT:**
> "Let's look at the code structure. All 600 lines in one file for simplicity."

**SCROLL TO:** `struct Metric` (line 48)

**SCRIPT:**
> "Data structures at the top: Metric is just a name, value, and timestamp. Simple. AlertRule defines thresholds and time windows."

**SCROLL TO:** `class MetricQueue` (line 67)

**SCRIPT:**
> "The queue is a std::queue wrapped with a mutex. Thread-safe push and pop. This is the classic concurrency pattern. Producer threads push, consumer thread pops."

**SCROLL TO:** `class StorageConsumer` (line 99)

**SCRIPT:**
> "Storage consumer is a background thread that polls the queue and writes to file. Blocking I/O, no buffering. We'll optimize this with async I/O in Craft #1."

**SCROLL TO:** `class IngestionServer::handle_client` (line 420)

**SCRIPT:**
> "And here's the ingestion bottleneck: handle_client is called once per connection, and it's blocking. Parse request, push to queue, send response. Single-threaded. This is the smoking gun."

**VISUAL:** Highlight the blocking operations

**SCRIPT:**
> "Every network read, every string operation, every queue push—all blocking the next request. Understanding this sets you up perfectly for the threading optimizations in Craft #1."

---

## [15:30 - 17:00] Review & Next Steps

**VISUAL:** Return to architecture diagram

**SCRIPT:**
> "Let's recap. You've built a complete monitoring platform with five components. Ingestion API, queue, storage, query, and alerting. It works. You can send metrics, query them, and get alerts. But it's slow—about 10 RPS for ingestion, full file scans for queries, and 10-second alert delays."

**VISUAL:** Highlight each bottleneck on diagram

**SCRIPT:**
> "These bottlenecks are intentional. They teach you what to optimize and why. In Craft #1, you'll tackle ingestion—add threading, rate limiting, and async I/O—and hit 2,253 RPS. In Craft #2, you'll replace the in-memory queue with a distributed message queue like Kafka. Craft #3 brings in a time-series database with LSM trees and indexing. Craft #4 adds parallel query execution. And Craft #5 makes alerting real-time with notifications."

**VISUAL:** Show progression: Phase 0 → Craft #1 → ... → Craft #5

**SCRIPT:**
> "Each craft takes 8 to 12 hours and makes one component production-grade. You'll go from this 600-line proof of concept to a real distributed system. But it all starts here—with architecture validation, not premature optimization."

---

## [17:00 - 18:00] Call to Action

**VISUAL:** GitHub repo page, README visible

**SCRIPT:**
> "Ready to start? Clone the repo, run through Phase 0, and try the exercises in EXERCISES.md. They'll deepen your understanding—adding features, measuring performance, and seeing trade-offs. When you're ready, move to Craft #1. The repository has everything: build scripts, demo scripts, and a full tutorial."

**VISUAL:** Show file structure

**SCRIPT:**
> "Links are in the description. Questions? Open a GitHub issue. I'd love to hear what you're building. See you in Craft #1."

---

## Production Notes

### B-Roll Suggestions
- Code scrolling at 2x speed with syntax highlighting
- Terminal output with zoomed-in focus on key lines
- Animated diagrams transitioning between states
- Side-by-side comparison: Phase 0 vs Craft #1 performance

### Visual Style
- Dark theme (Monokai or Dracula)
- Large font sizes (18-20pt for code, 24pt for terminal)
- Highlight key lines with colored boxes (green for success, red for bottlenecks)
- Use terminal recording tools (asciinema) for smooth playback

### Audio
- Clear, steady pace (not rushed)
- Pause after key concepts (2-3 seconds)
- Emphasize bottlenecks and learning objectives
- Friendly, conversational tone

### Chapters (YouTube timestamps)
```
00:00 - Introduction
00:30 - Architecture Overview
02:00 - Build & Run
04:00 - Ingestion → Storage Flow
06:30 - Query API
09:00 - Alerting Engine
12:00 - Expose the Bottleneck
14:00 - Code Walkthrough
15:30 - Review & Next Steps
17:00 - Call to Action
```

### Key Takeaway (Pin Comment)
> "Phase 0 = validate architecture. Craft #1-5 = optimize components. This is how senior engineers build systems: working first, fast second. The bottlenecks you saw (single-threaded ingestion, full file scans) are intentional—they teach you what to fix and why. Ready to optimize? Start Craft #1: https://github.com/kapil0x/systems-craft"

---

**Total Runtime:** 17-18 minutes
**Difficulty:** Beginner-friendly with depth for intermediate engineers
**Goal:** Show complete system, expose bottlenecks, motivate optimization
