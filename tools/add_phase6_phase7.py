#!/usr/bin/env python3
"""Add Phase 6 and Phase 7 technical decisions to the database."""

import sys
import os
sys.path.insert(0, os.path.dirname(__file__))

from conversation_db import ConversationDB

def main():
    db = ConversationDB("metricstream_conversations.db")

    # Phase 6: Thread Pool Architecture
    print("Adding Phase 6: Thread Pool Architecture...")
    conv_id_6 = db.add_conversation(
        session_date="2025-10-04",
        topic="Phase 6: Thread Pool Architecture",
        phase="Phase 6",
        summary="Implemented thread pool to eliminate per-request thread creation overhead (500μs → amortized)",
        raw_content="Thread pool implementation to replace thread-per-request model",
        duration_minutes=180,
        participants=["Human", "Claude"]
    )

    db.add_technical_decision(
        conversation_id=conv_id_6,
        decision_type="Concurrency Architecture",
        problem_description="Thread creation overhead (500μs) dwarfs actual request work (5μs). Thread-per-request doesn't scale beyond 2K RPS",
        solution_chosen="Thread pool with fixed worker threads: pre-allocate threads, queue incoming requests, workers process from queue",
        alternatives_considered="1) Keep thread-per-request - hits wall at 2K RPS; 2) Event loop (Node.js style) - requires complete rewrite; 3) Thread pool - optimal balance",
        rationale="Thread pool amortizes creation cost across thousands of requests. Fixed worker count prevents resource exhaustion. Lock-free queue enables non-blocking enqueue from accept thread",
        code_files_affected=["src/http_server.cpp", "include/thread_pool.h", "src/thread_pool.cpp"],
        performance_impact="Expected 10K+ RPS by eliminating 96% of thread creation overhead. Actual: revealed listen backlog bottleneck (Phase 7 discovery)"
    )

    # Phase 7: HTTP Keep-Alive & Listen Backlog
    print("Adding Phase 7: HTTP Keep-Alive & Listen Backlog...")
    conv_id_7 = db.add_conversation(
        session_date="2025-10-04",
        topic="Phase 7: HTTP Keep-Alive & Listen Backlog Discovery",
        phase="Phase 7",
        summary="Achieved 100% success rate (from 46-51%) by implementing persistent connections and increasing listen backlog from 10 to 1024",
        raw_content="Critical discovery: kernel accept queue size was the real bottleneck, not application code",
        duration_minutes=240,
        participants=["Human", "Claude"]
    )

    db.add_technical_decision(
        conversation_id=conv_id_7,
        decision_type="System Configuration & Protocol",
        problem_description="Success rate stuck at 46-51% despite thread pool optimization. 100 concurrent clients overwhelming kernel accept queue (backlog=10). No persistent connection support causing per-request TCP handshake overhead",
        solution_chosen="Two-part fix: 1) HTTP Keep-Alive loop to reuse connections, 2) Increase listen backlog from 10 to 1024",
        alternatives_considered="1) Keep small backlog - connection refusals continue; 2) Only add Keep-Alive - doesn't fix concurrent connection burst; 3) Both fixes - solved completely",
        rationale="Listen backlog controls kernel queue for pending connections. 100 simultaneous clients need 100 queue slots. HTTP Keep-Alive eliminates per-request handshake (1-2ms overhead). Both system-level and protocol-level optimizations required",
        code_files_affected=["src/http_server.cpp", "load_test_persistent.cpp", "CMakeLists.txt"],
        performance_impact="46-51% → 100% success rate (+49-54pp). Latency: 2.05ms → 0.65ms (new conn) / 0.25ms (persistent). Critical lesson: one parameter change (10→1024) fixed everything"
    )

    print(f"✅ Added Phase 6 (conv_id={conv_id_6}) and Phase 7 (conv_id={conv_id_7})")
    print("Now regenerating decision tree...")

if __name__ == "__main__":
    main()
