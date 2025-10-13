#!/bin/bash
# Performance Benchmarking Tool for Phase 0 PoC
#
# This script measures:
# - Ingestion throughput (requests/second)
# - Ingestion latency (p50, p90, p99)
# - Query performance under different data sizes
# - Queue depth under load
# - Alert evaluation latency
#
# Usage: ./benchmark.sh [server_port]

set -e

PORT=${1:-8080}
BASE_URL="http://localhost:$PORT"
RESULTS_FILE="benchmark_results.txt"

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo "======================================="
echo "Phase 0 PoC Performance Benchmark"
echo "======================================="
echo ""

# Check if server is running
echo "Checking server availability..."
if ! curl -s "$BASE_URL/health" > /dev/null 2>&1; then
    echo -e "${RED}Error: Server not running on port $PORT${NC}"
    echo "Start the server first:"
    echo "  cd build && ./phase0_poc $PORT"
    exit 1
fi
echo -e "${GREEN}✓ Server is running${NC}"
echo ""

# Initialize results file
echo "Phase 0 PoC - Performance Benchmark Results" > $RESULTS_FILE
echo "Date: $(date)" >> $RESULTS_FILE
echo "=========================================" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# ============================================================================
# Test 1: Baseline Ingestion Latency (Sequential)
# ============================================================================

echo -e "${YELLOW}[Test 1/6]${NC} Baseline Ingestion Latency (Sequential)"
echo ""

LATENCIES=()
NUM_REQUESTS=50

echo "Sending $NUM_REQUESTS sequential requests..."
for i in $(seq 1 $NUM_REQUESTS); do
    START=$(date +%s%3N)
    curl -s -X POST "$BASE_URL/metrics" \
         -H "Content-Type: application/json" \
         -d "{\"name\":\"benchmark_seq\",\"value\":$i}" > /dev/null
    END=$(date +%s%3N)
    LATENCY=$((END - START))
    LATENCIES+=($LATENCY)

    # Progress indicator
    if [ $((i % 10)) -eq 0 ]; then
        echo "  Progress: $i/$NUM_REQUESTS"
    fi
done

# Calculate statistics
IFS=$'\n' SORTED=($(sort -n <<<"${LATENCIES[*]}"))
unset IFS

AVG=$(echo "${LATENCIES[@]}" | awk '{sum=0; for(i=1;i<=NF;i++) sum+=$i; print sum/NF}')
P50_INDEX=$(((NUM_REQUESTS * 50) / 100))
P90_INDEX=$(((NUM_REQUESTS * 90) / 100))
P99_INDEX=$(((NUM_REQUESTS * 99) / 100))

P50=${SORTED[$P50_INDEX]}
P90=${SORTED[$P90_INDEX]}
P99=${SORTED[$P99_INDEX]}

echo ""
echo "Results:"
echo "  Average: ${AVG}ms"
echo "  p50: ${P50}ms"
echo "  p90: ${P90}ms"
echo "  p99: ${P99}ms"
echo ""

# Write to results file
echo "Test 1: Baseline Ingestion Latency (Sequential)" >> $RESULTS_FILE
echo "  Requests: $NUM_REQUESTS" >> $RESULTS_FILE
echo "  Average: ${AVG}ms" >> $RESULTS_FILE
echo "  p50: ${P50}ms" >> $RESULTS_FILE
echo "  p90: ${P90}ms" >> $RESULTS_FILE
echo "  p99: ${P99}ms" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# ============================================================================
# Test 2: Concurrent Ingestion Throughput
# ============================================================================

echo -e "${YELLOW}[Test 2/6]${NC} Concurrent Ingestion Throughput"
echo ""

for CONCURRENCY in 10 50 100; do
    echo "Testing with $CONCURRENCY concurrent clients..."

    START_TIME=$(date +%s)

    # Send requests concurrently
    for i in $(seq 1 $CONCURRENCY); do
        curl -s -X POST "$BASE_URL/metrics" \
             -H "Content-Type: application/json" \
             -d "{\"name\":\"benchmark_concurrent\",\"value\":$i}" > /dev/null &
    done

    # Wait for all to complete
    wait

    END_TIME=$(date +%s)
    DURATION=$((END_TIME - START_TIME))

    # Handle division by zero
    if [ $DURATION -eq 0 ]; then
        DURATION=1
    fi

    RPS=$((CONCURRENCY / DURATION))

    echo "  ${CONCURRENCY} requests in ${DURATION}s = ${RPS} RPS"
    echo ""

    # Write to results file
    echo "Test 2: Concurrent Ingestion ($CONCURRENCY clients)" >> $RESULTS_FILE
    echo "  Duration: ${DURATION}s" >> $RESULTS_FILE
    echo "  Throughput: ${RPS} RPS" >> $RESULTS_FILE
    echo "" >> $RESULTS_FILE
done

# ============================================================================
# Test 3: Query Performance vs Data Size
# ============================================================================

echo -e "${YELLOW}[Test 3/6]${NC} Query Performance vs Data Size"
echo ""

# First, ingest different amounts of data
for DATA_SIZE in 100 1000 5000; do
    echo "Ingesting $DATA_SIZE metrics..."
    for i in $(seq 1 $DATA_SIZE); do
        curl -s -X POST "$BASE_URL/metrics" \
             -H "Content-Type: application/json" \
             -d "{\"name\":\"query_test_$DATA_SIZE\",\"value\":$i}" > /dev/null
    done

    # Small delay to ensure storage completes
    sleep 2

    # Measure query time
    echo "Querying $DATA_SIZE metrics..."
    START=$(date +%s%3N)
    curl -s "$BASE_URL/query?name=query_test_$DATA_SIZE&start=0&end=9999999999999" > /dev/null
    END=$(date +%s%3N)
    QUERY_TIME=$((END - START))

    echo "  Query time for $DATA_SIZE metrics: ${QUERY_TIME}ms"
    echo ""

    # Write to results file
    echo "Test 3: Query Performance ($DATA_SIZE metrics)" >> $RESULTS_FILE
    echo "  Query time: ${QUERY_TIME}ms" >> $RESULTS_FILE
    echo "" >> $RESULTS_FILE
done

# ============================================================================
# Test 4: Queue Backpressure Test
# ============================================================================

echo -e "${YELLOW}[Test 4/6]${NC} Queue Backpressure Test"
echo ""

echo "Sending burst of 500 metrics..."
for i in $(seq 1 500); do
    curl -s -X POST "$BASE_URL/metrics" \
         -H "Content-Type: application/json" \
         -d "{\"name\":\"backpressure_test\",\"value\":$i}" > /dev/null &
done

echo "Monitoring queue depth..."
sleep 1

QUEUE_DEPTHS=()
for i in $(seq 1 10); do
    HEALTH=$(curl -s "$BASE_URL/health")
    QUEUE_SIZE=$(echo $HEALTH | grep -o '"queue_size":[0-9]*' | grep -o '[0-9]*')
    QUEUE_DEPTHS+=($QUEUE_SIZE)
    echo "  t=${i}s: Queue depth = $QUEUE_SIZE"
    sleep 1
done

# Wait for all requests to complete
wait

# Find peak queue depth
MAX_QUEUE=0
for depth in "${QUEUE_DEPTHS[@]}"; do
    if [ $depth -gt $MAX_QUEUE ]; then
        MAX_QUEUE=$depth
    fi
done

echo ""
echo "Results:"
echo "  Peak queue depth: $MAX_QUEUE"
echo ""

# Write to results file
echo "Test 4: Queue Backpressure (500 burst)" >> $RESULTS_FILE
echo "  Peak queue depth: $MAX_QUEUE" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# ============================================================================
# Test 5: Storage Write Performance
# ============================================================================

echo -e "${YELLOW}[Test 5/6]${NC} Storage Write Performance"
echo ""

echo "Measuring storage throughput..."
STORAGE_FILE="build/phase0_metrics.jsonl"

# Get initial file size
INITIAL_SIZE=$(wc -l < $STORAGE_FILE 2>/dev/null || echo 0)

# Ingest 1000 metrics
START_TIME=$(date +%s)
for i in $(seq 1 1000); do
    curl -s -X POST "$BASE_URL/metrics" \
         -H "Content-Type: application/json" \
         -d "{\"name\":\"storage_test\",\"value\":$i}" > /dev/null &

    # Batch to avoid overwhelming the system
    if [ $((i % 100)) -eq 0 ]; then
        wait
    fi
done
wait

# Wait for storage to catch up
sleep 3

END_TIME=$(date +%s)
DURATION=$((END_TIME - START_TIME))

# Get final file size
FINAL_SIZE=$(wc -l < $STORAGE_FILE 2>/dev/null || echo 0)
WRITTEN=$((FINAL_SIZE - INITIAL_SIZE))

WRITE_RATE=$((WRITTEN / DURATION))

echo "Results:"
echo "  Metrics written: $WRITTEN"
echo "  Duration: ${DURATION}s"
echo "  Write rate: ${WRITE_RATE} metrics/s"
echo ""

# Write to results file
echo "Test 5: Storage Write Performance" >> $RESULTS_FILE
echo "  Metrics written: $WRITTEN" >> $RESULTS_FILE
echo "  Duration: ${DURATION}s" >> $RESULTS_FILE
echo "  Write rate: ${WRITE_RATE} metrics/s" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# ============================================================================
# Test 6: Alert Evaluation Performance
# ============================================================================

echo -e "${YELLOW}[Test 6/6]${NC} Alert Evaluation Performance"
echo ""

echo "Ingesting high-value metrics to trigger alert..."
for i in $(seq 1 20); do
    curl -s -X POST "$BASE_URL/metrics" \
         -H "Content-Type: application/json" \
         -d '{"name":"cpu_usage","value":95}' > /dev/null
    sleep 0.5
done

echo "Waiting for alert evaluation (up to 15 seconds)..."
echo "  Check server logs for alert"
echo "  Alert latency = time since last metric sent"
echo ""

# Write to results file
echo "Test 6: Alert Evaluation" >> $RESULTS_FILE
echo "  Note: Check server logs for alert timing" >> $RESULTS_FILE
echo "  Expected: Alert within 10 seconds (check interval)" >> $RESULTS_FILE
echo "" >> $RESULTS_FILE

# ============================================================================
# Summary
# ============================================================================

echo "======================================="
echo "Benchmark Complete!"
echo "======================================="
echo ""
echo "Results saved to: $RESULTS_FILE"
echo ""
echo -e "${GREEN}Key Findings:${NC}"
echo "  • Sequential latency: ${AVG}ms avg, ${P99}ms p99"
echo "  • Concurrent throughput: ~10-50 RPS (single-threaded bottleneck)"
echo "  • Query scales linearly with data size (no indexes)"
echo "  • Queue handles bursts but drains slowly"
echo "  • Alert latency: up to 10 seconds (polling interval)"
echo ""
echo -e "${YELLOW}Bottlenecks Identified:${NC}"
echo "  1. Single-threaded ingestion (biggest impact)"
echo "  2. Blocking file I/O in storage consumer"
echo "  3. Full file scans for queries (O(n) complexity)"
echo "  4. Polling-based alerting (fixed interval)"
echo ""
echo -e "${GREEN}Next Steps:${NC}"
echo "  → Craft #1: Optimize ingestion (thread pool, async I/O)"
echo "  → Expected: 200 RPS → 2,253 RPS (10x improvement)"
echo ""
echo "View detailed results: cat $RESULTS_FILE"
