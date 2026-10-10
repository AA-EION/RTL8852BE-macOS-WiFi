# RTL8852BE development design

This document describes the intended boundaries of an incomplete driver. [AUDIT.md](AUDIT.md) records gaps and links directly to Linux v6.12.

The controller is an IOEthernetController with a workloop, command gate, timers, PCI MMIO, and DMA mappings. It presents an Ethernet-style interface and needs a complete 802.11 station implementation behind that interface. Linux divides this responsibility between rtw89, mac80211, cfg80211, and a user-space supplicant; rtw89 alone is not a complete replacement for those layers.

Hardware startup is blocked by the shared `RTL8852BE_HARDWARE_BRINGUP_READY` flag. Physical/logical eFuse reads, real cut detection and upstream power/XTAL operations are implemented with checked I/O. Missing firmware download returns failure. Firmware parsing does not imply execution. IQK and antenna switching return unsupported.

The portable firmware parser implements the embedded static v0 MFW container: cut/type selection, selected-image bounds, section/checksum lengths, and download-address masking. Dynamic headers, v1 headers, and signed security trailers require additional implementation and are rejected.

The TXWD and RXD structures are prototype layouts. Replace them using upstream AX descriptor fields, TX write-pointer/address records, RXBD info, and short/long receive descriptors. PHY/RF tables use the implemented upstream conditional interpreter and RFE/cut selection; gain callbacks, RF H2C configuration and calibration orchestration remain required.

IPC shares fixed-size structures between kernel, CLI, and Swift app. Portable helpers bound connect strings, beacon elements, and EAPOL-Key fields. They do not implement cipher negotiation, a supplicant, key reinstallation handling, rekeying, per-TID replay protection, or regulatory channel policy.

Hardware-facing and lifecycle changes require macOS compilation plus failure-path, cancellation, DMA-idle, and hardware validation before removing the startup block. Portable crypto vectors and simulator tests establish only the functions they exercise.

The SwiftUI app preserves the RTL8723BE layout and delegates hardware requests to an RTL8852BE-specific IOKit client. Each operation owns its connection; ABI results, strings and arrays are bounded. The injectable client lets native tests exercise the production model without hardware. Loading remains disabled by the same shared readiness flag.
