// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group
#include "RTL8852BE_firmware.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <vector>

static void put32(std::vector<uint8_t> &b, size_t at, uint32_t value) {
    for (unsigned i = 0; i < 4; ++i) b[at + i] = uint8_t(value >> (8 * i));
}

// Upstream v0 layout: two suits, each one section and four payload bytes.
static std::vector<uint8_t> image() {
    std::vector<uint8_t> b(152);
    b[0] = 0xff; b[1] = 2;
    for (size_t i = 0; i < 2; ++i) {
        const size_t entry = 16 + 16 * i, start = 48 + 52 * i;
        b[entry] = uint8_t(i + 1); b[entry + 1] = 1;
        put32(b, entry + 4, uint32_t(start)); put32(b, entry + 8, 52);
        put32(b, start + 4, 0x81321b00U + uint32_t(i));
        put32(b, start + 24, 1U << 8);
        put32(b, start + 32, 0xb8970000);
        put32(b, start + 36, 4);
    }
    return b;
}

int main() {
    Rtw89FwParsedInfo out;
    auto good = image();
    auto parse = [&](const std::vector<uint8_t> &b, uint8_t cut = 1) {
        return rtw89_parse_mfw_header(b.data(), b.size(), cut, &out);
    };
    auto rejects = [&](const std::vector<uint8_t> &b, uint8_t cut = 1) {
        memset(&out, 0xa5, sizeof(out));
        assert(!parse(b, cut));
        assert(out.section_count == 0 && out.sections[0].len == 0);
    };
    assert(parse(good));
    assert(out.major == 0 && out.minor == 27 && out.subver == 50 && out.subidx == 129);
    assert(out.sections[0].offset == 96 && out.sections[0].len == 4);
    assert(out.sections[0].dl_addr == 0x18970000);
    assert(parse(good, 2) && out.major == 1 && out.sections[0].offset == 148);
    assert(parse(good, 3) && out.major == 1); // closest lower cut
    rejects(good, 0); // never select a newer cut as fallback
    auto b = good; b[18] = 1; rejects(b); // manufacturing firmware excluded
    b = good; b[17] = 3; rejects(b); // WOWLAN excluded even when cv == 1
    b = good; b[16] = 2; rejects(b); // cv and type are distinct bytes
    for (size_t n = 0; n < 100; ++n) {
        b.assign(good.begin(), good.begin() + n); rejects(b);
    }
    b = good; b[1] = 0; rejects(b);
    b = good; b[1] = 255; rejects(b);
    b = good; b[0] = 0; rejects(b);
    b = good; put32(b, 20, 0xfffffff0); rejects(b);
    b = good; put32(b, 20, 16); rejects(b); // suit overlaps directory
    b = good; put32(b, 24, 31); rejects(b); // short subheader
    b = good; put32(b, 24, 51); rejects(b); // selected-suit bound, not whole file
    b = good; put32(b, 24, 53); rejects(b); // trailing bytes rejected
    b = good; put32(b, 72, 9U << 8); rejects(b); // no clamping excessive section counts
    b = good; put32(b, 72, 0); rejects(b);
    b = good; put32(b, 84, 5); rejects(b); // payload spills into next suit
    b = good; put32(b, 84, 0); rejects(b);
    b = good; put32(b, 60, 1U << 24); rejects(b); // unsupported header version
    b = good; put32(b, 76, 1U << 16); rejects(b); // unsupported dynamic header
    b = good; put32(b, 84, 0x09000004); put32(b, 88, 1); rejects(b); // signed trailer
    b = good; put32(b, 84, 0x10000004); rejects(b); // missing checksum trailer
    b[1] = 1; b.resize(160); put32(b, 24, 60);
    assert(parse(b)); assert(out.sections[0].len == 12); // include checksum in bounds
    memset(&out, 0xa5, sizeof(out));
    assert(!rtw89_parse_mfw_header(nullptr, 0, 1, &out) && out.section_count == 0);
    assert(!rtw89_parse_mfw_header(good.data(), good.size(), 1, nullptr));
    assert(rtw89_parse_mfw_header(rtw8852b_fw_bin, rtw8852b_fw_bin_len, 1, &out));
    assert(out.major == 0 && out.minor == 27 && out.subver == 32 && out.subidx == 1);
    assert(out.section_count == 3 && out.sections[0].offset == 269208);
    assert(out.sections[2].offset + out.sections[2].len == 269128 + 268992);
    puts("Firmware parser selection, truncation, and suit-bound regressions passed.");
}
