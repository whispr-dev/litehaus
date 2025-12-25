/**
 * 🏰 LITEHAUS HOME MASTER NODE - JSON SERIALIZATION
 * ==================================================
 * Ultra-fast JSON serialization using SimdJSON for parsing
 * and manual construction for serialization (fastest method).
 * 
 * Copyright (c) 2025 Litehaus Project
 * License: MIT
 */

#pragma once

#include "litehaus_types.hpp"
#include <simdjson.h>
#include <sstream>
#include <iomanip>
#include <mutex>
#include <chrono>

namespace litehaus {
namespace json {

// ============================================================================
// JSON SERIALIZER - Manual construction for speed
// ============================================================================

class BeaconSerializer {
public:
    /**
     * Serialize MasterBeaconPayload to JSON string
     * Manual construction is faster than using a JSON library for output
     */
    static std::string serialize(const MasterBeaconPayload& payload) {
        std::ostringstream json;
        json << std::fixed;
        
        json << "{";
        
        // Identity
        json << "\"node_id\":\"" << escapeString(payload.node_id) << "\",";
        json << "\"node_type\":\"" << escapeString(payload.node_type) << "\",";
        json << "\"lighthouse_version\":\"" << escapeString(payload.lighthouse_version) << "\",";
        
        // Timestamps
        json << "\"timestamp\":" << payload.timestamp_unix << ",";
        json << "\"timestamp_ns\":" << payload.timestamp_ns << ",";
        json << "\"beacon_sequence\":" << payload.beacon_sequence << ",";
        
        // Health
        json << "\"health\":\"" << healthToString(payload.health) << "\",";
        json << "\"status_message\":\"" << escapeString(payload.status_message) << "\",";
        
        // Time subsystem
        json << "\"time\":{";
        json << "\"discipline_state\":\"" << disciplineStateToString(payload.time.discipline_state) << "\",";
        json << "\"source\":\"" << escapeString(payload.time.source) << "\",";
        json << "\"pps_locked\":" << (payload.time.pps_locked ? "true" : "false") << ",";
        json << "\"estimated_error_ns\":" << std::setprecision(1) << payload.time.estimated_error_ns << ",";
        json << "\"holdover_seconds\":" << payload.time.holdover_seconds << ",";
        json << "\"satellites_visible\":" << payload.time.satellites_visible << ",";
        json << "\"stratum\":" << payload.time.stratum << ",";
        json << "\"system_offset_us\":" << std::setprecision(3) << payload.time.system_offset_us;
        if (payload.time.gps_3d_fix) {
            json << ",\"gps_3d_fix\":true";
            json << ",\"latitude\":" << std::setprecision(6) << payload.time.latitude;
            json << ",\"longitude\":" << std::setprecision(6) << payload.time.longitude;
            json << ",\"altitude_m\":" << std::setprecision(1) << payload.time.altitude_m;
        }
        json << "},";
        
        // Power subsystem
        json << "\"power\":{";
        json << "\"source\":\"" << powerSourceToString(payload.power.source) << "\",";
        json << "\"ups_present\":" << (payload.power.ups_present ? "true" : "false") << ",";
        json << "\"on_battery\":" << (payload.power.on_battery ? "true" : "false") << ",";
        json << "\"battery_percent\":" << std::setprecision(1) << payload.power.battery_percent << ",";
        json << "\"estimated_runtime_minutes\":" << payload.power.estimated_runtime_minutes;
        if (!payload.power.ups_status.empty()) {
            json << ",\"ups_status\":\"" << escapeString(payload.power.ups_status) << "\"";
        }
        json << "},";
        
        // System subsystem
        json << "\"system\":{";
        json << "\"cpu_load_1m\":" << std::setprecision(2) << payload.system.cpu_load_1m << ",";
        json << "\"cpu_load_5m\":" << std::setprecision(2) << payload.system.cpu_load_5m << ",";
        json << "\"cpu_temp_celsius\":" << std::setprecision(1) << payload.system.cpu_temp_celsius << ",";
        json << "\"memory_used_mb\":" << payload.system.memory_used_mb << ",";
        json << "\"memory_total_mb\":" << payload.system.memory_total_mb << ",";
        json << "\"disk_free_gb\":" << payload.system.disk_free_gb << ",";
        json << "\"uptime_seconds\":" << payload.system.uptime_seconds << ",";
        json << "\"wan_reachable\":" << (payload.system.wan_reachable ? "true" : "false");
        if (!payload.system.primary_ip.empty()) {
            json << ",\"primary_ip\":\"" << escapeString(payload.system.primary_ip) << "\"";
        }
        json << "},";
        
        // LoRa subsystem
        json << "\"lora\":{";
        json << "\"gateway_up\":" << (payload.lora.gateway_up ? "true" : "false") << ",";
        json << "\"rx_packets_total\":" << payload.lora.rx_packets_total << ",";
        json << "\"tx_packets_total\":" << payload.lora.tx_packets_total << ",";
        json << "\"rx_packets_ok\":" << payload.lora.rx_packets_ok << ",";
        json << "\"last_rssi_dbm\":" << std::setprecision(1) << payload.lora.last_rssi_dbm << ",";
        json << "\"last_snr_db\":" << std::setprecision(1) << payload.lora.last_snr_db;
        if (!payload.lora.gateway_id.empty()) {
            json << ",\"gateway_id\":\"" << escapeString(payload.lora.gateway_id) << "\"";
        }
        json << "},";
        
        // Radio bridge subsystem
        json << "\"radio_bridge\":{";
        json << "\"sdr_connected\":" << (payload.radio_bridge.sdr_connected ? "true" : "false") << ",";
        json << "\"requests_processed\":" << payload.radio_bridge.requests_processed << ",";
        json << "\"requests_rate_limited\":" << payload.radio_bridge.requests_rate_limited;
        if (!payload.radio_bridge.sdr_model.empty()) {
            json << ",\"sdr_model\":\"" << escapeString(payload.radio_bridge.sdr_model) << "\"";
        }
        json << "},";
        
        // WiFi halo subsystem
        json << "\"wifi_halo\":{";
        json << "\"ap_active\":" << (payload.wifi_halo.ap_active ? "true" : "false") << ",";
        json << "\"clients_connected\":" << payload.wifi_halo.clients_connected;
        if (!payload.wifi_halo.ssid.empty()) {
            json << ",\"ssid\":\"" << escapeString(payload.wifi_halo.ssid) << "\"";
        }
        json << "},";
        
        // Performance metrics (compatible with existing litehaus)
        json << "\"json_parse_time_microseconds\":" << std::setprecision(2) << payload.json_parse_time_us << ",";
        json << "\"total_requests_processed\":" << payload.total_requests_processed << ",";
        json << "\"average_throughput_mbps\":" << std::setprecision(1) << payload.average_throughput_mbps << ",";
        
        // Network probes
        json << "\"last_ping_status\":\"" << escapeString(payload.last_ping_status) << "\",";
        json << "\"ping_latency_ms\":" << std::setprecision(2) << payload.ping_latency_ms << ",";
        json << "\"signal_age_seconds\":" << payload.signal_age_seconds;
        if (!payload.connecting_ip.empty()) {
            json << ",\"connecting_ip\":\"" << escapeString(payload.connecting_ip) << "\"";
        }
        
        json << "}";
        
        return json.str();
    }
    
    /**
     * Serialize a compact beacon (minimal payload for high-frequency broadcasts)
     */
    static std::string serializeCompact(const MasterBeaconPayload& payload) {
        std::ostringstream json;
        json << std::fixed;
        
        json << "{";
        json << "\"id\":\"" << escapeString(payload.node_id) << "\",";
        json << "\"t\":" << payload.timestamp_unix << ",";
        json << "\"seq\":" << payload.beacon_sequence << ",";
        json << "\"h\":\"" << healthToString(payload.health)[0] << "\",";  // H/W/C/U
        json << "\"pps\":" << (payload.time.pps_locked ? 1 : 0) << ",";
        json << "\"err_ns\":" << static_cast<int64_t>(payload.time.estimated_error_ns) << ",";
        json << "\"ups\":" << (payload.power.on_battery ? 0 : 1) << ",";
        json << "\"cpu\":" << std::setprecision(1) << payload.system.cpu_load_1m << ",";
        json << "\"lora\":" << (payload.lora.gateway_up ? 1 : 0);
        json << "}";
        
        return json.str();
    }

private:
    static std::string escapeString(const std::string& s) {
        std::string result;
        result.reserve(s.size() + 16);
        for (char c : s) {
            switch (c) {
                case '"':  result += "\\\""; break;
                case '\\': result += "\\\\"; break;
                case '\n': result += "\\n"; break;
                case '\r': result += "\\r"; break;
                case '\t': result += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20) {
                        // Control character - encode as \u00XX
                        char buf[8];
                        snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                        result += buf;
                    } else {
                        result += c;
                    }
            }
        }
        return result;
    }
    
    static const char* healthToString(NodeHealth h) {
        switch (h) {
            case NodeHealth::HEALTHY:  return "healthy";
            case NodeHealth::WARNING:  return "warning";
            case NodeHealth::CRITICAL: return "critical";
            default:                   return "unknown";
        }
    }
    
    static const char* disciplineStateToString(TimeDisciplineState s) {
        switch (s) {
            case TimeDisciplineState::UNLOCKED:  return "unlocked";
            case TimeDisciplineState::ACQUIRING: return "acquiring";
            case TimeDisciplineState::LOCKED:    return "locked";
            case TimeDisciplineState::HOLDOVER:  return "holdover";
            default:                             return "unknown";
        }
    }
    
    static const char* powerSourceToString(PowerSource s) {
        switch (s) {
            case PowerSource::GRID:        return "grid";
            case PowerSource::UPS_BATTERY: return "ups_battery";
            case PowerSource::GENERATOR:   return "generator";
            default:                       return "unknown";
        }
    }
};

// ============================================================================
// JSON PARSER - SimdJSON powered for speed
// ============================================================================

class BeaconParser {
public:
    BeaconParser() = default;
    
    /**
     * Parse a beacon payload from JSON string
     * Returns parse time in microseconds via out parameter
     */
    bool parse(const std::string& json_data, MasterBeaconPayload& payload, double& parse_time_us) {
        auto start = std::chrono::high_resolution_clock::now();
        
        try {
            simdjson::dom::element doc = parser_.parse(json_data);
            
            // Identity
            if (auto r = doc["node_id"]; !r.error()) {
                payload.node_id = std::string(r.get_string().value());
            }
            if (auto r = doc["node_type"]; !r.error()) {
                payload.node_type = std::string(r.get_string().value());
            }
            
            // Timestamps
            if (auto r = doc["timestamp"]; !r.error()) {
                payload.timestamp_unix = r.get_uint64().value();
            }
            if (auto r = doc["beacon_sequence"]; !r.error()) {
                payload.beacon_sequence = static_cast<uint32_t>(r.get_uint64().value());
            }
            
            // Health
            if (auto r = doc["health"]; !r.error()) {
                std::string h = std::string(r.get_string().value());
                if (h == "healthy") payload.health = NodeHealth::HEALTHY;
                else if (h == "warning") payload.health = NodeHealth::WARNING;
                else if (h == "critical") payload.health = NodeHealth::CRITICAL;
                else payload.health = NodeHealth::UNKNOWN;
            }
            
            // Time subsystem
            if (auto time_obj = doc["time"]; !time_obj.error()) {
                if (auto r = time_obj["pps_locked"]; !r.error()) {
                    payload.time.pps_locked = r.get_bool().value();
                }
                if (auto r = time_obj["estimated_error_ns"]; !r.error()) {
                    payload.time.estimated_error_ns = r.get_double().value();
                }
                if (auto r = time_obj["stratum"]; !r.error()) {
                    payload.time.stratum = static_cast<int>(r.get_int64().value());
                }
            }
            
            // Power subsystem
            if (auto power_obj = doc["power"]; !power_obj.error()) {
                if (auto r = power_obj["ups_present"]; !r.error()) {
                    payload.power.ups_present = r.get_bool().value();
                }
                if (auto r = power_obj["on_battery"]; !r.error()) {
                    payload.power.on_battery = r.get_bool().value();
                }
                if (auto r = power_obj["battery_percent"]; !r.error()) {
                    payload.power.battery_percent = r.get_double().value();
                }
            }
            
            // System subsystem
            if (auto sys_obj = doc["system"]; !sys_obj.error()) {
                if (auto r = sys_obj["cpu_load_1m"]; !r.error()) {
                    payload.system.cpu_load_1m = r.get_double().value();
                }
                if (auto r = sys_obj["uptime_seconds"]; !r.error()) {
                    payload.system.uptime_seconds = r.get_uint64().value();
                }
                if (auto r = sys_obj["wan_reachable"]; !r.error()) {
                    payload.system.wan_reachable = r.get_bool().value();
                }
            }
            
            // Legacy fields (compatible with existing litehaus beacons)
            if (auto r = doc["last_ping_status"]; !r.error()) {
                payload.last_ping_status = std::string(r.get_string().value());
            }
            if (auto r = doc["ping_latency_ms"]; !r.error()) {
                payload.ping_latency_ms = r.get_double().value();
            }
            if (auto r = doc["signal_age_seconds"]; !r.error()) {
                payload.signal_age_seconds = static_cast<uint32_t>(r.get_uint64().value());
            }
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            parse_time_us = duration.count() / 1000.0;
            
            total_parses_++;
            successful_parses_++;
            total_bytes_ += json_data.size();
            total_parse_time_us_ += parse_time_us;
            
            return true;
            
        } catch (const simdjson::simdjson_error& e) {
            total_parses_++;
            
            auto end = std::chrono::high_resolution_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
            parse_time_us = duration.count() / 1000.0;
            
            return false;
        }
    }
    
    struct ParserStats {
        uint64_t total_parses{0};
        uint64_t successful_parses{0};
        uint64_t total_bytes{0};
        double average_parse_time_us{0.0};
        double throughput_mbps{0.0};
        double success_rate{0.0};
    };
    
    ParserStats getStats() const {
        ParserStats stats;
        stats.total_parses = total_parses_.load();
        stats.successful_parses = successful_parses_.load();
        stats.total_bytes = total_bytes_.load();
        
        if (stats.total_parses > 0) {
            stats.average_parse_time_us = total_parse_time_us_.load() / stats.total_parses;
            stats.success_rate = 100.0 * stats.successful_parses / stats.total_parses;
        }
        
        double total_time_s = total_parse_time_us_.load() / 1000000.0;
        if (total_time_s > 0) {
            double mb = stats.total_bytes / (1024.0 * 1024.0);
            stats.throughput_mbps = mb / total_time_s;
        }
        
        return stats;
    }

private:
    simdjson::dom::parser parser_;
    std::atomic<uint64_t> total_parses_{0};
    std::atomic<uint64_t> successful_parses_{0};
    std::atomic<uint64_t> total_bytes_{0};
    std::atomic<double> total_parse_time_us_{0.0};
};

} // namespace json
} // namespace litehaus
