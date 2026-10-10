# RTL8852BE project status

The project is a development prototype. Hardware bring-up, scanning, association, and network traffic have not been validated for this repository revision.

The authoritative comparison is [docs/AUDIT.md](docs/AUDIT.md), based directly on Linux v6.12 rtw89 and mac80211. The RTL8723BE repository supplied a hardware-tested macOS reference; its chip-specific implementation is not reusable as an AX hardware contract.

| Component | Current status |
| --- | --- |
| IOKit controller and DMA | Prototype; startup blocked; AX TX/RX transport incomplete |
| Firmware | Embedded image and bounded v0 parser; no download/boot transport |
| eFuse | Physical/logical OTP transport and MAC/crystal/RFE decoder implemented; full calibration application missing |
| PHY/RF tables | Exact Linux v6.12 pairs and conditional interpreter; gain/H2C/calibration integration missing |
| WPA2/CCMP | Portable crypto tests pass; network protocol and key lifecycle incomplete |
| IPC, CLI, GUI | RTL87 app layout/workflows, RTL88 ABI client and model regressions; native checks in CI |
| Packaging | Development artifacts only; loading blocked |
| Tests | Portable production regressions plus 57 legacy simulator cases; no hardware evidence |

Source-derived power/XTAL, chip-cut detection and eFuse identity are implemented with portable regressions. Remaining implementation order: calibration-map integration; MAC/DLE/H2C; firmware boot; AX DMA and interrupts; PHY/channel/calibration; MAC/key/regulatory integration; controlled hardware validation. Keep the shared readiness flag false until the audit's blockers and macOS lifecycle checks are resolved.
