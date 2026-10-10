# RTL8852BE audit against Linux kernel source

Audit date: 2026-10-10. RTL8852BE baseline: `af7c233`. RTL8723BE reference: `4f28a94b837c073f98416d84f8a14e967ce8477e` (left unchanged). Both working trees were initially clean.

**The RTL8852BE repository was not a complete working driver.** The baseline reported readiness without reading physical eFuse or downloading firmware, used incompatible AX packet layouts, and treated RF conditional commands as register writes. Passing simulator tests did not validate those paths. This audit corrects bounded, verifiable defects and blocks hardware startup; it does not finish an rtw89 port or establish hardware support.

## Evidence and reproducibility

Primary evidence is the upstream Linux kernel at **v6.12**, downloaded directly from torvalds/linux. This is a fixed implementation reference, not a claim to cover every newer kernel change. The [manifest](linux-source-manifest.json) records paths, URLs, and SHA-256 hashes of 29 inspected rtw89/mac80211 files. The offline checker verifies the hashes, 172 shared register/mask/queue definitions, and all five table arrays:

```sh
python3 tools/check_linux_source.py /path/to/linux-v6.12
make test CXX=clang++
```

The user's hardware-tested RTL8723BE project supplied a macOS lifecycle reference. Its rtlwifi firmware, descriptors, and RF operations describe a different chip generation. They cannot establish the RTL8852BE hardware contract. Linux also relies on mac80211/cfg80211 and a user-space supplicant: an Ethernet-style macOS interface needs equivalent station/key behavior beyond porting rtw89 registers.

## Findings and disposition

### 1. Firmware readiness was fabricated — critical, contained; transport still missing

Baseline `downloadFirmware()` parsed/logged sections, wrote a legacy readiness register, and set `fFwReady`; no firmware bytes reached the device. Linux sends a download header, packetizes each section, and checks WCPU state: [fw.c header transport](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/fw.c#L1191), [section transport](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/fw.c#L1273), and [download orchestration](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/fw.c#L1412).

The function now returns failure and never reports firmware readiness. WCPU control is at 0x1E0; there is no implemented H2C download/boot handshake. The embedded binary is unchanged: 1,035,232 bytes, SHA-256 `5b9c224ae014e0aeade8701412d820e213c8880e08615b3905dba89659f21baf`. Its original linux-firmware commit remains unrecorded; kernel-source inspection does not authenticate the separately distributed firmware binary.

### 2. Firmware container parsing was wrong — fixed for the supported static v0 image

The MFW entry starts with **cv, type, mp**, not type then cv: [fw.h](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/fw.h#L3748). Selection excludes manufacturing firmware and chooses the highest supported cut no greater than the detected cut: [fw.c](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/fw.c#L455). Section parsing includes checksum trailers and download-address masking: [v0 parser](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/fw.c#L123).

The production parser now validates the selected image's bounds, field order, version bytes, section count, payload extent, checksum trailers, and address mask. It rejects unsupported dynamic/v1/signed-trailer formats rather than guessing. Failure clears output. It selects the existing cut-1 image as 0.27.32.1 with three sections. The generator now emits bytes only, preserving the independently maintained parser. The new regression fails against the baseline parser and passes against the corrected implementation. The controller now detects the chip cut from SYS_CFG1 instead of hardcoding 1. The portable XTAL-SI analog-cut reader is implemented, but analog revision and full device identification still need integration into the remaining MAC initialization.

### 3. Physical eFuse was never read and constants were wrong — transport and identity repaired; calibration incomplete

Baseline filled the map with 0xFF and substituted a shared MAC/calibration fallback. Linux powers and polls OTP reads, handles the four-byte security prefix, and expands active-low words: [efuse.c](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/efuse.c). [RTL8852B chip declarations](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/rtw8852b.c#L847) specify physical size **1216**, logical size **2048**, and security prefix **4**. [The actual packed structure](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/rtw8852b_common.h#L36) places PCIe MAC at **0x400**, crystal at **0x2B9**, and RFE at **0x2CA**.

The production reader now performs OTP power/isolation sequencing, cut-0 burst handling, polling, and power cleanup on failure. The logical decoder handles the security prefix and active-low words with atomic failure output. `readEfuse()` uses the real MAC/crystal/RFE fields and rejects invalid identity instead of substituting a shared MAC. Tests derive offsets from an upstream structure snapshot and exercise actual transport/decoder functions, malformed maps, timeout and cleanup failures. Full calibration-map retention, gain/TSSI/power decoding and application remain required.

### 4. TX/RX packet contracts are incompatible — critical, blocked

Linux's AX [TX body/info structures](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/core.h#L998) are 24 bytes each; info is optional. [TX submission](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/pci.c#L1355) adds write-pointer sequence and DMA-address information and maps the payload separately. MACID, QSEL, and packet size occupy body dword 2: [txrx.h](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/txrx.h#L87). The repository's fixed 40-byte TXWD and field placements do not implement this.

RX has a four-byte [RXBD-info prefix](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/pci.h#L1411), first/last-buffer processing and recycling in [pci.c](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/pci.c#L321), and variable 16/32-byte packet descriptors with shift/driver-info lengths in [core.c](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/core.c#L2303). CRC/ICV flags are in dword 3, and RSSI comes from PHY status. The prototype misreads these, assumes a fixed header, and does not recycle RX slots to the host index.

Hardware ring-index reads now use the upper field and reject out-of-range indices; TX capacity, null buffers, and RX loop bounds are checked. Those containment fixes do **not** replace the packet engine. Tests no longer assert that the prototype 40/32-byte structures are Linux-verified.

### 5. Table bytes were correct; applying them was not — conditional interpreter repaired; gain/RFK incomplete

Every pair matches [rtw8852b_table.c](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/rtw8852b_table.c): BB 1013, BB gain 66, Radio A 8295, Radio B 8392, NCTL 1320. Radio A/B contain 4900/4952 control entries with addresses at or above 0x10000000. These select RFE/cut branches rather than RF registers.

Linux interprets headlines and conditional directives, invokes gain callbacks, and supplies RF configuration to firmware: [phy.c table interpretation](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/phy.c#L1459), [RF initialization](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/phy.c#L1640). The baseline truncated RF addresses and executed all variants. The production interpreter now selects headlines by RFE/cut, validates IF/ELIF/CHECK/ELSE/END grammar before writing, excludes directives from register writes and propagates callback failure. The controller uses it for BB, Radio A/B and NCTL. Tests cover independent branch fixtures and both complete radio tables. Gain callbacks, RF configuration H2C delivery and RFK integration remain missing; initialization returns failure until those stages exist. Generation is pinned to v6.12 and rejects missing/empty tables.

### 6. Power, MAC, channel and calibration are incomplete — critical, blocked

The baseline's abbreviated power sequence omitted the SPS/LDO/XTAL-SI/isolation/DMAC/CMAC steps and ignores the ONMAC timeout. Compare [power on/off](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/rtw8852b.c#L254) and [MAC initialization](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/mac.c#L3953), including DLE, scheduler and H2C infrastructure. Channel setup needs MAC, BB, RF and scheduler/ADC/TSSI/reset coordination: [channel path](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/rtw8852b.c#L485). Toggling an LC-lock bit is not [RTL8852B IQK](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/rtw8852b_rfk.c#L3771).

Incorrect AON/LPS bits, channel/LC RF addresses, DMA stop/busy masks, max-tag mask, and RF access semantics were corrected against upstream. RF v1 access uses path bits 10:8 for reads, PHY-relative result register 0x174C (BAR offset 0x1174C), shifted masks, busy polling, and the address-bit-16 direct-window selector: [RF read](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/phy.c#L812), [RF write](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/phy.c#L952). IQK and antenna switching now return unsupported instead of invented success. BB/SWSI/direct-RF accesses now add the AX PHY `cr_base` of 0x10000, as required by [phy.h](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/phy.h#L556) and [rtw89_phy_gen_ax](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/phy.c#L6648). The mapped-span minimum now covers the full PHY aperture; actual accesses still check the real BAR length. The production portable register callbacks test translated addresses, direct and SWSI masks/path selection, busy/read-completion timeouts, removed-device and I/O failures, BB delays and bypass records. MAC/power/eFuse offsets remain unshifted. Power-on/off and XTAL-SI operations now preserve the upstream routines through a reproducible generator and checked register-I/O adapter. Production tests cover reset/XTAL ordering, the RFE-5 SPS setting, power-off and failed polling/I/O. MAC/DLE/H2C, channel and calibration orchestration remain missing; these tests do not establish real-device behavior. Promiscuous/multicast callbacks now return unsupported instead of reporting an unimplemented filter change as successful; receive-filter configuration still needs a port.

### 7. Interrupt/lifetime assumptions could corrupt memory — partly contained; lifecycle verification required

Linux uses multiple interrupt banks: [PCI masks/status](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/pci.h#L161), [recognition/disable](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/pci.c#L738). ACH0 completion is bit 3; management completion is bit 11. HALT C2H belongs to HIMR0, not a fabricated PCI HIMR00 bit 15. Shared constants, band-0 queue selection (VO=3/VI=2), and three-bank masking were corrected; firmware/C2H dispatch remains absent.

DMA state is explicitly initialized. Bus mastering is revoked before freeing DMA mappings, DMA range checks resist uint64 wrap, and MMIO checks include access width, alignment, and actual mapped length. Startup failure cleanup now balances superclass startup and medium allocation is checked. These changes do not prove DMA idle, in-flight completion, read-method serialization against stop, or power-management correctness. Those require a completed transport and macOS lifecycle validation.

### 8. WPA2 primitives did not establish a complete secure station — high, partial bounds fixes

Production fixes reject truncated EAPOL-Key/IE data, bound connect strings and outgoing key data, use kernel entropy for SNonce, validate replay/ANonce before updates, and clear session keys on explicit reconnect/disconnect. MIC processing excludes Ethernet padding.

Association still lacks RSN negotiation, Message 2 lacks the negotiated RSN IE, GTK/rekey handling is incomplete, and duplicate Message 3/retry/timeouts are not a complete supplicant state machine. Data handling still needs correct QoS/address/header processing and per-TID/key replay state. Compare [mac80211 CCMP framing and replay](https://github.com/torvalds/linux/blob/v6.12/net/mac80211/wpa.c#L379), [rtw89 key setup](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/mac80211.c#L626), and [security CAM](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/cam.c#L368). A crypto known-answer test does not validate the network protocol. EAPOL supplicant negotiation is outside the kernel; kernel inspection alone cannot certify it.

The prototype also actively probes channels without Linux-equivalent regulatory/no-IR/DFS policy: [regd.c](https://github.com/torvalds/linux/blob/v6.12/drivers/net/wireless/realtek/rtw89/regd.c). Regulatory integration is required before exposing scanning/transmission.

### 9. Tests, UI, packaging and licensing overstated readiness — corrected

The 57-case simulator uses mock legacy descriptors and firmware headers rather than the production AX driver. Its passing result is retained and labeled accordingly. New production regressions cover firmware selection/truncation/selected-image bounds, protocol/IPC lengths, physical/logical eFuse and calibration identity, source-derived power/XTAL operations, RF conditional tables, DMA overflow and MMIO bounds. They do not compile the controller.

The kernel checks a shared false readiness flag before superclass startup or PCI access. The GUI disables loading and the helper refuses before privilege/system changes. Fake link rates, firmware readiness, Wi-Fi generation, calibration success, and antenna changes were removed. CLI/GUI scan requests now await completion rather than immediately reading stale results. Installer PCI IDs and copied HP test claims were corrected. Documentation no longer declares complete or verified hardware support.

The app retains the RTL8723BE navigation, visual assets, scan/join/disconnect workflows, Settings and Activity views. A driver-client layer adapts the RTL8852BE ABI, verifies output sizes, waits for scan completion, bounds strings and arrays, propagates IOKit errors and clears stale state after driver loss. Settings and Activity remain accessible when startup is blocked. Native app-model regressions exercise scan/join/disconnect, driver loss/reconnect and malformed ABI data; they are run on macOS CI. README styling, author/donation links and GitHub funding match the reference while describing this driver's actual status.

Linux-derived files now agree with the existing GPL notice; firmware bytes retain their separate Realtek licence. The root licensing overview describes the mixed components. CI runs production regressions and no longer swallows kext diagnostic failures; macOS minimum is consistently 14.

## Validation and limits

- Direct source check: all 29 SHA-256 hashes, 172 shared definitions, and all five complete table arrays pass.
- Clang 19 ASan/UBSan production regressions pass: hardware/constants/bounds, crypto vectors/bounds, firmware parser, beacon/EAPOL/connect helpers, power/XTAL, physical/logical eFuse, BB/RF register translation and conditional PHY/RF tables.
- Boot configuration regressions and all 57 legacy simulator cases pass.
- Shell/Python/plist checks, the loader refusal smoke check, and `git diff --check` pass; firmware regeneration preserves byte-for-byte image identity and leaves parser/header intact.
- LeakSanitizer was disabled locally because sandbox ptrace prevents it running. ASan/UBSan remained enabled; no leak-validation claim is made.
- [macOS, Linux and Windows CI passed for 6c720db](https://github.com/AA-EION/RTL8852BE-macOS-WiFi/actions/runs/38091167473): complete SwiftUI app, native production app-model tests, kext/CLI builds, signature/diagnostic checks and development DMG packaging. The subsequent PHY-aperture correction has local ASan/UBSan coverage and is checked by CI on push.
- The local Linux environment has no macOS SDK. Native app-model, IOKit/kext, signing and DMG checks run in the GitHub macOS CI workflow; consult its results for the pushed revision. No driver loading or hardware test was performed. No EFI, NVRAM, installed driver, or RTL8723BE source was changed.

## Work required before removing the startup block

1. Validate the implemented chip-cut detection and upstream power/XTAL-SI sequence on the real device; integrate full device/analog-revision identification.
2. Retain and apply the full eFuse calibration map (gain, power, TSSI and remaining chip-specific fields); validate the implemented OTP transport and RFE selection on hardware.
3. Port MAC/DLE/scheduler/H2C initialization and real firmware download/WCPU boot checks.
4. Replace TX/RX prototypes with complete AX descriptor, DMA, completion, prefix and recycling contracts; handle all interrupt banks and C2H.
5. Complete gain callbacks, RF configuration H2C, channel orchestration and real calibration; validate the implemented conditional PHY/RF interpreter on hardware.
6. Complete station, key, retry/rekey, data-frame and regulatory behavior.
7. Compile on macOS, validate cancellation/quiescence/suspend/resume/failure cleanup, and collect controlled hardware logs and traffic evidence for the exact revision.

Keep `RTL8852BE_HARDWARE_BRINGUP_READY` false until these conditions have evidence. Correct constants and passing portable tests alone cannot make this driver ready for hardware.
