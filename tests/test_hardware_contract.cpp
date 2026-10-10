// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#include "RTL8852BE_hw.hpp"
#include "RTL8852BE_tables.hpp"
#include "RTL8852BE_firmware.hpp"
#include "reference/rtw8852b_efuse_layout.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>

static uint32_t wordAt(const void *p, size_t offset) {
    uint32_t word;
    std::memcpy(&word, static_cast<const uint8_t *>(p) + offset, sizeof(word));
    return word;
}

static uint16_t shortAt(const void *p, size_t offset) {
    uint16_t val;
    std::memcpy(&val, static_cast<const uint8_t *>(p) + offset, sizeof(val));
    return val;
}

int main() {
    printf("[*] Running RTL8852BE portable header/bounds checks (not hardware validation)...\n");

    // 1. Verify Descriptor ABI and Byte Layouts
    assert(sizeof(Rtw89TxBd) == 8);
    assert(sizeof(Rtw89RxBd) == 8);

    // TXBD Layout & Operations
    Rtw89TxBd txbd = {};
    txbd.set_length(1500);
    txbd.set_dma(0x12345000);
    txbd.set_ls(true);

    assert(shortAt(&txbd, 0) == 1500);
    assert(wordAt(&txbd, 4) == 0x12345000);
    assert(txbd.get_ls() == true);
    assert((shortAt(&txbd, 2) & RTW89_TXBD_OPT_LS) != 0);

    txbd.set_ls(false);
    assert(txbd.get_ls() == false);
    assert((shortAt(&txbd, 2) & RTW89_TXBD_OPT_LS) == 0);

    // RXBD Layout & Operations
    Rtw89RxBd rxbd = {};
    rxbd.set_buf_size(2048);
    rxbd.set_dma(0x23456000);
    assert(shortAt(&rxbd, 0) == 2048);
    assert(wordAt(&rxbd, 4) == 0x23456000);

    // TXWD/RXD prototypes are incomplete; do not assert their layout as a
    // hardware contract. See Linux core.h/txrx.h and docs/AUDIT.md.

    // Queue selection mapping
    assert(rtl8852be_queue_select(Q_MGNT) == 0x12);
    assert(rtl8852be_queue_select(Q_BE) == 0);
    assert(rtl8852be_queue_select(Q_VO) == 3);
    assert(rtl8852be_queue_select(Q_VI) == 2);
    assert(rtl8852be_queue_select(Q_HI) == 0x11);
    assert(rtl8852be_queue_select(Q_BK) == 1);

    // 2. Hardware Registers & Offsets Contract (rtw89 pci.h / reg.h)
    assert(R_AX_PCIE_INIT_CFG1 == 0x1000);
    assert(R_AX_PCIE_DMA_STOP1 == 0x1010);
    assert(R_AX_PCIE_DMA_BUSY1 == 0x101C);
    assert(R_AX_RXQ_RXBD_NUM == 0x1020);
    assert(R_AX_ACH0_TXBD_NUM == 0x1024);
    assert(R_AX_RXQ_RXBD_IDX == 0x1050);
    assert(R_AX_ACH0_TXBD_IDX == 0x1058);
    assert(R_AX_PCIE_HIMR00 == 0x10B0);
    assert(R_AX_PCIE_HISR00 == 0x10B4);
    assert(R_AX_INT_MIT_RX == 0x10D4);
    assert(R_AX_RXQ_RXBD_DESA_L == 0x1100);
    assert(R_AX_ACH0_TXBD_DESA_L == 0x1110);
    assert(R_AX_PCIE_EXP_CTRL == 0x13F0);
    assert(R_SWSI_DATA_V1 == 0x0370);
    assert(R_SWSI_V1 == 0x174C);
    assert(R_AX_SYS_PW_CTRL == 0x0004);
    assert(R_AX_PLATFORM_ENABLE == 0x0088);
    assert(RTL_PCI_VENDOR_ID == 0x10EC);
    assert(RTL8852BE_PCI_DEVICE_ID == 0xB852);
    assert(RTL8852BE_PCI_DEVICE_ID_ALT == 0xB85B);
    assert(LENOVO_PCI_SUBVENDOR_ID == 0x17AA);
    assert(LENOVO_LOQ_82XV_SUBSYS_ID == 0x4853);
    assert(PCI_EXP_LNKCTL_OFFSET == 0x10);

    // 3. 32-bit DMA Range & Physical Boundary Enforcement
    assert(rtlDmaRangeValid(0x10000000, 4096, 256));
    assert(rtlDmaRangeValid(0xFFFFFF00, 256, 256));
    assert(!rtlDmaRangeValid(0xFFFFFF00, 257, 256)); // Exceeds 32-bit boundary
    assert(!rtlDmaRangeValid(0x100000000ULL, 256, 256)); // 64-bit address
    assert(!rtlDmaRangeValid(0, 256, 256)); // NULL address
    assert(!rtlDmaRangeValid(0x10000001, 256, 256)); // Misaligned
    assert(!rtlDmaRangeValid(0x10000000, 0, 256)); // Zero length
    assert(!rtlDmaRangeValid(UINT64_MAX - 15, 32, 1)); // addition must not wrap
    assert(!rtlDmaRangeValid(UINT64_MAX, 2, 1));
    assert(rtlMmioRangeValid(0xfffc, 4, RTL8852BE_BAR2_SIZE));
    assert(!rtlMmioRangeValid(0xffff, 4, RTL8852BE_BAR2_SIZE));
    assert(!rtlMmioRangeValid(UINT32_MAX, 4, RTL8852BE_BAR2_SIZE));
    assert(!rtlMmioRangeValid(1, 2, RTL8852BE_BAR2_SIZE));
    assert(!rtlMmioRangeValid(0, 4, 3));
    assert(!rtlMmioRangeValid(0, 3, RTL8852BE_BAR2_SIZE));
    assert(B_AX_ACH0_TXBD_INT == (1U << 3)); // Linux v6.12 pci.h
    assert(B_AX_MGNT_TXBD_INT == (1U << 11));
    assert(B_AX_AON_OFF_PC_EN == (1U << 23)); // Linux v6.12 reg.h
    assert(B_AX_DIS_WLBT_LPSEN_LOPC == (1U << 1));
    assert(EFUSE_PHYSICAL_SIZE_AX == 1216); // rtw8852b_chip_info
    assert(EFUSE_SECURITY_CTRL_SIZE_AX == 4);
    assert(EFUSE_MAC_ADDR_OFFSET_PCIE == offsetof(rtw8852bx_efuse, e));
    assert(EFUSE_XTAL_K_OFFSET == offsetof(rtw8852bx_efuse, xtal_k));
    assert(EFUSE_RFE_TYPE_OFFSET == offsetof(rtw8852bx_efuse, rfe_type));
    assert(RR_CFGCH_V1 == 0x10018 && RR_LCKST == 0xCF);

    // 4. Verification of Production PHY Tables
    assert(RTL8852BE_PHY_BB_REGS_COUNT == 1013);
    for (size_t i = 0; i < RTL8852BE_PHY_BB_REGS_COUNT; i++) {
        uint32_t addr = RTL8852BE_PHY_BB_REGS[i].addr;
        assert(addr < RTL8852BE_BAR2_SIZE);
        assert((addr & 0x03) == 0); // 32-bit aligned
    }

    // BB Gain Table entries are packed parameter arguments (union rtw89_phy_bb_gain_arg)
    assert(RTL8852BE_PHY_BB_REG_GAIN_COUNT == 66);
    for (size_t i = 0; i < RTL8852BE_PHY_BB_REG_GAIN_COUNT; i++) {
        uint32_t addr = RTL8852BE_PHY_BB_REG_GAIN[i].addr;
        assert(addr <= 0x03030303); // Valid packed gain argument
    }

    assert(RTL8852BE_PHY_RADIOA_REGS_COUNT == 8295);
    assert(RTL8852BE_PHY_RADIOB_REGS_COUNT == 8392);
    assert(RTL8852BE_PHY_NCTL_REGS_COUNT == 1320);

    assert(EFUSE_LOGICAL_SIZE_AX == 2048);
    assert(EFUSE_MAC_ADDR_OFFSET_PCIE + 6 <= EFUSE_LOGICAL_SIZE_AX);

    // 6. Firmware Image MFW Parsing Contract
    {
        assert(rtw8852b_fw_bin_len == 1035232);
        Rtw89FwParsedInfo fw_info;
        std::memset(&fw_info, 0, sizeof(fw_info));
        bool ok = rtw89_parse_mfw_header(rtw8852b_fw_bin, rtw8852b_fw_bin_len, 1, &fw_info);
        assert(ok);
        assert(fw_info.section_count == 3);
        assert(fw_info.sections[0].len > 0);
        assert(fw_info.sections[1].len > 0);
        assert(fw_info.sections[2].len > 0);
        assert(fw_info.sections[0].offset + fw_info.sections[0].len <= rtw8852b_fw_bin_len);
    }

    printf("[+] RTL8852BE portable header/bounds checks PASSED.\n");
    return 0;
}
