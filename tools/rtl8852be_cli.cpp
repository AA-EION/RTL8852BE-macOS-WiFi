// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <IOKit/IOKitLib.h>
#include "../src/RTL8852BE_ipc.h"

static const char *stateToString(uint32_t state) {
    switch (state) {
        case kRTLStateDisconnected:   return "DISCONNECTED";
        case kRTLStateScanning:       return "SCANNING";
        case kRTLStateAuthenticating: return "AUTHENTICATING";
        case kRTLStateAssociating:    return "ASSOCIATING";
        case kRTLStateHandshake:      return "WPA2_4WAY_HANDSHAKE";
        case kRTLStateConnected:      return "CONNECTED (Wi-Fi 6 AX)";
        default:                      return "UNKNOWN";
    }
}

static io_connect_t openDriverConnection(void) {
    CFMutableDictionaryRef matchDict = IOServiceMatching("RTL8852BE");
    if (!matchDict) {
        fprintf(stderr, "[!] Failed to create IOServiceMatching(\"RTL8852BE\") dictionary.\n");
        return IO_OBJECT_NULL;
    }

    io_service_t service = IOServiceGetMatchingService(kIOMainPortDefault, matchDict);
    if (service == IO_OBJECT_NULL) {
        fprintf(stderr, "[!] RTL8852BE kernel service not currently loaded in IORegistry (pci10ec,b852).\n");
        return IO_OBJECT_NULL;
    }

    io_connect_t connect = IO_OBJECT_NULL;
    kern_return_t kr = IOServiceOpen(service, mach_task_self(), 0, &connect);
    IOObjectRelease(service);
    if (kr != KERN_SUCCESS || connect == IO_OBJECT_NULL) {
        fprintf(stderr, "[!] IOServiceOpen failed with IOReturn 0x%08x.\n", kr);
        return IO_OBJECT_NULL;
    }
    return connect;
}

static int cmdStatus(io_connect_t conn) {
    RTL8852BEStatus st;
    memset(&st, 0, sizeof(st));
    size_t outSize = sizeof(st);
    kern_return_t kr = IOConnectCallStructMethod(conn, kMethodGetStatus, nullptr, 0, &st, &outSize);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[!] kMethodGetStatus failed (0x%08x)\n", kr);
        return 1;
    }
    printf("=== Realtek RTL8852BE Wi-Fi 6 PCIe Status ===\n");
    printf("  State      : %s (%u)\n", stateToString(st.state), st.state);
    printf("  SSID       : %s\n", st.connected_ssid[0] ? st.connected_ssid : "<none>");
    printf("  BSSID      : %02x:%02x:%02x:%02x:%02x:%02x\n",
           st.connected_bssid[0], st.connected_bssid[1], st.connected_bssid[2],
           st.connected_bssid[3], st.connected_bssid[4], st.connected_bssid[5]);
    printf("  Channel    : %u (%s)\n", st.channel, st.is_5ghz ? "5 GHz" : "2.4 GHz");
    printf("  Standard   : %s\n", st.is_wifi6 ? "Wi-Fi 6 (802.11ax)" : "802.11ac/n");
    printf("  Antenna    : %s\n", st.ant_mode == 0 ? "2x2 MIMO (Auto)" : (st.ant_mode == 1 ? "Path A (Main)" : "Path B (Aux)"));
    printf("  TX Rate    : %u Mbps\n", st.tx_rate_mbps);
    printf("  RX Rate    : %u Mbps\n", st.rx_rate_mbps);
    return 0;
}

static int cmdScan(io_connect_t conn) {
    printf("[*] Initiating 802.11ax 2.4/5GHz scan sweep...\n");
    kern_return_t kr = IOConnectCallMethod(conn, kMethodScan, nullptr, 0, nullptr, 0, nullptr, nullptr, nullptr, nullptr);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[!] kMethodScan failed (0x%08x)\n", kr);
        return 1;
    }
    usleep(800000); // Wait for sweep

    RTL8852BEScanResults results;
    memset(&results, 0, sizeof(results));
    size_t outSize = sizeof(results);
    kr = IOConnectCallStructMethod(conn, kMethodGetScanResults, nullptr, 0, &results, &outSize);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[!] kMethodGetScanResults failed (0x%08x)\n", kr);
        return 1;
    }

    printf("\nDiscovered %u Access Points:\n", results.count);
    printf("%-4s  %-32s  %-17s  %-4s  %-4s  %-6s  %-6s\n",
           "NUM", "SSID", "BSSID", "CHAN", "RSSI", "BAND", "SEC");
    printf("----------------------------------------------------------------------------------\n");
    for (uint32_t i = 0; i < results.count; i++) {
        const RTL8852BEDiscoveredAP &ap = results.aps[i];
        printf("%-4u  %-32s  %02X:%02X:%02X:%02X:%02X:%02X  %-4u  %3d   %-6s  %-6s\n",
               i + 1,
               ap.ssid[0] ? ap.ssid : "<hidden>",
               ap.bssid[0], ap.bssid[1], ap.bssid[2], ap.bssid[3], ap.bssid[4], ap.bssid[5],
               ap.channel,
               ap.rssi,
               ap.is_5ghz ? "5GHz" : "2.4GHz",
               ap.is_wpa2 ? "WPA2" : "Open");
    }
    return 0;
}

static int cmdConnect(io_connect_t conn, const char *ssid, const char *passphrase) {
    RTL8852BEConnectParams params;
    memset(&params, 0, sizeof(params));
    strncpy(params.ssid, ssid, RTL8852BE_MAX_SSID_LEN);
    if (passphrase) {
        strncpy(params.passphrase, passphrase, RTL8852BE_MAX_PASSPHRASE_LEN);
    }
    printf("[*] Connecting to SSID \"%s\"...\n", ssid);
    kern_return_t kr = IOConnectCallStructMethod(conn, kMethodConnect, &params, sizeof(params), nullptr, nullptr);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[!] kMethodConnect failed (0x%08x)\n", kr);
        return 1;
    }
    printf("[*] Connection request submitted.\n");
    return 0;
}

static int cmdDisconnect(io_connect_t conn) {
    kern_return_t kr = IOConnectCallMethod(conn, kMethodDisconnect, nullptr, 0, nullptr, 0, nullptr, nullptr, nullptr, nullptr);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[!] kMethodDisconnect failed (0x%08x)\n", kr);
        return 1;
    }
    printf("[*] Disconnected.\n");
    return 0;
}

static int cmdAntenna(io_connect_t conn, uint8_t ant) {
    uint64_t inScalar = ant;
    kern_return_t kr = IOConnectCallScalarMethod(conn, kMethodSetAntenna, &inScalar, 1, nullptr, nullptr);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[!] kMethodSetAntenna failed (0x%08x)\n", kr);
        return 1;
    }
    printf("[*] Antenna mode set to: %u (0=Auto 2x2, 1=Path A, 2=Path B)\n", ant);
    return 0;
}

static int cmdDiag(io_connect_t conn) {
    RTL8852BEDiag diag;
    memset(&diag, 0, sizeof(diag));
    size_t outSize = sizeof(diag);
    kern_return_t kr = IOConnectCallStructMethod(conn, kMethodGetDiag, nullptr, 0, &diag, &outSize);
    if (kr != KERN_SUCCESS) {
        fprintf(stderr, "[!] kMethodGetDiag failed (0x%08x)\n", kr);
        return 1;
    }
    printf("=== Realtek RTL8852BE Hardware Diagnostics ===\n");
    printf("  Driver Version : v%u\n", diag.version);
    printf("  Init Stage     : %u / 8\n", diag.init_stage);
    printf("  PCI Device ID  : 0x%04X (Subsystem 0x%04X:0x%04X Lenovo LOQ)\n",
           diag.pci_device_id, diag.subsys_vendor, diag.subsys_device);
    printf("  Firmware Vers  : 0x%08X (FreeRTOS Ready)\n", diag.fw_version);
    printf("  MAC Address    : %02X:%02X:%02X:%02X:%02X:%02X\n",
           diag.mac_addr[0], diag.mac_addr[1], diag.mac_addr[2],
           diag.mac_addr[3], diag.mac_addr[4], diag.mac_addr[5]);
    printf("  Channel / Band : Channel %u (%s, Wi-Fi %u)\n",
           diag.channel, diag.is_5ghz ? "5 GHz" : "2.4 GHz", diag.wifi_generation);
    printf("  Antenna Mode   : %u\n", diag.ant_mode);
    printf("  Crystal Cap    : 0x%02X\n", diag.xtal_cap);
    printf("  RFE Type       : 0x%02X\n", diag.rfe_type);
    printf("  Interrupts     : %u (HIMR: 0x%08X, HISR: 0x%08X)\n",
           diag.interrupts, diag.himr, diag.hisr);
    printf("  TX Ring WP/RP  : WP=%u, RP=%u (Submitted: %u, Full: %u)\n",
           diag.txbd_wp, diag.txbd_rp, diag.tx_submitted, diag.tx_ring_full);
    printf("  RX Ring WP/RP  : WP=%u, RP=%u (Descriptors Done: %u)\n",
           diag.rxbd_wp, diag.rxbd_rp, diag.rx_desc_done);
    printf("  RX Counters    : Mgmt=%u, Beacons=%u, Data=%u, CRC_Err=%u, Dropped=%u\n",
           diag.rx_mgmt, diag.rx_beacons, diag.rx_data, diag.rx_crc_err, diag.rx_dropped);
    printf("  Traffic Stats  : RX: %llu bytes, TX: %llu bytes\n",
           diag.rx_bytes, diag.tx_bytes);
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Realtek RTL8852BE Wi-Fi 6 Utility CLI (macOS)\n");
        printf("Usage: %s <status|scan|connect|disconnect|ant|diag|reg|rf|iqk>\n", argv[0]);
        printf("  status                   - Display current link and card status\n");
        printf("  scan                     - Scan for available 2.4 GHz & 5 GHz networks\n");
        printf("  connect <SSID> [pass]    - Connect to WPA2 network\n");
        printf("  disconnect               - Disconnect from current network\n");
        printf("  ant <0|1|2>              - Set antenna (0=Auto 2x2 MIMO, 1=Path A, 2=Path B)\n");
        printf("  diag                     - Display hardware registers and diagnostics\n");
        return 0;
    }

    io_connect_t conn = openDriverConnection();
    if (conn == IO_OBJECT_NULL) return 1;

    int ret = 0;
    if (strcmp(argv[1], "status") == 0) {
        ret = cmdStatus(conn);
    } else if (strcmp(argv[1], "scan") == 0) {
        ret = cmdScan(conn);
    } else if (strcmp(argv[1], "connect") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s connect <SSID> [password]\n", argv[0]);
            ret = 1;
        } else {
            ret = cmdConnect(conn, argv[2], argc >= 4 ? argv[3] : nullptr);
        }
    } else if (strcmp(argv[1], "disconnect") == 0) {
        ret = cmdDisconnect(conn);
    } else if (strcmp(argv[1], "ant") == 0) {
        if (argc < 3) {
            fprintf(stderr, "Usage: %s ant <0|1|2>\n", argv[0]);
            ret = 1;
        } else {
            ret = cmdAntenna(conn, (uint8_t)atoi(argv[2]));
        }
    } else if (strcmp(argv[1], "diag") == 0) {
        ret = cmdDiag(conn);
    } else {
        fprintf(stderr, "Unknown command: %s\n", argv[1]);
        ret = 1;
    }

    IOServiceClose(conn);
    return ret;
}
