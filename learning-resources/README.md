# Learning Resources

Educational materials created during the MetricStream development journey.

## Directory Structure

```
learning-resources/
├── visualizations/     # Interactive HTML visualizations
├── concepts/          # Concept explanations (text files)
├── diagrams/          # Architecture diagrams
└── README.md          # This file
```

## Visualizations

### [request_flow_visual.html](visualizations/request_flow_visual.html)
Interactive visualization of Phase 6 & 7 optimizations:
- Thread pool architecture
- HTTP Keep-Alive persistent connections
- Before/After comparison with toggle button
- Animated flow showing request handling

**When to use:** Understanding how thread pools eliminate per-request thread creation overhead

### [event_driven_architecture.html](visualizations/event_driven_architecture.html)
Visualization of event-driven architecture patterns:
- Event loops and callbacks
- Non-blocking I/O
- Async processing models

**When to use:** Comparing event-driven vs thread-based models

## Concepts

### [connection_explained.txt](concepts/connection_explained.txt)
Deep dive into TCP connection mechanics:
- Listen backlog behavior
- Kernel queue (SYN queue vs Accept queue)
- Connection establishment flow
- Why listen backlog matters for performance

**Phase:** Phase 7 (HTTP Keep-Alive & Listen Backlog)

### [drain_visualization.txt](concepts/drain_visualization.txt)
Visualization of queue draining behavior:
- Producer-consumer pattern
- Queue fill/drain dynamics
- Backpressure mechanisms

**Phase:** Phase 2 (Async I/O)

### [queue_drain_explained.txt](concepts/queue_drain_explained.txt)
Detailed explanation of queue management:
- When queues fill up
- How to handle backpressure
- Thread coordination with condition variables

**Phase:** Phase 2 (Async I/O)

## Integration with Learning Hub

These resources are referenced in [learning-hub-v2.html](../learning-hub-v2.html):

- **Phase 0:** Architecture diagrams for system design
- **Phase 1:** Link to request_flow_visual.html for thread-per-request comparison
- **Phase 2:** Queue drain concepts for async I/O
- **Phase 6:** Thread pool visualizations
- **Phase 7:** Connection mechanics and keep-alive

## How to Use

1. **Browse by Phase:** Each concept is tagged with the relevant optimization phase
2. **Interactive Learning:** Open HTML files in browser for interactive exploration
3. **Reference Material:** Use text files as quick reference during implementation
4. **Teaching Aid:** Share visualizations when explaining concepts to others

## Contributing

When creating new learning materials:
1. Add to appropriate subdirectory (visualizations/, concepts/, diagrams/)
2. Update this README with description and phase mapping
3. Link from learning-hub-v2.html if relevant
4. Commit to git for version control
