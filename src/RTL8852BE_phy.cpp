// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2026 Issen Software Group
// Copyright (c) 2020-2022 Realtek Corporation
#include "RTL8852BE_phy.hpp"
#include "RTL8852BE_hw.hpp"

// Linux phy.h adds cr_base to BB/SWSI/direct-RF offsets. MAC offsets use
// the unshifted register-I/O adapter (power and eFuse must not be translated).
static bool readPhy(const RtlRegisterIO &io, uint32_t address, uint32_t &value) {
    return address <= 0xfffc && !(address & 3) && io.read &&
           io.read(io.context, RTL8852BE_PHY_CR_BASE + address, 4, &value) && value != UINT32_MAX;
}
static bool writePhy(const RtlRegisterIO &io, uint32_t address, uint32_t value) {
    return address <= 0xfffc && !(address & 3) && io.write &&
           io.write(io.context, RTL8852BE_PHY_CR_BASE + address, 4, value);
}
static bool updatePhy(const RtlRegisterIO &io, uint32_t address, uint32_t mask, uint32_t value) {
    uint32_t current;
    return readPhy(io, address, current) && writePhy(io, address, (current & ~mask) | (value & mask));
}
static uint32_t maskShift(uint32_t mask) {
    uint32_t shift = 0;
    while (!(mask & (1U << shift))) ++shift;
    return shift;
}
static bool swsiIdle(const RtlRegisterIO &io) {
    if (!io.delay) return false;
    for (unsigned retry = 0; retry <= 30; ++retry) {
        uint32_t status;
        if (!readPhy(io, R_SWSI_V1, status)) return false;
        if (!(status & (B_SWSI_W_BUSY_V1 | B_SWSI_R_BUSY_V1))) return true;
        if (retry < 30) io.delay(io.context, 1);
    }
    return false;
}

bool rtl8852bWriteBB(const RtlRegisterIO &io, uint32_t address, uint32_t value) {
    if (address >= 0xf9 && address <= 0xfe) {
        const uint32_t delays[] = {1, 5, 50, 1000, 5000, 50000};
        if (!io.delay) return false;
        io.delay(io.context, delays[address - 0xf9]);
        return true;
    }
    if (value == 0xbabecafe) return true; // Linux BYPASS_CR_DATA.
    return writePhy(io, address, value);
}

uint32_t rtl8852bReadRF(const RtlRegisterIO &io, uint8_t path, uint32_t address, uint32_t mask) {
    mask &= 0xfffff;
    if (path > 1 || !mask || (address & ~0x100ffU) || !io.delay) return UINT32_MAX;
    uint32_t value;
    if (address & 0x10000) {
        const uint32_t offset = (path ? 0xf000 : 0xe000) + ((address & 0xff) << 2);
        return readPhy(io, offset, value) ? (value & mask) >> maskShift(mask) : UINT32_MAX;
    }
    if (!swsiIdle(io) || !updatePhy(io, R_SWSI_READ_ADDR_V1, 0x7ff,
                                   (uint32_t(path) << B_SWSI_READ_ADDR_PATH_V1_SHIFT) | address))
        return UINT32_MAX;
    io.delay(io.context, 2);
    for (unsigned retry = 0; retry <= 30; ++retry) {
        if (!readPhy(io, R_SWSI_V1, value)) return UINT32_MAX;
        if (value & B_SWSI_R_DATA_DONE_V1) return (value & mask) >> maskShift(mask);
        if (retry < 30) io.delay(io.context, 1);
    }
    return UINT32_MAX;
}

bool rtl8852bWriteRF(const RtlRegisterIO &io, uint8_t path, uint32_t address,
                   uint32_t mask, uint32_t value) {
    mask &= 0xfffff;
    if (path > 1 || !mask || (address & ~0x100ffU) || !io.delay) return false;
    value = (value << maskShift(mask)) & 0xfffff;
    if (address & 0x10000) {
        const uint32_t offset = (path ? 0xf000 : 0xe000) + ((address & 0xff) << 2);
        if (!updatePhy(io, offset, mask, value)) return false;
        io.delay(io.context, 1);
        return true;
    }
    if (!swsiIdle(io)) return false;
    if (mask != 0xfffff && !updatePhy(io, R_SWSI_BIT_MASK_V1, 0xfffff, mask)) return false;
    const uint32_t request = ((mask != 0xfffff) ? B_SWSI_DATA_BIT_MASK_EN_V1 : 0) |
        (uint32_t(path) << B_SWSI_DATA_PATH_V1_SHIFT) |
        (address << B_SWSI_DATA_ADDR_V1_SHIFT) | value;
    return writePhy(io, R_SWSI_DATA_V1, request);
}

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
