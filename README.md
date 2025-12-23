[README.md]


## litehaus
[the actual up'n'running litehaus project]

# LITEHAUS - Ultimate Lighthouse Beacon System


**Ultra-High Performance Network Monitoring with SIMD-Optimized JSON Processing**

Litehaus is a distributed lighthouse beacon system that combines blazing-fast C++ network monitoring with real-time web visualization. Built with SimdJSON for sub-microsecond JSON parsing and featuring a beautiful terminal-aesthetic web interface for monitoring beacons from anywhere in the world.


<p align="center">
  <a href="https://github.com/whisprer/1-gen-docs/releases"> 
    <img src="https://img.shields.io/github/v/release/whisprer/1-gen-docs?color=4CAF50&label=release" alt="Release Version"> 
  </a>
  <a href="https://github.com/whisprer/1-gen-docs/actions"> 
    <img src="https://img.shields.io/github/actions/workflow/status/whisprer/1-gen-docs/lint-and-plot.yml?label=build" alt="Build Status"> 
  </a>
  <a href="https://github.com/whisprer/litehaus/blob/main/LICENSE"> 
    <img src="https://img.shields.io/badge/license-MIT-green.svg" alt="License"> 
    <img src="https://img.shields.io/badge/License-CC0_1.0-lightgrey.svg" alt="License">
  </a>
  <img src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey.svg" alt="Platform">
</p>

![Version](https://img.shields.io/badge/version-3.0.0-brightgreen)
![C++](https://img.shields.io/badge/C++-17-00599C?logo=cplusplus)
![Node.js](https://img.shields.io/badge/Node.js-18+-339933?logo=node.js)
![Commits](https://img.shields.io/github/commit-activity/m/whisprer/litehaus?label=commits) 
![Last Commit](https://img.shields.io/github/last-commit/whisprer/litehaus) 
![Issues](https://img.shields.io/github/issues/whisprer/litehaus) 

<p align="center">
  <img src="litehaus.png" width="850" alt="litehaus Banner">


## Features

### Lighthouse Beacon (C++)
-**SIMD-Optimized JSON Parsing** - Powered by SimdJSON for gigabyte-per-second throughput
-**Sub-Microsecond Performance** - Average parse times under 10µs
- **UDP Beacon Broadcasting** - Broadcasts health metrics every 5 seconds
- **FastPing Integration** - Polls IP detection service for anonymity analysis
- **Multi-threaded Architecture** - Separate threads for listening, beaconing, and status reporting
- **Auto-Optimized SIMD** - Automatically detects and uses CPU-specific SIMD instructions



### FastPing Service (Node.js)
- **Ultra-Fast IP Detection** - Sub-millisecond response times
- **Anonymity Level Detection** - Analyzes proxy chains and forwarding headers
- **JSON API** - Clean REST endpoint for IP information
- **CORS Support** - Ready for cross-origin requests



### Beacon Monitor (Node.js + WebSocket)
- **Real-Time Streaming** - Live beacon data streamed via WebSocket
- **Beautiful Terminal UI** - Retro green-on-black aesthetic
- **UDP Listener** - Receives and parses lighthouse beacons
- **Web Dashboard** - Access from anywhere in the world
- **Auto-Reconnect** - Handles connection drops gracefully

## Architecture
```
┌─────────────────────────────────────────────────────────┐
│  LIGHTHOUSE (Your Machine)                              │
│  - Pings: FastPing API (port 9876)                     	│
│  - Sends UDP beacons to: Beacon Monitor (port 9876)    	│
└─────────────────────────────────────────────────────────┘
                         ↓ UDP every 5s
┌─────────────────────────────────────────────────────────┐
│  SERVER (nyc.litehaus.online / 137.184.105.114)        	│
│                                                         │
│  ├─ Port 9876:  FastPing API (HTTP)                    	│
│  ├─ Port 9876:  UDP Beacon Receiver                    	│
│  ├─ Port 8080:  Beacon Monitor (HTTP)                  	│
│  ├─ Port 8083:  WebSocket Server                       	│
│  └─ Port 80:    Nginx (reverse proxy - optional)       	│
└─────────────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────────────┐
│  YOUR BROWSER                                           │
│  http://137.184.105.114:8080                            │
└─────────────────────────────────────────────────────────┘
```

## Installation

### Prerequisites
**Lighthouse (C++)**
- GCC/Clang with C++17 support
- SimdJSON library
- libcurl
- pthread


**Server (Node.js)**
- Node.js 18+
- npm or yarn
- PM2 (for process management)

### Building the Lighthouse
```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get update
sudo apt-get install build-essential libsimdjson-dev libcurl4-openssl-dev


# Compile the lighthouse
g++ -std=c++17 -O3 -march=native \\
   litehaus-executable.cpp \\
   -o litehaus \\
   -lsimdjson \\
   -lpthread \\
   -lcurl

# Make executable
chmod +x litehaus
```

### Setting Up the Server

**1. FastPing Service**

```bash
cd fastping
npm install
```

Edit `fastping-server.js` to set your desired port (default: 9876)
```bash
# Start with PM2
pm2 start fastping-server.js --name fastping
```

**2. Beacon Monitor**
```bash
cd litehaus-monitor
npm install
```

Edit `beacon-bridge.js` if needed (default ports: 8080 HTTP, 8083 WebSocket, 9876 UDP)
```bash
# Start with PM2
pm2 start beacon-bridge.js --name beacon-monitor
```

**3. Configure the Lighthouse**
Edit `litehaus-executable.cpp` before compiling:
```cpp
std::string fastping\_url{ "http://your-server:9876/ping?format=json" };
std::string beacon\_ip{ "137.184.105.114" }; // Your server IP
int beacon\_port{ 9876 };
```

Then recompile.
**4. Optional: Nginx Reverse Proxy**
```bash
sudo apt install nginx

# Create nginx config
sudo nano /etc/nginx/sites-available/litehaus
```

Add:
```nginx
server {
   listen 80;
   server\_name nyc.litehaus.online;
   location / {
      proxy\_pass http://localhost:8080;
       proxy\_http\_version 1.1;
       proxy\_set\_header Upgrade $http\_upgrade;
       proxy\_set\_header Connection 'upgrade';
       proxy\_set\_header Host $host;
       proxy\_cache\_bypass $http\_upgrade;
   }

   location /ws {
       proxy\_pass http://localhost:8083;
       proxy\_http\_version 1.1;
       proxy\_set\_header Upgrade $http\_upgrade;

       proxy\_set\_header Connection "Upgrade";
   }
}
```

```bash
# Enable site
sudo ln -s /etc/nginx/sites-available/litehaus /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

## Usage

### Running the Lighthouse
```bash
# Start the lighthouse beacon
./litehaus

# Or with PM2 for auto-restart
pm2 start ./litehaus --name lighthouse
```

### Accessing the Monitor
Open your browser to:
- **Direct:** `http://137.184.105.114:8080`
- **With nginx:** `https://nyc.litehaus.online`

You'll see real-time beacon data streaming in a beautiful terminal interface!

### PM2 Management
```bash
# View all processes
pm2 status

# View logs
pm2 logs fastping
pm2 logs beacon-monitor
pm2 logs lighthouse

# Restart a service
pm2 restart fastping

# Save PM2 state
pm2 save

# Auto-start on boot
pm2 startup
```


## Performance Metrics

The system tracks and displays:
- **Beacon Status:** healthy/warning/critical based on signal age
- **Parse Time:** JSON parsing performance (typically 5-20µs)
- **Network Latency:** Round-trip time to FastPing API
- **Throughput:** JSON processing speed (MB/s)
- **Signal Age:** Time since last successful ping
- **Uptime:** System runtime
- **Sequence Number:** Total beacons transmitted


## Customization

### Beacon Intervals
Edit in `litehaus-executable.cpp`:

```cpp
std::chrono::seconds ping_interval{ 10 };    // FastPing polling
std::chrono::seconds beacon_interval{ 5 };   // UDP beacon broadcast
std::chrono::seconds status_interval{ 30 };  // Console status updates
```

### Web UI Theme

Edit `index.html` in the beacon monitor to customize colors, fonts, and styling.


## Troubleshooting
**Lighthouse can't reach FastPing:**
- Check firewall: `sudo ufw allow 9876/tcp`
- Verify FastPing is running: `pm2 status`
- Test manually: `curl http://your-server:9876/ping?format=json`

**No beacons appearing in web monitor:**
- Check UDP port: `sudo ufw allow 9876/udp`
- Verify beacon-monitor is running: `pm2 logs beacon-monitor`
- Test UDP reception: `sudo tcpdump -i any udp port 9876`

**WebSocket won't connect:**
- Check port 8083: `sudo ufw allow 8083/tcp`
- Check browser console (F12) for errors
- Verify WebSocket URL matches your setup

**521 Cloudflare Error:**
- Disable Cloudflare proxy (orange cloud → gray)
- Or set up SSL with: `sudo certbot --nginx -d nyc.litehaus.online`


## Configuration Files
```
litehaus/
├── litehaus-executable.cpp    # Main lighthouse beacon (C++)
├── fastping/
│   ├── fastping-server.js     # IP detection API
│   └── package.json
├── litehaus-monitor/
│   ├── beacon-bridge.js       # WebSocket + UDP listener
│   ├── index.html             # Web UI
│   └── package.json
└── README.md
```

## Technical Highlights
- **SIMD Optimization:** Uses CPU-specific SIMD instructions (SSE4.2, AVX2, NEON) for maximum performance
- **Zero-Copy Parsing:** SimdJSON achieves minimal memory overhead
- **Async I/O:** Non-blocking network operations throughout
- **Thread-Safe:** Lock-free atomic operations where possible
- **Graceful Degradation:** Continues operating even if FastPing is unreachable


## License
Hybrid MIT & CC0 License - See LICENSE file for details


## Acknowledgments
- **SimdJSON** - For the blazing-fast JSON parser
- **Express.js** - Web framework for Node.js services
- **ws** - WebSocket library for real-time communication
- **PM2** - Process manager for Node.js
- **Claude Sonnet4.5/Claude Opus4.5** - cos he's a star


## Contributing
Contributions welcome! Please feel free to submit pull requests or open issues.


---


**Built with ⚡ by whisprer**

*"They Came From Beyond... And They're Bringing The Beacons!"*
