// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
// Copyright (c) 2019-2022 Realtek Corporation
#include "RTL8852BE_efuse.hpp"
#include <string.h>

static bool efusePower(const RtlRegisterIO &io, uint8_t cut, bool enable) {
    bool ok = true;
    if (enable) {
        ok &= rtlUpdateRegister(io, R_AX_PMC_DBG_CTRL2, 1, B_AX_SYSON_DIS_PMCR_AX_WRMSK,
                                B_AX_SYSON_DIS_PMCR_AX_WRMSK);
        ok &= rtlUpdateRegister(io, R_AX_SYS_ISO_CTRL, 2, B_AX_PWC_EV2EF_B14, B_AX_PWC_EV2EF_B14);
        io.delay(io.context, 1000);
        ok &= rtlUpdateRegister(io, R_AX_SYS_ISO_CTRL, 2, B_AX_PWC_EV2EF_B15, B_AX_PWC_EV2EF_B15);
        ok &= rtlUpdateRegister(io, R_AX_SYS_ISO_CTRL, 2, B_AX_ISO_EB2CORE, 0);
        if (cut == 0) ok &= rtlUpdateRegister(io, R_AX_EFUSE_CTRL_1_V1, 4, B_AX_EF_BURST, B_AX_EF_BURST);
    } else {
        if (cut == 0) ok &= rtlUpdateRegister(io, R_AX_EFUSE_CTRL_1_V1, 4, B_AX_EF_BURST, 0);
        ok &= rtlUpdateRegister(io, R_AX_SYS_ISO_CTRL, 2, B_AX_ISO_EB2CORE, B_AX_ISO_EB2CORE);
        ok &= rtlUpdateRegister(io, R_AX_SYS_ISO_CTRL, 2, B_AX_PWC_EV2EF_B15, 0);
        io.delay(io.context, 1000);
        ok &= rtlUpdateRegister(io, R_AX_SYS_ISO_CTRL, 2, B_AX_PWC_EV2EF_B14, 0);
        ok &= rtlUpdateRegister(io, R_AX_PMC_DBG_CTRL2, 1, B_AX_SYSON_DIS_PMCR_AX_WRMSK, 0);
    }
    return ok;
}

bool rtl8852bReadPhysicalEfuse(const RtlRegisterIO &io, uint8_t cut,
                             uint8_t *physical, size_t capacity) {
    if (!physical || capacity < EFUSE_PHYSICAL_SIZE_AX || !io.read || !io.write || !io.delay) return false;
    memset(physical, 0xff, EFUSE_PHYSICAL_SIZE_AX);
    uint8_t map[EFUSE_PHYSICAL_SIZE_AX];
    bool ok = efusePower(io, cut, true);
    for (uint32_t address = 0; ok && address < sizeof(map); ++address) {
        ok = io.write(io.context, R_AX_EFUSE_CTRL, 4, (address << 16) & B_AX_EF_ADDR_MASK);
        bool ready = false;
        for (uint32_t elapsed = 0; ok && elapsed < 1000000; ++elapsed) {
            io.delay(io.context, 1);
            uint32_t status;
            ok = io.read(io.context, R_AX_EFUSE_CTRL, 4, &status);
            if (ok && status != UINT32_MAX && (status & B_AX_EF_RDY)) {
                map[address] = uint8_t(status);
                ready = true;
                break;
            }
            if (ok && status == UINT32_MAX) ok = false; // Removed or inaccessible PCI function.
        }
        ok &= ready;
    }
    // Restore isolation/power even on timeout or a failed read (upstream's early
    // error return is tightened here). Never publish a partial calibration map.
    const bool restored = efusePower(io, cut, false);
    if (!ok || !restored) return false;
    memcpy(physical, map, sizeof(map));
    return true;
}

bool rtl8852bExpandEfuse(const uint8_t *physical, size_t size,
                        uint8_t *logical, size_t capacity) {
    if (!logical || capacity < EFUSE_LOGICAL_SIZE_AX) return false;
    memset(logical, 0xff, EFUSE_LOGICAL_SIZE_AX);
    if (!physical || size < 2 * EFUSE_SECURITY_CTRL_SIZE_AX || size > EFUSE_PHYSICAL_SIZE_AX) return false;
    uint8_t map[EFUSE_LOGICAL_SIZE_AX];
    memset(map, 0xff, sizeof(map));
    const size_t end = size - EFUSE_SECURITY_CTRL_SIZE_AX;
    size_t position = EFUSE_SECURITY_CTRL_SIZE_AX;
    while (position < end) {
        if (end - position < 2) return false;
        const uint8_t first = physical[position++], second = physical[position++];
        if (first == 0xff || second == 0xff) break;
        const size_t block = ((second & 0xf0) >> 4) | ((first & 0xf) << 4);
        for (unsigned word = 0; word < 4; ++word) {
            if (second & (1U << word)) continue;
            const size_t target = (block << 3) + (word << 1);
            if (end - position < 2 || target + 1 >= sizeof(map)) return false;
            map[target] = physical[position++];
            map[target + 1] = physical[position++];
        }
    }
    memcpy(logical, map, sizeof(map));
    return true;
}

bool rtl8852bParseCalibration(const uint8_t *logical, size_t size,
                            RTL8852BECalibData *calibration) {
    if (!calibration) return false;
    *calibration = {};
    if (!logical || size < EFUSE_MAC_ADDR_OFFSET_PCIE + 6) return false;
    const uint8_t *mac = logical + EFUSE_MAC_ADDR_OFFSET_PCIE;
    uint8_t any = 0;
    for (unsigned i = 0; i < 6; ++i) any |= mac[i];
    if (!any || (mac[0] & 1) || logical[EFUSE_XTAL_K_OFFSET] == 0xff ||
        logical[EFUSE_RFE_TYPE_OFFSET] == 0xff) return false;
    memcpy(calibration->mac_addr, mac, 6);
    calibration->xtal_cap = logical[EFUSE_XTAL_K_OFFSET];
    calibration->rfe_type = logical[EFUSE_RFE_TYPE_OFFSET];
    // Country policy is not encoded by these fields; do not invent a country.
    return true;
}
