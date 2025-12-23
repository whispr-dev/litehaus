#include <simdjson.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <atomic>
#include <memory>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <random>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <wininet.h>
    #pragma comment(lib, "ws2_32.lib")
    #pragma comment(lib, "wininet.lib")
    #define close closesocket
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #include <curl/curl.h>
#endif

// 🏰 ULTIMATE LIGHTHOUSE BEACON SYSTEM - SIMDJSON POWERED 🏰
namespace UltimateLighthouse {

// FastPing Response Structure
struct FastPingResponse {
    std::string status{};
    std::string connecting_ip{};
    std::string anonymity_level{};
    std::string speed_hint{};
    double server_processing_latency_ms{ 0.0 };
    std::string client_ip_from_headers{};
    std::string message{};
    
    std::chrono::high_resolution_clock::time_point response_time{};
    std::chrono::microseconds parse_duration{ 0 };
    bool parse_success{ false };
};

// Beacon Payload Structure
struct UltimateBeaconPayload {
    std::string beacon_id{ "ultimate-lighthouse-002" };
    uint64_t timestamp{ 0 };
    std::string status{ "initializing" };
    std::string last_ping_status{};
    double ping_latency_ms{ 0.0 };
    uint32_t signal_age_seconds{ 0 };
    
    double json_parse_time_microseconds{ 0.0 };
    uint64_t total_requests_processed{ 0 };
    uint64_t successful_parses{ 0 };
    uint64_t failed_parses{ 0 };
    double average_throughput_mbps{ 0.0 };
    std::string cpu_optimization_level{};
    
    double system_uptime_hours{ 0.0 };
    uint32_t beacon_sequence_number{ 0 };
    std::string lighthouse_version{ "ULTIMATE-v3.0-SIMDJSON-POWERED" };
};

// JSON Processor
class UltimateJsonProcessor {
private:
    simdjson::dom::parser parser{};
    mutable std::mutex performance_mutex{};
    
    std::atomic<uint64_t> total_parses{ 0 };
    std::atomic<uint64_t> successful_parses{ 0 };
    std::atomic<uint64_t> total_bytes_processed{ 0 };
    std::atomic<double> total_parse_time_microseconds{ 0.0 };
    
public:
    UltimateJsonProcessor() {
        std::cout << "🚀 SimdJSON Ultimate Processor Initialized!\n";
        std::cout << "⚡ SIMD Optimization: SimdJSON Auto-Optimized\n";
        std::cout << "🔥 CPU Features: Auto-Detected\n\n";
    }
    
    bool parseFastPingResponse(FastPingResponse& response, const std::string& json_data) {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            simdjson::dom::element doc = parser.parse(json_data);
            
            if (auto status_result = doc["status"]; !status_result.error()) {
                response.status = std::string(status_result.get_string().value());
            }
            
            if (auto ip_result = doc["connecting_ip"]; !ip_result.error()) {
                response.connecting_ip = std::string(ip_result.get_string().value());
            }
            
            if (auto anon_result = doc["anonymity_level"]; !anon_result.error()) {
                response.anonymity_level = std::string(anon_result.get_string().value());
            }
            
            if (auto speed_result = doc["speed_hint"]; !speed_result.error()) {
                response.speed_hint = std::string(speed_result.get_string().value());
            }
            
            if (auto latency_result = doc["server_processing_latency_ms"]; !latency_result.error()) {
                response.server_processing_latency_ms = latency_result.get_double().value();
            }
            
            if (auto client_ip_result = doc["client_ip_from_headers"]; !client_ip_result.error()) {
                response.client_ip_from_headers = std::string(client_ip_result.get_string().value());
            }
            
            if (auto message_result = doc["message"]; !message_result.error()) {
                response.message = std::string(message_result.get_string().value());
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            double microseconds = duration.count() / 1000.0;
            
            total_parses.fetch_add(1);
            successful_parses.fetch_add(1);
            total_bytes_processed.fetch_add(json_data.size());
            
            double expected = total_parse_time_microseconds.load();
            while (!total_parse_time_microseconds.compare_exchange_weak(expected, expected + microseconds));
            
            response.parse_duration = std::chrono::microseconds(static_cast<uint64_t>(microseconds));
            response.parse_success = true;
            
            return true;
        } catch (const simdjson::simdjson_error& e) {
            total_parses.fetch_add(1);
            std::cerr << "🚨 SimdJSON Parse Error: " << e.what() << std::endl;
            response.parse_success = false;
            return false;
        }
    }
    
    std::string serializeBeaconPayload(const UltimateBeaconPayload& payload) {
        std::ostringstream json;
        json << "{"
             << "\"beacon_id\":\"" << payload.beacon_id << "\","
             << "\"timestamp\":" << payload.timestamp << ","
             << "\"status\":\"" << payload.status << "\","
             << "\"last_ping_status\":\"" << payload.last_ping_status << "\","
             << "\"ping_latency_ms\":" << std::fixed << std::setprecision(2) << payload.ping_latency_ms << ","
             << "\"signal_age_seconds\":" << payload.signal_age_seconds << ","
             << "\"json_parse_time_microseconds\":" << std::fixed << std::setprecision(2) << payload.json_parse_time_microseconds << ","
             << "\"total_requests_processed\":" << payload.total_requests_processed << ","
             << "\"successful_parses\":" << payload.successful_parses << ","
             << "\"failed_parses\":" << payload.failed_parses << ","
             << "\"average_throughput_mbps\":" << std::fixed << std::setprecision(1) << payload.average_throughput_mbps << ","
             << "\"cpu_optimization_level\":\"" << payload.cpu_optimization_level << "\","
             << "\"system_uptime_hours\":" << std::fixed << std::setprecision(1) << payload.system_uptime_hours << ","
             << "\"beacon_sequence_number\":" << payload.beacon_sequence_number << ","
             << "\"lighthouse_version\":\"" << payload.lighthouse_version << "\""
             << "}";
        
        return json.str();
    }
    
    bool parseBeaconPayload(UltimateBeaconPayload& payload, const std::string& json_data) {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            simdjson::dom::element doc = parser.parse(json_data);
            
            if (auto beacon_id_result = doc["beacon_id"]; !beacon_id_result.error()) {
                payload.beacon_id = std::string(beacon_id_result.get_string().value());
            }
            
            if (auto timestamp_result = doc["timestamp"]; !timestamp_result.error()) {
                payload.timestamp = timestamp_result.get_uint64().value();
            }
            
            if (auto status_result = doc["status"]; !status_result.error()) {
                payload.status = std::string(status_result.get_string().value());
            }
            
            if (auto ping_status_result = doc["last_ping_status"]; !ping_status_result.error()) {
                payload.last_ping_status = std::string(ping_status_result.get_string().value());
            }
            
            if (auto ping_latency_result = doc["ping_latency_ms"]; !ping_latency_result.error()) {
                payload.ping_latency_ms = ping_latency_result.get_double().value();
            }
            
            if (auto signal_age_result = doc["signal_age_seconds"]; !signal_age_result.error()) {
                payload.signal_age_seconds = static_cast<uint32_t>(signal_age_result.get_uint64().value());
            }
            
            if (auto parse_time_result = doc["json_parse_time_microseconds"]; !parse_time_result.error()) {
                payload.json_parse_time_microseconds = parse_time_result.get_double().value();
            }
            
            if (auto total_requests_result = doc["total_requests_processed"]; !total_requests_result.error()) {
                payload.total_requests_processed = total_requests_result.get_uint64().value();
            }
            
            if (auto successful_parses_result = doc["successful_parses"]; !successful_parses_result.error()) {
                payload.successful_parses = successful_parses_result.get_uint64().value();
            }
            
            if (auto failed_parses_result = doc["failed_parses"]; !failed_parses_result.error()) {
                payload.failed_parses = failed_parses_result.get_uint64().value();
            }
            
            if (auto throughput_result = doc["average_throughput_mbps"]; !throughput_result.error()) {
                payload.average_throughput_mbps = throughput_result.get_double().value();
            }
            
            if (auto cpu_opt_result = doc["cpu_optimization_level"]; !cpu_opt_result.error()) {
                payload.cpu_optimization_level = std::string(cpu_opt_result.get_string().value());
            }
            
            if (auto uptime_result = doc["system_uptime_hours"]; !uptime_result.error()) {
                payload.system_uptime_hours = uptime_result.get_double().value();
            }
            
            if (auto sequence_result = doc["beacon_sequence_number"]; !sequence_result.error()) {
                payload.beacon_sequence_number = static_cast<uint32_t>(sequence_result.get_uint64().value());
            }
            
            if (auto version_result = doc["lighthouse_version"]; !version_result.error()) {
                payload.lighthouse_version = std::string(version_result.get_string().value());
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            double microseconds = duration.count() / 1000.0;
            
            total_parses.fetch_add(1);
            successful_parses.fetch_add(1);
            total_bytes_processed.fetch_add(json_data.size());
            
            double expected = total_parse_time_microseconds.load();
            while (!total_parse_time_microseconds.compare_exchange_weak(expected, expected + microseconds));
            
            return true;
        } catch (const simdjson::simdjson_error& e) {
            total_parses.fetch_add(1);
            std::cerr << "🚨 SimdJSON Parse Error: " << e.what() << std::endl;
            return false;
        }
    }
    
    struct PerformanceMetrics {
        uint64_t total_parses;
        uint64_t successful_parses;
        uint64_t total_bytes;
        double average_parse_time_us;
        double throughput_mbps;
        double success_rate;
    };
    
    PerformanceMetrics getMetrics() const {
        std::lock_guard<std::mutex> lock(performance_mutex);
        
        uint64_t parses = total_parses.load();
        uint64_t successes = successful_parses.load();
        uint64_t bytes = total_bytes_processed.load();
        double total_time_us = total_parse_time_microseconds.load();
        
        PerformanceMetrics metrics{};
        metrics.total_parses = parses;
        metrics.successful_parses = successes;
        metrics.total_bytes = bytes;
        metrics.average_parse_time_us = parses > 0 ? total_time_us / parses : 0.0;
        metrics.success_rate = parses > 0 ? (double)successes / parses * 100.0 : 0.0;
        
        if (total_time_us > 0) {
            double seconds = total_time_us / 1000000.0;
            double mb = bytes / (1024.0 * 1024.0);
            metrics.throughput_mbps = mb / seconds;
        }
        
        return metrics;
    }
};

// HTTP Client
class UltimateHttpClient {
private:
    #ifdef _WIN32
        HINTERNET hInternet;
        HINTERNET hConnect;
    #else
        CURL* curl;
    #endif
    
public:
    UltimateHttpClient() {
        #ifdef _WIN32
            hInternet = InternetOpenA("Ultimate-Lighthouse-Agent/3.0", 
                                    INTERNET_OPEN_TYPE_DIRECT, nullptr, nullptr, 0);
            if (!hInternet) {
                throw std::runtime_error("Failed to initialize WinINet");
            }
        #else
            curl = curl_easy_init();
            if (!curl) {
                throw std::runtime_error("Failed to initialize libcurl");
            }
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Ultimate-Lighthouse-Agent/3.0");
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
        #endif
    }
    
    ~UltimateHttpClient() {
        #ifdef _WIN32
            if (hConnect) InternetCloseHandle(hConnect);
            if (hInternet) InternetCloseHandle(hInternet);
        #else
            if (curl) curl_easy_cleanup(curl);
        #endif
    }
    
    std::pair<bool, std::string> performRequest(const std::string& url) {
        #ifdef _WIN32
            return performWindowsRequest(url);
        #else
            return performLinuxRequest(url);
        #endif
    }
    
private:
    #ifdef _WIN32
    std::pair<bool, std::string> performWindowsRequest(const std::string& url) {
        HINTERNET hRequest = InternetOpenUrlA(hInternet, url.c_str(), 
                                            nullptr, 0, INTERNET_FLAG_RELOAD, 0);
        if (!hRequest) {
            return {false, ""};
        }
        
        std::string response;
        char buffer[8192];
        DWORD bytesRead;
        
        while (InternetReadFile(hRequest, buffer, sizeof(buffer) - 1, &bytesRead) && bytesRead > 0) {
            buffer[bytesRead] = '\0';
            response += buffer;
        }
        
        InternetCloseHandle(hRequest);
        return {!response.empty(), response};
    }
    #else
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* response) {
        size_t totalSize = size * nmemb;
        response->append(static_cast<char*>(contents), totalSize);
        return totalSize;
    }
    
    std::pair<bool, std::string> performLinuxRequest(const std::string& url) {
        std::string response;
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        
        CURLcode res = curl_easy_perform(curl);
        return {res == CURLE_OK && !response.empty(), response};
    }
    #endif
};

// The Ultimate Lighthouse Beacon System
class UltimateLighthouseBeacon {
private:
    std::unique_ptr<UltimateJsonProcessor> json_processor;
    std::unique_ptr<UltimateHttpClient> http_client;
    
    std::string fastping_url{ "http://fastping.it.com:9876/ping?format=json" };
    std::string beacon_ip{ "138.68.142.181" };
    int beacon_port{ 9876 };
    
    std::chrono::seconds ping_interval{ 10 };
    std::chrono::seconds beacon_interval{ 5 };
    std::chrono::seconds status_interval{ 30 };
    
    std::atomic<bool> running{ false };
    FastPingResponse last_response{};
    mutable std::mutex response_mutex{};
    
    std::chrono::high_resolution_clock::time_point start_time{};
    std::atomic<uint32_t> beacon_sequence{ 0 };
    std::atomic<uint64_t> total_requests{ 0 };
    
    std::vector<std::thread> worker_threads{};
    
public:
    UltimateLighthouseBeacon() {
        json_processor = std::make_unique<UltimateJsonProcessor>();
        http_client = std::make_unique<UltimateHttpClient>();
        start_time = std::chrono::high_resolution_clock::now();
        
        #ifdef _WIN32
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
        
        displayStartupBanner();
    }
    
    ~UltimateLighthouseBeacon() {
        stop();
        #ifdef _WIN32
            WSACleanup();
        #endif
    }
    
    void start() {
        if (running.exchange(true)) {
            std::cout << "⚠️  Lighthouse already running!\n";
            return;
        }
        
        std::cout << "🚀 Starting Ultimate Lighthouse Beacon System...\n\n";
        
        worker_threads.emplace_back(&UltimateLighthouseBeacon::listenerThread, this);
        worker_threads.emplace_back(&UltimateLighthouseBeacon::beaconThread, this);
        worker_threads.emplace_back(&UltimateLighthouseBeacon::statusThread, this);
        
        std::cout << "🔍 Ultra-Fast Listener Thread Started\n";
        std::cout << "📻 Ultra-Fast Beacon Thread Started\n";
        std::cout << "🕐 Enhanced Speaking Clock Started\n\n";
        
        std::cout << "🏰 ULTIMATE LIGHTHOUSE SYSTEM OPERATIONAL! 🏰\n";
        std::cout << "Press Enter to stop the lighthouse...\n";
    }
    
    void stop() {
        if (!running.exchange(false)) {
            return;
        }
        
        std::cout << "\n🛑 Stopping Ultimate Lighthouse System...\n";
        
        for (auto& thread : worker_threads) {
            if (thread.joinable()) {
                thread.join();
            }
        }
        worker_threads.clear();
        
        displayShutdownStats();
    }

private:
    void displayStartupBanner() {
        std::cout << R"(
🏰 ═══════════════════════════════════════════════════════════════════ 🏰
   ULTIMATE LIGHTHOUSE BEACON SYSTEM v3.0
🚀 Powered by SimdJSON - Parsing Gigabytes of JSON per Second 🚀
⚡ Ultra-High Performance JSON Processing ⚡
🔥 SIMD Optimized Parsing with Sub-Microsecond Performance 🔥
💎 Auto-detecting CPU Features for Maximum Speed 💎
🏰 ═══════════════════════════════════════════════════════════════════ 🏰

)" << std::endl;
        
        std::cout << "🎯 Configuration:\n";
        std::cout << "   FastPing URL: " << fastping_url << "\n";
        std::cout << "   Beacon Target: " << beacon_ip << ":" << beacon_port << "\n";
        std::cout << "   Ping Interval: " << ping_interval.count() << "s\n";
        std::cout << "   Beacon Interval: " << beacon_interval.count() << "s\n\n";
    }
    
    void listenerThread() {
        while (running.load()) {
            auto cycle_start = std::chrono::high_resolution_clock::now();
            
            try {
                auto [success, response_data] = http_client->performRequest(fastping_url);
                
                if (success && !response_data.empty()) {
                    FastPingResponse response;
                    response.response_time = std::chrono::high_resolution_clock::now();
                    
                    bool parse_success = json_processor->parseFastPingResponse(response, response_data);
                    
                    if (parse_success) {
                        std::lock_guard<std::mutex> lock(response_mutex);
                        last_response = std::move(response);
                        total_requests.fetch_add(1);
                        
                        auto cycle_end = std::chrono::high_resolution_clock::now();
                        auto cycle_time = std::chrono::duration_cast<std::chrono::milliseconds>(cycle_end - cycle_start);
                        
                        auto metrics = json_processor->getMetrics();
                        
                        std::cout << "🚀 FastPing Ultra-Fast Update:\n";
                        std::cout << "   Status: " << last_response.status << " | IP: " << last_response.connecting_ip << "\n";
                        std::cout << "   Parse: " << std::fixed << std::setprecision(2) 
                                 << metrics.average_parse_time_us << "µs | Network: " << cycle_time.count() << "ms | ";
                        std::cout << "Throughput: " << std::fixed << std::setprecision(1) 
                                 << metrics.throughput_mbps << " MB/s\n";
                        std::cout << "   🔥 Total cycle time: " << cycle_time.count() << "ms\n\n";
                    }
                } else {
                    std::cout << "⚠️  FastPing request failed - retrying...\n";
                }
            } catch (const std::exception& e) {
                std::cout << "🚨 Listener error: " << e.what() << "\n";
            }
            
            std::this_thread::sleep_for(ping_interval);
        }
    }
    
    void beaconThread() {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "🚨 Failed to create UDP socket\n";
            return;
        }
        
        sockaddr_in target_addr{};
        target_addr.sin_family = AF_INET;
        target_addr.sin_port = htons(beacon_port);
        inet_pton(AF_INET, beacon_ip.c_str(), &target_addr.sin_addr);
        
        while (running.load()) {
            try {
                UltimateBeaconPayload payload = createBeaconPayload();
                std::string json_payload = json_processor->serializeBeaconPayload(payload);
                
                ssize_t sent = sendto(sock, json_payload.c_str(), json_payload.length(), 0,
                                    reinterpret_cast<sockaddr*>(&target_addr), sizeof(target_addr));
                
                if (sent > 0) {
                    beacon_sequence.fetch_add(1);
                }
            } catch (const std::exception& e) {
                std::cout << "🚨 Beacon error: " << e.what() << "\n";
            }
            
            std::this_thread::sleep_for(beacon_interval);
        }
        
        close(sock);
    }
    
    void statusThread() {
        while (running.load()) {
            std::this_thread::sleep_for(status_interval);
            
            if (!running.load()) break;
            
            displayEnhancedStatus();
        }
    }
    
    UltimateBeaconPayload createBeaconPayload() {
        UltimateBeaconPayload payload;
        
        auto now = std::chrono::system_clock::now();
        payload.timestamp = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
        
        {
            std::lock_guard<std::mutex> lock(response_mutex);
            
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::high_resolution_clock::now() - last_response.response_time);
            
            payload.signal_age_seconds = static_cast<uint32_t>(age.count());
            payload.last_ping_status = last_response.status;
            payload.ping_latency_ms = last_response.server_processing_latency_ms;
            
            if (age.count() < 60 && last_response.status == "ok") {
                payload.status = "healthy";
            } else if (age.count() < 120) {
                payload.status = "warning";
            } else {
                payload.status = "critical";
            }
        }
        
        auto metrics = json_processor->getMetrics();
        payload.json_parse_time_microseconds = metrics.average_parse_time_us;
        payload.total_requests_processed = total_requests.load();
        payload.successful_parses = metrics.successful_parses;
        payload.failed_parses = metrics.total_parses - metrics.successful_parses;
        payload.average_throughput_mbps = metrics.throughput_mbps;
        payload.cpu_optimization_level = "SimdJSON-Optimized";
        
        auto uptime = std::chrono::duration_cast<std::chrono::hours>(
            std::chrono::high_resolution_clock::now() - start_time);
        payload.system_uptime_hours = uptime.count() + (uptime.count() % 1) / 60.0;
        
        payload.beacon_sequence_number = beacon_sequence.load();
        
        return payload;
    }
    
    void displayEnhancedStatus() {
        auto metrics = json_processor->getMetrics();
        
        std::cout << R"(
🏰 ═══════════════════════════════════════════════════════════════════ 🏰
   ULTIMATE LIGHTHOUSE STATUS REPORT
🏰 ═══════════════════════════════════════════════════════════════════ 🏰
)";
        
        {
            std::lock_guard<std::mutex> lock(response_mutex);
            auto age = std::chrono::duration_cast<std::chrono::seconds>(
                std::chrono::high_resolution_clock::now() - last_response.response_time);
            
            std::string health_indicator = "✅ HEALTHY";
            if (age.count() > 60) health_indicator = "⚠️  WARNING";
            if (age.count() > 120) health_indicator = "❌ CRITICAL";
            
            std::cout << "   Signal Health: " << health_indicator << "\n";
            std::cout << "   Last Status: " << last_response.status << "\n";
            std::cout << "   Current IP: " << last_response.connecting_ip << "\n";
            std::cout << "   Anonymity: " << last_response.anonymity_level << "\n";
            std::cout << "   Speed Hint: " << last_response.speed_hint << "\n";
            std::cout << "   Signal Age: " << age.count() << " seconds\n";
        }
        
        std::cout << "\n🚀 ULTRA-HIGH PERFORMANCE METRICS:\n";
        std::cout << "   Total Requests: " << total_requests.load() << "\n";
        std::cout << "   Parse Success Rate: " << std::fixed << std::setprecision(1) 
                 << metrics.success_rate << "%\n";
        std::cout << "   Average Parse Time: " << std::fixed << std::setprecision(2) 
                 << metrics.average_parse_time_us << " microseconds\n";
        std::cout << "   JSON Throughput: " << std::fixed << std::setprecision(1) 
                 << metrics.throughput_mbps << " MB/s\n";
        std::cout << "   Beacons Transmitted: " << beacon_sequence.load() << "\n";
        
        auto uptime = std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::high_resolution_clock::now() - start_time);
        std::cout << "   System Uptime: " << uptime.count() << " minutes\n";
        
        std::cout << "\n🔥 SIMDJSON OPTIMIZATION LEVEL: SimdJSON Auto-Optimized SIMD\n";
        
        std::cout << "🏰 ═══════════════════════════════════════════════════════════════════ 🏰\n";
    }
    
    void displayShutdownStats() {
        auto metrics = json_processor->getMetrics();
        auto uptime = std::chrono::duration_cast<std::chrono::minutes>(
            std::chrono::high_resolution_clock::now() - start_time);
        std::cout << "\n🏰 ULTIMATE LIGHTHOUSE SHUTDOWN STATISTICS:\n";
        std::cout << "   Total Runtime: " << uptime.count() << " minutes\n";
        std::cout << "   Total Requests: " << total_requests.load() << "\n";
        std::cout << "   Total JSON Parses: " << metrics.total_parses << "\n";
        std::cout << "   Success Rate: " << std::fixed << std::setprecision(1) 
                 << metrics.success_rate << "%\n";
        std::cout << "   Average Parse Time: " << std::fixed << std::setprecision(2) 
                 << metrics.average_parse_time_us << " microseconds\n";
        std::cout << "   Total Throughput: " << std::fixed << std::setprecision(1) 
                 << metrics.throughput_mbps << " MB/s\n";
        std::cout << "   Beacons Sent: " << beacon_sequence.load() << "\n";
        std::cout << "🏰 LIGHTHOUSE SECURED - Stay safe out there! 🏰\n\n";
    }
};

// Ultimate Beacon Listener for Testing
class UltimateBeaconListener {
private:
    std::unique_ptr<UltimateJsonProcessor> json_processor;
    int listen_port;
    std::atomic<bool> running{ false };
    
public:
    UltimateBeaconListener(int port = 9876) : listen_port(port) {
        json_processor = std::make_unique<UltimateJsonProcessor>();
        
        #ifdef _WIN32
            WSADATA wsaData;
            WSAStartup(MAKEWORD(2, 2), &wsaData);
        #endif
    }
    
    ~UltimateBeaconListener() {
        stop();
        #ifdef _WIN32
            WSACleanup();
        #endif
    }
    
    void start() {
        if (running.exchange(true)) {
            std::cout << "⚠️  Listener already running!\n";
            return;
        }
        
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "🚨 Failed to create UDP socket\n";
            return;
        }
        
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(listen_port);
        
        if (bind(sock, reinterpret_cast<sockaddr*>(&server_addr), sizeof(server_addr)) < 0) {
            std::cerr << "🚨 Failed to bind to port " << listen_port << "\n";
            close(sock);
            return;
        }
        
        std::cout << "🎯 Ultimate Beacon Listener bound to port " << listen_port << "\n";
        std::cout << "🎧 Listening for lighthouse beacons...\n";
        std::cout << "Press Ctrl+C to stop\n\n";
        
        char buffer[4096];
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        
        while (running.load()) {
            ssize_t received = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                                      reinterpret_cast<sockaddr*>(&client_addr), &client_len);
            
            if (received > 0) {
                buffer[received] = '\0';
                
                char client_ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
                
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                auto tm = *std::localtime(&time_t);
                
                std::cout << "📡 [" << std::put_time(&tm, "%H:%M:%S") << "] ";
                std::cout << "Received " << received << " bytes from " << client_ip << "\n";
                
                // Parse the beacon payload with ultra-fast SimdJSON
                UltimateBeaconPayload payload;
                auto start = std::chrono::high_resolution_clock::now();
                bool success = json_processor->parseBeaconPayload(payload, std::string(buffer, received));
                auto end = std::chrono::high_resolution_clock::now();
                
                auto parse_time = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
                double parse_microseconds = parse_time.count() / 1000.0;
                
                if (success) {
                    displayBeaconInfo(payload, parse_microseconds);
                } else {
                    std::cout << "🚨 Failed to parse beacon payload\n";
                    std::cout << "Raw data: " << std::string(buffer, received) << "\n\n";
                }
            }
        }
        
        close(sock);
    }
    
    void stop() {
        running.store(false);
    }
    
private:
    void displayBeaconInfo(const UltimateBeaconPayload& payload, double parse_time_us) {
        std::cout << "\n┌─────────────────────────────────────────┐\n";
        std::cout << "│ 🚨 ULTIMATE LIGHTHOUSE BEACON RECEIVED │\n";
        std::cout << "├─────────────────────────────────────────┤\n";
        std::cout << "│ ID: " << std::left << std::setw(31) << payload.beacon_id << " │\n";
        std::cout << "│ Status: " << std::left << std::setw(27) << payload.status << " │\n";
        std::cout << "│ Ping Status: " << std::left << std::setw(23) << payload.last_ping_status << " │\n";
        std::cout << "│ Ping Latency: " << std::left << std::setw(22) << (std::to_string(payload.ping_latency_ms) + "ms") << " │\n";
        std::cout << "│ Signal Age: " << std::left << std::setw(24) << (std::to_string(payload.signal_age_seconds) + "s") << " │\n";
        
        auto time_t = static_cast<std::time_t>(payload.timestamp);
        auto tm = *std::localtime(&time_t);
        std::ostringstream timestamp_str;
        timestamp_str << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
        std::cout << "│ Timestamp: " << std::left << std::setw(25) << timestamp_str.str() << " │\n";
        std::cout << "└─────────────────────────────────────────┘\n";
        
        // Health status indicator
        std::string health_status = "✅ HEALTHY";
        if (payload.status == "warning") health_status = "⚠️  WARNING";
        if (payload.status == "critical") health_status = "❌ CRITICAL";
        
        std::cout << health_status << "\n";
        std::cout << "   Parse time: " << std::fixed << std::setprecision(2) << parse_time_us << "µs | ";
        std::cout << "Validation: ✅\n";
        
        // Performance metrics display
        if (payload.total_requests_processed > 0) {
            std::cout << "\n🚀 LIGHTHOUSE PERFORMANCE METRICS:\n";
            std::cout << "   CPU Optimization: " << payload.cpu_optimization_level << "\n";
            std::cout << "   Total Requests: " << payload.total_requests_processed << "\n";
            std::cout << "   JSON Parse Time: " << std::fixed << std::setprecision(2) 
                     << payload.json_parse_time_microseconds << "µs\n";
            std::cout << "   JSON Throughput: " << std::fixed << std::setprecision(1) 
                     << payload.average_throughput_mbps << " MB/s\n";
            std::cout << "   System Uptime: " << std::fixed << std::setprecision(1) 
                     << payload.system_uptime_hours << " hours\n";
            std::cout << "   Beacon #" << payload.beacon_sequence_number << "\n";
        }
        
        std::cout << "\n";
    }
};

} // namespace UltimateLighthouse

// MAIN ENTRY POINT
int main(int argc, char* argv[]) {
    try {
        if (argc > 1 && std::string(argv[1]) == "--listener") {
            // Run as listener
            int port = 9876;
            if (argc > 2) {
                port = std::stoi(argv[2]);
            }
            
            UltimateLighthouse::UltimateBeaconListener listener(port);
            listener.start();
        } else {
            // Run as lighthouse beacon
            UltimateLighthouse::UltimateLighthouseBeacon lighthouse;
            lighthouse.start();
            
            // Wait for user input
            std::string input;
            std::getline(std::cin, input);
            
            lighthouse.stop();
        }
    } catch (const std::exception& e) {
        std::cerr << "🚨 Fatal Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}