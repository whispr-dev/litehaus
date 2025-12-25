/**
 * 🏰 LITEHAUS HOME MASTER NODE - TYPE DEFINITIONS
 * ================================================
 * Core data structures for the authoritative physical beacon node.
 * 
 * Copyright (c) 2025 Litehaus Project
 * License: MIT
 */

#pragma once

#include <string>
#include <cstdint>
#include <chrono>
#include <atomic>
#include <optional>

namespace litehaus {

// ============================================================================
// TIME DISCIPLINE TELEMETRY
// ============================================================================

enum class TimeDisciplineState {
    UNLOCKED,       // No GPS/PPS signal
    ACQUIRING,      // Acquiring lock
    LOCKED,         // Disciplined and stable
    HOLDOVER        // Lost signal, running on holdover
};

struct TimeStatus {
    TimeDisciplineState discipline_state{TimeDisciplineState::UNLOCKED};
    std::string source{"none"};          // "gpsdo", "ntp", "freerun"
    bool pps_locked{false};
    double estimated_error_ns{1000000.0}; // Nanoseconds
    uint64_t holdover_seconds{0};
    uint32_t satellites_visible{0};
    double frequency_offset_ppb{0.0};     // Parts per billion
    
    // Chrony stats
    double system_offset_us{0.0};
    double root_dispersion_us{0.0};
    std::string reference_id{};
    int stratum{16};  // 16 = unsynchronized
    
    // GPSDO specific
    bool gps_3d_fix{false};
    double latitude{0.0};
    double longitude{0.0};
    double altitude_m{0.0};
};

// ============================================================================
// POWER & UPS TELEMETRY  
// ============================================================================

enum class PowerSource {
    GRID,
    UPS_BATTERY,
    GENERATOR,
    UNKNOWN
};

struct PowerStatus {
    PowerSource source{PowerSource::UNKNOWN};
    bool ups_present{false};
    bool on_battery{false};
    double battery_percent{0.0};
    uint32_t estimated_runtime_minutes{0};
    double input_voltage{0.0};
    double load_percent{0.0};
    std::string ups_model{};
    std::string ups_status{};  // "OL" = Online, "OB" = On Battery, etc.
};

// ============================================================================
// SYSTEM HEALTH TELEMETRY
// ============================================================================

struct SystemStatus {
    double cpu_load_1m{0.0};
    double cpu_load_5m{0.0};
    double cpu_load_15m{0.0};
    double cpu_temp_celsius{0.0};
    
    uint64_t memory_total_mb{0};
    uint64_t memory_used_mb{0};
    uint64_t memory_available_mb{0};
    
    uint64_t disk_total_gb{0};
    uint64_t disk_used_gb{0};
    uint64_t disk_free_gb{0};
    
    uint64_t uptime_seconds{0};
    uint32_t process_count{0};
    
    // Network
    std::string primary_ip{};
    uint64_t bytes_sent{0};
    uint64_t bytes_recv{0};
    bool wan_reachable{false};
};

// ============================================================================
// LORA GATEWAY TELEMETRY
// ============================================================================

struct LoRaStatus {
    bool gateway_up{false};
    std::string gateway_id{};
    uint64_t rx_packets_total{0};
    uint64_t tx_packets_total{0};
    uint64_t rx_packets_ok{0};
    uint64_t rx_packets_bad_crc{0};
    uint64_t tx_packets_ok{0};
    uint64_t tx_packets_failed{0};
    
    // RF quality
    double last_rssi_dbm{0.0};
    double last_snr_db{0.0};
    uint32_t active_channels{0};
    
    // Timing
    bool gps_enabled{false};
    bool pps_locked{false};
    double timestamp_accuracy_us{0.0};
};

// ============================================================================
// SDR / RADIO BRIDGE TELEMETRY
// ============================================================================

struct RadioBridgeStatus {
    bool sdr_connected{false};
    std::string sdr_model{};
    double center_frequency_mhz{0.0};
    double sample_rate_msps{0.0};
    
    uint64_t requests_received{0};
    uint64_t requests_processed{0};
    uint64_t requests_rate_limited{0};
    uint64_t requests_invalid{0};
    
    // Last request info
    std::string last_request_type{};
    std::string last_request_target{};
    std::chrono::system_clock::time_point last_request_time{};
};

// ============================================================================
// WIFI HALO TELEMETRY
// ============================================================================

struct WifiHaloStatus {
    bool ap_active{false};
    std::string ssid{};
    uint32_t clients_connected{0};
    uint64_t bytes_sent{0};
    uint64_t bytes_recv{0};
    std::string channel{};
    double signal_quality_percent{0.0};
};

// ============================================================================
// MASTER BEACON PAYLOAD
// ============================================================================

enum class NodeHealth {
    HEALTHY,
    WARNING,
    CRITICAL,
    UNKNOWN
};

struct MasterBeaconPayload {
    // Identity
    std::string node_id{"litehaus-home-uk-01"};
    std::string node_type{"home_physical"};
    std::string lighthouse_version{"MARS-v1.0-HOME-MASTER"};
    
    // Timestamps
    uint64_t timestamp_unix{0};
    uint64_t timestamp_ns{0};  // Nanosecond precision if GPSDO locked
    uint32_t beacon_sequence{0};
    
    // Overall health
    NodeHealth health{NodeHealth::UNKNOWN};
    std::string status_message{};
    
    // Subsystem status
    TimeStatus time{};
    PowerStatus power{};
    SystemStatus system{};
    LoRaStatus lora{};
    RadioBridgeStatus radio_bridge{};
    WifiHaloStatus wifi_halo{};
    
    // Performance metrics (from existing litehaus.cpp)
    double json_parse_time_us{0.0};
    uint64_t total_requests_processed{0};
    double average_throughput_mbps{0.0};
    
    // Network probes (FastPing style)
    std::string last_ping_status{};
    double ping_latency_ms{0.0};
    uint32_t signal_age_seconds{0};
    std::string connecting_ip{};
};

// ============================================================================
// CONFIGURATION STRUCTURES
// ============================================================================

struct CoreConfig {
    // Identity
    std::string node_id{"litehaus-home-uk-01"};
    std::string node_domain{"mars.litehaus.online"};
    
    // Network
    std::string lan_interface{"eth0"};
    std::string lan_subnet{"10.0.0.0/24"};
    std::string node_ip{"10.0.0.10"};
    
    // Time
    std::string gpsdo_device{"/dev/ttyUSB0"};
    std::string pps_device{"/dev/pps0"};
    bool enable_chrony{true};
    
    // Intervals (seconds)
    uint32_t beacon_interval_ms{1000};   // 1Hz default
    uint32_t status_interval_s{30};
    uint32_t probe_interval_s{60};
    
    // Upstream targets
    std::string fastping_url{"http://fastping.it.com:9876/ping"};
    std::vector<std::pair<std::string, uint16_t>> beacon_targets{
        {"137.184.105.114", 9876},   // NYC
        {"lon.litehaus.online", 9876},
        {"syd.litehaus.online", 9876},
        {"sgp.litehaus.online", 9876}
    };
};

struct StreamConfig {
    uint16_t websocket_port{8083};
    uint16_t http_port{8080};
    uint16_t udp_listen_port{9876};
    uint16_t udp_broadcast_port{9877};
    std::string bind_address{"0.0.0.0"};
    uint32_t max_clients{100};
};

struct LoRaConfig {
    std::string device_path{"/dev/ttyACM0"};
    std::string config_file{"/etc/litehaus/lora_global_conf.json"};
    std::string gateway_id{};
    std::string region{"EU868"};
    bool enable_gps{false};
    uint16_t upstream_port{1700};
    uint16_t downstream_port{1700};
};

} // namespace litehaus
