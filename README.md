# 🛡️ Project Wire — Absolute Privacy P2P Messaging Engine

> **AETHER-seL4 Micro-Kernel Architecture • Physical Entanglement Genesis Key Engine • Double Ratchet AEAD • Dynamic UDP Port Hunting**

[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Tests](https://img.shields.io/badge/Tests-18%2F18%20Passed-brightgreen.svg)]()
[![Release](https://img.shields.io/badge/Release-v1.7.5-orange.svg)](https://github.com/Suprath/Wire/releases)

**Project Wire** is a zero-trust, peer-to-peer (P2P) secure messaging framework engineered for sovereign end-to-end communication. Built around an **AETHER-seL4 micro-kernel isolation paradigm**, Wire ensures complete message secrecy, forward secrecy, break-in recovery, and zero data leakage across local and remote network interfaces.

---

## 📑 Table of Contents

- [1. Core Architecture & Security Paradigm](#1-core-architecture--security-paradigm)
  - [seL4 Micro-Kernel Capability Severing](#sel4-micro-kernel-capability-severing)
  - [Physical Entanglement & Genesis Model](#physical-entanglement--genesis-model)
- [2. Cryptographic Specification & Algorithms](#2-cryptographic-specification--algorithms)
  - [Genesis Key Binary Layout (80-Byte Payload)](#genesis-key-binary-layout-80-byte-payload)
  - [Double Ratchet & HKDF Key Derivation](#double-ratchet--hkdf-key-derivation)
  - [ChaCha20-Poly1305 AEAD Encryption](#chacha20-poly1305-aead-encryption)
- [3. Network & Transport Mechanics](#3-network--transport-mechanics)
  - [Direct UDP Socket Transport](#direct-udp-socket-transport)
  - [Dynamic UDP Port Hunting Algorithm](#dynamic-udp-port-hunting-algorithm)
  - [Loopback Isolation & Network Security Audit](#loopback-isolation--network-security-audit)
  - [TLS Mimicry Engine](#tls-mimicry-engine)
- [4. Key Provisioning & Key Exchange](#4-key-provisioning--key-exchange)
  - [Exporting Entanglement Keys (`/exportid`)](#exporting-entanglement-keys-exportid)
  - [Importing Entanglement Keys (`/importid` & Auto-Detection)](#importing-entanglement-keys-importid--auto-detection)
- [5. Data Vault & Storage Management](#5-data-vault--storage-management)
  - [Session Persistence (`sessions.json`)](#session-persistence-sessionsjson)
  - [Isolated Vault Multi-Instance Mode (`WIRE_DATA_DIR`)](#isolated-vault-multi-instance-mode-wire_data_dir)
  - [Portable USB Mode (`--portable`)](#portable-usb-mode---portable)
- [6. Installation & Quick Start Guide](#6-installation--quick-start-guide)
  - [Prerequisites](#prerequisites)
  - [Building from Source](#building-from-source)
  - [Running Single Node](#running-single-node)
  - [Running Multi-Instance Dual Peer Setup (Local macOS)](#running-multi-instance-dual-peer-setup-local-macos)
  - [Docker Container Deployment](#docker-container-deployment)
- [7. Interactive TUI Command Reference](#7-interactive-tui-command-reference)
- [8. Automated Verification & Testing](#8-automated-verification--testing)

---

## 1. Core Architecture & Security Paradigm

### seL4 Micro-Kernel Capability Severing
Project Wire incorporates seL4 micro-kernel security concepts. Capabilities are explicit, unforgeable tokens required to access resources. Upon initialization, system components undergo **Capability Severing**:
- Network execution thread retains ONLY UDP socket binding capabilities.
- Storage thread retains ONLY encrypted file I/O capabilities.
- Memory pages containing ephemeral keying materials are locked via `mlock` and zeroed on deletion.

### Physical Entanglement & Genesis Model
Rather than relying on centralized Public Key Infrastructure (PKI) or vulnerable Certificate Authorities (CAs), Wire uses a **Physical Entanglement Genesis Key Exchange**. Out-of-band key establishment creates an entangled root state (`master_seed`) shared exclusively between two peer nodes.

```
       ┌────────────────────────┐                   ┌────────────────────────┐
       │     Peer A (Alice)     │                   │      Peer B (Bob)      │
       └───────────┬────────────┘                   └───────────┬────────────┘
                   │                                            │
                   ├─── [Out-of-Band Physical Entanglement] ────┤
                   │   Export Armored Genesis Key payload       │
                   │   (Master Seed + Pubkey + Timestamp)       │
                   ▼                                            ▼
       ┌────────────────────────┐                   ┌────────────────────────┐
       │ Double Ratchet Engine  │ ◄─── UDP Datagram ──► │ Double Ratchet Engine  │
       │ (HKDF-SHA256 + ChaCha) │     (Encrypted)   │ (HKDF-SHA256 + ChaCha) │
       └────────────────────────┘                   └────────────────────────┘
```

---

## 2. Cryptographic Specification & Algorithms

### Genesis Key Binary Layout (80-Byte Payload)
The Genesis Key is serialized into an 80-byte binary structure, base64 encoded, and wrapped in standard armored block formatting:

```text
-----BEGIN PROJECT WIRE GENESIS KEY-----
Version: Project Wire 1.0
Comment: Absolute Privacy Physical Entanglement Key

PvpYRzdd1Y1F/TIr0uKHMHdeDNVl3L9BqPp60epLFl9x+VnvYUmuKP1p/gs9L8cy
57cEKXrSrsXZdDRUJHhg5vOy2Hf+WwYAAAAAALgNASA=
-----END PROJECT WIRE GENESIS KEY-----
```

| Offset (Bytes) | Size (Bytes) | Field Name | Description |
|---|---|---|---|
| `0x00 - 0x1F` | 32 | `master_seed` | 256-bit cryptographically secure root seed used for session derivation |
| `0x20 - 0x3F` | 32 | `peer_pubkey` | 256-bit Ed25519/Curve25519 identity public key |
| `0x40 - 0x47` | 8 | `t0_timestamp` | Little-endian 64-bit epoch timestamp of entanglement generation |
| `0x48 - 0x4F` | 8 | `ipv6_prefix` | Little-endian 64-bit primary IPv6 subnet identifier |

### Double Ratchet & HKDF Key Derivation
Wire implements the **Double Ratchet Algorithm** for forward secrecy and self-healing break-in recovery:

1. **KDF Chains**: Uses `HKDF-SHA256` to derive new message keys for every payload sent or received.
2. **Symmetric Ratchet**:
   $$\text{KDF}_{\text{CK}}(\text{CK}) \longrightarrow (\text{CK}_{\text{next}}, \text{MK})$$
   - $\text{CK}$: Chain Key
   - $\text{MK}$: Message Key (used once per packet then securely wiped)
3. **DH Ratchet**: Executes an ephemeral Diffie-Hellman exchange whenever ratcheting triggers to update the Root Key ($\text{RK}$).

### ChaCha20-Poly1305 AEAD Encryption
Message payloads are encrypted using **ChaCha20-Poly1305 Authenticated Encryption with Associated Data (AEAD)**:
- **Cipher**: ChaCha20 stream cipher with a 256-bit key and 96-bit nonce.
- **Authenticator**: Poly1305 MAC tag (128-bit) verifying payload integrity and authenticity.
- **Associated Data**: Includes message sequence index, sender hash, and protocol version header.

---

## 3. Network & Transport Mechanics

### Direct UDP Socket Transport
Wire operates directly over raw **UDP sockets**, avoiding TCP handshake metadata leaks, connection-oriented socket leaks, and timing analysis side-channels.

### Dynamic UDP Port Hunting Algorithm
To support multi-instance deployment on single hosts without port collisions:
1. Wire attempts to bind to the requested port (default `9001`).
2. If the port is occupied by another process or container, Wire automatically executes **Dynamic UDP Port Hunting**, testing sequential ports ($\text{port} + 1, \text{port} + 2, \dots$) up to 50 attempts.

```cpp
// Dynamic UDP Port Hunting Algorithm
uint16_t initial_port = local_port;
uint16_t attempts = 0;
while (!socket.bind_port(local_port)) {
    local_port++;
    attempts++;
    if (attempts >= 50) {
        std::cerr << "Could not bind UDP socket in range " << initial_port << "-" << local_port << "\n";
        return 1;
    }
}
```

### Loopback Isolation & Network Security Audit
When testing two instances locally on macOS (`127.0.0.1`):
- **Kernel Routing**: UDP datagrams addressed to `127.0.0.1` remain strictly within the operating system's **Loopback Network Stack (`lo0`)**.
- **Zero LAN Exposure**: Datagrams never cross Wi-Fi or Ethernet interfaces and cannot be sniffed on external network switches.
- **AEAD Encryption**: All loopback datagram payloads remain fully encrypted with ChaCha20-Poly1305.

### TLS Mimicry Engine
For WAN transmissions, Wire includes a **TLS Mimicry Engine** that encapsulates UDP payloads within mock TLS 1.3 ClientHello / Application Data record layer headers (`0x17 0x03 0x03`), thwarting Deep Packet Inspection (DPI) and firewall throttling.

---

## 4. Key Provisioning & Key Exchange

### Exporting Entanglement Keys (`/exportid`)
Executing `/exportid` inside Wire:
1. Generates a new 256-bit randomized `master_seed`.
2. Encapsulates the seed, identity public key, timestamp, and IPv6 prefix into an 80-byte binary payload.
3. Automatically synchronizes the local active session's passphrase to match the newly generated `master_seed`.
4. Saves the armored key block to `wire_genesis_export.key`.

### Importing Entanglement Keys (`/importid` & Auto-Detection)
Wire provides flexible key import options:

1. **Multi-Line Capture inside `/importid`**:
   Type `/importid`, press Enter, and paste the multi-line Genesis block. Wire captures all lines until `-----END PROJECT WIRE GENESIS KEY-----`.
2. **Direct Paste Auto-Detection**:
   Paste the multi-line block directly into the main chat prompt without typing `/importid`. Wire detects the header and automatically routes to the peer import workflow.
3. **File Path Import**:
   Type `/importid /path/to/wire_genesis_export.key` or pass the path when prompted.

---

## 5. Data Vault & Storage Management

### Session Persistence (`sessions.json`)
Peer contact sessions, active ratchets, target IPs, and derived passphrases are stored in `<data_dir>/sessions.json`:

```json
[
  {
    "alias": "Peer_B",
    "target_ip": "127.0.0.1",
    "target_port": 9003,
    "passphrase": "<latin-1 decoded master_seed bytes>",
    "is_alice": true
  }
]
```

### Isolated Vault Multi-Instance Mode (`WIRE_DATA_DIR`)
To run isolated independent peers on a single machine, specify `WIRE_DATA_DIR`:

```bash
WIRE_DATA_DIR=/tmp/wire_peer_a MY_NICKNAME=Alice LOCAL_PORT=9001 wire_tui_interactive
```

### Portable USB Mode (`--portable`)
Supplying `--portable` forces Wire to store identity keys and sessions directly in a `data/` subdirectory relative to the binary path, suitable for airgapped USB flash drives.

---

## 6. Installation & Quick Start Guide

### Prerequisites
- **Compiler**: GCC 10+ or Clang 12+ (C++20 support required)
- **Build System**: CMake 3.20+
- **Platform**: macOS (ARM64 / x86_64), Linux (aarch64 / x86_64)

### Building from Source

```bash
# Clone the repository
git clone https://github.com/Suprath/Wire.git
cd Wire

# Configure CMake and build binary
cmake -B build -S .
cmake --build build --target wire_tui_interactive

# (Optional) Install binary to user PATH
mkdir -p ~/.wire/bin
cp build/wire_tui_interactive ~/.wire/bin/
```

### Running Single Node

```bash
# Launch interactive client
./build/wire_tui_interactive
```

### Running Multi-Instance Dual Peer Setup (Local macOS)

Open two terminal windows:

**Terminal 1 (Peer A - Alice):**
```bash
WIRE_DATA_DIR=/tmp/wire_alice MY_NICKNAME=Alice LOCAL_PORT=9001 ./build/wire_tui_interactive
```

**Terminal 2 (Peer B - Bob):**
```bash
WIRE_DATA_DIR=/tmp/wire_bob MY_NICKNAME=Bob LOCAL_PORT=9003 ./build/wire_tui_interactive
```

1. In **Terminal 1**, type `/exportid` and copy the armored key block.
2. In **Terminal 2**, paste the key block directly into the prompt. Set target IP:Port to `127.0.0.1:9001`.
3. Both terminals will show `Status: ● CONNECTED`!

### Docker Container Deployment

```bash
# Build Docker builder image
docker build -t wire-sel4-builder:latest -f Dockerfile .

# Run container node
docker run -d --name wire_docker_node \
  -p 9002:9002/udp \
  -v $(pwd):/workspace \
  wire-sel4-builder:latest ./build-arm64/wire_tui_interactive
```

---

## 7. Interactive TUI Command Reference

| Command | Description |
|---|---|
| `/myid` | Display local identity hash, listening UDP port, and identity key path |
| `/exportid` | Export Genesis Armored Key payload and update local session passphrase |
| `/importid` | Import a Genesis Armored Key payload or key file |
| `/add` | Manually add a peer contact (alias, target IP, target port, passphrase) |
| `/peers` | List all connected peer sessions and display active selection |
| `/switch <alias>` | Switch active chat target session |
| `/resync` | Reset and resynchronize active session Double Ratchet state |
| `/update` | Check GitHub Releases for automatic binary update patches |
| `/quit` | Securely exit Wire and wipe ephemeral session keys from memory |

---

## 8. Automated Verification & Testing

Wire includes a comprehensive suite of unit tests covering cryptography, PRNG security, transport layers, ratchet states, and TUI components.

```bash
# Run full test suite
ctest --test-dir build --output-on-failure
```

```text
100% tests passed out of 18

      Start  1: ChaCha20PRNGTest .................. Passed
      Start  2: PRNGSecurityAndPerfTest ........... Passed
      Start  3: KnockEngineTest .................. Passed
      Start  4: TLSMimicryTest .................. Passed
      Start  5: DiscoveryEngineTest .............. Passed
      Start  6: DoubleRatchetTest ................ Passed
      Start  7: MerkleDAGTest .................... Passed
      Start  8: CapabilitySeveringTest ........... Passed
      Start  9: GenesisManagerTest ............... Passed
      Start 10: VirtioSerialBridgeTest ........... Passed
      Start 11: TerminalUITest ................... Passed
      Start 12: UTF8SpecialCharsTest ............. Passed
      Start 13: ContactManagerTest ............... Passed
      Start 14: UpdateManagerTest ................ Passed
      Start 15: PortableUSBModeTest .............. Passed
      Start 16: iMessageTUITest .................. Passed
      Start 17: RealSocketTransportTest .......... Passed
      Start 18: VerifiedStatusAndQueueTest ....... Passed
```

---

## 📄 License

Project Wire is released under the **MIT License**. See [LICENSE](LICENSE) for details.
