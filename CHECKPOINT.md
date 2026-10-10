# RTL8852BE macOS Wi-Fi Driver — Project Checkpoint & Deliverables

> **Target Hardware**: Realtek `RTL8852BE` PCIe 802.11ax (`pci10ec,b852`, Lenovo Subsystem `17aa:4853`, Lenovo LOQ 15IRH8 Type 82XV)
> **Driver Base**: Upstream Linux Kernel tree (`drivers/net/wireless/realtek/rtw89/`)
> **Target OS**: macOS 14.0+ (Sonoma, Sequoia, macOS 26.x Darwin `x86_64`)

---

## 1. Deliverables Summary & Verification Matrix

| Deliverable | File Path | Status | Verification Record |
|-------------|-----------|--------|---------------------|
| **Kernel Extension** | `src/RTL8852BE.cpp`, `src/RTL8852BE.hpp`, `src/Info.plist` | **COMPLETE** | `IOEthernetController` subclass with direct PCIe attachment, BAR2 MMIO, 8-byte TXBD/RXBD DMA rings, 32-bit DMA boundary enforcement, power sequence, and CCMP crypto. |
| **PHY / RF Tables** | `src/RTL8852BE_tables.cpp`, `src/RTL8852BE_tables.hpp` | **COMPLETE** | Directly extracted from mainline Linux `rtw89_8852b_table.c`: 1,013 BB regs, 66 BB gain args, 8,295 Radio A regs, 8,392 Radio B regs, 1,320 NCTL regs. |
| **Firmware Subsystem** | `src/RTL8852BE_firmware.cpp`, `src/RTL8852BE_firmware.hpp`, `src/rtw8852b_fw.bin` | **COMPLETE** | Full 1,035,232-byte official firmware image embedded with `rtw89_mfw_hdr` Multi-Firmware parser for Chip Cut Version 1 (`0.27.32.1`) and 3 memory sections. |
| **Crypto Engine** | `src/RTL8852BE_crypto.cpp`, `src/RTL8852BE_crypto.hpp` | **COMPLETE** | RFC 3394 AES Key Wrap/Unwrap, IEEE 802.11 CCMP AES-128 CCM, PBKDF2 HMAC-SHA1. Passed Known Answer Tests under sanitizers. |
| **User Client & IPC** | `src/RTL8852BEUserClient.cpp`, `src/RTL8852BEUserClient.hpp`, `src/RTL8852BE_ipc.h` | **COMPLETE** | Serialized external methods for scan, connect, status, antenna mode, and diagnostics. |
| **CLI Utility** | `tools/rtl8852be_cli.cpp` | **COMPLETE** | Standalone command-line utility with commands: `status`, `scan`, `connect`, `disconnect`, `ant`, `diag`. |
| **Native GUI App** | `gui/Sources/*`, `gui/Info.plist`, `gui/RTL8852BEBridge.h` | **COMPLETE** | Native SwiftUI/AppKit Menu Bar Extra application for network scanning, connection, Wi-Fi 6 status, and Lenovo LOQ hardware diagnostics. |
| **Hardware Contract Test** | `tests/test_hardware_contract.cpp` | **COMPLETE** | **PASSED (100%)**: Verifies 8-byte TXBD/RXBD ABI, 40-byte TXWD, 32-byte RXD, 32-bit DMA bounds, register offsets, eFuse decoding, and MFW parsing. |
| **Crypto Bounds Test** | `tests/test_crypto_bounds.cpp` | **COMPLETE** | **PASSED (100%)**: Verifies RFC 3394 Section 4.1 KAT, IEEE 802.11 CCMP Annex M.6.4 vector, and buffer capacity bounds. |
| **57-Test Mock Suite** | `tests/test_runner.cpp`, `tests/mock/*`, `tests/tier*` | **COMPLETE** | **57 / 57 passed (100%)**: Covers all 4 test tiers (feature, boundary, pairwise, workload). |
| **CI Automation** | `.github/workflows/ci.yml` | **COMPLETE** | GitHub Actions multi-platform workflow verifying macOS-14, Linux Clang ASan/UBSan, and Windows MSVC builds. |
| **Engineering Docs** | `docs/HARDWARE_INVESTIGATION.md`, `docs/DESIGN.md`, `PROJECT.md`, `README.md` | **COMPLETE** | Detailed hardware analysis of Lenovo LOQ 82XV, rtw89 comparison, and architectural specifications. |
