#!/bin/bash

# MetricStream Load Test Monitor
# This script monitors system and application metrics during load testing

SERVER_PID=""
LOG_FILE="load_test_$(date +%Y%m%d_%H%M%S).log"
MONITOR_INTERVAL=1  # seconds

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

find_server_pid() {
    SERVER_PID=$(ps aux | grep metricstream_server | grep -v grep | awk '{print $2}' | head -1)
    if [ -z "$SERVER_PID" ]; then
        echo -e "${RED}ERROR: MetricStream server not found. Make sure it's running.${NC}"
        echo "Start the server with: ./build/metricstream_server"
        exit 1
    fi
    echo -e "${GREEN}Found MetricStream server with PID: $SERVER_PID${NC}"
}

monitor_system() {
    local duration=$1
    local counter=0

    echo -e "${BLUE}Starting system monitoring for $duration seconds...${NC}"
    echo "Time,CPU%,Memory_MB,RSS_KB,VSZ_KB,Load_1m,Load_5m,Load_15m,TCP_Connections" | tee -a "$LOG_FILE"

    while [ $counter -lt $duration ]; do
        # Get current timestamp
        timestamp=$(date '+%H:%M:%S')

        # Get server process stats
        if ps -p $SERVER_PID > /dev/null; then
            server_stats=$(ps -p $SERVER_PID -o pid,%cpu,%mem,rss,vsz --no-headers)
            cpu_percent=$(echo $server_stats | awk '{print $2}')
            mem_percent=$(echo $server_stats | awk '{print $3}')
            rss_kb=$(echo $server_stats | awk '{print $4}')
            vsz_kb=$(echo $server_stats | awk '{print $5}')
            memory_mb=$(echo "scale=2; $rss_kb / 1024" | bc -l)
        else
            echo -e "${RED}WARNING: Server process $SERVER_PID not found!${NC}"
            cpu_percent="N/A"
            mem_percent="N/A"
            memory_mb="N/A"
            rss_kb="N/A"
            vsz_kb="N/A"
        fi

        # Get system load
        load_avg=$(uptime | awk -F'load average:' '{print $2}' | tr -d ' ')
        load_1m=$(echo $load_avg | cut -d',' -f1)
        load_5m=$(echo $load_avg | cut -d',' -f2)
        load_15m=$(echo $load_avg | cut -d',' -f3)

        # Get TCP connections on port 8080
        tcp_connections=$(netstat -an 2>/dev/null | grep ":8080" | wc -l | tr -d ' ')

        # Log the data
        echo "$timestamp,$cpu_percent,$memory_mb,$rss_kb,$vsz_kb,$load_1m,$load_5m,$load_15m,$tcp_connections" | tee -a "$LOG_FILE"

        # Display real-time stats
        printf "\r${YELLOW}[%s] CPU: %s%% | Memory: %s MB | Load: %s | TCP Conn: %s${NC}" \
            "$timestamp" "$cpu_percent" "$memory_mb" "$load_1m" "$tcp_connections"

        sleep $MONITOR_INTERVAL
        ((counter++))
    done
    echo ""
}

run_load_test() {
    local clients=${1:-10}
    local requests=${2:-100}
    local interval=${3:-100}
    local port=${4:-8080}

    echo -e "${BLUE}Starting load test with:${NC}"
    echo "  - Clients: $clients"
    echo "  - Requests per client: $requests"
    echo "  - Interval: ${interval}ms"
    echo "  - Port: $port"

    log_message "Starting load test: $clients clients, $requests requests/client, ${interval}ms interval"

    # Calculate expected duration
    expected_duration=$((requests * interval / 1000 + 10))

    # Start monitoring in background
    monitor_system $expected_duration &
    monitor_pid=$!

    # Run the load test
    echo -e "${GREEN}Executing load test...${NC}"
    ./build/load_test $port $clients $requests $interval | tee -a "$LOG_FILE"

    # Stop monitoring
    kill $monitor_pid 2>/dev/null
    wait $monitor_pid 2>/dev/null

    log_message "Load test completed"
}

analyze_results() {
    echo -e "${BLUE}Analyzing results...${NC}"

    # Check if metrics.jsonl was updated
    if [ -f "metrics.jsonl" ]; then
        lines_before=${1:-0}
        lines_after=$(wc -l < metrics.jsonl)
        new_metrics=$((lines_after - lines_before))
        echo -e "${GREEN}Metrics stored: $new_metrics new entries${NC}"

        # Show last few entries
        echo -e "${YELLOW}Last 3 metric entries:${NC}"
        tail -3 metrics.jsonl
    else
        echo -e "${RED}No metrics.jsonl file found${NC}"
    fi

    echo -e "${GREEN}Full monitoring log saved to: $LOG_FILE${NC}"
}

print_usage() {
    echo "Usage: $0 [OPTIONS]"
    echo "Options:"
    echo "  -c, --clients NUM       Number of concurrent clients (default: 10)"
    echo "  -r, --requests NUM      Requests per client (default: 100)"
    echo "  -i, --interval MS       Interval between requests in ms (default: 100)"
    echo "  -p, --port PORT         Server port (default: 8080)"
    echo "  -m, --monitor-only SEC  Just monitor for SEC seconds (no load test)"
    echo "  -h, --help             Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                                    # Default test: 10 clients, 100 req/client"
    echo "  $0 -c 50 -r 200 -i 50                # Heavy load: 50 clients, 200 req/client"
    echo "  $0 --monitor-only 30                 # Just monitor for 30 seconds"
}

# Parse command line arguments
clients=10
requests=100
interval=100
port=8080
monitor_only=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clients)
            clients="$2"
            shift 2
            ;;
        -r|--requests)
            requests="$2"
            shift 2
            ;;
        -i|--interval)
            interval="$2"
            shift 2
            ;;
        -p|--port)
            port="$2"
            shift 2
            ;;
        -m|--monitor-only)
            monitor_only="$2"
            shift 2
            ;;
        -h|--help)
            print_usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            print_usage
            exit 1
            ;;
    esac
done

# Main execution
echo -e "${GREEN}MetricStream Load Test Monitor${NC}"
echo "================================"

# Check if server is running
find_server_pid

# Count existing metrics
metrics_before=0
if [ -f "metrics.jsonl" ]; then
    metrics_before=$(wc -l < metrics.jsonl)
fi

if [ -n "$monitor_only" ]; then
    echo -e "${YELLOW}Monitor-only mode for $monitor_only seconds${NC}"
    monitor_system $monitor_only
else
    run_load_test $clients $requests $interval $port
    analyze_results $metrics_before
fi

echo -e "${GREEN}Monitoring complete!${NC}"