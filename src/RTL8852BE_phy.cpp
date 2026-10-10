// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
// Copyright (c) 2020-2022 Realtek Corporation
#include "RTL8852BE_phy.hpp"

static bool selectHeadline(const Rtw89Reg2Def *regs, size_t count, uint8_t rfe,
                           uint8_t cut, size_t &start, uint32_t &selected) {
    start = 0;
    while (start < count && (regs[start].addr >> 28) == 0xf) ++start;
    selected = 0;
    if (!start) return true;
    const uint32_t exact = (uint32_t(rfe) << 16) | cut;
    const uint32_t anyCut = (uint32_t(rfe) << 16) | 0xff;
    const uint32_t matches[] = {exact, anyCut};
    for (const uint32_t match : matches) {
        for (size_t i = 0; i < start; ++i) {
            if ((regs[i].addr & 0xfffffff) == match) {
                selected = match;
                return true;
            }
        }
    }
    // Upstream fallback: maximum cut for matching RFE, then wildcard RFE.
    for (unsigned pass = 0; pass < 2; ++pass) {
        bool found = false;
        uint8_t maximum = 0;
        const uint8_t desired = pass ? 0xff : rfe;
        for (size_t i = 0; i < start; ++i) {
            const uint32_t address = regs[i].addr;
            if (((address >> 16) & 0xff) == desired && uint8_t(address) >= maximum) {
                maximum = uint8_t(address);
                selected = address & 0xfffffff;
                found = true;
            }
        }
        if (found) return true;
    }
    return false;
}

static bool walk(const Rtw89Reg2Def *regs, size_t count, size_t start, uint32_t selected,
                 RtlPhyWrite write, void *context) {
    bool active = true, found = false, branch = false, checkPending = false;
    bool inElse = false;
    uint32_t target = 0;
    for (size_t i = start; i < count; ++i) {
        const uint32_t address = regs[i].addr;
        switch (address >> 28) {
            case 0x8: // IF
                if (!start || branch || checkPending) return false;
                branch = true;
                inElse = false;
                target = address & 0xfffffff;
                checkPending = true;
                break;
            case 0x9: // ELIF
                if (!branch || checkPending || inElse) return false;
                target = address & 0xfffffff;
                checkPending = true;
                break;
            case 0x4: // CHECK
                if (!branch || !checkPending) return false;
                active = !found && target == selected;
                found |= active;
                checkPending = false;
                break;
            case 0xa: // ELSE indicates a missing table branch, not default writes.
                if (!branch || checkPending || inElse || !found) return false;
                active = false;
                inElse = true;
                break;
            case 0xb: // END
                if (!branch || checkPending) return false;
                branch = false;
                found = false;
                active = true;
                break;
            case 0x0:
                if (checkPending) return false;
                if (active && write && !write(context, address, regs[i].val)) return false;
                break;
            default:
                return false;
        }
    }
    return !branch && !checkPending;
}

bool rtl8852bApplyPhyTable(const Rtw89Reg2Def *registers, size_t count,
                         uint8_t rfe, uint8_t cut, RtlPhyWrite write, void *context) {
    if (!registers || !count || !write) return false;
    size_t start;
    uint32_t selected;
    if (!selectHeadline(registers, count, rfe, cut, start, selected)) return false;
    // No partially programmed device for a structurally malformed package.
    if (!walk(registers, count, start, selected, nullptr, nullptr)) return false;
    return walk(registers, count, start, selected, write, context);
}
