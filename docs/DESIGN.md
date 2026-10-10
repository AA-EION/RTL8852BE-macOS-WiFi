# Realtek RTL8852BE macOS Driver: Engineering Design Document

## 1. System Architecture
The RTL8852BE macOS driver is designed as a native `IOEthernetController` subclass for macOS 14.0+ (Sonoma, Sequoia, and macOS 26.x Darwin), attaching directly to the PCIe bus via `IOPCIDevice`.

```
+-------------------------------------------------------------------------+
|                  macOS Networking Stack / BSD Socket Layer              |
+-------------------------------------------------------------------------+
                                    |
                                    v
+-------------------------------------------------------------------------+
|                          IONetworkInterface                             |
+-------------------------------------------------------------------------+
                                    |
+-------------------------------------------------------------------------+
|                         IOEthernetController                            |
|                          (Class: RTL8852BE)                             |
|  - IOWorkLoop & IOCommandGate Serialization                             |
|  - IOGatedOutputQueue Packet Egress                                     |
|  - 802.11 Protocol Engine (Scan, Auth, Assoc, WPA2/WPA3 4-Way Handshake)|
|  - CCMP AES-128 Encryption & Decryption (RFC 3610 / IEEE 802.11)       |
+-------------------------------------------------------------------------+
       |                                                    |
       v                                                    v
+-----------------------------+           +-------------------------------+
|     RTL8852BEUserClient     |           |     PCIe Bus Layer & MMIO     |
|   (IOUserClient Interface)  |           |     - BAR2 64KB MMIO Window   |
|   - Scan / List / Connect   |           |     - 8-byte TXBD / RXBD Rings|
|   - Status / Antenna Switch |           |     - MSI Interrupt Filter    |
|   - Diagnostic Register/RF  |           |     - 32-bit Physical DMA     |
+-----------------------------+           +-------------------------------+
       |                                                    |
       v                                                    v
+-----------------------------+           +-------------------------------+
|    rtl8852be_cli / GUI      |           |     Realtek RTL8852BE ASIC    |
|  (RTL8852BE Utility.app)    |           |   (Lenovo LOQ 82XV PCIe 6)    |
+-----------------------------+           +-------------------------------+
```

## 2. Key Subsystems & Design Contracts

### 2.1 Hardware Layer (`RTL8852BE_hw.hpp`)
* **PCI Matching**: `IOPCIMatch = 0xB85210EC 0xB85B10EC`
* **Subsystem**: Lenovo `0x17AA:0x4853` (Lenovo LOQ 15IRH8 Type 82XV)
* **MMIO Window**: 64KB BAR2 space at `kIOPCIConfigBaseAddress2`.
* **Power Management**:
  * D0 power state transition.
  * ASPM L1/L1.2 disabled in PCIe configuration space to prevent sleep freeze.
  * System power sequence (`R_AX_SYS_PW_CTRL`, `R_AX_PLATFORM_ENABLE`).
* **DMA Ring Architecture**:
  * 8-byte TX Buffer Descriptors (`struct Rtw89TxBd`)
  * 8-byte RX Buffer Descriptors (`struct Rtw89RxBd`)
  * 40-byte TX Wireless Descriptor Header (`struct Rtw89TxWd`)
  * 32-byte RX Status Descriptor Header (`struct Rtw89RxDesc`)
  * 32-bit coherent DMA physical address masking (`< 4GB`).

### 2.2 Firmware Subsystem (`RTL8852BE_firmware.hpp`)
* Embedded official `rtw8852b_fw.bin` from `linux-firmware` (1,035,232 bytes).
* Multi-Firmware Container (`rtw89_mfw_hdr`) parser:
  * Automatically detects and loads `RTW89_FW_NORMAL` for Chip Cut Version 1 (`0.27.32.1`).
  * Downloads 3 firmware memory sections to chip.
  * Handshakes with hardware via `R_AX_MCUFW_CTRL` and FreeRTOS ready status.

### 2.3 Cryptographic Engine (`RTL8852BE_crypto.hpp`)
* **AES-128 Core**: FIPS-197 AES block cipher.
* **RFC 3394 Key Wrap / Unwrap**: Strictly bounded buffers, verified with Section 4.1 KAT.
* **IEEE 802.11 CCMP**: AES-128 CCM mode (RFC 3610) with exact AAD construction (omitting Duration field) and 13-byte Nonce (Priority || Addr2 || PN).
* **WPA2 PRF-512**: PTK derivation from PMK and nonces.
* **PBKDF2-HMAC-SHA1**: Passphrase to PMK derivation (4096 iterations).

### 2.4 User Client & IPC (`RTL8852BE_ipc.h`)
* Scalable external methods:
  * `kMethodScan` / `kMethodGetScanResults`
  * `kMethodConnect` / `kMethodDisconnect`
  * `kMethodGetStatus` / `kMethodSetAntenna`
  * `kMethodReadReg` / `kMethodWriteReg`
  * `kMethodReadRF` / `kMethodWriteRF`
  * `kMethodGetDiag` / `kMethodIQCalibrate`
* Supports both CLI (`rtl8852be_cli`) and Swift menu bar app (`RTL8852BE Wireless Utility.app`).
