# Project: Realtek RTL8852BE macOS Wi-Fi Driver

## Architecture
The RTL8852BE driver provides native Wi-Fi 6 (802.11ax) network support for macOS, based on the mainline Linux kernel `drivers/net/wireless/realtek/rtw89/` driver subsystem.

- **Target Card**: Realtek RTL8852BE PCIe 802.11ax (`pci10ec,b852` / `pci10ec,b85b`, Lenovo Subsystem `17aa:4853`).
- **Target Machine**: Lenovo LOQ 15IRH8 (Machine Type 82XV).
- **Target OS**: macOS 14.0+ (Sonoma, Sequoia, and macOS 26.x Darwin x86_64).

## Deliverables & Status
1. **Kernel Extension (`RTL8852BEWiFi.kext`)**:
   - Class `RTL8852BE : public IOEthernetController`.
   - BAR2 MMIO mapping, MSI interrupt handling, power management.
   - TXBD/RXBD DMA ring engine with 32-bit coherent mask.
   - Embedded `rtw8852b_fw.bin` (version 0.27.32.1) with MFW header parser.
   - eFuse reader with logical stream decoding.
   - PHY BB and RF register tables (Radio A & B via SWSI).
   - 802.11 scanner, auth/assoc, and WPA2 4-way handshake engine.
   - CCMP AES-128 CCM crypto engine.
   - Status: **COMPLETE**

2. **User Client & IPC (`RTL8852BEUserClient`)**:
   - `IOUserClient` subclass exposing external methods under command gate serialization.
   - Diagnostics, register inspection, RF inspection, and antenna switching.
   - Status: **COMPLETE**

3. **Command Line Utility (`rtl8852be_cli`)**:
   - Standalone CLI for status, scanning, connecting, disconnecting, and diagnostics.
   - Status: **COMPLETE**

4. **Native SwiftUI Menu Bar App (`RTL8852BE Wireless Utility.app`)**:
   - Status hero card, Wi-Fi 6 network scanner, WPA2 connection dialog, antenna control, Lenovo LOQ hardware diagnostics.
   - Status: **COMPLETE**

5. **Test Suite**:
   - `test_hardware_contract.cpp`: Production descriptor ABI, registers, DMA bounds, eFuse, firmware parsing.
   - `test_crypto_bounds.cpp`: RFC 3394 KAT, IEEE 802.11 CCMP vector, PBKDF2 HMAC-SHA1.
   - 57-Test Mock Hardware & 4-Tier Test Suite (`test_runner.cpp`): 57 / 57 passed (100%).
   - Status: **COMPLETE**

6. **Continuous Integration (CI)**:
   - GitHub Actions workflow (`.github/workflows/ci.yml`) for macOS-14, Linux, and Windows builds.
   - Status: **COMPLETE**
