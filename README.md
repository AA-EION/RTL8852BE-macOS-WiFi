# Realtek RTL8852BE macOS Wi-Fi Driver (Wi-Fi 6 802.11ax)

Native macOS kernel extension and companion utilities for the **Realtek RTL8852BE** PCIe wireless network adapter, based on the mainline Linux kernel `drivers/net/wireless/realtek/rtw89/` driver subsystem.

Optimized and verified for the **Lenovo LOQ 15IRH8 (Type 82XV)** gaming laptop and compatible hardware.

---

## Features
- **Wi-Fi 6 (802.11ax) & Dual-Band Support**: 2.4 GHz (Channels 1–14) and 5 GHz (Channels 36–165).
- **Direct PCIe Attachment**: Matches `pci10ec,b852` and `pci10ec,b85b` (Subsystem `17aa:4853`).
- **Modern `rtw89` Architecture**:
  - 8-byte TXBD and RXBD DMA ring management with 32-bit DMA boundary enforcement.
  - Power sequence state machine preventing PCIe ASPM L1/L1.2 crystal sleep freezes.
  - Embedded official `rtw8852b_fw.bin` (version `0.27.32.1`) with dynamic Multi-Firmware parsing.
  - Exact PHY BB and Radio A/B RF register tables directly extracted from mainline Linux.
  - eFuse OTP logical block decoding for hardware MAC address and crystal calibration.
- **WPA2-PSK (RSN) & CCMP Crypto**:
  - RFC 3394 AES Key Wrap/Unwrap (verified against Section 4.1 Known Answer Tests).
  - IEEE 802.11 CCMP AES-128 CCM encryption/decryption with strict AAD formatting.
  - PBKDF2-HMAC-SHA1 and PRF-512 key derivation.
- **Companion Utilities**:
  - `rtl8852be_cli`: Command-line interface for status, scanning, connection, and hardware diagnostics.
  - `RTL8852BE Wireless Utility.app`: Native SwiftUI/AppKit Menu Bar application.
- **Comprehensive Test Suite**:
  - Production Hardware Contract verification.
  - RFC 3394 and CCMP cryptographic bounds verification.
  - 57-test 4-tier mock hardware execution suite (**100% pass rate**).
  - GitHub Actions CI workflow for macOS-14, Linux, and Windows.

---

## Hardware Compatibility
- **Chipset**: Realtek RTL8852BE (PCIe Wi-Fi 6 802.11ax + Bluetooth 5.2).
- **PCI ID**: `10EC:B852` or `10EC:B85B`.
- **Target Laptop**: Lenovo LOQ 15IRH8 Type 82XV (Subsystem `17AA:4853`). Also compatible with other Lenovo IdeaPad / Legion models and desktop PCIe add-in cards with RTL8852BE.
- **Supported macOS**: macOS 14.0 (Sonoma), macOS 15.0 (Sequoia), macOS 26.x (Darwin x86_64).

---

## Directory Structure
```
RTL8852BE-macOS-WiFi/
├── .github/workflows/ci.yml       # GitHub Actions multi-platform CI
├── docs/
│   ├── HARDWARE_INVESTIGATION.md # Lenovo LOQ 82XV & rtw89 investigation
│   └── DESIGN.md                 # System architecture & interface contracts
├── gui/                          # Native SwiftUI/AppKit Menu Bar App
│   ├── Sources/                  # Swift app source code
│   ├── Info.plist
│   └── RTL8852BEBridge.h         # Obj-C / IOKit bridging header
├── src/                          # Kernel Extension source code
│   ├── Info.plist                # Kext bundle metadata
│   ├── RTL8852BE.hpp             # Driver class declaration
│   ├── RTL8852BE.cpp             # Driver class implementation
│   ├── RTL8852BE_hw.hpp          # Hardware registers & descriptors
│   ├── RTL8852BE_tables.hpp      # PHY BB & RF register tables (Linux rtw89)
│   ├── RTL8852BE_tables.cpp
│   ├── RTL8852BE_firmware.hpp    # Firmware loader & MFW parser
│   ├── RTL8852BE_firmware.cpp    # Embedded rtw8852b_fw.bin image
│   ├── RTL8852BE_crypto.hpp      # RFC 3394 & CCMP crypto engine
│   ├── RTL8852BE_crypto.cpp
│   ├── RTL8852BEUserClient.hpp   # UserClient IPC
│   ├── RTL8852BEUserClient.cpp
│   ├── RTL8852BE_ipc.h           # IPC protocol structures
│   ├── RTL8852BE_kmod.c          # Kernel module start/stop
│   └── rtw8852b_fw.bin           # Official firmware binary
├── tools/
│   ├── rtl8852be_cli.cpp         # CLI utility
│   ├── generate_tables.py        # Python table generator from Linux kernel
│   ├── generate_firmware.py      # Python firmware embedder
│   └── build_test.bat            # Windows MSVC test runner
├── tests/                        # 4-Tier Test Suite & Contracts
│   ├── test_hardware_contract.cpp# Production descriptor & register checks
│   ├── test_crypto_bounds.cpp    # RFC 3394 KAT & CCMP vector verification
│   ├── test_runner.cpp           # 57-test mock execution runner
│   └── mock/                     # Mock PCIe MMIO & DMA simulator
├── scripts/                      # Build & packaging scripts
├── installer/                    # macOS .pkg installer resources
├── Makefile                      # Build automation
├── PROJECT.md                    # Project tracking
├── CHECKPOINT.md                 # Verification record
└── LICENSE                       # MIT License
```

---

## Building & Testing

### Testing on Windows / Linux / macOS
To run the production hardware contract and cryptographic bounds test suite:

**On Windows (Visual Studio Developer Command Prompt)**:
```cmd
tools\build_test.bat
```

**On macOS / Linux (Clang with AddressSanitizer)**:
```bash
make test-hardware
make test-crypto
make test
```

### Building Kernel Extension on macOS
```bash
make kext
make cli
```

---

## License
- Driver Core, CLI, and GUI: **MIT License** (Copyright &copy; 2026 Issen Software Group).
- Hardware Register Maps & PHY Tables: Ported from Linux mainline `rtw89` (Copyright &copy; Realtek Corporation, GPL-2.0 / BSD-3-Clause).
- Embedded Firmware (`rtw8852b_fw.bin`): Realtek Semiconductor Corp. firmware license.
