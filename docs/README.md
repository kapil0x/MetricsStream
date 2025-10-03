# MetricStream Decision Tree

Interactive visualization of technical decisions made during MetricStream's optimization journey from 200 RPS to a high-performance metrics platform.

## View Live

Visit: [MetricStream Decision Tree](https://kapil0x.github.io/MetricsStream/)

## What's Inside

This visualization shows the complete decision history across 5 optimization phases:

- **Phase 1**: Threading per request
- **Phase 2**: Async I/O with producer-consumer pattern
- **Phase 3**: Custom JSON parser optimization
- **Phase 4**: Hash-based mutex pool
- **Phase 5**: Lock-free ring buffer + profiling analysis

Each node shows:
- Problem encountered
- Solution chosen
- Alternatives considered
- Rationale for decision
- Performance impact
- Files affected
- Session date and reference

## Features

- **Interactive Tree**: Click nodes to see detailed information
- **Multiple Layouts**: Tree, Cluster, and Radial views
- **Drill-Down**: Click any decision for comprehensive details
- **Responsive**: Works on desktop and mobile
- **Exportable**: Download as SVG

---

*Built with D3.js | Data generated from conversation database*
