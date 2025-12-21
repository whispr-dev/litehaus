const express = require('express');
const app = express();
const PORT = 9876;

console.log('🏃 Starting FastPing Service...');

app.use(express.json());
app.use((req, res, next) => {
    res.header('Access-Control-Allow-Origin', '*');
    res.header('Access-Control-Allow-Headers', 'Origin, X-Requested-With, Content-Type, Accept');
    next();
});

app.get('/ping', (req, res) => {
    const startTime = process.hrtime.bigint();
    
    const clientIp = req.headers['x-forwarded-for']?.split(',')[0].trim() 
                  || req.headers['x-real-ip']
                  || req.connection.remoteAddress
                  || req.socket.remoteAddress
                  || 'unknown';
    
    const cleanIp = clientIp.replace(/^::ffff:/, '');
    
    let anonymityLevel = 'direct';
    if (req.headers['x-forwarded-for']) {
        const hops = req.headers['x-forwarded-for'].split(',').length;
        if (hops > 2) anonymityLevel = 'high';
        else if (hops > 1) anonymityLevel = 'medium';
        else anonymityLevel = 'low';
    }
    
    const endTime = process.hrtime.bigint();
    const latencyNs = endTime - startTime;
    const latencyMs = Number(latencyNs) / 1_000_000;
    
    let speedHint = 'fast';
    if (latencyMs > 10) speedHint = 'slow';
    else if (latencyMs > 5) speedHint = 'normal';
    
    const response = {
        status: 'ok',
        connecting_ip: cleanIp,
        anonymity_level: anonymityLevel,
        speed_hint: speedHint,
        server_processing_latency_ms: parseFloat(latencyMs.toFixed(3)),
        client_ip_from_headers: cleanIp,
        message: 'FastPing response successful'
    };
    
    res.json(response);
    
    console.log(`📡 Ping from ${cleanIp} | Latency: ${latencyMs.toFixed(3)}ms`);
});

app.get('/health', (req, res) => {
    res.json({ status: 'healthy', service: 'FastPing', uptime: process.uptime() });
});

app.listen(PORT, '0.0.0.0', () => {
    console.log(`🏰 FastPing running on port ${PORT}`);
});
