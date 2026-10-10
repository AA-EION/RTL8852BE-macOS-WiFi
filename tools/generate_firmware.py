# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group

import os

def main():
    fw_path = "src/rtw8852b_fw.bin"
    if not os.path.exists(fw_path):
        print(f"Error: {fw_path} not found!")
        return

    with open(fw_path, "rb") as f:
        data = f.read()

    print(f"Read {len(data)} bytes from {fw_path}")

    hpp_content = """// SPDX-License-Identifier: MIT
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
"""

    with open("src/RTL8852BE_firmware.hpp", "w", encoding="utf-8") as f:
        f.write(hpp_content)

    print("Generating src/RTL8852BE_firmware.cpp...")
    with open("src/RTL8852BE_firmware.cpp", "w", encoding="utf-8") as f:
        f.write("// SPDX-License-Identifier: MIT\n")
        f.write("// Copyright (c) 2026 Issen Software Group\n\n")
        f.write('#include "RTL8852BE_firmware.hpp"\n')
        f.write('#include <string.h>\n\n')
        f.write(f"const size_t rtw8852b_fw_bin_len = {len(data)};\n\n")
        f.write(f"alignas(16)\nconst uint8_t rtw8852b_fw_bin[{len(data)}] = {{\n")
        
        # Write bytes in chunks of 16
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
            f.write(f"    {hex_str},\n")
        
        f.write("};\n\n")

        # Include parser implementation
        parser_impl = """bool rtw89_parse_mfw_header(const uint8_t *data, size_t len, uint8_t target_cv, Rtw89FwParsedInfo *out) {
    if (!data || len < 32 || !out) return false;
    if (data[0] != 0xff) return false; // MFW signature
    uint8_t fw_nr = data[1];
    if (fw_nr == 0 || fw_nr > 16) return false;

    // Scan through MFW info entries (16 bytes each starting at offset 16)
    size_t info_pos = 16;
    size_t chosen_shift = 0;
    size_t chosen_size = 0;
    bool found = false;

    for (uint8_t i = 0; i < fw_nr; i++) {
        if (info_pos + 16 > len) break;
        uint8_t type = data[info_pos];
        uint8_t cv = data[info_pos + 1];
        uint32_t shift = data[info_pos + 4] | (data[info_pos + 5] << 8) | (data[info_pos + 6] << 16) | (data[info_pos + 7] << 24);
        uint32_t size = data[info_pos + 8] | (data[info_pos + 9] << 8) | (data[info_pos + 10] << 16) | (data[info_pos + 11] << 24);
        info_pos += 16;

        // Type 1 is RTW89_FW_NORMAL
        if (type == 1) {
            if (!found || cv == target_cv) {
                chosen_shift = shift;
                chosen_size = size;
                found = true;
                if (cv == target_cv) break;
            }
        }
    }

    if (!found || chosen_shift + 32 > len) return false;

    const uint8_t *fw = data + chosen_shift;
    // Sub-header
    uint32_t w1 = fw[4] | (fw[5] << 8) | (fw[6] << 16) | (fw[7] << 24);
    out->major = w1 & 0xff;
    out->minor = (w1 >> 8) & 0xff;
    out->subver = (w1 >> 16) & 0xff;
    out->subidx = (w1 >> 24) & 0xff;

    uint32_t w6 = fw[24] | (fw[25] << 8) | (fw[26] << 16) | (fw[27] << 24);
    uint8_t sec_num = (w6 >> 8) & 0xff;
    if (sec_num > 8) sec_num = 8;
    out->section_count = sec_num;

    // Section headers start at byte 32 (16 bytes per section)
    size_t sec_hdr_pos = chosen_shift + 32;
    size_t payload_pos = sec_hdr_pos + (sec_num * 16);

    for (uint8_t s = 0; s < sec_num; s++) {
        if (sec_hdr_pos + 16 > len) break;
        uint32_t dl_addr = data[sec_hdr_pos] | (data[sec_hdr_pos + 1] << 8) | (data[sec_hdr_pos + 2] << 16) | (data[sec_hdr_pos + 3] << 24);
        uint32_t w1_sec = data[sec_hdr_pos + 4] | (data[sec_hdr_pos + 5] << 8) | (data[sec_hdr_pos + 6] << 16) | (data[sec_hdr_pos + 7] << 24);
        uint32_t sec_len = w1_sec & 0x00ffffff; // low 24 bits

        out->sections[s].dl_addr = dl_addr;
        out->sections[s].len = sec_len;
        out->sections[s].offset = payload_pos;

        sec_hdr_pos += 16;
        payload_pos += sec_len;
    }

    return true;
}
"""
        f.write(parser_impl)

    print("Generated src/RTL8852BE_firmware.hpp and src/RTL8852BE_firmware.cpp successfully!")

if __name__ == "__main__":
    main()
