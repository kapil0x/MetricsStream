/*
 * Phase 0: Monitoring Platform Proof of Concept
 *
 * A complete end-to-end monitoring system in one file.
 * Build this in 2-3 hours to understand how all components connect.
 *
 * 5 Components:
 * 1. Ingestion API - Accept metrics via HTTP POST
 * 2. In-Memory Queue - Thread-safe buffer between ingestion and storage
 * 3. Storage Consumer - Write metrics to file
 * 4. Query API - Read and filter metrics via HTTP GET
 * 5. Alerting Engine - Evaluate rules and trigger alerts
 *
 * Design Philosophy:
 * - Simple over optimized (single-threaded ingestion, blocking I/O)
 * - Readable over performant (clear code, obvious flow)
 * - Working over scalable (validate architecture first)
 *
 * What's NOT here (but will be in Crafts #1-5):
 * - Threading per request (Craft #1)
 * - Distributed queue (Craft #2)
 * - Indexed storage engine (Craft #3)
 * - Parallel query execution (Craft #4)
 * - Multi-channel notifications (Craft #5)
 */

#include <iostream>
#include <string>
#include <queue>
#include <mutex>
#include <thread>
#include <chrono>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <ctime>
#include <iomanip>
#include <cmath>
#include <climits>

// ============================================================================
// DATA STRUCTURES
// ============================================================================

struct Metric {
    std::string name;
    double value;
    long long timestamp;  // Unix timestamp in milliseconds

    Metric() : value(0.0), timestamp(0) {}
    Metric(const std::string& n, double v, long long ts)
        : name(n), value(v), timestamp(ts) {}

    // Serialize to JSON string
    std::string to_json() const {
        std::ostringstream oss;
        oss << "{\"name\":\"" << name << "\",\"value\":" << value
            << ",\"timestamp\":" << timestamp << "}";
        return oss.str();
    }
};

struct AlertRule {
    std::string metric_name;
    double threshold;
    std::string condition;  // ">" or "<"
    int window_seconds;     // Time window for evaluation

    AlertRule(const std::string& name, double thresh, const std::string& cond, int window)
        : metric_name(name), threshold(thresh), condition(cond), window_seconds(window) {}
};

// ============================================================================
// COMPONENT 2: IN-MEMORY QUEUE
// ============================================================================

class MetricQueue {
private:
    std::queue<Metric> queue_;
    std::mutex mutex_;

public:
    void push(const Metric& metric) {
        std::lock_guard<std::mutex> lock(mutex_);
        queue_.push(metric);
    }

    bool try_pop(Metric& metric) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (queue_.empty()) {
            return false;
        }
        metric = queue_.front();
        queue_.pop();
        return true;
    }

    size_t size() {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }
};

// Global queue (shared between ingestion and storage consumer)
MetricQueue g_metric_queue;

// ============================================================================
// COMPONENT 3: STORAGE CONSUMER
// ============================================================================

class StorageConsumer {
private:
    std::string filename_;
    bool running_;
    std::thread worker_thread_;

    void worker() {
        std::cout << "[Storage] Consumer started, writing to " << filename_ << std::endl;

        while (running_) {
            Metric metric;
            if (g_metric_queue.try_pop(metric)) {
                // Write to file (append mode)
                std::ofstream file(filename_, std::ios::app);
                if (file.is_open()) {
                    file << metric.to_json() << "\n";
                    file.close();
                } else {
                    std::cerr << "[Storage] Error: Could not open file" << std::endl;
                }
            } else {
                // Queue empty, sleep a bit
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }

        std::cout << "[Storage] Consumer stopped" << std::endl;
    }

public:
    StorageConsumer(const std::string& filename)
        : filename_(filename), running_(false) {}

    ~StorageConsumer() {
        stop();
    }

    void start() {
        if (!running_) {
            running_ = true;
            worker_thread_ = std::thread(&StorageConsumer::worker, this);
        }
    }

    void stop() {
        if (running_) {
            running_ = false;
            if (worker_thread_.joinable()) {
                worker_thread_.join();
            }
        }
    }
};

// ============================================================================
// COMPONENT 4: QUERY API
// ============================================================================

class QueryEngine {
private:
    std::string filename_;

    // Parse JSON metric from string (simple parser)
    bool parse_metric(const std::string& line, Metric& metric) {
        // Find "name":"..."
        size_t name_pos = line.find("\"name\":\"");
        if (name_pos == std::string::npos) return false;
        name_pos += 8;  // Skip past "name":"
        size_t name_end = line.find("\"", name_pos);
        if (name_end == std::string::npos) return false;
        metric.name = line.substr(name_pos, name_end - name_pos);

        // Find "value":...
        size_t value_pos = line.find("\"value\":", name_end);
        if (value_pos == std::string::npos) return false;
        value_pos += 8;  // Skip past "value":
        size_t value_end = line.find_first_of(",}", value_pos);
        if (value_end == std::string::npos) return false;
        metric.value = std::stod(line.substr(value_pos, value_end - value_pos));

        // Find "timestamp":...
        size_t ts_pos = line.find("\"timestamp\":", value_end);
        if (ts_pos == std::string::npos) return false;
        ts_pos += 12;  // Skip past "timestamp":
        size_t ts_end = line.find_first_of(",}", ts_pos);
        if (ts_end == std::string::npos) return false;
        metric.timestamp = std::stoll(line.substr(ts_pos, ts_end - ts_pos));

        return true;
    }

public:
    QueryEngine(const std::string& filename) : filename_(filename) {}

    // Query metrics by name and time range
    std::vector<Metric> query(const std::string& name, long long start_ts, long long end_ts) {
        std::vector<Metric> results;

        std::ifstream file(filename_);
        if (!file.is_open()) {
            std::cerr << "[Query] Error: Could not open file " << filename_ << std::endl;
            return results;
        }

        std::string line;
        while (std::getline(file, line)) {
            Metric metric;
            if (parse_metric(line, metric)) {
                // Filter by name and time range
                if (metric.name == name &&
                    metric.timestamp >= start_ts &&
                    metric.timestamp <= end_ts) {
                    results.push_back(metric);
                }
            }
        }

        file.close();
        return results;
    }

    // Get all metrics in time range (no name filter)
    std::vector<Metric> query_all(long long start_ts, long long end_ts) {
        std::vector<Metric> results;

        std::ifstream file(filename_);
        if (!file.is_open()) {
            return results;
        }

        std::string line;
        while (std::getline(file, line)) {
            Metric metric;
            if (parse_metric(line, metric)) {
                if (metric.timestamp >= start_ts && metric.timestamp <= end_ts) {
                    results.push_back(metric);
                }
            }
        }

        file.close();
        return results;
    }
};

// ============================================================================
// COMPONENT 5: ALERTING ENGINE
// ============================================================================

class AlertingEngine {
private:
    std::vector<AlertRule> rules_;
    QueryEngine* query_engine_;
    bool running_;
    std::thread worker_thread_;
    int check_interval_seconds_;

    void evaluate_rule(const AlertRule& rule) {
        // Get current time and calculate window
        auto now = std::chrono::system_clock::now();
        long long now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
        long long window_ms = rule.window_seconds * 1000LL;
        long long start_ts = now_ms - window_ms;

        // Query metrics in the window
        std::vector<Metric> metrics = query_engine_->query(rule.metric_name, start_ts, now_ms);

        if (metrics.empty()) {
            return;  // No data to evaluate
        }

        // Calculate average
        double sum = 0.0;
        for (const auto& m : metrics) {
            sum += m.value;
        }
        double avg = sum / metrics.size();

        // Check condition
        bool triggered = false;
        if (rule.condition == ">" && avg > rule.threshold) {
            triggered = true;
        } else if (rule.condition == "<" && avg < rule.threshold) {
            triggered = true;
        }

        if (triggered) {
            // Format timestamp for human readability
            std::time_t now_time = std::chrono::system_clock::to_time_t(now);
            char time_buf[80];
            std::strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));

            std::cout << "\n🚨 [ALERT] " << time_buf << std::endl;
            std::cout << "   Metric: " << rule.metric_name << std::endl;
            std::cout << "   Condition: avg(" << rule.window_seconds << "s) "
                      << rule.condition << " " << rule.threshold << std::endl;
            std::cout << "   Current: " << avg << " (from " << metrics.size() << " samples)" << std::endl;
            std::cout << std::endl;
        }
    }

    void worker() {
        std::cout << "[Alerting] Engine started, checking every "
                  << check_interval_seconds_ << " seconds" << std::endl;

        while (running_) {
            // Evaluate all rules
            for (const auto& rule : rules_) {
                evaluate_rule(rule);
            }

            // Sleep until next check
            std::this_thread::sleep_for(std::chrono::seconds(check_interval_seconds_));
        }

        std::cout << "[Alerting] Engine stopped" << std::endl;
    }

public:
    AlertingEngine(QueryEngine* qe, int check_interval = 10)
        : query_engine_(qe), running_(false), check_interval_seconds_(check_interval) {}

    ~AlertingEngine() {
        stop();
    }

    void add_rule(const AlertRule& rule) {
        rules_.push_back(rule);
        std::cout << "[Alerting] Added rule: " << rule.metric_name
                  << " " << rule.condition << " " << rule.threshold
                  << " (window: " << rule.window_seconds << "s)" << std::endl;
    }

    void start() {
        if (!running_) {
            running_ = true;
            worker_thread_ = std::thread(&AlertingEngine::worker, this);
        }
    }

    void stop() {
        if (running_) {
            running_ = false;
            if (worker_thread_.joinable()) {
                worker_thread_.join();
            }
        }
    }
};

// ============================================================================
// COMPONENT 1: INGESTION API (HTTP SERVER)
// ============================================================================

class IngestionServer {
private:
    int port_;
    int server_socket_;
    bool running_;

    // Get current timestamp in milliseconds
    long long get_current_timestamp() {
        auto now = std::chrono::system_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()).count();
    }

    // Simple JSON parser for ingestion (extracts name and value)
    bool parse_ingestion_json(const std::string& body, std::string& name, double& value) {
        // Find "name":"..."
        size_t name_pos = body.find("\"name\":\"");
        if (name_pos == std::string::npos) return false;
        name_pos += 8;
        size_t name_end = body.find("\"", name_pos);
        if (name_end == std::string::npos) return false;
        name = body.substr(name_pos, name_end - name_pos);

        // Find "value":...
        size_t value_pos = body.find("\"value\":");
        if (value_pos == std::string::npos) return false;
        value_pos += 8;
        size_t value_end = body.find_first_of(",}", value_pos);
        if (value_end == std::string::npos) return false;
        value = std::stod(body.substr(value_pos, value_end - value_pos));

        return true;
    }

    // Parse query parameters from URL
    std::map<std::string, std::string> parse_query_params(const std::string& url) {
        std::map<std::string, std::string> params;
        size_t query_start = url.find('?');
        if (query_start == std::string::npos) {
            return params;
        }

        std::string query = url.substr(query_start + 1);
        size_t pos = 0;

        while (pos < query.length()) {
            size_t eq_pos = query.find('=', pos);
            if (eq_pos == std::string::npos) break;

            size_t amp_pos = query.find('&', eq_pos);
            if (amp_pos == std::string::npos) {
                amp_pos = query.length();
            }

            std::string key = query.substr(pos, eq_pos - pos);
            std::string value = query.substr(eq_pos + 1, amp_pos - eq_pos - 1);
            params[key] = value;

            pos = amp_pos + 1;
        }

        return params;
    }

    void handle_client(int client_socket, QueryEngine* query_engine) {
        char buffer[4096];
        ssize_t bytes_read = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes_read <= 0) {
            close(client_socket);
            return;
        }

        buffer[bytes_read] = '\0';
        std::string request(buffer);

        // Parse HTTP request line
        std::istringstream request_stream(request);
        std::string method, url, version;
        request_stream >> method >> url >> version;

        std::string response;

        // Route: POST /metrics - Ingest metric
        if (method == "POST" && url == "/metrics") {
            // Find body (after \r\n\r\n)
            size_t body_pos = request.find("\r\n\r\n");
            if (body_pos != std::string::npos) {
                std::string body = request.substr(body_pos + 4);

                std::string name;
                double value;
                if (parse_ingestion_json(body, name, value)) {
                    // Create metric and push to queue
                    Metric metric(name, value, get_current_timestamp());
                    g_metric_queue.push(metric);

                    response = "HTTP/1.1 202 Accepted\r\n";
                    response += "Content-Type: application/json\r\n";
                    response += "Content-Length: 21\r\n";
                    response += "\r\n";
                    response += "{\"status\":\"accepted\"}";
                } else {
                    response = "HTTP/1.1 400 Bad Request\r\n";
                    response += "Content-Type: application/json\r\n";
                    response += "Content-Length: 30\r\n";
                    response += "\r\n";
                    response += "{\"error\":\"invalid JSON format\"}";
                }
            } else {
                response = "HTTP/1.1 400 Bad Request\r\n\r\n";
            }
        }
        // Route: GET /query - Query metrics
        else if (method == "GET" && url.find("/query") == 0) {
            auto params = parse_query_params(url);

            // Get parameters
            std::string name = params["name"];
            long long start_ts = params.count("start") ? std::stoll(params["start"]) : 0;
            long long end_ts = params.count("end") ? std::stoll(params["end"]) : LLONG_MAX;

            // Query metrics
            std::vector<Metric> results = query_engine->query(name, start_ts, end_ts);

            // Build JSON response
            std::ostringstream json;
            json << "[";
            for (size_t i = 0; i < results.size(); i++) {
                json << results[i].to_json();
                if (i < results.size() - 1) {
                    json << ",";
                }
            }
            json << "]";

            std::string body = json.str();
            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: application/json\r\n";
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "\r\n";
            response += body;
        }
        // Route: GET /health - Health check
        else if (method == "GET" && url == "/health") {
            std::string body = "{\"status\":\"healthy\",\"queue_size\":"
                             + std::to_string(g_metric_queue.size()) + "}";
            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: application/json\r\n";
            response += "Content-Length: " + std::to_string(body.length()) + "\r\n";
            response += "\r\n";
            response += body;
        }
        // Unknown route
        else {
            response = "HTTP/1.1 404 Not Found\r\n";
            response += "Content-Type: application/json\r\n";
            response += "Content-Length: 28\r\n";
            response += "\r\n";
            response += "{\"error\":\"route not found\"}";
        }

        send(client_socket, response.c_str(), response.length(), 0);
        close(client_socket);
    }

public:
    IngestionServer(int port) : port_(port), server_socket_(-1), running_(false) {}

    bool start(QueryEngine* query_engine) {
        // Create socket
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            std::cerr << "[Ingestion] Error: Could not create socket" << std::endl;
            return false;
        }

        // Set socket options (reuse address)
        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        // Bind socket
        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port_);

        if (bind(server_socket_, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "[Ingestion] Error: Could not bind to port " << port_ << std::endl;
            return false;
        }

        // Listen
        if (listen(server_socket_, 10) < 0) {
            std::cerr << "[Ingestion] Error: Could not listen" << std::endl;
            return false;
        }

        running_ = true;
        std::cout << "[Ingestion] Server started on port " << port_ << std::endl;
        std::cout << "[Ingestion] Endpoints:" << std::endl;
        std::cout << "  POST /metrics - Ingest metric" << std::endl;
        std::cout << "  GET  /query?name=<name>&start=<ts>&end=<ts> - Query metrics" << std::endl;
        std::cout << "  GET  /health - Health check" << std::endl;
        std::cout << std::endl;

        // Accept connections (blocking, single-threaded)
        while (running_) {
            sockaddr_in client_address;
            socklen_t client_len = sizeof(client_address);

            int client_socket = accept(server_socket_, (struct sockaddr*)&client_address, &client_len);
            if (client_socket < 0) {
                if (running_) {
                    std::cerr << "[Ingestion] Error: Could not accept connection" << std::endl;
                }
                continue;
            }

            // Handle request (blocking, synchronous)
            handle_client(client_socket, query_engine);
        }

        return true;
    }

    void stop() {
        running_ = false;
        if (server_socket_ >= 0) {
            close(server_socket_);
        }
    }
};

// ============================================================================
// MAIN - Wire all components together
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "=======================================" << std::endl;
    std::cout << "Phase 0: Monitoring Platform PoC" << std::endl;
    std::cout << "=======================================" << std::endl;
    std::cout << std::endl;

    int port = 8080;
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    std::string storage_file = "phase0_metrics.jsonl";

    // Component 3: Start storage consumer
    StorageConsumer storage(storage_file);
    storage.start();

    // Component 4: Create query engine
    QueryEngine query_engine(storage_file);

    // Component 5: Start alerting engine with example rules
    AlertingEngine alerting(&query_engine, 10);  // Check every 10 seconds

    // Add example alert rules
    alerting.add_rule(AlertRule("cpu_usage", 80.0, ">", 60));      // CPU > 80% avg over 60s
    alerting.add_rule(AlertRule("memory_usage", 90.0, ">", 60));   // Memory > 90% avg over 60s
    alerting.add_rule(AlertRule("error_rate", 5.0, ">", 30));      // Errors > 5/s avg over 30s

    alerting.start();

    std::cout << std::endl;
    std::cout << "All components running! Try:" << std::endl;
    std::cout << "  curl -X POST http://localhost:" << port << "/metrics -d '{\"name\":\"cpu_usage\",\"value\":85}'" << std::endl;
    std::cout << "  curl http://localhost:" << port << "/health" << std::endl;
    std::cout << std::endl;

    // Component 1: Start ingestion server (blocks until Ctrl+C)
    IngestionServer server(port);
    server.start(&query_engine);

    // Cleanup
    alerting.stop();
    storage.stop();

    std::cout << "Shutdown complete." << std::endl;
    return 0;
}
