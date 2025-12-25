/**
 * 🏰 LITEHAUS HOME MASTER NODE - SYSTEM COLLECTORS
 * =================================================
 * Collects telemetry from various system sources:
 * - Time discipline (chrony, gpsd, PPS)
 * - Power/UPS status (NUT/apcupsd)
 * - System health (proc filesystem)
 * 
 * Copyright (c) 2025 Litehaus Project
 * License: MIT
 */

#pragma once

#include "litehaus_types.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <array>
#include <regex>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <dirent.h>

namespace litehaus {
namespace collectors {

// ============================================================================
// TIME COLLECTOR - Chrony & GPSD Interface
// ============================================================================

class TimeCollector {
public:
    TimeCollector() = default;
    
    /**
     * Collect time discipline status from chrony and gpsd
     */
    TimeStatus collect() {
        TimeStatus status;
        
        // Try chrony first
        if (collectChrony(status)) {
            // Enhance with gpsd if available
            collectGpsd(status);
        } else {
            // Fall back to basic NTP check
            collectNtpFallback(status);
        }
        
        return status;
    }
    
private:
    /**
     * Parse chrony tracking output
     * Example output:
     * Reference ID    : 50505300 (PPS)
     * Stratum         : 1
     * Ref time (UTC)  : Thu Dec 25 12:00:00 2025
     * System time     : 0.000000123 seconds fast of NTP time
     * Last offset     : +0.000000045 seconds
     * RMS offset      : 0.000000089 seconds
     * Frequency       : 0.123 ppm slow
     * ...
     */
    bool collectChrony(TimeStatus& status) {
        FILE* pipe = popen("chronyc tracking 2>/dev/null", "r");
        if (!pipe) return false;
        
        char buffer[256];
        bool found_data = false;
        
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            
            if (line.find("Reference ID") != std::string::npos) {
                // Extract reference ID (e.g., "PPS" or "GPS")
                size_t paren_start = line.find('(');
                size_t paren_end = line.find(')');
                if (paren_start != std::string::npos && paren_end != std::string::npos) {
                    status.reference_id = line.substr(paren_start + 1, paren_end - paren_start - 1);
                    
                    // Determine source from reference ID
                    if (status.reference_id == "PPS" || status.reference_id.find("GPS") != std::string::npos) {
                        status.source = "gpsdo";
                        status.discipline_state = TimeDisciplineState::LOCKED;
                    } else if (status.reference_id.find("NTP") != std::string::npos || 
                               status.reference_id.find("pool") != std::string::npos) {
                        status.source = "ntp";
                        status.discipline_state = TimeDisciplineState::LOCKED;
                    }
                }
                found_data = true;
            }
            else if (line.find("Stratum") != std::string::npos) {
                // Extract stratum number
                std::regex stratum_regex(R"(Stratum\s*:\s*(\d+))");
                std::smatch match;
                if (std::regex_search(line, match, stratum_regex)) {
                    status.stratum = std::stoi(match[1].str());
                }
            }
            else if (line.find("System time") != std::string::npos) {
                // Extract system offset in seconds, convert to microseconds
                std::regex offset_regex(R"(:\s*([-+]?\d+\.?\d*)\s*seconds)");
                std::smatch match;
                if (std::regex_search(line, match, offset_regex)) {
                    double offset_s = std::stod(match[1].str());
                    status.system_offset_us = offset_s * 1000000.0;
                    status.estimated_error_ns = std::abs(offset_s) * 1000000000.0;
                }
            }
            else if (line.find("Root dispersion") != std::string::npos) {
                std::regex disp_regex(R"(:\s*([-+]?\d+\.?\d*)\s*seconds)");
                std::smatch match;
                if (std::regex_search(line, match, disp_regex)) {
                    status.root_dispersion_us = std::stod(match[1].str()) * 1000000.0;
                }
            }
            else if (line.find("Frequency") != std::string::npos) {
                // Extract frequency offset in ppm, convert to ppb
                std::regex freq_regex(R"(:\s*([-+]?\d+\.?\d*)\s*ppm)");
                std::smatch match;
                if (std::regex_search(line, match, freq_regex)) {
                    status.frequency_offset_ppb = std::stod(match[1].str()) * 1000.0;
                }
            }
            else if (line.find("Leap status") != std::string::npos) {
                if (line.find("Normal") != std::string::npos) {
                    status.pps_locked = true;
                }
            }
        }
        
        pclose(pipe);
        return found_data;
    }
    
    /**
     * Collect GPS info from gpsd via gpspipe or direct socket
     */
    bool collectGpsd(TimeStatus& status) {
        // Try gpspipe for a quick status
        FILE* pipe = popen("gpspipe -w -n 5 2>/dev/null | grep -m1 TPV", "r");
        if (!pipe) return false;
        
        char buffer[1024];
        bool found = false;
        
        if (fgets(buffer, sizeof(buffer), pipe)) {
            std::string json(buffer);
            
            // Quick parse for key fields (not using full JSON parser for speed)
            // Looking for: "mode":3, "lat":, "lon":, "alt":, "nSat":
            
            if (json.find("\"mode\":3") != std::string::npos) {
                status.gps_3d_fix = true;
            } else if (json.find("\"mode\":2") != std::string::npos) {
                status.gps_3d_fix = false;  // 2D fix
            }
            
            // Extract lat/lon/alt with regex
            std::regex lat_regex(R"("lat":\s*([-+]?\d+\.?\d*))");
            std::regex lon_regex(R"("lon":\s*([-+]?\d+\.?\d*))");
            std::regex alt_regex(R"("alt(?:MSL|HAE)":\s*([-+]?\d+\.?\d*))");
            std::regex nsat_regex(R"("nSat":\s*(\d+))");
            
            std::smatch match;
            if (std::regex_search(json, match, lat_regex)) {
                status.latitude = std::stod(match[1].str());
            }
            if (std::regex_search(json, match, lon_regex)) {
                status.longitude = std::stod(match[1].str());
            }
            if (std::regex_search(json, match, alt_regex)) {
                status.altitude_m = std::stod(match[1].str());
            }
            if (std::regex_search(json, match, nsat_regex)) {
                status.satellites_visible = std::stoul(match[1].str());
            }
            
            found = true;
        }
        
        pclose(pipe);
        return found;
    }
    
    /**
     * Fallback NTP check via timedatectl
     */
    bool collectNtpFallback(TimeStatus& status) {
        FILE* pipe = popen("timedatectl show 2>/dev/null", "r");
        if (!pipe) {
            status.source = "freerun";
            status.discipline_state = TimeDisciplineState::UNLOCKED;
            status.stratum = 16;
            return false;
        }
        
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            if (line.find("NTPSynchronized=yes") != std::string::npos) {
                status.source = "ntp";
                status.discipline_state = TimeDisciplineState::LOCKED;
                status.stratum = 3;  // Assume stratum 3 for basic NTP
            }
        }
        
        pclose(pipe);
        return true;
    }
};

// ============================================================================
// POWER COLLECTOR - UPS Status via NUT or apcupsd
// ============================================================================

class PowerCollector {
public:
    PowerCollector() = default;
    
    PowerStatus collect() {
        PowerStatus status;
        
        // Try NUT (Network UPS Tools) first
        if (collectNut(status)) {
            return status;
        }
        
        // Try apcupsd
        if (collectApcupsd(status)) {
            return status;
        }
        
        // No UPS monitoring available - assume grid power
        status.source = PowerSource::GRID;
        status.ups_present = false;
        
        return status;
    }
    
private:
    /**
     * Collect from NUT (upsc command)
     */
    bool collectNut(PowerStatus& status) {
        // First, list available UPS devices
        FILE* pipe = popen("upsc -l 2>/dev/null | head -1", "r");
        if (!pipe) return false;
        
        char ups_name[64] = {0};
        if (!fgets(ups_name, sizeof(ups_name), pipe)) {
            pclose(pipe);
            return false;
        }
        pclose(pipe);
        
        // Trim newline
        size_t len = strlen(ups_name);
        if (len > 0 && ups_name[len-1] == '\n') ups_name[len-1] = '\0';
        if (strlen(ups_name) == 0) return false;
        
        // Query the UPS
        char cmd[128];
        snprintf(cmd, sizeof(cmd), "upsc %s 2>/dev/null", ups_name);
        pipe = popen(cmd, "r");
        if (!pipe) return false;
        
        status.ups_present = true;
        char buffer[256];
        
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            
            if (line.find("ups.status:") != std::string::npos) {
                status.ups_status = line.substr(line.find(':') + 2);
                // Trim
                status.ups_status.erase(status.ups_status.find_last_not_of(" \n\r") + 1);
                
                if (status.ups_status.find("OL") != std::string::npos) {
                    status.source = PowerSource::GRID;
                    status.on_battery = false;
                } else if (status.ups_status.find("OB") != std::string::npos) {
                    status.source = PowerSource::UPS_BATTERY;
                    status.on_battery = true;
                }
            }
            else if (line.find("battery.charge:") != std::string::npos) {
                status.battery_percent = std::stod(line.substr(line.find(':') + 2));
            }
            else if (line.find("battery.runtime:") != std::string::npos) {
                // Runtime in seconds, convert to minutes
                status.estimated_runtime_minutes = std::stoul(line.substr(line.find(':') + 2)) / 60;
            }
            else if (line.find("input.voltage:") != std::string::npos) {
                status.input_voltage = std::stod(line.substr(line.find(':') + 2));
            }
            else if (line.find("ups.load:") != std::string::npos) {
                status.load_percent = std::stod(line.substr(line.find(':') + 2));
            }
            else if (line.find("ups.model:") != std::string::npos) {
                status.ups_model = line.substr(line.find(':') + 2);
                status.ups_model.erase(status.ups_model.find_last_not_of(" \n\r") + 1);
            }
        }
        
        pclose(pipe);
        return status.ups_present;
    }
    
    /**
     * Collect from apcupsd
     */
    bool collectApcupsd(PowerStatus& status) {
        FILE* pipe = popen("apcaccess 2>/dev/null", "r");
        if (!pipe) return false;
        
        char buffer[256];
        bool found_data = false;
        
        while (fgets(buffer, sizeof(buffer), pipe)) {
            std::string line(buffer);
            
            if (line.find("STATUS") != std::string::npos && line.find(':') != std::string::npos) {
                status.ups_status = line.substr(line.find(':') + 2);
                status.ups_status.erase(status.ups_status.find_last_not_of(" \n\r") + 1);
                status.ups_present = true;
                found_data = true;
                
                if (status.ups_status == "ONLINE") {
                    status.source = PowerSource::GRID;
                    status.on_battery = false;
                } else if (status.ups_status == "ONBATT") {
                    status.source = PowerSource::UPS_BATTERY;
                    status.on_battery = true;
                }
            }
            else if (line.find("BCHARGE") != std::string::npos) {
                std::regex pct_regex(R"(:\s*(\d+\.?\d*))");
                std::smatch match;
                if (std::regex_search(line, match, pct_regex)) {
                    status.battery_percent = std::stod(match[1].str());
                }
            }
            else if (line.find("TIMELEFT") != std::string::npos) {
                std::regex time_regex(R"(:\s*(\d+\.?\d*))");
                std::smatch match;
                if (std::regex_search(line, match, time_regex)) {
                    status.estimated_runtime_minutes = static_cast<uint32_t>(std::stod(match[1].str()));
                }
            }
            else if (line.find("LINEV") != std::string::npos) {
                std::regex volt_regex(R"(:\s*(\d+\.?\d*))");
                std::smatch match;
                if (std::regex_search(line, match, volt_regex)) {
                    status.input_voltage = std::stod(match[1].str());
                }
            }
            else if (line.find("LOADPCT") != std::string::npos) {
                std::regex load_regex(R"(:\s*(\d+\.?\d*))");
                std::smatch match;
                if (std::regex_search(line, match, load_regex)) {
                    status.load_percent = std::stod(match[1].str());
                }
            }
            else if (line.find("MODEL") != std::string::npos) {
                status.ups_model = line.substr(line.find(':') + 2);
                status.ups_model.erase(status.ups_model.find_last_not_of(" \n\r") + 1);
            }
        }
        
        pclose(pipe);
        return found_data;
    }
};

// ============================================================================
// SYSTEM COLLECTOR - CPU, Memory, Disk, Network
// ============================================================================

class SystemCollector {
public:
    SystemCollector() = default;
    
    SystemStatus collect() {
        SystemStatus status;
        
        collectLoadAverage(status);
        collectMemory(status);
        collectDisk(status);
        collectUptime(status);
        collectCpuTemp(status);
        collectNetwork(status);
        checkWanReachability(status);
        
        return status;
    }
    
private:
    void collectLoadAverage(SystemStatus& status) {
        std::ifstream loadavg("/proc/loadavg");
        if (loadavg.is_open()) {
            loadavg >> status.cpu_load_1m >> status.cpu_load_5m >> status.cpu_load_15m;
        }
    }
    
    void collectMemory(SystemStatus& status) {
        std::ifstream meminfo("/proc/meminfo");
        if (!meminfo.is_open()) return;
        
        std::string line;
        uint64_t mem_total = 0, mem_free = 0, mem_available = 0, buffers = 0, cached = 0;
        
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") == 0) {
                sscanf(line.c_str(), "MemTotal: %lu", &mem_total);
            } else if (line.find("MemFree:") == 0) {
                sscanf(line.c_str(), "MemFree: %lu", &mem_free);
            } else if (line.find("MemAvailable:") == 0) {
                sscanf(line.c_str(), "MemAvailable: %lu", &mem_available);
            } else if (line.find("Buffers:") == 0) {
                sscanf(line.c_str(), "Buffers: %lu", &buffers);
            } else if (line.find("Cached:") == 0 && line.find("SwapCached:") == std::string::npos) {
                sscanf(line.c_str(), "Cached: %lu", &cached);
            }
        }
        
        status.memory_total_mb = mem_total / 1024;
        status.memory_available_mb = mem_available / 1024;
        status.memory_used_mb = status.memory_total_mb - status.memory_available_mb;
    }
    
    void collectDisk(SystemStatus& status, const char* path = "/") {
        struct statvfs stat;
        if (statvfs(path, &stat) == 0) {
            uint64_t block_size = stat.f_frsize;
            status.disk_total_gb = (stat.f_blocks * block_size) / (1024ULL * 1024 * 1024);
            status.disk_free_gb = (stat.f_bavail * block_size) / (1024ULL * 1024 * 1024);
            status.disk_used_gb = status.disk_total_gb - status.disk_free_gb;
        }
    }
    
    void collectUptime(SystemStatus& status) {
        struct sysinfo info;
        if (sysinfo(&info) == 0) {
            status.uptime_seconds = info.uptime;
            status.process_count = info.procs;
        }
    }
    
    void collectCpuTemp(SystemStatus& status) {
        // Try hwmon thermal zones
        const char* thermal_paths[] = {
            "/sys/class/thermal/thermal_zone0/temp",
            "/sys/class/hwmon/hwmon0/temp1_input",
            "/sys/devices/platform/coretemp.0/hwmon/hwmon*/temp1_input"
        };
        
        for (const auto& path : thermal_paths) {
            std::ifstream temp_file(path);
            if (temp_file.is_open()) {
                int temp_millidegrees;
                temp_file >> temp_millidegrees;
                status.cpu_temp_celsius = temp_millidegrees / 1000.0;
                return;
            }
        }
        
        // Try lm-sensors fallback
        FILE* pipe = popen("sensors 2>/dev/null | grep -m1 'Core 0' | awk '{print $3}'", "r");
        if (pipe) {
            char buffer[32];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                // Parse "+45.0°C" format
                status.cpu_temp_celsius = std::stod(buffer + 1);  // Skip '+'
            }
            pclose(pipe);
        }
    }
    
    void collectNetwork(SystemStatus& status) {
        // Get primary IP
        FILE* pipe = popen("hostname -I 2>/dev/null | awk '{print $1}'", "r");
        if (pipe) {
            char buffer[64];
            if (fgets(buffer, sizeof(buffer), pipe)) {
                status.primary_ip = std::string(buffer);
                status.primary_ip.erase(status.primary_ip.find_last_not_of(" \n\r") + 1);
            }
            pclose(pipe);
        }
        
        // Get network stats from /proc/net/dev
        std::ifstream netdev("/proc/net/dev");
        if (netdev.is_open()) {
            std::string line;
            while (std::getline(netdev, line)) {
                // Skip loopback, look for eth0 or similar
                if (line.find("eth0:") != std::string::npos || 
                    line.find("ens") != std::string::npos ||
                    line.find("enp") != std::string::npos) {
                    
                    uint64_t rx_bytes, tx_bytes;
                    // Format: iface: rx_bytes rx_packets ... tx_bytes tx_packets ...
                    char iface[32];
                    sscanf(line.c_str(), "%31s %lu %*u %*u %*u %*u %*u %*u %*u %lu",
                           iface, &rx_bytes, &tx_bytes);
                    status.bytes_recv = rx_bytes;
                    status.bytes_sent = tx_bytes;
                    break;
                }
            }
        }
    }
    
    void checkWanReachability(SystemStatus& status) {
        // Quick ping to a reliable target
        int result = system("ping -c 1 -W 2 1.1.1.1 >/dev/null 2>&1");
        status.wan_reachable = (result == 0);
    }
};

// ============================================================================
// MASTER COLLECTOR - Aggregates all subsystems
// ============================================================================

class MasterCollector {
public:
    MasterCollector() = default;
    
    MasterBeaconPayload collect() {
        MasterBeaconPayload payload;
        
        // Collect all subsystems
        payload.time = time_collector_.collect();
        payload.power = power_collector_.collect();
        payload.system = system_collector_.collect();
        
        // Set timestamps
        auto now = std::chrono::system_clock::now();
        payload.timestamp_unix = std::chrono::duration_cast<std::chrono::seconds>(
            now.time_since_epoch()).count();
        
        // High-resolution timestamp if PPS locked
        if (payload.time.pps_locked) {
            auto now_hr = std::chrono::high_resolution_clock::now();
            payload.timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                now_hr.time_since_epoch()).count();
        }
        
        // Determine overall health
        payload.health = determineHealth(payload);
        payload.status_message = generateStatusMessage(payload);
        
        return payload;
    }
    
private:
    TimeCollector time_collector_;
    PowerCollector power_collector_;
    SystemCollector system_collector_;
    
    NodeHealth determineHealth(const MasterBeaconPayload& payload) {
        // Critical conditions
        if (payload.power.on_battery && payload.power.battery_percent < 20) {
            return NodeHealth::CRITICAL;
        }
        if (payload.system.cpu_load_1m > 10.0) {
            return NodeHealth::CRITICAL;
        }
        if (payload.system.disk_free_gb < 1) {
            return NodeHealth::CRITICAL;
        }
        
        // Warning conditions
        if (payload.time.discipline_state == TimeDisciplineState::HOLDOVER) {
            return NodeHealth::WARNING;
        }
        if (payload.time.discipline_state == TimeDisciplineState::UNLOCKED) {
            return NodeHealth::WARNING;
        }
        if (payload.power.on_battery) {
            return NodeHealth::WARNING;
        }
        if (payload.system.cpu_load_1m > 4.0) {
            return NodeHealth::WARNING;
        }
        if (payload.system.memory_available_mb < 1024) {
            return NodeHealth::WARNING;
        }
        if (!payload.system.wan_reachable) {
            return NodeHealth::WARNING;
        }
        
        return NodeHealth::HEALTHY;
    }
    
    std::string generateStatusMessage(const MasterBeaconPayload& payload) {
        std::ostringstream msg;
        
        switch (payload.health) {
            case NodeHealth::HEALTHY:
                msg << "All systems nominal";
                if (payload.time.pps_locked) {
                    msg << " | PPS locked, stratum " << payload.time.stratum;
                }
                break;
                
            case NodeHealth::WARNING:
                if (payload.time.discipline_state == TimeDisciplineState::HOLDOVER) {
                    msg << "Time in holdover mode";
                } else if (payload.power.on_battery) {
                    msg << "Running on UPS battery (" << payload.power.battery_percent << "%)";
                } else if (!payload.system.wan_reachable) {
                    msg << "WAN unreachable";
                } else {
                    msg << "System under load";
                }
                break;
                
            case NodeHealth::CRITICAL:
                if (payload.power.on_battery && payload.power.battery_percent < 20) {
                    msg << "CRITICAL: Low battery (" << payload.power.battery_percent << "%)";
                } else if (payload.system.disk_free_gb < 1) {
                    msg << "CRITICAL: Disk space low";
                } else {
                    msg << "CRITICAL: System overloaded";
                }
                break;
                
            default:
                msg << "Status unknown";
        }
        
        return msg.str();
    }
};

} // namespace collectors
} // namespace litehaus
