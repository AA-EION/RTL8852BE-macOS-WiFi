// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group
#include "RTL8852BE_efuse.hpp"
#include "RTL8852BE_phy.hpp"
#include "RTL8852BE_power.hpp"
#include "RTL8852BE_linux_regs.hpp"
#include <cassert>
#include <cstring>
#include <cstdio>
#include <map>
#include <vector>

struct EfuseBus {
    uint8_t physical[EFUSE_PHYSICAL_SIZE_AX];
    std::map<uint32_t, uint32_t> registers;
    uint32_t address = 0, readCount = 0, writes = 0;
    int failAddress = -1;
    bool timeout = false, cutBurstSeen = false;
    RtlRegisterIO io() {
        return {this,
            [](void *context, uint32_t address, uint32_t, uint32_t *value) {
                auto &bus = *static_cast<EfuseBus *>(context);
                if (address == R_AX_EFUSE_CTRL) {
                    if (int(bus.address) == bus.failAddress) return false;
                    *value = bus.timeout ? 0 : (++bus.readCount % 2 ? 0 :
                             B_AX_EF_RDY | bus.physical[bus.address]);
                } else *value = bus.registers[address];
                return true;
            },
            [](void *context, uint32_t address, uint32_t, uint32_t value) {
                auto &bus = *static_cast<EfuseBus *>(context);
                ++bus.writes;
                bus.registers[address] = value;
                if (address == R_AX_EFUSE_CTRL) {
                    assert(!(value & B_AX_EF_RDY));
                    bus.address = (value & B_AX_EF_ADDR_MASK) >> 16;
                    assert(bus.address < sizeof(bus.physical));
                }
                if (address == R_AX_EFUSE_CTRL_1_V1 && (value & B_AX_EF_BURST)) bus.cutBurstSeen = true;
                return true;
            }, [](void *, uint32_t) {}};
    }
    void restored() {
        assert(registers[R_AX_SYS_ISO_CTRL] & B_AX_ISO_EB2CORE);
        assert(!(registers[R_AX_SYS_ISO_CTRL] & (B_AX_PWC_EV2EF_B14 | B_AX_PWC_EV2EF_B15)));
        assert(!(registers[R_AX_PMC_DBG_CTRL2] & B_AX_SYSON_DIS_PMCR_AX_WRMSK));
        assert(!(registers[R_AX_EFUSE_CTRL_1_V1] & B_AX_EF_BURST));
    }
};

int main() {
    struct PhyBus {
        std::map<uint32_t, uint32_t> registers;
        uint32_t writes = 0, delayed = 0;
        bool failRead = false, failWrite = false;
        RtlRegisterIO io() {
            return {this,
                [](void *ctx, uint32_t address, uint32_t width, uint32_t *value) {
                    auto &bus = *static_cast<PhyBus *>(ctx);
                    // A missing base translation cannot pass this bus model.
                    assert(address >= 0x10000 && address < 0x20000 && width == 4);
                    if (bus.failRead) return false;
                    *value = bus.registers[address];
                    return true;
                },
                [](void *ctx, uint32_t address, uint32_t width, uint32_t value) {
                    auto &bus = *static_cast<PhyBus *>(ctx);
                    assert(address >= 0x10000 && address < 0x20000 && width == 4);
                    if (bus.failWrite) return false;
                    bus.registers[address] = value;
                    ++bus.writes;
                    return true;
                }, [](void *ctx, uint32_t us) { static_cast<PhyBus *>(ctx)->delayed += us; }};
        }
    } phy;
    assert(rtl8852bWriteBB(phy.io(), 0x4000, 0x12345678));
    assert(phy.registers[0x14000] == 0x12345678);
    assert(rtl8852bWriteBB(phy.io(), 0xfe, 0));
    assert(phy.delayed == 50000 && phy.writes == 1);
    assert(rtl8852bWriteBB(phy.io(), 0x4004, 0xbabecafe) && phy.writes == 1);
    assert(!rtl8852bWriteBB(phy.io(), 0x10000, 0));
    phy.registers[0x1f060] = 0xf03a0;
    assert(rtl8852bReadRF(phy.io(), 1, 0x10018, 0xff00) == 3);
    assert(rtl8852bWriteRF(phy.io(), 1, 0x10018, 0xff00, 0x12));
    assert(phy.registers[0x1f060] == 0xf12a0);
    phy.registers[0x1174c] = B_SWSI_R_DATA_DONE_V1 | 0x54321;
    assert(rtl8852bReadRF(phy.io(), 1, 0x18, 0xfffff) == 0x54321);
    assert((phy.registers[0x10378] & 0x7ff) == 0x118);
    assert(rtl8852bWriteRF(phy.io(), 1, 0x18, 0xff00, 0x5a));
    assert((phy.registers[0x10374] & 0xfffff) == 0xff00);
    assert(phy.registers[0x10370] == (B_SWSI_DATA_BIT_MASK_EN_V1 |
           (1U << B_SWSI_DATA_PATH_V1_SHIFT) | (0x18U << B_SWSI_DATA_ADDR_V1_SHIFT) | 0x5a00));
    phy.registers[0x1174c] = B_SWSI_R_BUSY_V1;
    const uint32_t before = phy.writes;
    assert(!rtl8852bWriteRF(phy.io(), 0, 0x18, 0xfffff, 0));
    assert(rtl8852bReadRF(phy.io(), 0, 0x18, 0xfffff) == UINT32_MAX);
    assert(phy.writes == before);
    phy.registers[0x1174c] = 0; // Idle but read completion never arrives.
    assert(rtl8852bReadRF(phy.io(), 0, 0x18, 0xfffff) == UINT32_MAX);
    phy.registers[0x1174c] = UINT32_MAX;
    assert(!rtl8852bWriteRF(phy.io(), 0, 0x18, 0xfffff, 0));
    phy.failRead = true;
    assert(rtl8852bReadRF(phy.io(), 0, 0x10018, 0xfffff) == UINT32_MAX);
    assert(!rtl8852bWriteRF(phy.io(), 0, 0x10018, 0xfffff, 0));
    phy.failRead = false; phy.failWrite = true;
    assert(!rtl8852bWriteRF(phy.io(), 0, 0x10018, 0xfffff, 0));
    assert(!rtl8852bWriteBB(phy.io(), 0x4000, 0));
    assert(!rtl8852bWriteRF(phy.io(), 2, 0x18, 0xfffff, 0));
    assert(!rtl8852bWriteRF(phy.io(), 0, 0x80000000, 0xfffff, 0));
    assert(!rtl8852bWriteRF(phy.io(), 0, 0x18, 0, 0));

    struct PowerBus {
        std::map<uint32_t, uint32_t> regs;
        unsigned resets = 0, crystalWrites = 0, delays = 0;
        bool onMacStuck = false, crystalStuck = false, failedRead = false;
        RtlRegisterIO io() {
            return {this,
                [](void *ctx, uint32_t address, uint32_t, uint32_t *value) {
                    auto &bus = *static_cast<PowerBus *>(ctx);
                    if (bus.failedRead) return false;
                    if (address == R_AX_SYS_PW_CTRL && !bus.onMacStuck)
                        bus.regs[address] &= ~(B_AX_APFN_ONMAC | B_AX_APFM_OFFMAC);
                    if (address == R_AX_WLAN_XTAL_SI_CTRL && !bus.crystalStuck)
                        bus.regs[address] &= ~B_AX_WL_XTAL_SI_CMD_POLL;
                    *value = bus.regs[address];
                    return true;
                },
                [](void *ctx, uint32_t address, uint32_t, uint32_t value) {
                    auto &bus = *static_cast<PowerBus *>(ctx);
                    if (address == R_AX_PLATFORM_ENABLE) ++bus.resets;
                    if (address == R_AX_WLAN_XTAL_SI_CTRL) ++bus.crystalWrites;
                    bus.regs[address] = value;
                    return true;
                }, [](void *ctx, uint32_t) { ++static_cast<PowerBus *>(ctx)->delays; }};
        }
    } power;
    power.regs[R_AX_SYS_PW_CTRL] = B_AX_RDY_SYSPWR;
    assert(rtl8852bPowerOn(power.io(), 1, 5));
    assert(power.resets == 5 && power.crystalWrites == 11);
    assert(power.regs[R_AX_SPS_ANA_ON_CTRL2] == RTL8852B_RFE_05_SPS_ANA);
    assert((power.regs[R_AX_DMAC_FUNC_EN] & (B_AX_MAC_FUNC_EN | B_AX_DMAC_FUNC_EN)) ==
           (B_AX_MAC_FUNC_EN | B_AX_DMAC_FUNC_EN));
    assert(power.regs[R_AX_CMAC_FUNC_EN] & B_AX_CMAC_EN);
    assert(rtl8852bPowerOff(power.io(), 1, 5));
    assert(power.regs[R_AX_SYS_PW_CTRL] & B_AX_APFM_SWLPS);
    power = {}; power.regs[R_AX_SYS_PW_CTRL] = B_AX_RDY_SYSPWR; power.onMacStuck = true;
    assert(!rtl8852bPowerOn(power.io(), 1, 1));
    assert(power.resets == 0); // ONMAC timeout must stop the sequence.
    power = {}; power.regs[R_AX_SYS_PW_CTRL] = B_AX_RDY_SYSPWR; power.crystalStuck = true;
    assert(!rtl8852bPowerOn(power.io(), 1, 1));
    assert(!(power.regs[R_AX_DMAC_FUNC_EN] & B_AX_DMAC_FUNC_EN));
    power = {}; power.failedRead = true;
    assert(!rtl8852bPowerOn(power.io(), 1, 1));

    EfuseBus bus;
    memset(bus.physical, 0xff, sizeof(bus.physical));
    // Security prefix must never be interpreted as an eFuse header.
    memset(bus.physical, 0, EFUSE_SECURITY_CTRL_SIZE_AX);
    const uint8_t fields[] = {0x08, 0x08, 0x02, 0x11, 0x22, 0x33, 0x44, 0x55,
                             0x05, 0x7e, 0x00, 0x23, 0x05, 0x9d, 0x01, 0x00};
    memcpy(bus.physical + 4, fields, sizeof(fields));
    uint8_t physical[EFUSE_PHYSICAL_SIZE_AX], logical[EFUSE_LOGICAL_SIZE_AX];
    assert(rtl8852bReadPhysicalEfuse(bus.io(), 0, physical, sizeof(physical)));
    assert(bus.cutBurstSeen && !memcmp(physical, bus.physical, sizeof(physical)));
    bus.restored();
    assert(rtl8852bExpandEfuse(physical, sizeof(physical), logical, sizeof(logical)));
    RTL8852BECalibData calibration;
    assert(rtl8852bParseCalibration(logical, sizeof(logical), &calibration));
    assert(calibration.mac_addr[0] == 2 && calibration.mac_addr[5] == 0x55);
    assert(calibration.xtal_cap == 0x23 && calibration.rfe_type == 1);
    logical[0x400] |= 1; assert(!rtl8852bParseCalibration(logical, sizeof(logical), &calibration));
    assert(calibration.mac_addr[0] == 0);
    bus.failAddress = 9;
    assert(!rtl8852bReadPhysicalEfuse(bus.io(), 1, physical, sizeof(physical)));
    for (const uint8_t value : physical) assert(value == 0xff);
    bus.restored();
    bus.failAddress = -1; bus.timeout = true;
    assert(!rtl8852bReadPhysicalEfuse(bus.io(), 1, physical, sizeof(physical)));
    bus.restored();
    uint8_t shortMap[11] = {0, 0, 0, 0, 8, 8, 2, 0, 0, 0, 0};
    assert(!rtl8852bExpandEfuse(shortMap, sizeof(shortMap), logical, sizeof(logical)));
    for (const uint8_t value : logical) assert(value == 0xff);
    assert(!rtl8852bExpandEfuse(nullptr, 0, logical, sizeof(logical)));

    // Headline selection and IF/CHECK/ELIF/ELSE/END are exercised independently
    // from the copied tables, so an unconditional raw-write loop cannot pass.
    const Rtw89Reg2Def table[] = {
        {0xf0010000, 0}, {0xf0010001, 0}, {0xf0ff0002, 0},
        {0x80010000, 0}, {0x40000000, 0}, {0x18, 10},
        {0x90010001, 0}, {0x40000000, 0}, {0x18, 11},
        {0x90ff0002, 0}, {0x40000000, 0}, {0x18, 12},
        {0xa0000000, 0}, {0xb0000000, 0}, {0x19, 13}};
    std::vector<uint32_t> writes;
    auto write = [](void *context, uint32_t address, uint32_t value) {
        assert(address < 0x10000);
        static_cast<std::vector<uint32_t> *>(context)->push_back(value);
        return true;
    };
    assert(rtl8852bApplyPhyTable(table, 15, 1, 0, write, &writes));
    assert((writes == std::vector<uint32_t>{10, 13}));
    writes.clear();
    assert(rtl8852bApplyPhyTable(table, 15, 1, 8, write, &writes));
    assert((writes == std::vector<uint32_t>{11, 13})); // matching RFE/max cut
    writes.clear();
    assert(rtl8852bApplyPhyTable(table, 15, 8, 0, write, &writes));
    assert((writes == std::vector<uint32_t>{12, 13})); // wildcard RFE
    writes.clear();
    auto malformed = std::vector<Rtw89Reg2Def>(table, table + 15);
    malformed[4].addr = 0xb0000000;
    assert(!rtl8852bApplyPhyTable(malformed.data(), malformed.size(), 1, 0, write, &writes));
    assert(writes.empty()); // validate before touching hardware
    assert(!rtl8852bApplyPhyTable(table, 15, 1, 0,
                                 [](void *, uint32_t, uint32_t) { return false; }, nullptr));
    auto realWrite = [](void *context, uint32_t address, uint32_t) {
        assert(!(address & ~0x100ffU));
        ++*static_cast<size_t *>(context);
        return true;
    };
    size_t selected = 0;
    assert(rtl8852bApplyPhyTable(RTL8852BE_PHY_RADIOA_REGS, RTL8852BE_PHY_RADIOA_REGS_COUNT,
                               1, 1, realWrite, &selected));
    assert(selected > 100 && selected < RTL8852BE_PHY_RADIOA_REGS_COUNT / 2);
    assert(rtl8852bApplyPhyTable(RTL8852BE_PHY_RADIOB_REGS, RTL8852BE_PHY_RADIOB_REGS_COUNT,
                               1, 1, realWrite, &selected));
    printf("BB/RF translation, power/XTAL, physical eFuse cleanup/timeout, logical calibration and conditional RF table tests passed (%zu selected writes).\n", selected);
}
