// SPDX-License-Identifier: MIT
// Copyright (c) 2026 Issen Software Group

#ifndef RTL8852BE_HW_HPP
#define RTL8852BE_HW_HPP

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Realtek PCI IDs
#define RTL_PCI_VENDOR_ID                0x10EC
#define RTL8852BE_PCI_DEVICE_ID          0xB852
#define RTL8852BE_PCI_DEVICE_ID_ALT      0xB85B

// Lenovo LOQ 15IRH8 (Type 82XV) Subsystem Identifiers
#define LENOVO_PCI_SUBVENDOR_ID          0x17AA
#define LENOVO_LOQ_82XV_SUBSYS_ID        0x4853

// PCIe Configuration Space Offsets
#define PCI_CFG_COMMAND                  0x04
#define PCI_CFG_BAR2                     0x18
#define PCI_CFG_EXP_LNKCTL               0x10 // In PCI Express Capability
#define PCI_COMMAND_BUS_MASTER           (1 << 2)
#define PCI_COMMAND_MEM_ENABLE           (1 << 1)

// MMIO BAR2 Window Sizes
#define RTL8852BE_BAR2_SIZE              0x10000 // 64KB MMIO window

// Hardware Generation and Chip Constants
#define RTL_CHIP_AX                      0x01
#define RTL_TX_RING_SIZE                 128
#define RTL_RX_RING_SIZE                 128
#define RTL_RX_BUF_SIZE                  2048
#define RTL_TX_MAX_BUF_SIZE              2048

// PCIe Interface Registers (rtw89 pci.h / reg.h)
#define R_AX_PCIE_INIT_CFG1              0x1000
#define B_AX_TXHCI_EN                    (1U << 11)
#define B_AX_RXHCI_EN                    (1U << 13)
#define B_AX_RXBD_MODE                   (1U << 18)
#define B_AX_LATENCY_CONTROL             (1U << 12)

#define R_AX_PCIE_DMA_STOP1              0x1010
#define B_AX_STOP_PCIEIO                 (1U << 20)
#define B_AX_TX_STOP1_MASK_V1            0x000FFFFF

#define R_AX_TXBD_RWPTR_CLR1             0x1014
#define R_AX_RXBD_RWPTR_CLR              0x1018

#define R_AX_PCIE_DMA_BUSY1              0x101C
#define B_AX_DMA_BUSY1_CHECK             0x000FFFFF

#define R_AX_RXQ_RXBD_NUM                0x1020
#define R_AX_ACH0_TXBD_NUM               0x1024

#define R_AX_RXQ_RXBD_IDX                0x1050
#define R_AX_ACH0_TXBD_IDX               0x1058

#define R_AX_PCIE_HIMR00                 0x10B0
#define R_AX_PCIE_HISR00                 0x10B4
#define B_AX_RXQ_RXBD_INT                (1U << 0)
#define B_AX_ACH0_TXBD_INT               (1U << 8)
#define B_AX_MGNT_TXBD_INT               (1U << 9)
#define B_AX_HALT_C2H_INT                (1U << 15)

#define R_AX_INT_MIT_RX                  0x10D4

#define R_AX_TXDMA_ADDR_H                0x10F0
#define R_AX_RXDMA_ADDR_H                0x10F4

#define R_AX_RXQ_RXBD_DESA_L             0x1100
#define R_AX_RXQ_RXBD_DESA_H             0x1104

#define R_AX_ACH0_TXBD_DESA_L            0x1110
#define R_AX_ACH0_TXBD_DESA_H            0x1114

#define R_AX_PCIE_EXP_CTRL               0x13F0
#define B_AX_MAX_TAG_NUM                 0x000000FF

// SWSI RF Control Registers (rtw89 reg.h / phy.c)
#define R_SWSI_DATA_V1                   0x0370
#define B_SWSI_DATA_VAL_V1_MASK          0x000FFFFF
#define B_SWSI_DATA_ADDR_V1_SHIFT        20
#define B_SWSI_DATA_PATH_V1_SHIFT        28
#define B_SWSI_DATA_BIT_MASK_EN_V1       (1U << 31)

#define R_SWSI_BIT_MASK_V1               0x0374
#define R_SWSI_READ_ADDR_V1              0x0378
#define R_SWSI_V1                        0x174C
#define B_SWSI_W_BUSY_V1                 (1U << 24)
#define B_SWSI_R_BUSY_V1                 (1U << 25)
#define B_SWSI_R_DATA_DONE_V1            (1U << 26)

// Power Sequence and System Control Registers (rtw89 reg.h)
#define R_AX_SYS_ISO_CTRL                0x0000
#define R_AX_SYS_FUNC_EN                 0x0002
#define B_AX_FEN_BBRSTB                  (1U << 0)
#define B_AX_FEN_BB_GLB_RSTN             (1U << 1)

#define R_AX_SYS_PW_CTRL                 0x0004
#define B_AX_APFN_ONMAC                  (1U << 8)
#define B_AX_APFM_OFFMAC                 (1U << 9)
#define B_AX_APFM_SWLPS                  (1U << 10)
#define B_AX_AFSM_WLSUS_EN               (1U << 11)
#define B_AX_AFSM_PCIE_SUS_EN            (1U << 12)
#define B_AX_APDM_HPDN                   (1U << 15)
#define B_AX_EN_WLON                     (1U << 16)
#define B_AX_RDY_SYSPWR                  (1U << 17)
#define B_AX_DIS_WLBT_PDNSUSEN_SOPC      (1U << 18)

#define R_AX_SYS_CLK_CTRL                0x0008
#define B_AX_CPU_CLK_EN                  (1U << 14)

#define R_AX_SYS_WL_EFUSE_CTRL           0x000A
#define B_AX_AUTOLOAD_SUS                (1U << 5)

#define R_AX_WLLPS_CTRL                  0x0090
#define B_AX_DIS_WLBT_LPSEN_LOPC         (1U << 0)

#define R_AX_AFE_LDO_CTRL                0x0020
#define B_AX_AON_OFF_PC_EN               (1U << 1)

#define R_AX_PLATFORM_ENABLE             0x0088
#define B_AX_PLATFORM_EN                 (1U << 0)

#define R_AX_MCUFW_CTRL                  0x0080
#define B_AX_MCUFWDL_RDY                 (1U << 1)

// RF Synth & Channel Configuration
#define RR_CFGCH                         0x18
#define RR_CFGCH_V1                      0x19
#define RR_LCKST                         0x1C
#define RR_LCKST_BIN                     (1U << 0)

// Queue selection codes
#define Q_VO                             6
#define Q_VI                             5
#define Q_BE                             0
#define Q_BK                             1
#define Q_MGNT                           0x12
#define Q_HI                             0x11

// eFuse Structure Constants
#define EFUSE_PHYSICAL_SIZE_AX           1024
#define EFUSE_LOGICAL_SIZE_AX            1024
#define EFUSE_MAC_ADDR_OFFSET_PCIE       0x423
#define EFUSE_XTAL_K_OFFSET              0x2DB
#define EFUSE_RFE_TYPE_OFFSET            0x2EC

#pragma pack(push, 1)

// TX Buffer Descriptor (8 bytes) - Production rtw89 PCI layout
struct Rtw89TxBd {
    uint16_t length;
    uint16_t opt;
#define RTW89_TXBD_OPT_LS                (1U << 14)
#define RTW89_TXBD_OPT_DMA_HI_MASK       0x3FC0
    uint32_t dma;

    inline void set_length(uint16_t len) { length = len; }
    inline uint16_t get_length() const { return length; }
    inline void set_dma(uint32_t addr) { dma = addr; }
    inline uint32_t get_dma() const { return dma; }
    inline void set_ls(bool is_last) {
        if (is_last) opt |= RTW89_TXBD_OPT_LS;
        else opt &= ~RTW89_TXBD_OPT_LS;
    }
    inline bool get_ls() const { return (opt & RTW89_TXBD_OPT_LS) != 0; }
};

// RX Buffer Descriptor (8 bytes) - Production rtw89 PCI layout
struct Rtw89RxBd {
    uint16_t buf_size;
    uint16_t opt;
#define RTW89_RXBD_OPT_DMA_HI_MASK       0x3FC0
    uint32_t dma;

    inline void set_buf_size(uint16_t sz) { buf_size = sz; }
    inline uint16_t get_buf_size() const { return buf_size; }
    inline void set_dma(uint32_t addr) { dma = addr; }
    inline uint32_t get_dma() const { return dma; }
};

// TX Wireless Descriptor Header (40 bytes) - Prepended to 802.11 frames for AX MAC
struct Rtw89TxWd {
    uint32_t dword0;
#define RTW89_TXWD_WP_OFFSET_SHIFT       24
#define RTW89_TXWD_MORE_DATA             (1U << 23)
#define RTW89_TXWD_WD_INFO_EN            (1U << 22)
#define RTW89_TXWD_CHANNEL_DMA_SHIFT     16
#define RTW89_TXWD_HDR_LLC_LEN_SHIFT     11
    uint32_t dword1;
#define RTW89_TXWD_MAC_ID_SHIFT          0
#define RTW89_TXWD_QSEL_SHIFT            8
#define RTW89_TXWD_PKT_LEN_SHIFT         16
    uint32_t dword2;
    uint32_t dword3;
    uint32_t dword4;
    uint32_t dword5;
    uint32_t dword6;
    uint32_t dword7;
    uint32_t dword8;
    uint32_t dword9;

    inline void init(uint16_t pkt_len, uint8_t qsel, uint8_t mac_id) {
        dword0 = (40U << RTW89_TXWD_WP_OFFSET_SHIFT) | RTW89_TXWD_WD_INFO_EN;
        dword1 = (mac_id & 0xFF) | ((qsel & 0x1F) << RTW89_TXWD_QSEL_SHIFT) | ((pkt_len & 0xFFFF) << RTW89_TXWD_PKT_LEN_SHIFT);
        dword2 = 0;
        dword3 = 0;
        dword4 = 0;
        dword5 = 0;
        dword6 = 0;
        dword7 = 0;
        dword8 = 0;
        dword9 = 0;
    }
};

// RX Status Descriptor (32 bytes)
struct Rtw89RxDesc {
    uint32_t dword0;
#define RTW89_RXD_PKT_LEN_MASK           0x00003FFF
#define RTW89_RXD_SHIFT_MASK             0x00030000
#define RTW89_RXD_CRC_ERR                (1U << 18)
#define RTW89_RXD_ICV_ERR                (1U << 19)
    uint32_t dword1;
#define RTW89_RXD_MAC_ID_MASK            0x000000FF
    uint32_t dword2;
#define RTW89_RXD_PAGING_MASK            0x000000FF
    uint32_t dword3;
    uint32_t dword4;
    int8_t   rssi;
    uint8_t  rx_rate;
    uint16_t rsvd;
    uint32_t dword6;
    uint32_t dword7;

    inline uint16_t get_pkt_len() const { return (dword0 & RTW89_RXD_PKT_LEN_MASK); }
    inline bool is_crc_err() const { return (dword0 & RTW89_RXD_CRC_ERR) != 0; }
    inline bool is_icv_err() const { return (dword0 & RTW89_RXD_ICV_ERR) != 0; }
    inline int8_t get_rssi() const { return rssi; }
};

#pragma pack(pop)

// Queue selection mapping helper
static inline uint8_t rtl8852be_queue_select(uint8_t q) {
    switch (q) {
        case Q_MGNT: return 0x12;
        case Q_VO:   return 6;
        case Q_VI:   return 5;
        case Q_BK:   return 1;
        case Q_BE:
        default:     return 0;
    }
}

// 32-bit DMA boundary and alignment validation
static inline bool rtlDmaRangeValid(uint64_t addr, uint32_t size, uint32_t align) {
    if (size == 0) return false;
    if (addr == 0) return false;
    if (align > 1 && (addr % align) != 0) return false;
    if ((addr + size - 1) > 0xFFFFFFFFULL) return false;
    return true;
}

// eFuse logical decoding helpers (rtw89 efuse.c)
#define INVALID_EFUSE_HEADER(h1, h2) ((h1) == 0xFF || (h2) == 0xFF)
#define INVALID_EFUSE_CONTENT(word_en, i) (((word_en) & (1U << (i))) != 0)
#define GET_EFUSE_BLK_IDX(h1, h2) ((((h2) & 0xF0) >> 4) | (((h1) & 0x0F) << 4))
#define BLOCK_IDX_TO_LOGICAL_IDX(blk, i) (((blk) << 3) + ((i) << 1))

#endif // RTL8852BE_HW_HPP
