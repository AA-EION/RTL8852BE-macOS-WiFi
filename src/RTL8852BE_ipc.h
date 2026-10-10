// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#ifndef RTL8852BE_IPC_H
#define RTL8852BE_IPC_H

#include <stdint.h>

#define RTL8852BE_MAX_SSID_LEN        32
#define RTL8852BE_MAX_PASSPHRASE_LEN  64
#define RTL8852BE_MAX_SCAN_APS        64

// Shared with the GUI; startup stays blocked until docs/AUDIT.md blockers close.
#define RTL8852BE_HARDWARE_BRINGUP_READY 0

enum RTL8852BEMethodIndex {
    kMethodScan           = 0,
    kMethodGetScanResults = 1,
    kMethodConnect        = 2,
    kMethodDisconnect     = 3,
    kMethodGetStatus      = 4,
    kMethodSetAntenna     = 5, // 0 = 2x2 MIMO, 1 = Main (Path A), 2 = Aux (Path B)
    kMethodReadReg        = 6, // in: addr, width(1/2/4) out: value
    kMethodWriteReg       = 7, // in: addr, width, value (root only)
    kMethodReadRF         = 8, // in: rf path, rf addr  out: value
    kMethodWriteRF        = 9, // in: rf path, rf addr, val (root only)
    kMethodGetDiag        = 10,
    kMethodIQCalibrate    = 11,
    kNumberOfMethods      = 12
};

#define RTL8852BE_DIAG_VERSION 1

struct RTL8852BEDiag {
    uint32_t version;
    uint32_t init_stage;
    uint32_t fw_version;       // (major << 24) | (minor << 16) | (subver << 8) | subidx
    uint32_t pci_device_id;    // 0xB852
    uint32_t himr, hisr;
    uint32_t txbd_wp, txbd_rp;
    uint32_t rxbd_wp, rxbd_rp;
    uint32_t rf_chnlbw;        // RF 0x18
    uint32_t interrupts;
    uint32_t rx_desc_done;
    uint32_t rx_mgmt, rx_beacons, rx_data, rx_crc_err, rx_dropped;
    uint32_t tx_submitted, tx_ring_full;
    uint8_t  mac_addr[6];
    uint8_t  xtal_cap;
    uint8_t  rfe_type;
    uint8_t  ant_mode;         // 0 = 2x2 Auto, 1 = Main (A), 2 = Aux (B)
    uint8_t  channel;
    uint8_t  is_5ghz;
    uint8_t  wifi_generation;  // 6 (802.11ax)
    uint16_t subsys_vendor;    // 0x17AA (Lenovo)
    uint16_t subsys_device;    // 0x4853 (LOQ 82XV)
    uint64_t rx_bytes;
    uint64_t tx_bytes;
};

enum RTL8852BEWifiState {
    kRTLStateDisconnected   = 0,
    kRTLStateScanning       = 1,
    kRTLStateAuthenticating = 2,
    kRTLStateAssociating    = 3,
    kRTLStateHandshake      = 4,
    kRTLStateConnected      = 5
};

struct RTL8852BEDiscoveredAP {
    uint8_t  bssid[6];
    char     ssid[RTL8852BE_MAX_SSID_LEN + 1];
    uint8_t  channel;
    int8_t   rssi;
    uint8_t  is_wpa2;
    uint8_t  is_wpa3;
    uint8_t  is_wifi6;         // HE (802.11ax) capable
    uint8_t  is_5ghz;
    uint16_t beacon_interval;
    uint16_t capabilities;
};

struct RTL8852BEScanResults {
    uint32_t count;
    struct RTL8852BEDiscoveredAP aps[RTL8852BE_MAX_SCAN_APS];
};

struct RTL8852BEConnectParams {
    char    ssid[RTL8852BE_MAX_SSID_LEN + 1];
    char    passphrase[RTL8852BE_MAX_PASSPHRASE_LEN + 1];
    uint8_t bssid[6];
};

struct RTL8852BEStatus {
    uint32_t state;            // RTL8852BEWifiState
    char     connected_ssid[RTL8852BE_MAX_SSID_LEN + 1];
    uint8_t  connected_bssid[6];
    uint8_t  channel;
    int8_t   rssi;
    uint8_t  is_wifi6;
    uint8_t  is_5ghz;
    uint8_t  ant_mode;
    uint32_t tx_rate_mbps;
    uint32_t rx_rate_mbps;
};

#endif // RTL8852BE_IPC_H
