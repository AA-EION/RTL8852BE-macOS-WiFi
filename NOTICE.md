# Notices

RTL8852BE Wi-Fi is made by **Issen Software Group**. See [LICENSE](LICENSE) for which
licence applies to which part.

## Linux rtw89 / rtl8852be (GPL-2.0)

The kernel extension's hardware support — PCIe DMA descriptors (TXBD/RXBD/TXWD/RXDESC),
power sequences, MAC/BB/RF initialisation, eFuse decoding, PHY/RF/AGC tables,
IQ calibration, SWSI RF register access, and the H2C command protocol — is ported from the
Linux kernel driver `drivers/net/wireless/realtek/rtw89` (`rtw8852b.c`, `rtw8852b_rfk.c`,
`pci.c`, `mac.c`, `phy.c`, `efuse.c`).

    Copyright(c) 2020-2024  Realtek Corporation.
    SPDX-License-Identifier: GPL-2.0

The ported files are licensed GPL-2.0-only ([LICENSES/GPL-2.0-only.txt](LICENSES/GPL-2.0-only.txt)).

## Realtek firmware

`rtw8852b_fw.bin` comes from
[linux-firmware](https://git.kernel.org/pub/scm/linux/kernel/git/firmware/linux-firmware.git)
(`rtw89/`), Copyright (c) Realtek Semiconductor Corporation, redistributed
unmodified under [LICENSES/LicenseRef-rtw89-firmware.txt](LICENSES/LicenseRef-rtw89-firmware.txt).

## TetherKit / TetherKitNext (MIT)

Parts of the app's user interface (the design system, status ring, log view and page
layout) are adapted from [TetherKitNext](https://github.com/AA-EION/TetherKitNext) by
Issen Software Group, itself based on [TetherKit](https://github.com/XiaoMiku01/TetherKit)
by XiaoMiku01.

    Copyright (c) 2026 Issen Software Group (TetherKitNext)
    Copyright (c) 2026 TetherKit contributors
    SPDX-License-Identifier: MIT
