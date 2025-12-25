/**
 * 🏰 LITEHAUS HOME MASTER NODE - STREAM SERVICE
 * ==============================================
 * WebSocket & UDP broadcast service for real-time telemetry.
 * Replaces the Node.js beacon-bridge.js with native C++.
 * 
 * Features:
 * - UDP listener (receives from litehaus-core)
 * - WebSocket server (broadcasts to web clients)
 * - HTTP server (serves dashboard & API)
 * - SSE endpoint for lightweight streaming
 * 
 * Copyright (c) 2025 Litehaus Project
 * License: MIT
 */

#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>
#include <cstring>
#include <csignal>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <queue>
#include <condition_variable>

// Networking
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <poll.h>
#include <fcntl.h>

// Local includes
#include "litehaus_types.hpp"
#include "litehaus_json.hpp"

namespace litehaus {
namespace stream {

// ============================================================================
// GLOBAL STATE
// ============================================================================

static std::atomic<bool> g_running{true};
static std::atomic<uint64_t> g_messages_received{0};
static std::atomic<uint64_t> g_messages_broadcast{0};

void signal_handler(int signum) {
    std::cout << "\n🛑 Signal " << signum << " received. Shutting down...\n";
    g_running.store(false);
}

// ============================================================================
// THREAD-SAFE MESSAGE QUEUE
// ============================================================================

template<typename T>
class MessageQueue {
public:
    void push(T item) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(item));
        }
        cv_.notify_one();
    }
    
    bool pop(T& item, std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lock(mutex_);
        if (!cv_.wait_for(lock, timeout, [this] { return !queue_.empty(); })) {
            return false;
        }
        item = std::move(queue_.front());
        queue_.pop();
        return true;
    }
    
    size_t size() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return queue_.size();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<T> queue_;
};

// ============================================================================
// WEBSOCKET FRAME HELPER (RFC 6455 minimal implementation)
// ============================================================================

class WebSocketFrame {
public:
    static std::vector<uint8_t> encode(const std::string& message) {
        std::vector<uint8_t> frame;
        
        // Text frame, FIN bit set
        frame.push_back(0x81);
        
        // Payload length
        if (message.size() < 126) {
            frame.push_back(static_cast<uint8_t>(message.size()));
        } else if (message.size() < 65536) {
            frame.push_back(126);
            frame.push_back(static_cast<uint8_t>((message.size() >> 8) & 0xFF));
            frame.push_back(static_cast<uint8_t>(message.size() & 0xFF));
        } else {
            frame.push_back(127);
            for (int i = 7; i >= 0; --i) {
                frame.push_back(static_cast<uint8_t>((message.size() >> (i * 8)) & 0xFF));
            }
        }
        
        // Payload
        frame.insert(frame.end(), message.begin(), message.end());
        
        return frame;
    }
    
    static bool decode(const std::vector<uint8_t>& data, std::string& message, bool& is_close) {
        if (data.size() < 2) return false;
        
        uint8_t opcode = data[0] & 0x0F;
        is_close = (opcode == 0x08);
        
        if (opcode == 0x09) {
            // Ping - we should respond with pong
            return true;
        }
        
        bool masked = (data[1] & 0x80) != 0;
        uint64_t payload_len = data[1] & 0x7F;
        size_t offset = 2;
        
        if (payload_len == 126) {
            if (data.size() < 4) return false;
            payload_len = (static_cast<uint64_t>(data[2]) << 8) | data[3];
            offset = 4;
        } else if (payload_len == 127) {
            if (data.size() < 10) return false;
            payload_len = 0;
            for (int i = 0; i < 8; ++i) {
                payload_len = (payload_len << 8) | data[2 + i];
            }
            offset = 10;
        }
        
        std::array<uint8_t, 4> mask = {0, 0, 0, 0};
        if (masked) {
            if (data.size() < offset + 4) return false;
            for (int i = 0; i < 4; ++i) {
                mask[i] = data[offset + i];
            }
            offset += 4;
        }
        
        if (data.size() < offset + payload_len) return false;
        
        message.resize(payload_len);
        for (uint64_t i = 0; i < payload_len; ++i) {
            message[i] = data[offset + i] ^ mask[i % 4];
        }
        
        return true;
    }
    
    static std::string generateAcceptKey(const std::string& key) {
        // SHA-1 of key + magic GUID, then base64
        // For simplicity, we'll use a minimal implementation
        std::string magic = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        
        // Simple SHA-1 implementation (in production, use OpenSSL)
        // For now, return a placeholder that works for testing
        return sha1_base64(magic);
    }

private:
    static std::string sha1_base64(const std::string& input) {
        // Minimal SHA-1 for WebSocket handshake
        // In production, link against OpenSSL and use proper crypto
        
        // SHA-1 constants
        uint32_t h0 = 0x67452301;
        uint32_t h1 = 0xEFCDAB89;
        uint32_t h2 = 0x98BADCFE;
        uint32_t h3 = 0x10325476;
        uint32_t h4 = 0xC3D2E1F0;
        
        // Preprocessing
        std::vector<uint8_t> msg(input.begin(), input.end());
        uint64_t original_len = msg.size() * 8;
        
        msg.push_back(0x80);
        while ((msg.size() % 64) != 56) {
            msg.push_back(0x00);
        }
        
        for (int i = 7; i >= 0; --i) {
            msg.push_back(static_cast<uint8_t>((original_len >> (i * 8)) & 0xFF));
        }
        
        // Process each 512-bit chunk
        for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
            uint32_t w[80];
            
            for (int i = 0; i < 16; ++i) {
                w[i] = (static_cast<uint32_t>(msg[chunk + i * 4]) << 24) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 1]) << 16) |
                       (static_cast<uint32_t>(msg[chunk + i * 4 + 2]) << 8) |
                       static_cast<uint32_t>(msg[chunk + i * 4 + 3]);
            }
            
            for (int i = 16; i < 80; ++i) {
                uint32_t val = w[i-3] ^ w[i-8] ^ w[i-14] ^ w[i-16];
                w[i] = (val << 1) | (val >> 31);
            }
            
            uint32_t a = h0, b = h1, c = h2, d = h3, e = h4;
            
            for (int i = 0; i < 80; ++i) {
                uint32_t f, k;
                if (i < 20) {
                    f = (b & c) | ((~b) & d);
                    k = 0x5A827999;
                } else if (i < 40) {
                    f = b ^ c ^ d;
                    k = 0x6ED9EBA1;
                } else if (i < 60) {
                    f = (b & c) | (b & d) | (c & d);
                    k = 0x8F1BBCDC;
                } else {
                    f = b ^ c ^ d;
                    k = 0xCA62C1D6;
                }
                
                uint32_t temp = ((a << 5) | (a >> 27)) + f + e + k + w[i];
                e = d;
                d = c;
                c = (b << 30) | (b >> 2);
                b = a;
                a = temp;
            }
            
            h0 += a;
            h1 += b;
            h2 += c;
            h3 += d;
            h4 += e;
        }
        
        // Create hash bytes
        uint8_t hash[20];
        for (int i = 0; i < 4; ++i) {
            hash[i] = (h0 >> (24 - i * 8)) & 0xFF;
            hash[4 + i] = (h1 >> (24 - i * 8)) & 0xFF;
            hash[8 + i] = (h2 >> (24 - i * 8)) & 0xFF;
            hash[12 + i] = (h3 >> (24 - i * 8)) & 0xFF;
            hash[16 + i] = (h4 >> (24 - i * 8)) & 0xFF;
        }
        
        // Base64 encode
        static const char b64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        std::string result;
        
        for (int i = 0; i < 20; i += 3) {
            uint32_t n = (static_cast<uint32_t>(hash[i]) << 16);
            if (i + 1 < 20) n |= (static_cast<uint32_t>(hash[i + 1]) << 8);
            if (i + 2 < 20) n |= static_cast<uint32_t>(hash[i + 2]);
            
            result += b64[(n >> 18) & 0x3F];
            result += b64[(n >> 12) & 0x3F];
            result += (i + 1 < 20) ? b64[(n >> 6) & 0x3F] : '=';
            result += (i + 2 < 20) ? b64[n & 0x3F] : '=';
        }
        
        return result;
    }
};

// ============================================================================
// WEBSOCKET CLIENT CONNECTION
// ============================================================================

struct WsClient {
    int fd;
    std::string remote_addr;
    std::chrono::steady_clock::time_point connected_at;
    bool handshake_done{false};
    std::vector<uint8_t> recv_buffer;
};

// ============================================================================
// STREAM SERVICE
// ============================================================================

class StreamService {
public:
    explicit StreamService(const StreamConfig& cfg) : config_(cfg) {}
    
    void run() {
        displayBanner();
        
        // Start UDP listener thread
        std::thread udp_thread(&StreamService::udpListenerLoop, this);
        
        // Start WebSocket server thread
        std::thread ws_thread(&StreamService::websocketServerLoop, this);
        
        // Start HTTP server thread
        std::thread http_thread(&StreamService::httpServerLoop, this);
        
        // Start broadcaster thread
        std::thread broadcast_thread(&StreamService::broadcastLoop, this);
        
        std::cout << "🚀 Litehaus Stream Service Started\n";
        std::cout << "   UDP Listen: " << config_.udp_listen_port << "\n";
        std::cout << "   WebSocket:  " << config_.websocket_port << "\n";
        std::cout << "   HTTP:       " << config_.http_port << "\n\n";
        
        // Wait for shutdown
        while (g_running.load()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        
        // Cleanup
        if (udp_thread.joinable()) udp_thread.join();
        if (ws_thread.joinable()) ws_thread.join();
        if (http_thread.joinable()) http_thread.join();
        if (broadcast_thread.joinable()) broadcast_thread.join();
        
        std::cout << "🏰 Stream service stopped\n";
        std::cout << "   Messages received: " << g_messages_received.load() << "\n";
        std::cout << "   Messages broadcast: " << g_messages_broadcast.load() << "\n";
    }

private:
    StreamConfig config_;
    MessageQueue<std::string> message_queue_;
    
    std::mutex clients_mutex_;
    std::vector<WsClient> ws_clients_;
    
    std::string last_beacon_;
    std::mutex beacon_mutex_;
    
    void displayBanner() {
        std::cout << R"(
🏰 ═══════════════════════════════════════════════════════════════════ 🏰
   LITEHAUS STREAM SERVICE v1.0
   Real-time telemetry broadcast
📡 UDP → WebSocket → World 📡
🏰 ═══════════════════════════════════════════════════════════════════ 🏰

)" << std::endl;
    }
    
    /**
     * UDP Listener - Receives beacons from litehaus-core
     */
    void udpListenerLoop() {
        int sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) {
            std::cerr << "🚨 Failed to create UDP socket\n";
            return;
        }
        
        // Allow address reuse
        int opt = 1;
        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(config_.udp_listen_port);
        
        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "🚨 Failed to bind UDP socket to port " << config_.udp_listen_port << "\n";
            close(sock);
            return;
        }
        
        std::cout << "🎧 UDP Listener bound to port " << config_.udp_listen_port << "\n";
        
        // Set non-blocking for clean shutdown
        fcntl(sock, F_SETFL, fcntl(sock, F_GETFL) | O_NONBLOCK);
        
        char buffer[8192];
        sockaddr_in sender{};
        socklen_t sender_len = sizeof(sender);
        
        while (g_running.load()) {
            struct pollfd pfd = {sock, POLLIN, 0};
            int ret = poll(&pfd, 1, 100);  // 100ms timeout
            
            if (ret > 0 && (pfd.revents & POLLIN)) {
                ssize_t len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                                       reinterpret_cast<sockaddr*>(&sender), &sender_len);
                
                if (len > 0) {
                    buffer[len] = '\0';
                    std::string message(buffer, len);
                    
                    // Store as last beacon
                    {
                        std::lock_guard<std::mutex> lock(beacon_mutex_);
                        last_beacon_ = message;
                    }
                    
                    // Queue for broadcast
                    message_queue_.push(message);
                    g_messages_received.fetch_add(1);
                    
                    char sender_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &sender.sin_addr, sender_ip, sizeof(sender_ip));
                    
                    // Log occasionally
                    static uint64_t log_counter = 0;
                    if (++log_counter % 60 == 1) {
                        std::cout << "📡 Received beacon from " << sender_ip 
                                  << " (" << len << " bytes)\n";
                    }
                }
            }
        }
        
        close(sock);
    }
    
    /**
     * WebSocket Server - Accepts and manages WebSocket connections
     */
    void websocketServerLoop() {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "🚨 Failed to create WebSocket server socket\n";
            return;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(config_.websocket_port);
        
        if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "🚨 Failed to bind WebSocket server to port " << config_.websocket_port << "\n";
            close(server_fd);
            return;
        }
        
        listen(server_fd, 10);
        fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL) | O_NONBLOCK);
        
        std::cout << "🔌 WebSocket server listening on port " << config_.websocket_port << "\n";
        
        while (g_running.load()) {
            struct pollfd pfd = {server_fd, POLLIN, 0};
            int ret = poll(&pfd, 1, 100);
            
            if (ret > 0 && (pfd.revents & POLLIN)) {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                
                if (client_fd >= 0) {
                    char client_ip[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                    
                    // Handle WebSocket handshake in separate thread
                    std::thread([this, client_fd, ip = std::string(client_ip)]() {
                        handleWebSocketClient(client_fd, ip);
                    }).detach();
                }
            }
        }
        
        close(server_fd);
    }
    
    void handleWebSocketClient(int fd, const std::string& remote_addr) {
        // Read HTTP upgrade request
        char buffer[4096];
        ssize_t len = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            close(fd);
            return;
        }
        buffer[len] = '\0';
        
        std::string request(buffer);
        
        // Extract Sec-WebSocket-Key
        std::string ws_key;
        size_t key_pos = request.find("Sec-WebSocket-Key:");
        if (key_pos != std::string::npos) {
            size_t start = key_pos + 18;
            while (start < request.size() && request[start] == ' ') ++start;
            size_t end = request.find("\r\n", start);
            if (end != std::string::npos) {
                ws_key = request.substr(start, end - start);
            }
        }
        
        if (ws_key.empty()) {
            close(fd);
            return;
        }
        
        // Send upgrade response
        std::string accept_key = WebSocketFrame::generateAcceptKey(ws_key);
        std::ostringstream response;
        response << "HTTP/1.1 101 Switching Protocols\r\n"
                 << "Upgrade: websocket\r\n"
                 << "Connection: Upgrade\r\n"
                 << "Sec-WebSocket-Accept: " << accept_key << "\r\n"
                 << "\r\n";
        
        std::string resp_str = response.str();
        send(fd, resp_str.c_str(), resp_str.size(), 0);
        
        // Add to clients list
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            ws_clients_.push_back({
                fd,
                remote_addr,
                std::chrono::steady_clock::now(),
                true,
                {}
            });
        }
        
        std::cout << "🔗 WebSocket client connected: " << remote_addr << "\n";
        
        // Send welcome message
        std::string welcome = "🏰 Connected to Litehaus Stream Service\n";
        auto frame = WebSocketFrame::encode(welcome);
        send(fd, reinterpret_cast<char*>(frame.data()), frame.size(), 0);
        
        // Send last beacon if available
        {
            std::lock_guard<std::mutex> lock(beacon_mutex_);
            if (!last_beacon_.empty()) {
                auto beacon_frame = WebSocketFrame::encode(last_beacon_);
                send(fd, reinterpret_cast<char*>(beacon_frame.data()), beacon_frame.size(), 0);
            }
        }
        
        // Keep connection alive, handle pings
        fcntl(fd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
        
        while (g_running.load()) {
            struct pollfd pfd = {fd, POLLIN, 0};
            int ret = poll(&pfd, 1, 1000);
            
            if (ret > 0 && (pfd.revents & POLLIN)) {
                char recv_buf[1024];
                ssize_t n = recv(fd, recv_buf, sizeof(recv_buf), 0);
                
                if (n <= 0) {
                    // Connection closed
                    break;
                }
                
                std::vector<uint8_t> data(recv_buf, recv_buf + n);
                std::string message;
                bool is_close = false;
                
                if (WebSocketFrame::decode(data, message, is_close)) {
                    if (is_close) break;
                    
                    // Handle ping with pong
                    if ((data[0] & 0x0F) == 0x09) {
                        std::vector<uint8_t> pong = data;
                        pong[0] = (pong[0] & 0xF0) | 0x0A;  // Change opcode to pong
                        send(fd, reinterpret_cast<char*>(pong.data()), pong.size(), 0);
                    }
                }
            }
            
            if (pfd.revents & (POLLERR | POLLHUP)) {
                break;
            }
        }
        
        // Remove from clients list
        {
            std::lock_guard<std::mutex> lock(clients_mutex_);
            ws_clients_.erase(
                std::remove_if(ws_clients_.begin(), ws_clients_.end(),
                    [fd](const WsClient& c) { return c.fd == fd; }),
                ws_clients_.end()
            );
        }
        
        std::cout << "🔌 WebSocket client disconnected: " << remote_addr << "\n";
        close(fd);
    }
    
    /**
     * HTTP Server - Serves dashboard and API
     */
    void httpServerLoop() {
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "🚨 Failed to create HTTP server socket\n";
            return;
        }
        
        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(config_.http_port);
        
        if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
            std::cerr << "🚨 Failed to bind HTTP server to port " << config_.http_port << "\n";
            close(server_fd);
            return;
        }
        
        listen(server_fd, 10);
        fcntl(server_fd, F_SETFL, fcntl(server_fd, F_GETFL) | O_NONBLOCK);
        
        std::cout << "🌐 HTTP server listening on port " << config_.http_port << "\n";
        
        while (g_running.load()) {
            struct pollfd pfd = {server_fd, POLLIN, 0};
            int ret = poll(&pfd, 1, 100);
            
            if (ret > 0 && (pfd.revents & POLLIN)) {
                sockaddr_in client_addr{};
                socklen_t client_len = sizeof(client_addr);
                int client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
                
                if (client_fd >= 0) {
                    handleHttpRequest(client_fd);
                }
            }
        }
        
        close(server_fd);
    }
    
    void handleHttpRequest(int fd) {
        char buffer[4096];
        ssize_t len = recv(fd, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            close(fd);
            return;
        }
        buffer[len] = '\0';
        
        std::string request(buffer);
        std::string path = "/";
        
        // Extract path
        size_t get_pos = request.find("GET ");
        if (get_pos != std::string::npos) {
            size_t start = get_pos + 4;
            size_t end = request.find(" ", start);
            if (end != std::string::npos) {
                path = request.substr(start, end - start);
            }
        }
        
        std::string response;
        std::string content_type = "text/html";
        
        if (path == "/api/status" || path == "/status") {
            content_type = "application/json";
            std::lock_guard<std::mutex> lock(beacon_mutex_);
            if (!last_beacon_.empty()) {
                response = last_beacon_;
            } else {
                response = "{\"error\":\"no beacon data\"}";
            }
        }
        else if (path == "/api/stats") {
            content_type = "application/json";
            std::ostringstream json;
            json << "{\"messages_received\":" << g_messages_received.load()
                 << ",\"messages_broadcast\":" << g_messages_broadcast.load()
                 << ",\"clients_connected\":" << ws_clients_.size()
                 << "}";
            response = json.str();
        }
        else {
            // Serve dashboard
            response = generateDashboardHtml();
        }
        
        std::ostringstream http_response;
        http_response << "HTTP/1.1 200 OK\r\n"
                      << "Content-Type: " << content_type << "\r\n"
                      << "Content-Length: " << response.size() << "\r\n"
                      << "Access-Control-Allow-Origin: *\r\n"
                      << "Connection: close\r\n"
                      << "\r\n"
                      << response;
        
        std::string http_str = http_response.str();
        send(fd, http_str.c_str(), http_str.size(), 0);
        close(fd);
    }
    
    std::string generateDashboardHtml() {
        return R"(<!DOCTYPE html>
<html>
<head>
    <title>🏰 Litehaus Home Master Node</title>
    <meta charset="utf-8">
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { 
            background: linear-gradient(135deg, #0a0a1a 0%, #1a1a3a 100%);
            color: #e0e0e0; 
            font-family: 'Fira Code', monospace;
            min-height: 100vh;
            padding: 20px;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { color: #00ff88; margin-bottom: 20px; text-shadow: 0 0 10px #00ff8844; }
        .grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(300px, 1fr)); gap: 20px; }
        .card {
            background: rgba(30, 30, 50, 0.8);
            border: 1px solid #333;
            border-radius: 10px;
            padding: 20px;
            backdrop-filter: blur(10px);
        }
        .card h2 { color: #00aaff; margin-bottom: 15px; font-size: 1.1em; }
        .status { font-size: 2em; }
        .healthy { color: #00ff88; }
        .warning { color: #ffaa00; }
        .critical { color: #ff4444; }
        .metric { display: flex; justify-content: space-between; padding: 5px 0; border-bottom: 1px solid #333; }
        .metric:last-child { border-bottom: none; }
        .label { color: #888; }
        .value { color: #fff; font-weight: bold; }
        #log { 
            background: #0a0a1a; 
            border: 1px solid #333;
            border-radius: 5px;
            padding: 15px;
            height: 300px;
            overflow-y: auto;
            font-size: 0.85em;
            margin-top: 20px;
        }
        .log-entry { padding: 3px 0; border-bottom: 1px solid #222; }
        .timestamp { color: #666; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🏰 LITEHAUS HOME MASTER NODE</h1>
        <div class="grid">
            <div class="card">
                <h2>⏱️ Time Discipline</h2>
                <div class="status" id="time-status">--</div>
                <div class="metric"><span class="label">Source</span><span class="value" id="time-source">--</span></div>
                <div class="metric"><span class="label">Stratum</span><span class="value" id="time-stratum">--</span></div>
                <div class="metric"><span class="label">Error</span><span class="value" id="time-error">--</span></div>
            </div>
            <div class="card">
                <h2>⚡ Power</h2>
                <div class="status" id="power-status">--</div>
                <div class="metric"><span class="label">Source</span><span class="value" id="power-source">--</span></div>
                <div class="metric"><span class="label">Battery</span><span class="value" id="power-battery">--</span></div>
                <div class="metric"><span class="label">Runtime</span><span class="value" id="power-runtime">--</span></div>
            </div>
            <div class="card">
                <h2>💻 System</h2>
                <div class="metric"><span class="label">Load</span><span class="value" id="sys-load">--</span></div>
                <div class="metric"><span class="label">Memory</span><span class="value" id="sys-memory">--</span></div>
                <div class="metric"><span class="label">Disk</span><span class="value" id="sys-disk">--</span></div>
                <div class="metric"><span class="label">WAN</span><span class="value" id="sys-wan">--</span></div>
            </div>
            <div class="card">
                <h2>📻 Beacon</h2>
                <div class="status" id="health">--</div>
                <div class="metric"><span class="label">Sequence</span><span class="value" id="beacon-seq">--</span></div>
                <div class="metric"><span class="label">Ping</span><span class="value" id="ping-latency">--</span></div>
                <div class="metric"><span class="label">External IP</span><span class="value" id="external-ip">--</span></div>
            </div>
        </div>
        <div id="log"></div>
    </div>
    <script>
        const wsPort = )" + std::to_string(config_.websocket_port) + R"(;
        const log = document.getElementById('log');
        
        function connect() {
            const ws = new WebSocket('ws://' + location.hostname + ':' + wsPort);
            
            ws.onmessage = (e) => {
                try {
                    const data = JSON.parse(e.data);
                    updateDashboard(data);
                    addLog(e.data);
                } catch(err) {
                    addLog(e.data);
                }
            };
            
            ws.onclose = () => setTimeout(connect, 3000);
            ws.onerror = () => ws.close();
        }
        
        function updateDashboard(d) {
            // Time
            const pps = d.time?.pps_locked;
            document.getElementById('time-status').textContent = pps ? '🔒 LOCKED' : '🔓 UNLOCKED';
            document.getElementById('time-status').className = 'status ' + (pps ? 'healthy' : 'warning');
            document.getElementById('time-source').textContent = d.time?.source || '--';
            document.getElementById('time-stratum').textContent = d.time?.stratum || '--';
            document.getElementById('time-error').textContent = (d.time?.estimated_error_ns || 0).toFixed(0) + ' ns';
            
            // Power
            const onBatt = d.power?.on_battery;
            document.getElementById('power-status').textContent = onBatt ? '🔋 BATTERY' : '🔌 GRID';
            document.getElementById('power-status').className = 'status ' + (onBatt ? 'warning' : 'healthy');
            document.getElementById('power-source').textContent = d.power?.source || '--';
            document.getElementById('power-battery').textContent = (d.power?.battery_percent || 0) + '%';
            document.getElementById('power-runtime').textContent = (d.power?.estimated_runtime_minutes || 0) + ' min';
            
            // System
            document.getElementById('sys-load').textContent = d.system?.cpu_load_1m?.toFixed(2) || '--';
            document.getElementById('sys-memory').textContent = (d.system?.memory_used_mb || 0) + ' / ' + (d.system?.memory_total_mb || 0) + ' MB';
            document.getElementById('sys-disk').textContent = (d.system?.disk_free_gb || 0) + ' GB free';
            document.getElementById('sys-wan').textContent = d.system?.wan_reachable ? '✅' : '❌';
            
            // Beacon
            const health = d.health;
            document.getElementById('health').textContent = health === 'healthy' ? '✅ HEALTHY' : health === 'warning' ? '⚠️ WARNING' : '❌ CRITICAL';
            document.getElementById('health').className = 'status ' + health;
            document.getElementById('beacon-seq').textContent = d.beacon_sequence || '--';
            document.getElementById('ping-latency').textContent = (d.ping_latency_ms || 0).toFixed(1) + ' ms';
            document.getElementById('external-ip').textContent = d.connecting_ip || '--';
        }
        
        function addLog(msg) {
            const ts = new Date().toISOString().substr(11, 8);
            const entry = document.createElement('div');
            entry.className = 'log-entry';
            entry.innerHTML = '<span class="timestamp">' + ts + '</span> ' + msg.substring(0, 100) + '...';
            log.insertBefore(entry, log.firstChild);
            if (log.children.length > 50) log.removeChild(log.lastChild);
        }
        
        connect();
    </script>
</body>
</html>)";
    }
    
    /**
     * Broadcast Loop - Sends queued messages to all WebSocket clients
     */
    void broadcastLoop() {
        while (g_running.load()) {
            std::string message;
            if (message_queue_.pop(message, std::chrono::milliseconds(100))) {
                auto frame = WebSocketFrame::encode(message);
                
                std::lock_guard<std::mutex> lock(clients_mutex_);
                for (auto& client : ws_clients_) {
                    if (client.handshake_done) {
                        ssize_t sent = send(client.fd, 
                                           reinterpret_cast<char*>(frame.data()), 
                                           frame.size(), 
                                           MSG_NOSIGNAL);
                        if (sent > 0) {
                            g_messages_broadcast.fetch_add(1);
                        }
                    }
                }
            }
        }
    }
};

} // namespace stream
} // namespace litehaus

// ============================================================================
// MAIN ENTRY POINT
// ============================================================================

int main(int argc, char* argv[]) {
    std::signal(SIGINT, litehaus::stream::signal_handler);
    std::signal(SIGTERM, litehaus::stream::signal_handler);
    std::signal(SIGPIPE, SIG_IGN);  // Ignore broken pipe
    
    litehaus::StreamConfig config;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if ((arg == "-u" || arg == "--udp-port") && i + 1 < argc) {
            config.udp_listen_port = std::stoi(argv[++i]);
        } else if ((arg == "-w" || arg == "--ws-port") && i + 1 < argc) {
            config.websocket_port = std::stoi(argv[++i]);
        } else if ((arg == "-p" || arg == "--http-port") && i + 1 < argc) {
            config.http_port = std::stoi(argv[++i]);
        } else if (arg == "-h" || arg == "--help") {
            std::cout << "Usage: litehaus-stream [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  -u, --udp-port <port>   UDP listen port (default: 9876)\n";
            std::cout << "  -w, --ws-port <port>    WebSocket port (default: 8083)\n";
            std::cout << "  -p, --http-port <port>  HTTP port (default: 8080)\n";
            std::cout << "  -h, --help              Show this help\n";
            return 0;
        }
    }
    
    try {
        litehaus::stream::StreamService service(config);
        service.run();
    } catch (const std::exception& e) {
        std::cerr << "🚨 Fatal error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}
