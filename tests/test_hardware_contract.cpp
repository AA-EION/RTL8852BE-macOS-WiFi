// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#include "RTL8852BE_hw.hpp"
#include "RTL8852BE_tables.hpp"
#include "RTL8852BE_firmware.hpp"
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
    printf("[*] Running RTL8852BE Production Hardware Contract Verification...\n");

    // 1. Verify Descriptor ABI and Byte Layouts
    assert(sizeof(Rtw89TxBd) == 8);
    assert(sizeof(Rtw89RxBd) == 8);
    assert(sizeof(Rtw89TxWd) == 40);
    assert(sizeof(Rtw89RxDesc) == 32);

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

    // TXWD Layout
    Rtw89TxWd txwd;
    txwd.init(1500, Q_BE, 1);
    assert(((txwd.dword0 >> RTW89_TXWD_WP_OFFSET_SHIFT) & 0xFF) == 40);
    assert((txwd.dword0 & RTW89_TXWD_WD_INFO_EN) != 0);
    assert((txwd.dword1 & 0xFF) == 1); // mac_id
    assert(((txwd.dword1 >> RTW89_TXWD_QSEL_SHIFT) & 0x1F) == 0); // Q_BE
    assert(((txwd.dword1 >> RTW89_TXWD_PKT_LEN_SHIFT) & 0xFFFF) == 1500);

    // Queue selection mapping
    assert(rtl8852be_queue_select(Q_MGNT) == 0x12);
    assert(rtl8852be_queue_select(Q_BE) == 0);
    assert(rtl8852be_queue_select(Q_VO) == 6);
    assert(rtl8852be_queue_select(Q_VI) == 5);
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

    // 3. 32-bit DMA Range & Physical Boundary Enforcement
    assert(rtlDmaRangeValid(0x10000000, 4096, 256));
    assert(rtlDmaRangeValid(0xFFFFFF00, 256, 256));
    assert(!rtlDmaRangeValid(0xFFFFFF00, 257, 256)); // Exceeds 32-bit boundary
    assert(!rtlDmaRangeValid(0x100000000ULL, 256, 256)); // 64-bit address
    assert(!rtlDmaRangeValid(0, 256, 256)); // NULL address
    assert(!rtlDmaRangeValid(0x10000001, 256, 256)); // Misaligned
    assert(!rtlDmaRangeValid(0x10000000, 0, 256)); // Zero length

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

    // 5. eFuse Logical Stream Decoding Contract
    {
        uint8_t phys[EFUSE_PHYSICAL_SIZE_AX];
        uint8_t log[EFUSE_LOGICAL_SIZE_AX];
        std::memset(phys, 0xFF, sizeof(phys));
        std::memset(log, 0xFF, sizeof(log));

        // Synthesize an eFuse entry: block 0x84 (logical 0x420..0x427), word 1..3 present
        // hdr1: (blk & 0x0F) << 4 | 0x00? Wait: GET_EFUSE_BLK_IDX: ((hdr2 & 0xF0) >> 4) | ((hdr1 & 0x0F) << 4)
        // For blk = 0x84: (hdr1 & 0x0F) = 0x08, (hdr2 & 0xF0) = 0x40 -> hdr1 = 0x08, hdr2 = 0x40 | word_en
        // word_en: active-low. words 1,2,3 present -> word 0 disabled (bit 0 = 1, bit 1,2,3 = 0) -> word_en = 0x01
        phys[0] = 0x08;
        phys[1] = 0x41; // word_en = 0001b (words 1, 2, 3 present)
        // word 1 (0x422, 0x423): 0xAA, 0x1C (MAC byte 0 at 0x423)
        phys[2] = 0xAA; phys[3] = 0x1C;
        // word 2 (0x424, 0x425): 0x39, 0x47 (MAC bytes 1, 2)
        phys[4] = 0x39; phys[5] = 0x47;
        // word 3 (0x426, 0x427): 0xAB, 0xCD (MAC bytes 3, 4)
        phys[6] = 0xAB; phys[7] = 0xCD;

        size_t phy_idx = 0;
        while (phy_idx < 8) {
            uint8_t h1 = phys[phy_idx];
            uint8_t h2 = phys[phy_idx + 1];
            if (INVALID_EFUSE_HEADER(h1, h2)) break;
            uint8_t blk = GET_EFUSE_BLK_IDX(h1, h2);
            uint8_t word_en = h2 & 0x0F;
            phy_idx += 2;
            for (int w = 0; w < 4; w++) {
                if (INVALID_EFUSE_CONTENT(word_en, w)) continue;
                size_t l_idx = BLOCK_IDX_TO_LOGICAL_IDX(blk, w);
                log[l_idx] = phys[phy_idx];
                log[l_idx + 1] = phys[phy_idx + 1];
                phy_idx += 2;
            }
        }
        assert(log[0x422] == 0xAA);
        assert(log[0x423] == 0x1C);
        assert(log[0x424] == 0x39);
        assert(log[0x425] == 0x47);
        assert(log[0x426] == 0xAB);
        assert(log[0x427] == 0xCD);
    }

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

    printf("[+] RTL8852BE Hardware Contract Verification PASSED!\n");
    return 0;
}
