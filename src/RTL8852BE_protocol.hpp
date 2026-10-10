// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group
#ifndef RTL8852BE_PROTOCOL_HPP
#define RTL8852BE_PROTOCOL_HPP

#include "RTL8852BE_ipc.h"
#include <stddef.h>
#include <string.h>

// Fixed-size IPC strings must terminate within their fields. This prototype
// accepts WPA2 passphrases (8..63 bytes), not raw 64-digit PSKs or open networks.
static inline size_t rtl8852beBoundedLength(const char *s, size_t capacity) {
    size_t n = 0;
    while (n < capacity && s[n]) ++n;
    return n;
}

static inline bool rtl8852beConnectStringsValid(const char *ssid, const char *password) {
    if (!ssid || !password) return false;
    const size_t ssidLen = rtl8852beBoundedLength(ssid, RTL8852BE_MAX_SSID_LEN + 1);
    const size_t passLen = rtl8852beBoundedLength(password, RTL8852BE_MAX_PASSPHRASE_LEN + 1);
    return ssidLen > 0 && ssidLen <= RTL8852BE_MAX_SSID_LEN && passLen >= 8 && passLen <= 63;
}

static inline bool rtl8852beParseBeacon(const uint8_t *frame, size_t len,
                                      uint8_t currentChannel, RTL8852BEDiscoveredAP *out) {
    if (!out) return false;
    memset(out, 0, sizeof(*out));
    if (!frame || len < 36 || (frame[0] != 0x80 && frame[0] != 0x50)) return false;
    RTL8852BEDiscoveredAP ap = {};
    memcpy(ap.bssid, frame + 16, 6);
    ap.beacon_interval = uint16_t(frame[32]) | (uint16_t(frame[33]) << 8);
    ap.capabilities = uint16_t(frame[34]) | (uint16_t(frame[35]) << 8);
    size_t pos = 36;
    while (pos < len) {
        if (len - pos < 2) return false;
        const uint8_t id = frame[pos++];
        const size_t size = frame[pos++];
        if (size > len - pos) return false;
        if (id == 0) {
            if (size > RTL8852BE_MAX_SSID_LEN) return false;
            memcpy(ap.ssid, frame + pos, size);
            ap.ssid[size] = '\0';
        } else if (id == 3) {
            if (size != 1) return false;
            ap.channel = frame[pos];
        } else if (id == 48) {
            // Capability hint only; cipher/AKM negotiation remains a release blocker.
            if (size < 2) return false;
            ap.is_wpa2 = 1;
        } else if (id == 255 && size >= 1 && frame[pos] == 35) {
            ap.is_wifi6 = 1;
        }
        pos += size;
    }
    if (!ap.channel) ap.channel = currentChannel;
    ap.is_5ghz = ap.channel > 14;
    *out = ap;
    return true;
}

struct RTL8852BEEapolView {
    const uint8_t *packet;
    size_t packetLen;
    uint16_t keyInfo;
    uint16_t keyDataLen;
};

static inline bool rtl8852beParseEapol(const uint8_t *frame, size_t len, size_t offset,
                                     RTL8852BEEapolView *out) {
    if (!out) return false;
    *out = {};
    if (!frame || offset > len || len - offset < 99) return false;
    const uint8_t *p = frame + offset;
    if ((p[0] != 1 && p[0] != 2) || p[1] != 3 || p[4] != 2) return false;
    const size_t bodyLen = (size_t(p[2]) << 8) | p[3];
    const uint16_t dataLen = (uint16_t(p[97]) << 8) | p[98];
    if (bodyLen < 95 || bodyLen > len - offset - 4 || dataLen != bodyLen - 95) return false;
    const uint16_t info = (uint16_t(p[5]) << 8) | p[6];
    if ((info & 7) != 2) return false; // WPA2 HMAC-SHA1/AES descriptor version
    *out = {p, 4 + bodyLen, info, dataLen};
    return true;
}

#endif
