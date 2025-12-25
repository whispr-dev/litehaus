/**
 * 🏰 LITEHAUS HOME MASTER NODE - CORE SERVICE
 * ============================================
 * The authoritative physical beacon node service.
 * Collects telemetry from all subsystems and broadcasts
 * to the global Litehaus network.
 * 
 * Architecture inspired by:
 * - Existing litehaus.cpp (SimdJSON beacon)
 * - TorVPN status_server.rs (state management)
 * - Semtech LoRa packet forwarder (timer sync)
 * 
 * Copyright (c) 2025 Litehaus Project
 * License: MIT
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <memory>
#include <csignal>
#include <cstring>

// Networking
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <curl/curl.h>

// Local includes
#include "litehaus_types.hpp"
#include "litehaus_json.hpp"
#include "litehaus_collectors.hpp"

namespace litehaus {

// ============================================================================
// GLOBAL STATE
// ============================================================================

static std::atomic<bool> g_running{true};
static std::atomic<uint32_t> g_beacon_sequence{0};
static std::chrono::steady_clock::time_point g_start_time;

void signal_handler(int signum) {
    std::cout << "\n🛑 Signal " << signum << " received. Shutting down gracefully...\n";
    g_running.store(false);
}

// ============================================================================
// CONFIGURATION LOADER
// ============================================================================

class ConfigLoader {
public:
    static CoreConfig loadFromFile(const std::string& path) {
        CoreConfig cfg;
        std::ifstream file(path);
        if (!file.is_open()) {
            std::cerr << "⚠️  Config file not found, using defaults: " << path << "\n";
            return cfg;
        }
        
        std::string line;
        std::string section;
        
        while (std::getline(file, line)) {
            // Trim whitespace
            size_t start = line.find_first_not_of(" \t");
            if (start == std::string::npos) continue;
            line = line.substr(start);
            
            // Skip comments
            if (line[0] == '#' || line[0] == ';') continue;
            
            // Section header
            if (line[0] == '[') {
                size_t end = line.find(']');
                if (end != std::string::npos) {
                    section = line.substr(1, end - 1);
                }
                continue;
            }
            
            // Key=value
            size_t eq = line.find('=');
            if (eq == std::string::npos) continue;
            
            std::string key = line.substr(0, eq);
            std::string value = line.substr(eq + 1);
            
            // Trim
            key.erase(key.find_last_not_of(" \t\"") + 1);
            key.erase(0, key.find_first_not_of(" \t\""));
            value.erase(value.find_last_not_of(" \t\"") + 1);
            value.erase(0, value.find_first_not_of(" \t\""));
            
            // Apply to config
            if (section == "identity") {
                if (key == "node_id") cfg.node_id = value;
                else if (key == "node_domain") cfg.node_domain = value;
            }
            else if (section == "network") {
                if (key == "lan_interface") cfg.lan_interface = value;
                else if (key == "lan_subnet") cfg.lan_subnet = value;
                else if (key == "node_ip") cfg.node_ip = value;
            }
            else if (section == "time") {
                if (key == "gpsdo_device") cfg.gpsdo_device = value;
                else if (key == "pps_device") cfg.pps_device = value;
                else if (key == "enable_chrony") cfg.enable_chrony = (value == "true" || value == "1");
            }
            else if (section == "intervals") {
                if (key == "beacon_interval_ms") cfg.beacon_interval_ms = std::stoul(value);
                else if (key == "status_interval_s") cfg.status_interval_s = std::stoul(value);
                else if (key == "probe_interval_s") cfg.probe_interval_s = std::stoul(value);
            }
            else if (section == "upstream") {
                if (key == "fastping_url") cfg.fastping_url = value;
            }
        }
        
        return cfg;
    }
    
    static CoreConfig loadDefault() {
        return CoreConfig{};
    }
};

// ============================================================================
// HTTP CLIENT - For FastPing probes
// ============================================================================

class HttpClient {
public:
    HttpClient() {
        curl_ = curl_easy_init();
        if (curl_) {
            curl_easy_setopt(curl_, CURLOPT_USERAGENT, "Litehaus-Home-Master/1.0");
            curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 10L);
            curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT, 5L);
            curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeCallback);
        }
    }
    
    ~HttpClient() {
        if (curl_) curl_easy_cleanup(curl_);
    }
    
    struct Response {
        bool success{false};
        std::string body;
        long http_code{0};
        double total_time_ms{0.0};
    };
    
    Response get(const std::string& url) {
        Response resp;
        if (!curl_) return resp;
        
        std::string response_body;
        curl_easy_setopt(curl_, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_body);
        
        CURLcode res = curl_easy_perform(curl_);
        
        if (res == CURLE_OK) {
            resp.success = true;
            resp.body = response_body;
            curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &resp.http_code);
            curl_easy_getinfo(curl_, CURLINFO_TOTAL_TIME, &resp.total_time_ms);
            resp.total_time_ms *= 1000.0;  // Convert to ms
        }
        
        return resp;
    }

private:
    CURL* curl_{nullptr};
    
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, std::string* out) {
        size_t total = size * nmemb;
        out->append(static_cast<char*>(contents), total);
        return total;
    }
};

// ============================================================================
// UDP BROADCASTER - Sends beacons to remote nodes
// ============================================================================

class UdpBroadcaster {
public:
    UdpBroadcaster() {
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ < 0) {
            std::cerr << "🚨 Failed to create UDP socket\n";
        }
    }
    
    ~UdpBroadcaster() {
        if (sock_ >= 0) close(sock_);
    }
    
    bool send(const std::string& host, uint16_t port, const std::string& data) {
        if (sock_ < 0) return false;
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        
        // Resolve hostname
        struct hostent* he = gethostbyname(host.c_str());
        if (he) {
            memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
        } else {
            inet_pton(AF_INET, host.c_str(), &addr.sin_addr);
        }
        
        ssize_t sent = sendto(sock_, data.c_str(), data.length(), 0,
                              reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        
        return sent > 0;
    }

private:
    int sock_{-1};
};

// ============================================================================
// STATE MANAGER - Persists state between restarts (like TorVPN hop_state.json)
// ============================================================================

class StateManager {
public:
    explicit StateManager(const std::string& state_dir) : state_dir_(state_dir) {
        // Create state directory if needed
        std::string cmd = "mkdir -p " + state_dir_;
        system(cmd.c_str());
    }
    
    void saveBeaconSequence(uint32_t seq) {
        std::ofstream file(state_dir_ + "/beacon_state.json");
        if (file.is_open()) {
            file << "{\"sequence\":" << seq << ",\"timestamp\":" 
                 << std::chrono::system_clock::now().time_since_epoch().count() << "}";
        }
    }
    
    uint32_t loadBeaconSequence() {
        std::ifstream file(state_dir_ + "/beacon_state.json");
        if (!file.is_open()) return 0;
        
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        
        // Quick parse for sequence
        size_t pos = content.find("\"sequence\":");
        if (pos != std::string::npos) {
            return std::stoul(content.substr(pos + 11));
        }
        return 0;
    }
    
    void saveLastPing(const std::string& status, double latency_ms, const std::string& ip) {
        std::ofstream file(state_dir_ + "/ping_state.json");
        if (file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto epoch = std::chrono::duration_cast<std::chrono::milliseconds>(
                now.time_since_epoch()).count();
            
            file << "{\"status\":\"" << status << "\","
                 << "\"latency_ms\":" << latency_ms << ","
                 << "\"ip\":\"" << ip << "\","
                 << "\"timestamp_ms\":" << epoch << "}";
        }
    }

private:
    std::string state_dir_;
};

// ============================================================================
// LITEHAUS CORE SERVICE
// ============================================================================

class LitehausCore {
public:
    explicit LitehausCore(const CoreConfig& cfg) 
        : config_(cfg)
        , state_mgr_("/var/lib/litehaus")
        , json_parser_()
    {
        g_start_time = std::chrono::steady_clock::now();
        g_beacon_sequence.store(state_mgr_.loadBeaconSequence());
        
        curl_global_init(CURL_GLOBAL_DEFAULT);
    }
    
    ~LitehausCore() {
        curl_global_cleanup();
    }
    
    void run() {
        displayBanner();
        
        // Start worker threads
        std::thread beacon_thread(&LitehausCore::beaconLoop, this);
        std::thread probe_thread(&LitehausCore::probeLoop, this);
        std::thread status_thread(&LitehausCore::statusLoop, this);
        
        std::cout << "🚀 Litehaus Core Service Started\n";
        std::cout << "   Node ID: " << config_.node_id << "\n";
        std::cout << "   Beacon interval: " << config_.beacon_interval_ms << "ms\n";
        std::cout << "   Probe interval: " << config_.probe_interval_s << "s\n\n";
        
        // Wait for shutdown signal
        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Graceful shutdown
        std::cout << "🛑 Stopping worker threads...\n";
        
        if (beacon_thread.joinable()) beacon_thread.join();
        if (probe_thread.joinable()) probe_thread.join();
        if (status_thread.joinable()) status_thread.join();
        
        // Save state
        state_mgr_.saveBeaconSequence(g_beacon_sequence.load());
        
        displayShutdownStats();
    }

private:
    CoreConfig config_;
    StateManager state_mgr_;
    json::BeaconParser json_parser_;
    collectors::MasterCollector collector_;
    
    // Shared state (protected by mutex)
    mutable std::mutex state_mutex_;
    std::string last_ping_status_;
    double last_ping_latency_ms_{0.0};
    std::string last_ping_ip_;
    std::chrono::steady_clock::time_point last_ping_time_;
    
    void displayBanner() {
        std::cout << R"(
🏰 ═══════════════════════════════════════════════════════════════════ 🏰
   LITEHAUS HOME MASTER NODE v1.0
   mars.litehaus.online
🔬 Physical Time Authority • LoRa Gateway • RF Bridge 🔬
⚡ GPS-Disciplined Rubidium Clock • Stratum 1 NTP ⚡
🏰 ═══════════════════════════════════════════════════════════════════ 🏰

)" << std::endl;
    }
    
    void displayShutdownStats() {
        auto uptime = std::chrono::steady_clock::now() - g_start_time;
        auto uptime_s = std::chrono::duration_cast<std::chrono::seconds>(uptime).count();
        
        auto stats = json_parser_.getStats();
        
        std::cout << "\n🏰 LITEHAUS CORE SHUTDOWN STATISTICS:\n";
        std::cout << "   Uptime: " << uptime_s << " seconds\n";
        std::cout << "   Beacons transmitted: " << g_beacon_sequence.load() << "\n";
        std::cout << "   JSON parses: " << stats.total_parses << "\n";
        std::cout << "   Parse success rate: " << stats.success_rate << "%\n";
        std::cout << "   Average parse time: " << stats.average_parse_time_us << "µs\n";
        std::cout << "🏰 LIGHTHOUSE SECURED 🏰\n\n";
    }
    
    /**
     * Beacon Loop - Collects telemetry and broadcasts at configured interval
     */
    void beaconLoop() {
        UdpBroadcaster broadcaster;
        
        while (g_running.load()) {
            auto cycle_start = std::chrono::steady_clock::now();
            
            // Collect all telemetry
            MasterBeaconPayload payload = collector_.collect();
            
            // Add identity
            payload.node_id = config_.node_id;
            payload.beacon_sequence = g_beacon_sequence.fetch_add(1);
            
            // Add probe data
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                payload.last_ping_status = last_ping_status_;
                payload.ping_latency_ms = last_ping_latency_ms_;
                payload.connecting_ip = last_ping_ip_;
                
                if (last_ping_time_.time_since_epoch().count() > 0) {
                    auto age = std::chrono::steady_clock::now() - last_ping_time_;
                    payload.signal_age_seconds = std::chrono::duration_cast<std::chrono::seconds>(age).count();
                }
            }
            
            // Add parser stats
            auto stats = json_parser_.getStats();
            payload.json_parse_time_us = stats.average_parse_time_us;
            payload.total_requests_processed = stats.total_parses;
            payload.average_throughput_mbps = stats.throughput_mbps;
            
            // Serialize
            std::string json_full = json::BeaconSerializer::serialize(payload);
            std::string json_compact = json::BeaconSerializer::serializeCompact(payload);
            
            // Broadcast to all targets
            for (const auto& target : config_.beacon_targets) {
                bool sent = broadcaster.send(target.first, target.second, json_full);
                if (!sent) {
                    // Log but don't spam
                    static int fail_count = 0;
                    if (++fail_count % 60 == 1) {
                        std::cerr << "⚠️  Failed to send beacon to " << target.first << ":" << target.second << "\n";
                    }
                }
            }
            
            // Also broadcast compact format on local subnet (for WiFi halo)
            broadcaster.send("255.255.255.255", 9877, json_compact);
            
            // Sleep for remainder of interval
            auto elapsed = std::chrono::steady_clock::now() - cycle_start;
            auto sleep_time = std::chrono::milliseconds(config_.beacon_interval_ms) - elapsed;
            if (sleep_time.count() > 0) {
                std::this_thread::sleep_for(sleep_time);
            }
        }
    }
    
    /**
     * Probe Loop - Periodically checks external connectivity (FastPing style)
     */
    void probeLoop() {
        HttpClient http;
        
        while (g_running.load()) {
            try {
                auto response = http.get(config_.fastping_url);
                
                if (response.success && response.http_code == 200) {
                    // Parse the FastPing response
                    double parse_time_us = 0;
                    MasterBeaconPayload temp;
                    
                    // Try to extract IP from response (FastPing returns JSON with connecting_ip)
                    std::string ip;
                    size_t ip_pos = response.body.find("\"connecting_ip\"");
                    if (ip_pos != std::string::npos) {
                        size_t start = response.body.find("\"", ip_pos + 15) + 1;
                        size_t end = response.body.find("\"", start);
                        if (end != std::string::npos) {
                            ip = response.body.substr(start, end - start);
                        }
                    }
                    
                    {
                        std::lock_guard<std::mutex> lock(state_mutex_);
                        last_ping_status_ = "ok";
                        last_ping_latency_ms_ = response.total_time_ms;
                        last_ping_ip_ = ip;
                        last_ping_time_ = std::chrono::steady_clock::now();
                    }
                    
                    state_mgr_.saveLastPing("ok", response.total_time_ms, ip);
                    
                } else {
                    std::lock_guard<std::mutex> lock(state_mutex_);
                    last_ping_status_ = "fail";
                    last_ping_time_ = std::chrono::steady_clock::now();
                }
                
            } catch (const std::exception& e) {
                std::lock_guard<std::mutex> lock(state_mutex_);
                last_ping_status_ = "error";
            }
            
            // Sleep for probe interval
            for (uint32_t i = 0; i < config_.probe_interval_s && g_running.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    
    /**
     * Status Loop - Periodic status display (like TorVPN daemon)
     */
    void statusLoop() {
        while (g_running.load()) {
            // Sleep first
            for (uint32_t i = 0; i < config_.status_interval_s && g_running.load(); ++i) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            
            if (!g_running.load()) break;
            
            // Collect and display status
            MasterBeaconPayload payload = collector_.collect();
            
            std::cout << "\n🏰 ═══════════════════════════════════════════════════════════════════ 🏰\n";
            std::cout << "   LITEHAUS HOME MASTER STATUS\n";
            std::cout << "🏰 ═══════════════════════════════════════════════════════════════════ 🏰\n";
            
            // Time status
            std::cout << "⏱️  TIME DISCIPLINE:\n";
            std::cout << "   Source: " << payload.time.source << "\n";
            std::cout << "   PPS Locked: " << (payload.time.pps_locked ? "✅ YES" : "❌ NO") << "\n";
            std::cout << "   Stratum: " << payload.time.stratum << "\n";
            std::cout << "   Error estimate: " << payload.time.estimated_error_ns << " ns\n";
            if (payload.time.gps_3d_fix) {
                std::cout << "   GPS: " << payload.time.latitude << ", " << payload.time.longitude << "\n";
                std::cout << "   Satellites: " << payload.time.satellites_visible << "\n";
            }
            
            // Power status
            std::cout << "\n⚡ POWER:\n";
            std::cout << "   Source: " << (payload.power.on_battery ? "🔋 BATTERY" : "🔌 GRID") << "\n";
            if (payload.power.ups_present) {
                std::cout << "   UPS: " << payload.power.battery_percent << "% (" 
                          << payload.power.estimated_runtime_minutes << " min)\n";
            }
            
            // System status
            std::cout << "\n💻 SYSTEM:\n";
            std::cout << "   Load: " << payload.system.cpu_load_1m << " / " 
                      << payload.system.cpu_load_5m << " / " << payload.system.cpu_load_15m << "\n";
            std::cout << "   Memory: " << payload.system.memory_used_mb << " / " 
                      << payload.system.memory_total_mb << " MB\n";
            std::cout << "   Disk: " << payload.system.disk_free_gb << " GB free\n";
            std::cout << "   WAN: " << (payload.system.wan_reachable ? "✅ REACHABLE" : "❌ UNREACHABLE") << "\n";
            
            // Probe status
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                std::cout << "\n📡 PROBE:\n";
                std::cout << "   Status: " << last_ping_status_ << "\n";
                std::cout << "   Latency: " << last_ping_latency_ms_ << " ms\n";
                std::cout << "   External IP: " << last_ping_ip_ << "\n";
            }
            
            // Beacon stats
            std::cout << "\n📻 BEACON:\n";
            std::cout << "   Sequence: " << g_beacon_sequence.load() << "\n";
            std::cout << "   Health: " << (payload.health == NodeHealth::HEALTHY ? "✅ HEALTHY" :
                                           payload.health == NodeHealth::WARNING ? "⚠️  WARNING" :
                                           payload.health == NodeHealth::CRITICAL ? "❌ CRITICAL" : "❓ UNKNOWN") << "\n";
            std::cout << "   Message: " << payload.status_message << "\n";
            
            std::cout << "🏰 ═══════════════════════════════════════════════════════════════════ 🏰\n";
        }
    }
};

} // namespace litehaus

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

int main(int argc, char* argv[]) {
    // Setup signal handlers
    std::signal(SIGINT, litehaus::signal_handler);
    std::signal(SIGTERM, litehaus::signal_handler);
    std::signal(SIGQUIT, litehaus::signal_handler);
    
    // Parse arguments
    std::string config_path = "/etc/litehaus/core.conf";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-c" || arg == "--config") && i + 1 < argc) {
            config_path = argv[++i];
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: litehaus-core [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -c, --config <path>  Config file (default: /etc/litehaus/core.conf)\n";
            std::cout << "  -h, --help           Show this help\n";
            return 0;
        }
    }
    
    // Load configuration
    litehaus::CoreConfig config = litehaus::ConfigLoader::loadFromFile(config_path);
    
    // Run the service
    try {
        litehaus::LitehausCore core(config);
        core.run();
    } catch (const std::exception& e) {
        std::cerr << "🚨 Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
