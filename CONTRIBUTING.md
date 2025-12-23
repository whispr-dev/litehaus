CONTRIBUTING.md
markdown# Contributing to Litehaus

Thanks for your interest in improving the **Litehaus Global Lighthouse Network**!

## Code Style

**C++:**
- Follow **C++17** standards
- Use `-std=c++17 -O3 -march=native` for compilation
- Keep SIMD optimizations intact
- Run `clang-format` before commits (if available)

**JavaScript/Node.js:**
- Follow **ES6+** conventions
- Use `const` and `let` (avoid `var`)
- Keep async/await patterns consistent
- Run `npm run lint` if configured

## Development Workflow

1. Fork the repository
2. Create a feature branch:
```bash
   git checkout -b feature/your-feature
```
3. Make your changes
4. Test locally:
```bash
   # Compile C++
   g++ -std=c++17 -O3 -march=native litehaus.cpp -o litehaus -lsimdjson -lpthread -lcurl
   
   # Test Node.js services
   npm install
   npm test
```
5. Commit with clear messages:
```
   feat: add latency histogram visualization
   fix: handle UDP packet fragmentation
   docs: update deployment guide for AWS
```
6. Push and open a Pull Request against `main`

## Testing

**C++ Lighthouse:**
- Verify compilation on Ubuntu 22.04/24.04
- Test SimdJSON parse times (should be <20µs)
- Confirm UDP beacons broadcast correctly
- Check memory leaks with `valgrind`

**Node.js Services:**
- Test FastPing API responses
- Verify WebSocket streaming
- Check PM2 process management
- Test nginx/Caddy reverse proxy configs

**Manual Verification:**
- Deploy to test server
- Monitor beacon reception
- Check web dashboard connectivity
- Verify SSL certificates

## Documentation

If you add a feature, please update:
- `README.md` - Main documentation
- `CHANGELOG.md` - Version history
- Inline code comments (especially in C++)
- API documentation (if adding endpoints)

## Communication

Open an Issue for:
- Feature requests
- Bug reports
- Performance improvements
- Security concerns
- Deployment questions

## Areas for Contribution

**High Priority:**
- Historical data storage (database integration)
- Performance graphs and analytics
- Mobile-responsive web interface
- Automated health checks and alerting
- Geographic visualization of network

**Medium Priority:**
- Additional lighthouse deployments
- Lighthouse auto-discovery protocol
- Mesh network topology
- Distributed consensus algorithms
- API rate limiting

**Low Priority:**
- Alternative transport protocols (QUIC, WebRTC)
- Blockchain integration (optional)
- Custom monitoring dashboards
- Third-party integrations (Grafana, Prometheus)

## Performance Standards

All contributions must maintain:
- **Parse time:** <20µs for JSON processing
- **Network latency:** <1ms to FastPing API
- **Beacon frequency:** 5 seconds (configurable)
- **Memory footprint:** <10MB per lighthouse process
- **CPU utilization:** <5% per lighthouse

## Security Guidelines

- Never commit credentials or API keys
- Use environment variables for sensitive config
- Test firewall rules before deployment
- Ensure SSL/TLS on all web endpoints
- Follow principle of least privilege

---

Thanks for helping make Litehaus the most efficient lighthouse network in existence! 🏰✨