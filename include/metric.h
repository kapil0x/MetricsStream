#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <vector>

namespace metricstream {

using Timestamp = std::chrono::time_point<std::chrono::system_clock>;
using Tags = std::unordered_map<std::string, std::string>;

enum class MetricType {
    COUNTER,
    GAUGE,
    HISTOGRAM,
    SUMMARY
};

struct Metric {
    std::string name;
    double value;
    MetricType type;
    Tags tags;
    Timestamp timestamp;
    
    // Constructor
    Metric(const std::string& name, double value, MetricType type, 
           const Tags& tags = {}, Timestamp ts = std::chrono::system_clock::now())
        : name(name), value(value), type(type), tags(tags), timestamp(ts) {}
};

struct MetricBatch {
    std::vector<Metric> metrics;
    std::string source_id;
    Timestamp received_at;
    
    MetricBatch() : received_at(std::chrono::system_clock::now()) {}
    
    void add_metric(Metric&& metric) {
        metrics.emplace_back(std::move(metric));
    }
    
    size_t size() const { return metrics.size(); }
    bool empty() const { return metrics.empty(); }
};

} // namespace metricstream