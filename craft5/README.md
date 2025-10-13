# Craft #5: Alerting & Notification System

**Status:** 🔜 Coming Soon

**Goal:** Build a real-time alerting platform (like Prometheus Alertmanager or PagerDuty) for monitoring and notifications.

## What You'll Build

A production-grade alerting system with:
- **Rule evaluation engine** for real-time checks
- **Alert state machine** (pending → firing → resolved)
- **Multi-channel notifications** (email, Slack, PagerDuty, webhooks)
- **Escalation policies** for unacknowledged alerts
- **Alert grouping and deduplication**

## Systems Design Focus

- Event-driven architecture vs polling
- State management for alerts at scale
- Reliable delivery patterns for notifications
- Notification channel abstractions
- Scalability of rule evaluation

## Target Performance

- **Evaluation throughput:** 10K+ rules/sec
- **Detection latency:** <1 min from threshold breach
- **Delivery reliability:** 99.99% (at-least-once)
- **Deduplication:** Group similar alerts effectively

## Service API Contract

See [API.md](API.md) for the complete API specification.

**Rule Definition:**
```
POST /rules
Body: {
  "name": "high_cpu_alert",
  "query": "avg(cpu_usage) > 80",
  "duration": "5m",
  "severity": "warning",
  "annotations": {
    "summary": "High CPU usage detected",
    "description": "CPU usage is {{ $value }}% on {{ $labels.host }}"
  }
}
```

**Alert API:**
```
GET /alerts
Response: {
  "alerts": [{
    "rule": "high_cpu_alert",
    "state": "firing",
    "value": 85.5,
    "labels": {"host": "server01"},
    "started_at": 1696789234567,
    "annotations": {...}
  }]
}
```

**Notification Channels:**
```
POST /channels
Body: {
  "type": "slack",
  "config": {
    "webhook_url": "https://hooks.slack.com/...",
    "channel": "#alerts"
  }
}
```

## Architecture Overview

```
Query Engine (Craft #4)
    ↓
┌──────────────────────────────────────────┐
│  Alerting System                         │
│                                          │
│  1. Rule Evaluator (every 15s)          │
│     ├─ Query execution                   │
│     ├─ Threshold comparison              │
│     └─ State transition logic            │
│      ↓                                   │
│  2. Alert Manager                        │
│     ├─ State: pending → firing → resolved│
│     ├─ Grouping & deduplication          │
│     └─ Inhibition rules                  │
│      ↓                                   │
│  3. Notification Dispatcher              │
│     ├─ Channel: Email                    │
│     ├─ Channel: Slack                    │
│     ├─ Channel: PagerDuty                │
│     └─ Channel: Webhook                  │
│      ↓                                   │
│  4. Escalation Manager                   │
│     ├─ Unacknowledged alerts             │
│     ├─ Retry logic with backoff          │
│     └─ Escalation policies               │
└──────────────────────────────────────────┘
    ↓
On-Call Engineers / Dashboards
```

## Implementation Plan

### Phase 1: Rule Evaluation
- Periodic query execution
- Threshold comparison
- Alert state machine (pending → firing → resolved)

### Phase 2: Notification Channels
- Email notifications (SMTP)
- Slack webhooks
- Generic webhook support

### Phase 3: Alert Management
- Grouping by labels
- Deduplication of similar alerts
- Inhibition rules (suppress related alerts)

### Phase 4: Escalation
- Acknowledgment tracking
- Retry with exponential backoff
- Multi-level escalation policies

### Phase 5: Optimization
- Event-driven evaluation (not just polling)
- Parallel rule evaluation
- Notification batching

## Key Learnings

- **State machines** are essential for alert lifecycle
- **Grouping** reduces notification fatigue
- **Reliable delivery** requires retries and idempotency
- **Event-driven** is more efficient than polling
- **Escalation** ensures critical alerts aren't missed

## Prerequisites

- Complete [Craft #4: Query Engine](../craft4/)
- Understand alerting from [Craft #0](../phase0/)
- Familiarity with notification systems helpful

## Next Steps

1. **[→ Read API Specification](API.md)**
2. **[→ Review Design Decisions](DESIGN.md)**
3. **[→ Start Implementation Guide](TUTORIAL.md)** (Coming soon)

## Service Decomposition for AI Agents

This craft can be built by 3 parallel agents:

**Agent 1: Rule Evaluator**
- Periodic query execution
- Threshold logic and state machine
- Rule storage and management

**Agent 2: Notification Dispatcher**
- Channel abstraction layer
- Email, Slack, webhook implementations
- Retry and delivery tracking

**Agent 3: Alert Manager**
- Grouping and deduplication logic
- Inhibition rules
- Escalation policy engine

**Integration point:** All agents implement interfaces defined in `API.md` and `DESIGN.md`.

---

**This is Craft #5 of Systems Craft.** Once complete, you'll understand how Prometheus Alertmanager, PagerDuty, and other alerting platforms work internally.

---

## 🎉 You Did It!

By completing all 5 crafts, you've built a **complete production-grade monitoring platform**:

```
Applications → Ingestion → Queue → Storage → Query → Alerting → On-Call
```

You now understand:
- ✅ High-throughput data ingestion
- ✅ Distributed message queues
- ✅ Time-series storage engines
- ✅ Query processing and optimization
- ✅ Real-time alerting systems

**More importantly:** You can design, decompose, and coordinate teams (or AI agents) to build complex distributed systems.

**Welcome to senior/staff engineering.**
