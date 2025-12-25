#!/bin/bash
# 🏰 LITEHAUS HOME MASTER NODE - CHRONY GPSDO SETUP
# ==================================================
# Configures chrony for GPS-disciplined timing with PPS
# 
# Hardware: Spectrum Instruments TM4-M+ Rubidium GPSDO
# - NMEA via USB (/dev/ttyUSB0)
# - PPS via kernel (/dev/pps0)
# - 10MHz reference output (for LoRa gateway)

set -e

echo "⏱️  Litehaus GPSDO Time Authority Setup"
echo "========================================"

# Check for root
if [[ $EUID -ne 0 ]]; then
   echo "🚨 Run as root: sudo $0" 
   exit 1
fi

# ============================================================================
# CHECK HARDWARE
# ============================================================================
echo ""
echo "🔍 Checking hardware..."

# Check for GPSDO serial port
if [[ ! -c /dev/ttyUSB0 ]]; then
    echo "⚠️  /dev/ttyUSB0 not found - GPSDO may not be connected"
    echo "   Connect GPSDO and try again"
    exit 1
fi

# Check for PPS device
if [[ ! -c /dev/pps0 ]]; then
    echo "⚠️  /dev/pps0 not found"
    echo "   Loading pps-gpio module..."
    modprobe pps-gpio
    
    if [[ ! -c /dev/pps0 ]]; then
        echo "❌ Still no PPS device. Check kernel config and GPIO wiring."
        echo "   For USB-based PPS, ensure pps-ldisc is loaded."
        exit 1
    fi
fi

echo "✅ Hardware detected:"
ls -la /dev/ttyUSB0 /dev/pps0 2>/dev/null || true

# ============================================================================
# TEST PPS SIGNAL
# ============================================================================
echo ""
echo "📡 Testing PPS signal..."

if command -v ppstest &> /dev/null; then
    echo "   Running ppstest for 5 seconds..."
    timeout 5 ppstest /dev/pps0 || true
else
    echo "   ppstest not installed, skipping test"
fi

# ============================================================================
# CONFIGURE GPSD
# ============================================================================
echo ""
echo "🛰️  Configuring gpsd..."

cat > /etc/default/gpsd << 'EOF'
# Litehaus GPSDO Configuration
# Spectrum Instruments TM4-M+ uses USB NMEA output
START_DAEMON="true"
USBAUTO="false"
DEVICES="/dev/ttyUSB0"
GPSD_OPTIONS="-n -b"
EOF

# Create udev rule for consistent device naming
cat > /etc/udev/rules.d/99-litehaus-gpsdo.rules << 'EOF'
# Litehaus GPSDO USB serial
# Adjust idVendor/idProduct for your specific GPSDO
SUBSYSTEM=="tty", ATTRS{idVendor}=="0403", ATTRS{idProduct}=="6001", SYMLINK+="gpsdo", MODE="0666"
EOF

udevadm control --reload-rules

# ============================================================================
# CONFIGURE CHRONY
# ============================================================================
echo ""
echo "⏱️  Configuring chrony for stratum-1 operation..."

# Backup original config
if [[ -f /etc/chrony/chrony.conf ]] && [[ ! -f /etc/chrony/chrony.conf.bak ]]; then
    cp /etc/chrony/chrony.conf /etc/chrony/chrony.conf.bak
fi

# Create GPSDO-specific config
cat > /etc/chrony/conf.d/litehaus-gpsdo.conf << 'EOF'
# ============================================================================
# LITEHAUS HOME MASTER NODE - GPSDO TIME AUTHORITY
# ============================================================================
# Spectrum Instruments TM4-M+ Rubidium GPSDO via gpsd
#
# This makes the node a Stratum 1 NTP server with ~50ns accuracy

# Primary: GPS time via NMEA (shared memory from gpsd)
# SHM 0 is the main GPS time fix
# offset accounts for serial port latency (~200ms typical)
refclock SHM 0 refid GPS precision 1e-1 offset 0.0 delay 0.2 noselect

# Secondary: PPS pulse for microsecond precision
# The PPS signal is the authoritative time edge
# Lock to GPS for sanity checking
refclock PPS /dev/pps0 refid PPS precision 1e-9 lock GPS prefer

# Fallback: If GPS fails, use these NTP servers briefly
# Then enter holdover mode
pool uk.pool.ntp.org iburst maxsources 2

# Stratum 1 local reference
# Only orphan if all refs fail
local stratum 1 orphan

# Allow clients on local network
allow 10.0.0.0/24
allow 192.168.0.0/16

# Allow larger initial frequency error (GPS cold start)
maxupdateskew 1000

# Holdover configuration
# Continue running for up to 24 hours without GPS
driftfile /var/lib/chrony/chrony.drift

# Leap second handling
leapsectz right/UTC

# Hardware timestamping if available (reduces jitter)
hwtimestamp eth0

# Statistics logging for monitoring
log statistics measurements tracking refclocks
logdir /var/log/chrony

# RTC synchronization
rtcsync

# Mail notifications (optional)
# mailonchange root@localhost 0.5
EOF

# ============================================================================
# VERIFY CONFIGURATION
# ============================================================================
echo ""
echo "🔧 Verifying chrony configuration..."

if chronyc -v >/dev/null 2>&1; then
    echo "   chronyc available: $(chronyc -v)"
fi

# Syntax check
if chronyd -p -f /etc/chrony/chrony.conf >/dev/null 2>&1; then
    echo "✅ Configuration syntax OK"
else
    echo "❌ Configuration error - check /etc/chrony/chrony.conf"
    exit 1
fi

# ============================================================================
# RESTART SERVICES
# ============================================================================
echo ""
echo "🔄 Restarting services..."

systemctl restart gpsd
sleep 2
systemctl restart chrony

# ============================================================================
# VERIFY OPERATION
# ============================================================================
echo ""
echo "📊 Checking operation..."

echo ""
echo "=== GPSD Status ==="
if gpspipe -w -n 3 2>/dev/null | head -3; then
    echo "✅ gpsd is producing data"
else
    echo "⚠️  No data from gpsd yet (may need time to acquire)"
fi

echo ""
echo "=== Chrony Sources ==="
chronyc sources -v || true

echo ""
echo "=== Chrony Tracking ==="
chronyc tracking || true

# ============================================================================
# SUMMARY
# ============================================================================
echo ""
echo "🏰 ═══════════════════════════════════════════════════════════════════ 🏰"
echo "   GPSDO TIME AUTHORITY CONFIGURATION COMPLETE"
echo "🏰 ═══════════════════════════════════════════════════════════════════ 🏰"
echo ""
echo "📋 Monitor commands:"
echo "   chronyc tracking    - Show current sync status"
echo "   chronyc sources     - Show time sources"
echo "   chronyc sourcestats - Show source statistics"
echo "   gpsmon              - Interactive GPS monitor"
echo "   ppstest /dev/pps0   - Test PPS signal"
echo ""
echo "📈 Expected results after GPS lock:"
echo "   - Stratum: 1"
echo "   - Reference: PPS"
echo "   - System time offset: <100ns"
echo "   - Frequency error: <1ppm (improving over time)"
echo ""
echo "⏳ Note: Full accuracy takes 24-48 hours of continuous operation"
echo "   as the rubidium oscillator reaches thermal equilibrium."
echo ""
