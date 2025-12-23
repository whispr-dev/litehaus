SECURITY.md
markdown# Security Policy

## Supported Versions

| Version  | Supported          |
|----------|--------------------|
| 3.x      | ✅ Active          |
| 2.x      | ⚠️ Security fixes only |
| 1.x      | ❌ Unsupported     |

## Reporting a Vulnerability

If you discover a potential vulnerability:

1. **Do not** open a public GitHub issue immediately
2. Contact the maintainer privately via:
   - Email: **security@whispr.dev**
   - Or via GitHub's "Report a vulnerability" feature

We aim to respond within **72 hours**.

## Security Scope

This project focuses on **network monitoring** and **distributed beacon broadcasting** for infrastructure health tracking.

It **does not** claim:
- Full physical server security
- Protection against DDoS attacks
- Encryption of beacon payload data (beacons are public)
- Defense against state-level adversaries

## Security Features

**Current Protections:**
- ✅ SSL/TLS encryption on all web endpoints
- ✅ UFW firewall configuration
- ✅ PM2 process isolation
- ✅ No authentication required (public beacons)
- ✅ Rate limiting on FastPing API
- ✅ Input validation on all API endpoints

**Planned Enhancements:**
- 🔄 HMAC signature verification for beacons
- 🔄 API key authentication for private deployments
- 🔄 DDoS mitigation (Cloudflare integration)
- 🔄 Intrusion detection logging

## Known Limitations

**Public Beacons:**
- Lighthouse beacons are **intentionally public** and contain server IP addresses, health status, and performance metrics
- This is by design for network transparency
- Do not deploy Litehaus if you require server IP anonymity

**UDP Protocol:**
- UDP beacons are **not encrypted** (performance trade-off)
- Beacon tampering is theoretically possible
- Future versions may add HMAC signatures

**Rate Limiting:**
- FastPing API has basic rate limiting
- Sophisticated attackers could still overwhelm the service
- Consider Cloudflare or nginx rate limiting for production

**Dependency Security:**
- SimdJSON: Regularly updated, mature library
- libcurl: Follow Ubuntu security updates
- Node.js: Use LTS versions only
- Express/ws: Monitor npm audit warnings

## Best Practices for Deployment

1. **Firewall Configuration:**
```bash
   sudo ufw default deny incoming
   sudo ufw allow 80/tcp
   sudo ufw allow 443/tcp
   sudo ufw allow 8080/tcp  # Beacon monitor
   sudo ufw allow 8083/tcp  # WebSocket
   sudo ufw allow 9876/udp  # UDP beacons
   sudo ufw enable
```

2. **SSL/TLS:**
   - Use Let's Encrypt with auto-renewal
   - Force HTTPS redirects
   - Use strong cipher suites

3. **Process Management:**
   - Run services as non-root user
   - Use PM2 with `--max-memory-restart` flag
   - Enable PM2 startup scripts

4. **Monitoring:**
   - Monitor PM2 logs for suspicious activity
   - Track CPU/memory usage
   - Set up alerts for service downtime

5. **Updates:**
```bash
   # Regular system updates
   sudo apt update && sudo apt upgrade
   
   # npm packages
   npm audit fix
   
   # Recompile C++ with latest libraries
   sudo apt install libsimdjson-dev libcurl4-openssl-dev
```

## Incident Response

If a security incident occurs:

1. **Isolate:** Stop affected services (`pm2 stop all`)
2. **Investigate:** Check logs (`pm2 logs`, nginx logs, system logs)
3. **Report:** Contact security@whispr.dev with details
4. **Patch:** Apply fixes and redeploy
5. **Disclose:** Public disclosure after patch deployment

## Verification

Users are encouraged to:
- Inspect, audit, and rebuild from source before deployment
- Review all nginx/Caddy configurations
- Verify SSL certificates
- Test firewall rules
- Run security scans (nmap, nikto, etc.)

## Acknowledgments

We thank the security research community for responsible disclosure and appreciate all efforts to improve Litehaus security.

---

**Security Contact:** security@whispr.dev  
**PGP Key:** Available on request
```

---