<p align="center">
  <img src="docs/assets/icon.png" width="128" alt="RTL8852BE Wi-Fi icon">
</p>

<h1 align="center">RTL8852BE Wi-Fi</h1>

<p align="center">
  <b>A native macOS utility and driver project for the Realtek RTL8852BE Wi-Fi card.</b><br>
  The RTL8723BE menu bar experience, adapted for RTL8852BE's 2.4 GHz and 5 GHz hardware.
</p>

<p align="center">
  <sub>By <a href="https://issen.kurokamicorp.com/"><b>Issen Software Group</b></a> · hardware support derived from the Linux <a href="https://github.com/torvalds/linux/tree/v6.12/drivers/net/wireless/realtek/rtw89">rtw89</a> driver</sub>
</p>

<p align="center">
  <a href="https://www.paypal.com/donate/?business=juanesgtgt2%40gmail.com&no_recurring=0&item_name=Issen+Software+Group">
    <img src="https://img.shields.io/badge/Donate-PayPal-00457C.svg?logo=paypal&logoColor=white&style=flat-square" alt="Donate with PayPal">
  </a>
</p>

---

## What it does

RTL8852BE Wi-Fi uses the same SwiftUI/AppKit design, menu bar controls, installer layout,
and command-line interface as [RTL8723BE Wi-Fi](https://github.com/AA-EION/RTL8723BE-macOS-WiFi).
The target card is a Realtek RTL8852BE PCIe Wi-Fi 6 adapter (PCI IDs `10ec:b852` and `10ec:b85b`),
with two RF paths and support for 2.4 GHz and 5 GHz.

**Development status:** the app and portable driver routines are under active development.
The complete rtw89 MAC/firmware/DMA and station integration is not ready, so hardware loading
remains disabled. This revision does not provide a working Wi-Fi connection and has no RTL8852BE
hardware test result. See the [kernel-source audit and repair status](docs/AUDIT.md).

- **Familiar menu bar app** with Overview, Networks, Activity, and Settings.
- **Scan and join controls** adapted to RTL8852BE's IPC, both bands, and WPA2 Personal passphrases.
  Radio operations require a functioning driver backend.
- **Live connection details and throughput** from actual status and diagnostics; unknown values stay unknown.
- **Settings, logs, donations, and licenses remain accessible** even when the driver is unavailable.
- **Command-line tool included** for driver status and development diagnostics.
- **Same package layout as RTL8723BE.** The app goes in `/Applications`, CLI in `/usr/local/bin`,
  and the kext stays inside the app. No installer writes to EFI or NVRAM.

> **RTL8852BE Wi-Fi** is made by [**Issen Software Group**](https://issen.kurokamicorp.com/).
> Hardware investigation and repairs use the Linux kernel's `rtw89` source directly. The existing
> embedded Realtek firmware is distributed separately from kernel source (see [NOTICE.md](NOTICE.md)).

---

## Install

There is no hardware-ready release of this audited revision. Existing release packages do not
establish that firmware boots or that networking works. The app explains the development status;
its loading button and the kernel's startup guard remain disabled until the missing backend is implemented.

For development builds on an Intel macOS system, see **For developers** below. You can build and
open the app without a loaded driver to inspect Settings, Activity, support links, and licenses.
Do not enable an OpenCore boot-time entry for this prototype.

### Updating

Build the latest source or check the [Releases page](https://github.com/AA-EION/RTL8852BE-macOS-WiFi/releases).
A downloadable package is a build artifact, not evidence of hardware validation. Read its release notes.

### Uninstalling

If a development package is installed, open its .dmg and run:

```bash
sudo /Volumes/RTL8852BE\ WiFi*/uninstall.sh
```

Then restart.

---

## Using it

- **Overview** shows driver onboarding, or connection details, IP address, and live speeds when available.
- **Networks** uses the driver's results for both bands and supports the same hover-to-Join and double-click flow.
- **Activity** is a filterable log with copy controls. Include it when reporting a problem.
- **Settings** includes driver details, launch at login, updates, support/donations, and licenses.
  Manual RF-path selection is unavailable until the hardware implementation supports it.

The toolbar and menu bar use the same main actions as RTL8723BE: **Scan** while disconnected,
**Disconnect** while connected. Controls reflect the backend's current availability.

The command-line interface is `rtl8852be_cli`:

```bash
rtl8852be_cli status
rtl8852be_cli diag
rtl8852be_cli scan
rtl8852be_cli connect "Network Name" 'password'
rtl8852be_cli disconnect
```

These radio commands cannot succeed while hardware startup is disabled. Passphrases are 8–63
UTF-8 bytes; open, WEP, WPA3, and enterprise authentication are not implemented.

---

## Troubleshooting

| Problem | What to do |
|---|---|
| **The app says hardware support is in development** | This is the current implementation status. Boot flags do not bypass the compiled startup block. See [AUDIT.md](docs/AUDIT.md). |
| **The card is not detected** | Confirm it is RTL8852BE (`10ec:b852` / `10ec:b85b`), rather than RTL8723BE or another Realtek chip. |
| **A command reports the driver is unavailable** | No usable controller is registered while startup is blocked. The app's Settings and Activity pages still work. |
| **Other issues** | Include the commit/build, macOS version, device/subsystem IDs, and Activity log in an [issue](https://github.com/AA-EION/RTL8852BE-macOS-WiFi/issues). |

---

## Common questions

**Does RTL8723BE hardware testing validate this driver?**
No. RTL8723BE supplies the reference app and macOS architecture, but RTL8852BE uses different
rtw89 descriptors, firmware transport, power, and RF protocols. RTL8852BE needs its own hardware validation.

**Will it use the macOS Wi-Fi menu?**
The controller design presents an Ethernet-style network port (`en…`), like RTL8723BE.
Network selection belongs in the app and its menu bar panel.

**Does Bluetooth work?**
This project covers the Wi-Fi side only.

**Does a Wi-Fi 6 card mean the driver negotiates Wi-Fi 6?**
No. Hardware capability is separate from implemented and negotiated protocol support.
The app and CLI do not fabricate a negotiated generation or a 1200 Mbps link rate.

**Which laptops work?**
The PCI IDs identify the target card. This revision makes no hardware-tested laptop support claim.

---

## How it works (short version)

```text
 RTL8852BE card ──PCIe──▶ RTL8852BEWiFi.kext ──▶ Ethernet interface (en…) ──▶ macOS network stack
                         (rtw89 port in progress) ▲
                                                 └── RTL8852BE Wireless Utility.app / rtl8852be_cli
```

The app uses the driver's own IOKit user-client structures. Chip-level routines are separated
from macOS glue so portable regressions can exercise actual power operations, physical eFuse
reads and cleanup, logical calibration, firmware parsing, and conditional RF-table selection.
A complete firmware/MAC/DMA, calibration, station/key, and regulatory backend is still required.

---

## For developers

Use macOS 14 or later, Xcode command-line tools, and an SDK with the Intel Kernel.framework headers.

```bash
make all CXX=clang++ CC=clang       # kext + command-line tool
make app                           # SwiftUI app; includes built driver/tools when present
make test-app                      # production app model and IPC regressions (macOS)
make test CXX=clang++               # portable production regressions + legacy simulator
./scripts/build_dmg.sh              # development app, package and .dmg
./gui/Scripts/make-icon.sh          # re-render the app icon
```

Hardware source is pinned to Linux v6.12. With an upstream checkout or the [manifest](docs/linux-source-manifest.json)'s files:

```bash
python3 tools/check_linux_source.py /path/to/linux-v6.12
python3 tools/generate_power.py /path/to/linux-v6.12
```

The offline comparison verifies source hashes, shared definitions, and every copied table pair.
Portable and app-model tests do not constitute hardware validation. The 57-case legacy simulator
uses older RTL8723-style mock contracts; see [TESTING.md](docs/TESTING.md) for exact limits.

---

## Support & Donations

RTL8852BE Wi-Fi is free software. If you would like to support its ongoing development:

<p align="center">
  <a href="https://www.paypal.com/donate/?business=juanesgtgt2%40gmail.com&no_recurring=0&item_name=Issen+Software+Group">
    <img src="https://img.shields.io/badge/Donate-PayPal-00457C.svg?logo=paypal&logoColor=white&style=for-the-badge" alt="Donate with PayPal">
  </a>
</p>

- **PayPal**: [Donate via PayPal](https://www.paypal.com/donate/?business=juanesgtgt2%40gmail.com&no_recurring=0&item_name=Issen+Software+Group)
- **PayPal Email**: `juanesgtgt2@gmail.com`

---

## License

Copyright © 2026 Issen Software Group. Licensed as openly as its origins allow (details in [LICENSE](LICENSE)):

- **MIT** ([LICENSES/MIT.txt](LICENSES/MIT.txt)) for original app, CLI, IPC, crypto, installer, scripts, and documentation.
  Parts of the interface are adapted from [TetherKitNext](https://github.com/AA-EION/TetherKitNext) / [TetherKit](https://github.com/XiaoMiku01/TetherKit).
- **GPL-2.0-only** ([LICENSES/GPL-2.0-only.txt](LICENSES/GPL-2.0-only.txt)) for Linux-derived hardware routines,
  tables, and firmware-container parsing, and for the compiled `RTL8852BEWiFi.kext` as a whole.
- **Realtek firmware licence** ([LICENSES/LicenseRef-rtw89-firmware.txt](LICENSES/LicenseRef-rtw89-firmware.txt))
  for the unmodified embedded `rtw8852b_fw.bin`.

See [NOTICE.md](NOTICE.md) for third-party notices.
