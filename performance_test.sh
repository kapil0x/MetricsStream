#!/bin/bash

# Performance Testing Script for MetricStream
# Maps performance curve from 100 to 2500 RPS

echo "=== MetricStream Performance Analysis ==="
echo "Phase 1: Performance Profiling & Bottleneck Analysis"
echo "Target: Reproduce 99.25% → 91.4% success rate degradation"
echo

# Check if server is running
if ! pgrep -x "metricstream_server" > /dev/null; then
    echo "❌ MetricStream server not running. Start it first:"
    echo "   ./metricstream_server"
    exit 1
fi

echo "✅ MetricStream server detected"
echo

# Performance test configurations
# Format: "RPS_TARGET CLIENTS REQUESTS_PER_CLIENT INTERVAL_MS"
declare -a TEST_CONFIGS=(
    "100 10 100 100"    # 100 RPS baseline
    "500 25 100 20"     # 500 RPS moderate load  
    "1000 50 100 10"    # 1000 RPS high load
    "2000 100 100 5"    # 2000 RPS stress test
    "2500 125 100 4"    # 2500 RPS degradation point
)

RESULTS_FILE="performance_results.txt"
echo "Timestamp,RPS_Target,Total_Requests,Successful,Failed,Success_Rate,Avg_Latency" > $RESULTS_FILE

for config in "${TEST_CONFIGS[@]}"; do
    read -r rps clients requests_per_client interval <<< "$config"
    
    echo "🧪 Testing: Target ${rps} RPS (${clients} clients, ${interval}ms interval)"
    echo "   Expected: $((clients * requests_per_client)) total requests"
    
    # System monitoring before test
    echo "📊 System state before test:"
    echo "   CPU: $(top -l1 -n0 | grep "CPU usage" | awk '{print $3}' | sed 's/%//')"
    echo "   Memory: $(top -l1 -n0 | grep "PhysMem" | awk '{print $2}')"
    echo "   Open files: $(lsof -p $(pgrep metricstream_server) | wc -l)"
    
    # Run load test
    echo "⏱️  Starting load test..."
    ./build/load_test 8080 $clients $requests_per_client $interval > temp_results.txt
    
    # Parse results
    success_rate=$(grep "Success Rate:" temp_results.txt | awk '{print $3}' | sed 's/%//')
    avg_latency=$(grep "Avg Latency:" temp_results.txt | awk '{print $3}')
    total_requests=$(grep "Total Requests:" temp_results.txt | awk '{print $3}')
    successful=$(grep "Successful:" temp_results.txt | awk '{print $2}')
    failed=$(grep "Failed:" temp_results.txt | awk '{print $2}')
    
    # Record results
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    echo "$timestamp,$rps,$total_requests,$successful,$failed,$success_rate,$avg_latency" >> $RESULTS_FILE
    
    # Display immediate results
    echo "   ✅ Success Rate: ${success_rate}%"
    echo "   ⏱️  Avg Latency: ${avg_latency} ms"
    echo "   📈 Requests: ${successful}/${total_requests} successful"
    
    # Check for performance degradation
    if (( $(echo "$success_rate < 95.0" | bc -l) )); then
        echo "   ⚠️  PERFORMANCE DEGRADATION DETECTED!"
        echo "   🔍 Success rate below 95% - bottleneck identified"
    fi
    
    echo "   💤 Cooling down (5s)..."
    sleep 5
    echo
done

echo "=== Performance Analysis Complete ==="
echo "📄 Results saved to: $RESULTS_FILE"
echo
echo "📊 Performance Summary:"
cat $RESULTS_FILE | column -t -s ','

echo
echo "🔬 Next Steps for Principal Engineer Analysis:"
echo "1. Run: perf record -g ./metricstream_server"
echo "2. Run: valgrind --tool=callgrind ./metricstream_server" 
echo "3. Analyze: perf report to identify CPU hotspots"
echo "4. Check: strace -c -p \$(pgrep metricstream_server) for syscall analysis"
echo
echo "💡 Hypothesis: File I/O mutex contention becomes bottleneck above 1000 RPS"

# Cleanup
rm -f temp_results.txt