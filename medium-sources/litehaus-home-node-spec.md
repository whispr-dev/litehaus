alright Petey fren — let’s lock this in properly.

What follows is a clean, explicit, future-proof specification you can treat as:



a design contract



a README core



the basis of implementation tickets



something you can hand to future-you without swearing



I’ll write this like a real systems spec, not hand-wavy prose.



📜 Litehaus Home Master Node

Authoritative Physical Beacon Node Specification

1\. Purpose \& Role



The Litehaus Home Master Node (LHMN) is a physically rooted, continuously operating lighthouse that:



Emits live telemetry describing network health, system performance, and time quality



Serves as a radio ↔ internet bridge for limited, rate-controlled measurements



Acts as a local-access beacon via high-gain Wi-Fi



Provides authoritative time discipline backed by GPS-disciplined rubidium reference



Remains operational during extended power and WAN outages



It is not:



A general-purpose router



A free internet gateway



A command-and-control surface



It is:



A measurement anchor



A teaching instrument



A physical trust root within the Litehaus network



2\. Node Classification

Node Class:        Home / Physical / Authoritative

Trust Level:       High (locally owned hardware)

Availability Goal: 24/7 continuous operation

Power Domain:      Grid + UPS + Holdover

Connectivity:      Wired Ethernet primary



3\. Hardware Baseline

3.1 Core Compute



x86\_64 system



≥ 2.4 GHz CPU



32 GB RAM (DDR3 acceptable)



≥ 1 TB SSD/HDD (NVMe optional)



Wired Ethernet (primary)



3.2 Power



UPS with EBMs



≥ 20 hours runtime at idle + beacon load



Surge \& lightning protection for all external antennas



3.3 Network Perimeter



Dedicated hardware firewall (opnSense)



Multi-NIC separation (WAN / LAN / RF / Wi-Fi halo)



3.4 Time \& Frequency



GPS-disciplined Rubidium Oscillator



PPS + 10 MHz outputs



External GPS antenna with lightning arrestor



3.5 RF \& Wireless



LoRaWAN Gateway (SX1308 Corecell class)



LimeSDR Mini v2 (experimental RF)



High-gain USB Wi-Fi adapter (+6 dB antennas)



4\. Operating System

4.1 OS Choice

OS: Ubuntu Server LTS (22.04 or newer)

Kernel: Stock LTS

GUI: None



4.2 System Principles



systemd-managed services only



No desktop environment



SSH key access only



Immutable service boundaries



5\. Network Architecture

5.1 External Connectivity



Outbound WAN allowed



Inbound WAN denied by default



Explicitly allowed inbound:



HTTPS (Litehaus public endpoint)



WebSocket stream ports



5.2 Internal Segmentation



Management LAN



RF services subnet



Wi-Fi halo subnet



No lateral movement permitted between zones



6\. Time Discipline Subsystem

6.1 Software Stack



gpsd — GPS + PPS ingestion



chrony — system clock discipline



Optional: ptp4l for LAN experiments



6.2 Time Authority Policy



Priority order:



PPS from GPSDO



GPS time



External NTP (sanity only)



6.3 Exposed Telemetry

"time": {

&nbsp; "source": "gpsdo",

&nbsp; "pps\_locked": true,

&nbsp; "estimated\_error\_ns": 50,

&nbsp; "holdover\_seconds": 0,

&nbsp; "discipline\_state": "locked"

}



7\. Litehaus Software Stack

7.1 Service Overview

Service	Role

litehaus-core	Metric aggregation \& state

litehaus-stream	Telemetry broadcast

litehaus-radio-bridge	RF-triggered probes

litehaus-ui	Dashboards \& local portal

litehaus-lora	LoRa gateway supervision



All services:



Run as non-root



Communicate via localhost or Unix sockets



Restart automatically via systemd



8\. Telemetry Model

8.1 Beacon Cadence



Default: 1 Hz



Adaptive under load



Timestamped with disciplined clock



8.2 Core Telemetry Fields

{

&nbsp; "node\_id": "litehaus-home-uk-01",

&nbsp; "node\_type": "home\_physical",

&nbsp; "uptime\_s": 123456,

&nbsp; "power": {

&nbsp;   "source": "grid",

&nbsp;   "ups": true

&nbsp; },

&nbsp; "system": {

&nbsp;   "cpu\_load": 0.42,

&nbsp;   "mem\_used\_mb": 8192,

&nbsp;   "disk\_free\_gb": 812

&nbsp; }

}



9\. RF Subsystems

9.1 LoRaWAN Gateway



Passive gateway operation



No user shell access



Telemetry only:



RX/TX counters



Gateway health



Timestamp quality



9.2 SDR Radio Bridge (LimeSDR)

Function



Allows radio-originated requests to trigger limited internet measurements.



Allowed Actions



ICMP ping (single host)



DNS resolve



HTTP HEAD request



Forbidden



Arbitrary outbound traffic



Sustained probing



Data exfiltration



Security



Strict framing



CRC required



Per-callsign / per-ID rate limits



Stateless request handling



10\. Wi-Fi “Halo” Access

10.1 Purpose



Provide local physical access to Litehaus information.



10.2 Services Available



Read-only dashboard



Node comparison views



Time \& RF status



10.3 Restrictions



No LAN access



No WAN routing



No authentication by default (optional later)



11\. Failure \& Degradation Modes

11.1 WAN Loss



Beacon continues locally



RF + Wi-Fi remain operational



Time enters holdover



11.2 Power Loss



UPS supplies node



Power telemetry updated



Graceful shutdown when threshold reached



11.3 GPS Loss



Holdover mode



Time error estimate increases



Explicit telemetry flag raised



12\. Security Posture



Firewall-first design



Minimal exposed surface



No general compute access



RF commands are declarative, not imperative



Logging is append-only



13\. Extensibility (Explicitly Allowed)



Additional beacon transports (SSE, UDP)



Additional RF protocols



Distributed compute offload



Public datasets \& visualization



Explicitly out of scope:



User accounts



Arbitrary scripting



Command shells



VPN access



14\. Philosophy



The Litehaus Home Master Node is designed to be:



Observable, disciplined, resilient, and boring to attack —

but fascinating to study.



Next steps (your call)



We can now:



Break this into implementation tickets



Define service configs \& ports



Design radio packet formats



Write the chrony + gpsd config



Start coding litehaus-core



Just point, fren — the lighthouse is officially specced 🌊📡

