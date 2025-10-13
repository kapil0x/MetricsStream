#!/bin/bash
# Demo script for Phase 0 PoC
# Shows end-to-end flow: ingest → storage → query → alert

set -e

PORT=8080
BASE_URL="http://localhost:$PORT"

echo "======================================="
echo "Phase 0 PoC Demo"
echo "======================================="
echo ""

# Check if server is running
echo "1. Checking if server is running..."
if curl -s "$BASE_URL/health" > /dev/null 2>&1; then
    echo "   ✅ Server is running"
else
    echo "   ❌ Server is not running!"
    echo ""
    echo "Start the server first:"
    echo "  cd phase0/build && ./phase0_poc"
    echo ""
    echo "Then run this demo in another terminal:"
    echo "  cd phase0 && ./demo.sh"
    exit 1
fi

echo ""
echo "2. Ingesting sample metrics..."
echo ""

# Ingest CPU metrics (normal load)
echo "   📊 CPU usage (normal)..."
for i in {1..5}; do
    curl -s -X POST "$BASE_URL/metrics" \
         -H "Content-Type: application/json" \
         -d "{\"name\":\"cpu_usage\",\"value\":$((60 + RANDOM % 15))}" > /dev/null
    sleep 0.2
done

# Ingest CPU metrics (high load - will trigger alert)
echo "   🔥 CPU usage (high - will trigger alert)..."
for i in {1..5}; do
    curl -s -X POST "$BASE_URL/metrics" \
         -H "Content-Type: application/json" \
         -d "{\"name\":\"cpu_usage\",\"value\":$((85 + RANDOM % 10))}" > /dev/null
    sleep 0.2
done

# Ingest memory metrics
echo "   💾 Memory usage..."
for i in {1..5}; do
    curl -s -X POST "$BASE_URL/metrics" \
         -H "Content-Type: application/json" \
         -d "{\"name\":\"memory_usage\",\"value\":$((70 + RANDOM % 10))}" > /dev/null
    sleep 0.2
done

# Ingest error rate
echo "   ⚠️  Error rate..."
for i in {1..3}; do
    curl -s -X POST "$BASE_URL/metrics" \
         -H "Content-Type: application/json" \
         -d "{\"name\":\"error_rate\",\"value\":2.5}" > /dev/null
    sleep 0.2
done

echo ""
echo "3. Querying metrics..."
echo ""

# Get current timestamp
NOW=$(date +%s)000  # milliseconds
START=$((NOW - 60000))  # 60 seconds ago

# Query CPU usage
echo "   📈 Querying CPU usage (last 60 seconds):"
RESPONSE=$(curl -s "$BASE_URL/query?name=cpu_usage&start=$START&end=$NOW")
echo "   $RESPONSE" | python3 -m json.tool 2>/dev/null || echo "   $RESPONSE"

echo ""
echo "   📈 Querying memory usage (last 60 seconds):"
RESPONSE=$(curl -s "$BASE_URL/query?name=memory_usage&start=$START&end=$NOW")
echo "   $RESPONSE" | python3 -m json.tool 2>/dev/null || echo "   $RESPONSE"

echo ""
echo "4. Checking health..."
HEALTH=$(curl -s "$BASE_URL/health")
echo "   $HEALTH" | python3 -m json.tool 2>/dev/null || echo "   $HEALTH"

echo ""
echo "5. Waiting for alert evaluation..."
echo "   (Alert engine checks every 10 seconds)"
echo "   Watch the server logs for 🚨 ALERT messages"
echo ""
echo "======================================="
echo "Demo complete!"
echo "======================================="
echo ""
echo "What you just saw:"
echo "  ✅ Component 1 (Ingestion): POST /metrics accepted metrics"
echo "  ✅ Component 2 (Queue): Buffered metrics between ingestion and storage"
echo "  ✅ Component 3 (Storage): Wrote metrics to phase0_metrics.jsonl"
echo "  ✅ Component 4 (Query): GET /query retrieved filtered metrics"
echo "  ✅ Component 5 (Alerting): Evaluating rules (check server logs)"
echo ""
echo "Next steps:"
echo "  - Check phase0_metrics.jsonl to see stored data"
echo "  - Try more queries with different time ranges"
echo "  - Watch for alerts when metrics exceed thresholds"
echo "  - Move to Craft #1 to optimize ingestion (200 → 2,253 RPS)"
