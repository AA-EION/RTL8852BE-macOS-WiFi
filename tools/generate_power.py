#!/usr/bin/env python3
# SPDX-License-Identifier: MIT
# Copyright (c) 2026 Issen Software Group
"""Generate the RTL8852B power routines directly from the audited Linux v6.12 files.

Usage: python3 tools/generate_power.py /path/to/linux-v6.12
Only MMIO/poll/delay calls are adapted; source operations retain their order.
"""
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location("source_check", ROOT / "tools/check_linux_source.py")
check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(check)


def function(source, name):
    match = re.search(r"(?m)^(?:static )?(?:void|int) " + name + r"\(", source)
    if not match:
        raise ValueError(f"Missing source function: {name}")
    start = source.index("{", match.start())
    depth = 1
    end = start + 1
    while depth:
        depth += (source[end] == "{") - (source[end] == "}")
        end += 1
    return source[match.start():end]


ADAPTER = r'''
#include "RTL8852BE_power.hpp"
#include "RTL8852BE_linux_regs.hpp"
namespace {
using u8 = uint8_t;
using u32 = uint32_t;
struct rtw89_efuse { u8 rfe_type; bool valid; bool power_k_valid; };
struct rtw89_dev {
    const RtlRegisterIO &io;
    rtw89_efuse efuse;
    struct { u8 cv; } hal;
    bool failed = false;
};
static uint32_t readRegister(rtw89_dev *dev, uint32_t address, uint32_t width) {
    uint32_t value = UINT32_MAX;
    if (dev->failed || !dev->io.read(dev->io.context, address, width, &value)) dev->failed = true;
    return value;
}
static u32 rtw89_read32(rtw89_dev *dev, u32 address) { return readRegister(dev, address, 4); }
static u8 rtw89_read8(rtw89_dev *dev, u32 address) { return u8(readRegister(dev, address, 1)); }
static void writeRegister(rtw89_dev *dev, u32 address, u32 width, u32 value) {
    if (!dev->failed && !dev->io.write(dev->io.context, address, width, value)) dev->failed = true;
}
static void rtw89_write32(rtw89_dev *dev, u32 address, u32 value) { writeRegister(dev, address, 4, value); }
static void rtw89_write16(rtw89_dev *dev, u32 address, u32 value) { writeRegister(dev, address, 2, value); }
static u32 fieldPrep(u32 mask, u32 value) {
    u32 shift = 0;
    while (!(mask & (1U << shift))) ++shift;
    return (value << shift) & mask;
}
static void update(rtw89_dev *dev, u32 address, u32 width, u32 mask, u32 value) {
    const u32 current = readRegister(dev, address, width);
    writeRegister(dev, address, width, (current & ~mask) | (value & mask));
}
static void rtw89_write32_set(rtw89_dev *d, u32 a, u32 m) { update(d, a, 4, m, m); }
static void rtw89_write32_clr(rtw89_dev *d, u32 a, u32 m) { update(d, a, 4, m, 0); }
static void rtw89_write8_set(rtw89_dev *d, u32 a, u32 m) { update(d, a, 1, m, m); }
static void rtw89_write8_clr(rtw89_dev *d, u32 a, u32 m) { update(d, a, 1, m, 0); }
static void rtw89_write32_mask(rtw89_dev *d, u32 a, u32 m, u32 v) { update(d, a, 4, m, fieldPrep(m, v)); }
static void rtw89_write16_mask(rtw89_dev *d, u32 a, u32 m, u32 v) { update(d, a, 2, m, fieldPrep(m, v)); }
static void rtlDelay(rtw89_dev *d, u32 us) { if (!d->failed) d->io.delay(d->io.context, us); }
#define FIELD_PREP(mask, val) fieldPrep(mask, val)
#define rtw89_warn(...) ((void)0)
#define read_poll_timeout(read, val, condition, interval, timeout, initial, dev, reg) \
    ([&]() -> int { \
        if (initial) rtlDelay(dev, interval); \
        for (u32 elapsed = 0; elapsed <= timeout; elapsed += interval) { \
            val = read(dev, reg); \
            if (dev->failed || val == UINT32_MAX) return -1; \
            if (condition) return 0; \
            if (elapsed == timeout) break; \
            rtlDelay(dev, interval); \
        } \
        return -1; \
    }())
'''

FOOTER = r'''
} // namespace
bool rtl8852bPowerOn(const RtlRegisterIO &io, uint8_t cut, uint8_t rfe,
                    bool efuseValid, bool powerCalibrationValid) {
    if (!io.read || !io.write || !io.delay) return false;
    rtw89_dev device{io, {rfe, efuseValid, powerCalibrationValid}, {cut}};
    return rtw8852b_pwr_on_func(&device) == 0 && !device.failed;
}
bool rtl8852bPowerOff(const RtlRegisterIO &io, uint8_t cut, uint8_t rfe) {
    if (!io.read || !io.write || !io.delay) return false;
    rtw89_dev device{io, {rfe, false, false}, {cut}};
    return rtw8852b_pwr_off_func(&device) == 0 && !device.failed;
}
bool rtl8852bReadAnalogCut(const RtlRegisterIO &io, uint8_t *cut) {
    if (!cut || !io.read || !io.write || !io.delay) return false;
    *cut = 0;
    rtw89_dev device{io, {0, false, false}, {0}};
    u8 value = 0;
    if (rtw89_mac_read_xtal_si(&device, XTAL_SI_CV, &value) != 0 || device.failed) return false;
    *cut = value & XTAL_SI_ACV_MASK;
    return true;
}
'''


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("linux_source", type=Path)
    args = parser.parse_args()
    records = json.loads((ROOT / "docs/linux-source-manifest.json").read_text())["files"]
    source = {}
    for record in records:
        data = (args.linux_source / record["path"]).read_bytes()
        if hashlib.sha256(data).hexdigest() != record["sha256"]:
            raise ValueError(f"Source hash mismatch: {record['path']}")
        source[Path(record["path"]).name] = data.decode()
    bodies = [function(source["mac.c"], name) for name in
              ("rtw89_mac_write_xtal_si_ax", "rtw89_mac_read_xtal_si_ax")]
    bodies += [function(source["rtw8852b.c"], name) for name in
               ("rtw8852b_pwr_sps_ana", "rtw8852b_pwr_on_func", "rtw8852b_pwr_off_func")]
    routines = "\n\n".join(bodies).replace("_xtal_si_ax", "_xtal_si")
    routines = routines.replace("fsleep(", "rtlDelay(rtwdev, ")
    definitions = {}
    for name in ("reg.h", "mac.h", "core.h"):
        definitions.update(check.definitions(source[name]))
        definitions.update(re.findall(r"\b([A-Z][A-Z_0-9]+)\s*=\s*(0x[0-9a-fA-F]+)", source[name]))
    definitions["CHIP_CBV"] = "1"
    constants = sorted(set(re.findall(r"\b(?:R_AX_|B_AX_|XTAL_SI_|SW_LPS_|PINMUX_|RTL8852B_RFE_)[A-Z_0-9]+", routines + FOOTER)))
    header = "// SPDX-License-Identifier: GPL-2.0-only\n// Copyright (c) 2020-2022 Realtek Corporation\n// Generated from audited Linux v6.12 reg.h/mac.h; do not edit.\n#pragma once\n\n"
    for name in constants + ["CHIP_CBV"]:
        header += f"#ifndef {name}\n#define {name} 0x{check.value(name, definitions):08X}U\n#endif\n"
    (ROOT / "src/RTL8852BE_linux_regs.hpp").write_text(header)
    notice = "// SPDX-License-Identifier: GPL-2.0-only\n// Copyright (c) 2020-2022 Realtek Corporation\n// Copyright (c) 2026 Issen Software Group\n// Generated from Linux v6.12 rtw8852b.c/mac.c by tools/generate_power.py.\n"
    (ROOT / "src/RTL8852BE_power.cpp").write_text(notice + ADAPTER + "\n" + routines + "\n" + FOOTER)
    print(f"Generated {len(bodies)} kernel routines and {len(constants) + 1} source constants.")


if __name__ == "__main__":
    main()
