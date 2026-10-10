// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group
#include "RTL8852BE_protocol.hpp"
#include <cassert>
#include <cstdio>
#include <vector>

int main() {
    char ssid[33], password[65];
    memset(ssid, 's', sizeof(ssid)); memset(password, 'p', sizeof(password));
    assert(!rtl8852beConnectStringsValid(ssid, password)); // unterminated IPC fields
    ssid[32] = 0; password[63] = 0;
    assert(rtl8852beConnectStringsValid(ssid, password));
    password[63] = 'p'; password[64] = 0;
    assert(!rtl8852beConnectStringsValid(ssid, password)); // raw PSK not implemented
    assert(!rtl8852beConnectStringsValid("", "password"));
    assert(!rtl8852beConnectStringsValid("ssid", "short"));
    assert(!rtl8852beConnectStringsValid("ssid", nullptr));

    RTL8852BEDiscoveredAP ap;
    std::vector<uint8_t> b(36); b[0] = 0x80; b[32] = 100;
    assert(rtl8852beParseBeacon(b.data(), b.size(), 36, &ap));
    assert(ap.channel == 36 && ap.is_5ghz && ap.beacon_interval == 100);
    b.insert(b.end(), {0, 3, 'a', 'p', '1', 3, 1, 6});
    assert(rtl8852beParseBeacon(b.data(), b.size(), 1, &ap));
    assert(strcmp(ap.ssid, "ap1") == 0 && ap.channel == 6);
    b.resize(36); b.insert(b.end(), {3, 0}); // zero-length DS IE formerly read past input
    assert(!rtl8852beParseBeacon(b.data(), b.size(), 1, &ap));
    b.resize(36); b.insert(b.end(), {0, 2, 'a'}); // truncated IE
    assert(!rtl8852beParseBeacon(b.data(), b.size(), 1, &ap));
    b.resize(36); b.push_back(0);
    assert(!rtl8852beParseBeacon(b.data(), b.size(), 1, &ap));
    assert(!rtl8852beParseBeacon(nullptr, 36, 1, &ap));

    std::vector<uint8_t> e(99); e[0] = 2; e[1] = 3; e[3] = 95; e[4] = 2; e[6] = 2;
    RTL8852BEEapolView view;
    assert(rtl8852beParseEapol(e.data(), e.size(), 0, &view) && view.packetLen == 99);
    for (size_t n = 0; n < 99; ++n) {
        std::vector<uint8_t> shortFrame(e.begin(), e.begin() + n);
        assert(!rtl8852beParseEapol(shortFrame.data(), n, 0, &view));
    }
    assert(!rtl8852beParseEapol(e.data(), e.size(), SIZE_MAX, &view));
    e[3] = 96; assert(!rtl8852beParseEapol(e.data(), e.size(), 0, &view));
    e[3] = 95; e[98] = 1; assert(!rtl8852beParseEapol(e.data(), e.size(), 0, &view));
    e[98] = 0; e[6] = 3; assert(!rtl8852beParseEapol(e.data(), e.size(), 0, &view));
    e[6] = 2; e.push_back(0); // link-layer padding is outside the MIC input
    assert(rtl8852beParseEapol(e.data(), e.size(), 0, &view) && view.packetLen == 99);
    puts("IPC, beacon, and EAPOL bounds regressions passed.");
}
