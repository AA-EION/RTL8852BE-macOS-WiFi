# Testing the development prototype

Hardware loading is blocked while [AUDIT.md](AUDIT.md)'s gaps remain. Kernel startup, app, and loader agree on this status. The loader exits before requesting privilege or changing the system.

## Portable checks

```sh
make test CXX=clang++
python3 tools/check_linux_source.py /path/to/linux-v6.12
```

Production regressions exercise the actual firmware parser, protocol helpers, crypto, power/XTAL sequencing, physical/logical OTP reads, conditional RF tables, DMA-address bounds, selected constants and upstream eFuse offsets. The separate 57-case simulator exercises mocks with legacy RTL8723-style contracts. Its results do not validate AX hardware or IOKit.

The audit ran Clang 19 with ASan/UBSan. LeakSanitizer was disabled because this sandbox restricts ptrace; this is not a leak check. CI runs normal platform checks without that local restriction.

## macOS build checks

Use a macOS SDK with Kernel.framework headers and an Intel target:

```sh
make app test-app
make kext cli CXX=clang++ CC=clang
codesign --verify --strict build/RTL8852BEWiFi.kext
kmutil print-diagnostics --bundle-path build/RTL8852BEWiFi.kext
```

App regressions use the production AppModel and ABI parser with an injectable driver client: scan/join/disconnect, driver loss/reconnect, failed requests, UTF-8 fields and malformed result counts. CI compiles the full SwiftUI app and runs these tests before checking the kext, CLI and development DMG. Check the GitHub Actions result for the exact pushed revision; portable tests cannot establish macOS compilation.

## Conditions before hardware testing

Implement and review the kernel-sourced power, eFuse, firmware, MAC, descriptor, RF, and key paths. Verify startup failure stages, event cancellation/serialization, stop/free idempotence, DMA idle before unmapping, and suspend/resume on macOS.

Then collect actual chip cut, eFuse-derived MAC/RFE, firmware boot status, TX completion/RX recycling evidence, calibration results, association/key exchange captures, and bidirectional traffic. Exercise retries, replay rejection, rekey, disconnect, teardown, and sleep/wake. Record the exact commit, system, device/subsystem IDs, boot configuration, and logs before claiming hardware support.
