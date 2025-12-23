CHANGELOG.mdmarkdown# Litehaus Changelog

## v3.0.0 - Global Network Deployment (2025-12-23) ✅
**MILESTONE: Four-continent lighthouse network operational!**

**Achievements:**
- ✅ NYC lighthouse deployed (137.184.105.114)
- ✅ London lighthouse deployed (138.68.142.181)
- ✅ Sydney lighthouse deployed (134.199.170.197)
- ✅ Singapore lighthouse deployed (68.183.227.135)
- ✅ All four lighthouses broadcasting healthy beacons
- ✅ SSL/TLS encryption on all endpoints
- ✅ Real-time WebSocket streaming
- ✅ FastPing API service operational
- ✅ Sub-microsecond JSON parsing (7-20µs average)
- ✅ PM2 process management with auto-restart
- ✅ Nginx and Caddy reverse proxy configurations
- ✅ UDP beacon broadcasting every 5 seconds

**Global Endpoints:**
- `https://nyc.litehaus.online` - New York City 🗽
- `https://lon.litehaus.online` - London 🇬🇧
- `https://syd.litehaus.online` - Sydney 🦘
- `https://sgp.litehaus.online` - Singapore 🇸🇬

**Technical Details:**
- Beacon frequency: 5 seconds
- FastPing polling: 10 seconds
- Parse performance: 7-20µs (SimdJSON SIMD-optimized)
- Throughput: 9-27 MB/s
- Beacon size: 443-449 bytes per packet
- Network latency: 0.02-0.5ms to FastPing API

**Infrastructure:**
- C++ lighthouse executable (SimdJSON, libcurl, pthread)
- Node.js beacon monitor (Express, ws WebSocket)
- Node.js FastPing API service
- Nginx reverse proxy (NYC, LON, SYD)
- Caddy reverse proxy (SGP - Docker environment)
- PM2 process management on all servers

---

## v2.0.0 - Multi-Region Alpha (2025-12-22)
**MILESTONE: First multi-region deployment!**

**Achievements:**
- ✅ NYC primary lighthouse operational
- ✅ London secondary lighthouse deployed
- ✅ Sydney tertiary lighthouse deployed
- ✅ Network topology established
- ✅ DNS configuration (litehaus.online domain)
- ✅ Cloudflare DNS management
- ✅ Let's Encrypt SSL automation

**Bug Fixes:**
- Fixed nginx default site conflicts
- Resolved Cloudflare proxy interference
- Fixed port conflicts (8080 vs 8081)
- Corrected Docker network gateway addressing
- Fixed DNS propagation issues

---

## v1.0.0 - Single Node Beta (2025-12-21)
**MILESTONE: First operational lighthouse!**

**Achievements:**
- ✅ C++ lighthouse with SimdJSON integration
- ✅ UDP beacon broadcasting
- ✅ FastPing API for IP detection
- ✅ WebSocket real-time streaming
- ✅ Beautiful terminal-aesthetic web UI

---

## Statistics

**Total Development Time:** 7 days  
**Total Servers:** 4 (NYC, LON, SYD, SGP)  
**Total Code:** ~1500 lines  
**Languages:** C++ (60%), JavaScript (35%), HTML/CSS (5%)  

**Performance Metrics:**
- Parse Time: 7-20µs average
- Network Latency: 0.02-0.5ms
- Throughput: 9-27 MB/s
- Uptime: 99.9%+ target

**Built with ⚡ by woflfren**  
*"They Came From Beyond... And They're Bringing The Beacons!"* 🏰