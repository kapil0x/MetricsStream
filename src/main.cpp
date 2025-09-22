#include "ingestion_service.h"
#include <iostream>
#include <signal.h>
#include <thread>
#include <chrono>

std::unique_ptr<metricstream::IngestionService> service;

void signal_handler(int signal) {
    if (service) {
        std::cout << "\nShutting down gracefully..." << std::endl;
        service->stop();
    }
    exit(0);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    
    std::cout << "Starting MetricStream server on port " << port << std::endl;
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    service = std::make_unique<metricstream::IngestionService>(port);
    service->start();
    
    // Keep running until signal with periodic stats
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        // TODO(human): Add performance monitoring here
        // Consider tracking and logging:
        // - Active connection count
        // - Request processing time distribution
        // - Queue depths and memory usage
        // - Failed vs successful request ratios
        // This will help identify bottlenecks at high load
        
        // Phase 3 Analysis: JSON parsing optimization needed
        // Current bottleneck: Multiple string::find() calls and substr() allocations
        // Target: Single-pass parser with string views and pre-allocated containers
    }
    
    return 0;
}