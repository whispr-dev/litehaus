const WebSocket = require('ws');
const dgram = require('dgram');
const express = require('express');

// 🏰 LITEHAUS BEACON WEB MONITOR 🏰
console.log('🚀 Starting Litehaus Beacon Web Monitor...');

const HTTP_PORT = 8080;
const WS_PORT = 8083;
const UDP_PORT = 9876;

// Create Express app for serving static files
const app = express();
app.use(express.static(__dirname));

// Start HTTP server
const httpServer = app.listen(HTTP_PORT, '0.0.0.0', () => {
    console.log(`📡 HTTP Server running on port ${HTTP_PORT}`);
    console.log(`🌐 Open http://138.68.142.181:${HTTP_PORT} in your browser`);
});

// Create WebSocket server
const wss = new WebSocket.Server({ port: WS_PORT });
console.log(`🔌 WebSocket server running on port ${WS_PORT}`);

// Track connected web clients
let webClients = new Set();

wss.on('connection', (ws) => {
    console.log('🔗 Web client connected');
    webClients.add(ws);
    
    ws.send('🏰 Connected to Litehaus Beacon Monitor\n');
    ws.send('🎧 Listening for lighthouse beacons on UDP port 9876...\n\n');
    
    ws.on('close', () => {
        console.log('🔌 Web client disconnected');
        webClients.delete(ws);
    });
    
    ws.on('error', (error) => {
        console.error('🚨 WebSocket error:', error);
        webClients.delete(ws);
    });
});

// Create UDP server to receive beacons
const udpServer = dgram.createSocket('udp4');

udpServer.on('listening', () => {
    const address = udpServer.address();
    console.log(`🎧 UDP Listener bound to ${address.address}:${address.port}`);
    console.log('✅ Ready to receive lighthouse beacons!\n');
});

udpServer.on('message', (msg, rinfo) => {
    const timestamp = new Date().toISOString();
    const receivedData = msg.toString();
    
    console.log(`📡 [${timestamp}] Beacon from ${rinfo.address}:${rinfo.port}`);
    console.log(`   Size: ${msg.length} bytes`);
    
    // Parse the JSON beacon
    try {
        const beacon = JSON.parse(receivedData);
        
        // Format output for web clients
        const output = `
┌─────────────────────────────────────────┐
│ 🚨 LIGHTHOUSE BEACON RECEIVED │
├─────────────────────────────────────────┤
│ Time: ${timestamp}
│ From: ${rinfo.address}:${rinfo.port}
│ Size: ${msg.length} bytes
├─────────────────────────────────────────┤
│ Beacon ID: ${beacon.beacon_id || 'N/A'}
│ Status: ${beacon.status || 'N/A'}
│ Ping Status: ${beacon.last_ping_status || 'N/A'}
│ Ping Latency: ${beacon.ping_latency_ms || 0}ms
│ Signal Age: ${beacon.signal_age_seconds || 0}s
│ Parse Time: ${beacon.json_parse_time_microseconds || 0}µs
│ Throughput: ${beacon.average_throughput_mbps || 0} MB/s
│ Uptime: ${beacon.system_uptime_hours || 0}h
│ Sequence #${beacon.beacon_sequence_number || 0}
└─────────────────────────────────────────┘

`;
        
        // Broadcast to all connected web clients
        webClients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(output);
            }
        });
        
        console.log(`   ✅ Broadcasted to ${webClients.size} web client(s)`);
        
    } catch (err) {
        console.error('🚨 Failed to parse beacon JSON:', err.message);
        
        // Send raw data to web clients anyway
        const errorOutput = `
🚨 Received beacon but failed to parse JSON:
${receivedData}
Error: ${err.message}

`;
        webClients.forEach(client => {
            if (client.readyState === WebSocket.OPEN) {
                client.send(errorOutput);
            }
        });
    }
});

udpServer.on('error', (err) => {
    console.error('🚨 UDP Server error:', err);
    udpServer.close();
});

// Bind UDP server
udpServer.bind(UDP_PORT, '0.0.0.0');

// Graceful shutdown
process.on('SIGINT', () => {
    console.log('\n🛑 Shutting down Litehaus Beacon Monitor...');
    
    webClients.forEach(ws => {
        if (ws.readyState === WebSocket.OPEN) {
            ws.send('\n🛑 Server shutting down...\n');
            ws.close();
        }
    });
    
    udpServer.close();
    wss.close();
    httpServer.close(() => {
        console.log('✅ Shutdown complete');
        process.exit(0);
    });
});

console.log('\n🏰 Litehaus Beacon Monitor Ready! 🏰');
console.log(`📋 Open http://138.68.142.181:${HTTP_PORT} to watch beacons live!`);
console.log('🚀 Press Ctrl+C to stop\n');
