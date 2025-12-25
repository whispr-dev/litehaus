#!/bin/bash
# 🏰 LITEHAUS HOME MASTER NODE - INSTALLATION SCRIPT
# ===================================================
# Installs and configures the Litehaus Home Master Node
# Run as root: sudo ./install.sh

set -e

echo "🏰 ═══════════════════════════════════════════════════════════════════ 🏰"
echo "   LITEHAUS HOME MASTER NODE INSTALLER"
echo "🏰 ═══════════════════════════════════════════════════════════════════ 🏰"
echo ""

# Check for root
if [[ $EUID -ne 0 ]]; then
   echo "🚨 This script must be run as root (sudo)" 
   exit 1
fi

# Detect OS
if [[ -f /etc/os-release ]]; then
    . /etc/os-release
    OS=$ID
    VER=$VERSION_ID
else
    echo "🚨 Cannot detect OS"
    exit 1
fi

echo "📋 Detected: $OS $VER"
echo ""

# ============================================================================
# INSTALL DEPENDENCIES
# ============================================================================
echo "📦 Installing dependencies..."

if [[ "$OS" == "ubuntu" ]] || [[ "$OS" == "debian" ]]; then
    apt-get update
    apt-get install -y \
        build-essential \
        cmake \
        git \
        libcurl4-openssl-dev \
        libssl-dev \
        chrony \
        gpsd \
        gpsd-clients \
        pps-tools \
        nut \
        nut-client \
        hostapd \
        dnsmasq \
        nginx
        
elif [[ "$OS" == "fedora" ]] || [[ "$OS" == "centos" ]] || [[ "$OS" == "rhel" ]]; then
    dnf install -y \
        gcc-c++ \
        cmake \
        git \
        libcurl-devel \
        openssl-devel \
        chrony \
        gpsd \
        gpsd-clients \
        pps-tools \
        nut \
        hostapd \
        dnsmasq \
        nginx
else
    echo "⚠️  Unsupported OS: $OS. Please install dependencies manually."
fi

# ============================================================================
# CREATE USER & DIRECTORIES
# ============================================================================
echo ""
echo "👤 Creating litehaus user and directories..."

# Create user if doesn't exist
if ! id -u litehaus >/dev/null 2>&1; then
    useradd -r -s /bin/false -d /var/lib/litehaus litehaus
fi

# Create directories
mkdir -p /etc/litehaus
mkdir -p /var/lib/litehaus
mkdir -p /var/log/litehaus
mkdir -p /var/www/litehaus

# Set ownership
chown -R litehaus:litehaus /var/lib/litehaus
chown -R litehaus:litehaus /var/log/litehaus

# ============================================================================
# BUILD FROM SOURCE
# ============================================================================
echo ""
echo "🔨 Building litehaus-home..."

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$SCRIPT_DIR/../build"

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# ============================================================================
# INSTALL BINARIES
# ============================================================================
echo ""
echo "📥 Installing binaries..."

install -m 755 litehaus-core /usr/local/bin/
install -m 755 litehaus-stream /usr/local/bin/

# ============================================================================
# INSTALL CONFIGURATION
# ============================================================================
echo ""
echo "⚙️  Installing configuration..."

if [[ ! -f /etc/litehaus/core.conf ]]; then
    install -m 644 "$SCRIPT_DIR/../config/core.conf" /etc/litehaus/
fi

# ============================================================================
# INSTALL SYSTEMD SERVICES
# ============================================================================
echo ""
echo "🔧 Installing systemd services..."

install -m 644 "$SCRIPT_DIR/../systemd/litehaus-core.service" /etc/systemd/system/
install -m 644 "$SCRIPT_DIR/../systemd/litehaus-stream.service" /etc/systemd/system/

systemctl daemon-reload

# ============================================================================
# CONFIGURE CHRONY FOR GPSDO
# ============================================================================
echo ""
echo "⏱️  Configuring chrony for GPSDO..."

cat > /etc/chrony/conf.d/gpsdo.conf << 'EOF'
# Litehaus GPSDO Configuration
# Spectrum Instruments TM4-M+ Rubidium GPSDO

# GPS reference via gpsd (NMEA)
refclock SHM 0 refid GPS precision 1e-1 offset 0.0 delay 0.2

# PPS reference (high precision)
refclock PPS /dev/pps0 refid PPS precision 1e-9 lock GPS

# Allow being a stratum 1 server
local stratum 1

# Allow NTP clients on LAN
allow 10.0.0.0/24

# Log statistics
log statistics measurements tracking
EOF

# ============================================================================
# CONFIGURE GPSD
# ============================================================================
echo ""
echo "🛰️  Configuring gpsd..."

cat > /etc/default/gpsd << 'EOF'
# Litehaus GPSD Configuration
START_DAEMON="true"
USBAUTO="false"
DEVICES="/dev/ttyUSB0"
GPSD_OPTIONS="-n -b"
EOF

# ============================================================================
# FINAL STEPS
# ============================================================================
echo ""
echo "✅ Installation complete!"
echo ""
echo "🏰 ═══════════════════════════════════════════════════════════════════ 🏰"
echo "   NEXT STEPS:"
echo "🏰 ═══════════════════════════════════════════════════════════════════ 🏰"
echo ""
echo "1. Edit configuration:"
echo "   sudo nano /etc/litehaus/core.conf"
echo ""
echo "2. Connect your hardware:"
echo "   - GPSDO to /dev/ttyUSB0"
echo "   - PPS to /dev/pps0"
echo "   - LoRa gateway to /dev/ttyACM0"
echo ""
echo "3. Start services:"
echo "   sudo systemctl enable --now chrony gpsd"
echo "   sudo systemctl enable --now litehaus-core litehaus-stream"
echo ""
echo "4. Check status:"
echo "   sudo systemctl status litehaus-core"
echo "   chronyc tracking"
echo ""
echo "5. View dashboard:"
echo "   http://localhost:8080"
echo ""
echo "🏰 Happy beaconing! 🏰"
