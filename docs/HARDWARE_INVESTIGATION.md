# RTL8852BE hardware source investigation

This investigation uses Linux kernel v6.12 directly. Findings and implementation references are in [AUDIT.md](AUDIT.md); exact source hashes are in [linux-source-manifest.json](linux-source-manifest.json).

[rtw8852be.c](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/rtw8852be.c) binds vendor 10ec and devices b852/b85b to RTL8852B chip operations. IDs establish matching, not laptop-specific validation. This audit has no Lenovo LOQ hardware result.

[rtw8852b.c](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/rtw8852b.c) declares physical eFuse size 1216, logical size 2048, and a four-byte security prefix. The PCIe MAC address is at logical offset 0x400; crystal and RFE fields are at 0x2B9 and 0x2CA. Tests calculate those offsets from an upstream structure snapshot.

AX transport uses eight-byte buffer descriptors, but this alone does not define a valid packet. TX needs a 24-byte body, optional 24-byte info, write-pointer and DMA-address data. RX needs RXBD-info processing, variable descriptor lengths, synchronization, and recycling. The existing packet path lacks these contracts.

All five table arrays match upstream. Radio A/B also contain conditional directives, which must be interpreted using detected cut and RFE. The production interpreter now selects the matching branches and validates directives before writes. Gain, RF H2C and calibration integration still need a port.

The RTL8723BE reference is useful for macOS integration and observed lifecycle behavior. RTL8852BE uses rtw89 protocols; older rtlwifi firmware headers, descriptor ownership, RF operations, and calibration cannot be transferred by renaming identifiers.
