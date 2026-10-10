// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#ifndef RTL8852BE_FIRMWARE_HPP
#define RTL8852BE_FIRMWARE_HPP

#include <stdint.h>
#include <stddef.h>

// Full embedded Realtek RTL8852B firmware image (linux-firmware rtw89/rtw8852b_fw.bin)
extern const uint8_t rtw8852b_fw_bin[];
extern const size_t rtw8852b_fw_bin_len;

// Section info for parsed firmware suits
struct Rtw89FwSectionInfo {
    uint32_t dl_addr;
    uint32_t len;
    uint32_t offset; // offset in rtw8852b_fw_bin
};

struct Rtw89FwParsedInfo {
    uint8_t major;
    uint8_t minor;
    uint8_t subver;
    uint8_t subidx;
    uint8_t section_count;
    Rtw89FwSectionInfo sections[8];
};

// Parser helpers
bool rtw89_parse_mfw_header(const uint8_t *data, size_t len, uint8_t target_cv, Rtw89FwParsedInfo *out);

#endif // RTL8852BE_FIRMWARE_HPP
