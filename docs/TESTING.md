# Testing RTL8852BE Driver on Lenovo LOQ 15IRH8 (Type 82XV)

This testing guide details the safe hardware verification workflow for the RTL8852BE
Wi-Fi 6 PCIe driver on the Lenovo LOQ 15IRH8 (Type 82XV) laptop (PCI ID `10EC:B852`,
Subsystem `17AA:4853`).

## 1. Verify Normal OpenCore Boot First

Ensure your OpenCore EFI boots cleanly without driver injection enabled.
The kext is loaded after login using `scripts/load_test_kext.sh` or the
RTL8852BE Wireless Utility app to protect against kernel panics at boot time.

## 2. Test the GUI Application

Open `build/RTL8852BE Wireless Utility.app`.
The app will display the current driver state, menu bar item, status indicator,
and diagnostics. It should report that the driver is inactive before loading.

## 3. Prepare a Manual Hardware-Test Boot

Configure the test boot flag:

```sh
sudo diskutil mount disk0s1
python3 scripts/configure_test_boot.py --config /Volumes/EFI/EFI/OC/config.plist --apply
```

This helper ensures `rtl8852be_experimental=1` is added to boot-args while keeping
kext injection disabled at early kernel startup.

Reboot, log in, and verify boot-args:

```sh
sysctl -n kern.bootargs
```

## 4. Load After Login With Kernel Log Stream

In one Terminal, start the kernel logger:

```sh
sudo log stream --style compact --level debug --predicate 'process == "kernel" AND eventMessage CONTAINS[c] "RTL8852BE"'
```

In a second Terminal, load the kernel extension:

```sh
sudo ./scripts/load_test_kext.sh "$PWD/build/RTL8852BEWiFi.kext"
./tools/rtl8852be_cli status
./tools/rtl8852be_cli diag
```

Check the output for:
- Successful PCI matching (`10EC:B852`, Lenovo subsystem `17AA:4853`).
- ASPM L1/L1ss disabled via dynamic PCI Express Capability traversal (`pcieCap + 0x10`).
- Firmware handshake ready (`info.section_count == 3`).
- Valid MAC address decoded from eFuse logical map (`0x423`).

## 5. Wi-Fi Scan & Connection Testing

Trigger a Wi-Fi scan:

```sh
./tools/rtl8852be_cli scan
```

View discovered networks:

```sh
./tools/rtl8852be_cli results
```

Connect to a WPA2-PSK network:

```sh
./tools/rtl8852be_cli connect "YourSSID" "YourPassphrase"
```

Verify link status, CCMP encryption, and IP traffic:

```sh
./tools/rtl8852be_cli status
ping -c 4 1.1.1.1
```

## 6. Teardown and Cleanup

To unload the driver safely:

```sh
./tools/rtl8852be_cli disconnect
sudo kmutil unload -b com.realtek.driver.RTL8852BEWiFi
```

To remove the experimental boot-arg:

```sh
sudo diskutil mount disk0s1
python3 scripts/configure_test_boot.py --config /Volumes/EFI/EFI/OC/config.plist --disable --apply
```
