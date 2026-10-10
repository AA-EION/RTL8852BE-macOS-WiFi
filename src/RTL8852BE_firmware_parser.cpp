// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
// Container layout and selection based on Linux v6.12 rtw89/fw.{h,c}.

#include "RTL8852BE_firmware.hpp"

static uint32_t readLe32(const uint8_t *p) {
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
           (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

bool rtw89_parse_mfw_header(const uint8_t *data, size_t len, uint8_t target_cv,
                          Rtw89FwParsedInfo *out) {
    if (!out) return false;
    *out = {};
    if (!data || len < 16 || data[0] != 0xff) return false;
    const size_t count = data[1];
    const size_t directoryEnd = 16 + count * 16;
    if (!count || directoryEnd > len) return false;

    // rtw89_mfw_info starts with cv, type, mp. Select the closest supported cut
    // at or below the detected cut, excluding manufacturing-test firmware.
    const uint8_t *selected = nullptr;
    for (size_t i = 0; i < count; ++i) {
        const uint8_t *entry = data + 16 + i * 16;
        if (entry[1] != 1 || entry[2] != 0 || entry[0] > target_cv) continue;
        if (!selected || entry[0] > selected[0]) selected = entry;
    }
    if (!selected) return false;
    const size_t shift = readLe32(selected + 4);
    const size_t size = readLe32(selected + 8);
    if (shift < directoryEnd || shift > len || size < 32 || size > len - shift) return false;
    // Offsets in the public result are 32 bits.
    if (shift > UINT32_MAX || size > UINT32_MAX - shift) return false;

    const uint8_t *fw = data + shift;
    const uint32_t w3 = readLe32(fw + 12);
    const uint32_t w7 = readLe32(fw + 28);
    // This parser supports the embedded v0, static-header image. Dynamic/v1
    // headers and signed security trailers need explicit support, not a guess.
    if ((w3 >> 24) != 0 || (w7 & (1U << 16))) return false;
    const size_t sectionCount = (readLe32(fw + 24) >> 8) & 0xff;
    Rtw89FwParsedInfo parsed = {};
    if (!sectionCount || sectionCount > sizeof(parsed.sections) / sizeof(parsed.sections[0])) return false;
    size_t payload = 32 + sectionCount * 16;
    if (payload > size) return false;
    const uint32_t version = readLe32(fw + 4);
    parsed.major = version & 0xff;
    parsed.minor = (version >> 8) & 0xff;
    parsed.subver = (version >> 16) & 0xff;
    parsed.subidx = (version >> 24) & 0xff;
    parsed.section_count = uint8_t(sectionCount);

    for (size_t i = 0; i < sectionCount; ++i) {
        const uint8_t *section = fw + 32 + i * 16;
        const uint32_t flags = readLe32(section + 4);
        const uint32_t type = (flags >> 24) & 0xf;
        if (type == 9 && readLe32(section + 8) != 0) return false;
        uint32_t sectionSize = flags & 0xffffff;
        if (flags & (1U << 28)) sectionSize += 8; // firmware section checksum trailer
        if (!sectionSize || sectionSize > size - payload) return false;
        parsed.sections[i].dl_addr = readLe32(section) & 0x1fffffff;
        parsed.sections[i].len = sectionSize;
        parsed.sections[i].offset = uint32_t(shift + payload);
        payload += sectionSize;
    }
    if (payload != size) return false;
    *out = parsed;
    return true;
}
